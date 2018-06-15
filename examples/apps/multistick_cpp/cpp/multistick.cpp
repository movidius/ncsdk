// Copyright 2018 Intel Corporation.
// The source code, information and material ("Material") contained herein is  
// owned by Intel Corporation or its suppliers or licensors, and title to such  
// Material remains with Intel Corporation or its suppliers or licensors.  
// The Material contains proprietary information of Intel or its suppliers and  
// licensors. The Material is protected by worldwide copyright laws and treaty  
// provisions.  
// No part of the Material may be used, copied, reproduced, modified, published,  
// uploaded, posted, transmitted, distributed or disclosed in any way without  
// Intel's prior express written permission. No license under any patent,  
// copyright or other intellectual property rights in the Material is granted to  
// or conferred upon you, either expressly, by implication, inducement, estoppel  
// or otherwise.  
// Any license under such intellectual property rights must be express and  
// approved by Intel in writing. 

#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include <mvnc.h>

// from current director to examples base director
// #define APP_BASE_DIR "../"

#define EXAMPLES_BASE_DIR "../../../"

// graph file names - assume the graph file is in the current directory.
#define GOOGLENET_GRAPH_FILE_NAME "googlenet.graph"
#define SQUEEZENET_GRAPH_FILE_NAME "squeezenet.graph"

// image file name - assume we are running in this directory: ncsdk/examples/caffe/GoogLeNet/cpp
#define GOOGLENET_IMAGE_FILE_NAME EXAMPLES_BASE_DIR "data/images/nps_electric_guitar.png"
#define SQUEEZENET_IMAGE_FILE_NAME EXAMPLES_BASE_DIR "data/images/nps_baseball.png"


// GoogleNet image dimensions, network mean values for each channel in BGR order.
const int networkDimGoogleNet = 224;
const int networkDimSqueezeNet = 227;
float networkMeanGoogleNet[] = {0.40787054*255.0, 0.45752458*255.0, 0.48109378*255.0};
float networkMeanSqueezeNet[] = {0.40787054*255.0, 0.45752458*255.0, 0.48109378*255.0};

// Prototypes
void *LoadGraphFile(const char *path, unsigned int *length);
float *LoadImage(const char *path, int reqsize, float *mean);
// end prototypes

// Reads a graph file from the file system and copies it to a buffer
// that is allocated internally via malloc.
// Param path is a pointer to a null terminate string that must be set to the path to the 
//            graph file on disk before calling
// Param length must must point to an integer that will get set to the number of bytes 
//              allocated for the buffer
// Returns pointer to the buffer allcoated.
// Note: The caller must free the buffer returned.
void *LoadGraphFile(const char *path, unsigned int *length)
{
    FILE *fp;
    char *buf;

    fp = fopen(path, "rb");
    if(fp == NULL)
        return 0;
    fseek(fp, 0, SEEK_END);
    *length = ftell(fp);
    rewind(fp);
    if(!(buf = (char*) malloc(*length)))
    {
        fclose(fp);
        return 0;
    }
    if(fread(buf, 1, *length, fp) != *length)
    {
        fclose(fp);
        free(buf);
        return 0;
    }
    fclose(fp);
    return buf;
}

// Reads an image file from disk (8 bit per channel RGB .jpg or .png or other formats 
// supported by stbi_load.)  Resizes it, subtracts the mean from each channel, and then 
// converts to an array of half precision floats that is suitable to pass to ncFifoWriteElem.  
// The returned array will contain 3 floats for each pixel in the image the first float 
// for a pixel is it's the Blue channel value the next is Green and then Red.  The array 
// contains the pixel values in row major order.
// Param path is a pointer to a null terminated string that must be set to the path of the 
//            to read before calling.
// Param reqsize must be set to the width and height that the image will be resized to.  
//               Its assumed width and height are the same size.
// Param mean must be set to point to an array of 3 floating point numbers.  The three
//            numbers are the mean values for the blue, green, and red channels in that order.
//            each B, G, and R value from the image will have this value subtracted from it.
// Returns a pointer to a buffer that is allocated internally via malloc.  this buffer contains
//         the 32 bit float values that can be passed to ncFifoWriteElem().  The returned buffer
//         will contain reqSize*reqSize*3 half floats.
float *LoadImage(const char *path, int reqSize, float *mean)
{
    int width, height, cp, i;
    unsigned char *img, *imgresized;
    float *imgfp32;

    img = stbi_load(path, &width, &height, &cp, 3);
    if(!img)
    {
        printf("Error - the image file %s could not be loaded\n", path);
        return NULL;
    }
    imgresized = (unsigned char*) malloc(3*reqSize*reqSize);
    if(!imgresized)
    {
        free(img);
        perror("malloc");
        return NULL;
    }
    stbir_resize_uint8(img, width, height, 0, imgresized, reqSize, reqSize, 0, 3);
    free(img);
    imgfp32 = (float*) malloc(sizeof(*imgfp32) * reqSize * reqSize * 3);
    if(!imgfp32)
    {
        free(imgresized);
        perror("malloc");
        return 0;
    }
    for(i = 0; i < reqSize * reqSize * 3; i++)
        imgfp32[i] = imgresized[i];
    free(imgresized);

    for(i = 0; i < reqSize*reqSize; i++)
    {
        float blue, green, red;
        blue = imgfp32[3*i+2];
        green = imgfp32[3*i+1];
        red = imgfp32[3*i+0];

        imgfp32[3*i+0] = blue-mean[0];
        imgfp32[3*i+1] = green-mean[1];
        imgfp32[3*i+2] = red-mean[2];
    }
    return imgfp32;
}


// Opens one NCS device.
// Param deviceIndex is the zero-based index of the device to open
// Param deviceHandle is the address of a device handle that will be set 
//                    if opening is successful
// Returns true if works or false if doesn't.
bool OpenOneNCS(int deviceIndex, struct ncDeviceHandle_t **deviceHandle)
{
    ncStatus_t retCode;
    retCode = ncDeviceCreate(deviceIndex, deviceHandle);
    if (retCode != NC_OK)
    {   // failed to get this device's name, maybe none plugged in.
        printf("Error - NCS device at index %d not found\n", deviceIndex);
        return false;
    }
    
    // Try to open the NCS device via the device name
    retCode = ncDeviceOpen(*deviceHandle);
    if (retCode != NC_OK)
    {   // failed to open the device.  
        printf("Error - Could not open NCS device at index %d\n", deviceIndex);
        return false;
    }
    
    // deviceHandle is ready to use now.  
    // Pass it to other NC API calls as needed and close it when finished.
    printf("Successfully opened NCS device at index %d %p!\n", deviceIndex, *deviceHandle);
    return true;
}


// Loads a compiled network graph onto the NCS device.
// Param deviceHandle is the open device handle for the device that will allocate the graph
// Param graphFilename is the name of the compiled network graph file to load on the NCS
// Param graphHandle is the address of the graph handle that will be created internally.
//                   the caller must call mvncDeallocateGraph when done with the handle.
// Returns true if works or false if doesn't.
bool LoadGraphToNCS(struct ncDeviceHandle_t* deviceHandle, const char* graphFilename, struct ncGraphHandle_t** graphHandle)
{
    ncStatus_t retCode;
    int rc = 0;
    unsigned int mem, memMax, length;

    // Read in a graph file
    unsigned int graphFileLen;
    void* graphFileBuf = LoadGraphFile(graphFilename, &graphFileLen);

    length = sizeof(unsigned int);
    // Read device memory info
    rc = ncDeviceGetOption(deviceHandle, NC_RO_DEVICE_CURRENT_MEMORY_USED, (void **)&mem, &length);
    rc += ncDeviceGetOption(deviceHandle, NC_RO_DEVICE_MEMORY_SIZE, (void **)&memMax, &length);
    if(rc)
        printf("ncDeviceGetOption failed, rc=%d\n", rc);
    else
        printf("Current memory used on device is %d out of %d\n", mem, memMax);

    // allocate the graph
    retCode = ncGraphCreate("graph", graphHandle);
    if (retCode)
    {
        printf("ncGraphCreate failed, retCode=%d\n", retCode);
        return retCode;
    }

    // Send graph to device
    retCode = ncGraphAllocate(deviceHandle, *graphHandle, graphFileBuf, graphFileLen);
    if (retCode != NC_OK)
    {   // error allocating graph
        printf("Could not allocate graph for file: %s\n", graphFilename); 
        printf("Error from ncGraphAllocate is: %d\n", retCode);
        return false;
    }

    // successfully allocated graph.  Now graphHandle is ready to go.  
    // use graphHandle for other API calls and call ncGraphDestroy
    // when done with it.
    printf("Successfully allocated graph for %s\n", graphFilename);

    return true;
}


// Runs an inference and outputs result to console
// Param graphHandle is the graphHandle from mvncAllocateGraph for the network that 
//                   will be used for the inference
// Param imageFileName is the name of the image file that will be used as input for
//                     the neural network for the inference
// Param networkDim is the height and width (assumed to be the same) for images that the
//                     network expects. The image will be resized to this prior to inference.
// Param networkMean is pointer to array of 3 floats that are the mean values for the network
//                   for each color channel, blue, green, and red in that order.
// Returns tru if works or false if doesn't
bool DoInferenceOnImageFile(struct ncGraphHandle_t *graphHandle, struct ncDeviceHandle_t *dev,
                            struct ncFifoHandle_t *bufferIn, struct ncFifoHandle_t *bufferOut,
                            const char* imageFileName, int networkDim, float* networkMean)
{
    ncStatus_t retCode;
    struct ncTensorDescriptor_t td;
    struct ncTensorDescriptor_t resultDesc;
    unsigned int length;

    // LoadImage will read image from disk, convert channels to floats
    // subtract network mean for each value in each channel.  Then, 
    // return pointer to the buffer of 32Bit floats
    float* imageBuf = LoadImage(imageFileName, networkDim, networkMean);

    // calculate the length of the buffer that contains the floats. 
    // 3 channels * width * height * sizeof a 32bit float 
    unsigned int lenBuf = 3*networkDim*networkDim*sizeof(*imageBuf);

    // Read descriptor for input tensor
    length = sizeof(struct ncTensorDescriptor_t);
    retCode = ncFifoGetOption(bufferIn, NC_RO_FIFO_GRAPH_TENSOR_DESCRIPTOR, &td, &length);
    if (retCode || length != sizeof(td)){
        printf("ncFifoGetOption failed, retCode=%d\n", retCode);
        return false;
    }

    // Write tensor to input fifo
    retCode = ncFifoWriteElem(bufferIn, imageBuf, &lenBuf, NULL);
    if (retCode != NC_OK)
    {   // error loading tensor
        printf("Error - Could not load tensor\n");
        printf("    ncStatus_t from mvncLoadTensor is: %d\n", retCode);
        return false;
    }

    // Start inference
    retCode = ncGraphQueueInference(graphHandle, &bufferIn, 1, &bufferOut, 1);
    if (retCode)
    {
        free(imageBuf);
        printf("ncGraphQueueInference failed, retCode=%d\n", retCode);
        return false;
    }
    free(imageBuf);

    // the inference has been started, now call ncFifoReadElem() for the
    // inference result 
    printf("Successfully loaded the tensor for image %s\n", imageFileName);
    
    unsigned int outputDataLength;
    length = sizeof(unsigned int);
    retCode = ncFifoGetOption(bufferOut, NC_RO_FIFO_ELEMENT_DATA_SIZE, &outputDataLength, &length);
    if (retCode || length != sizeof(unsigned int)){
        printf("ncFifoGetOption failed, rc=%d\n", retCode);
        exit(-1);
    }
    float* resultData = (float*) malloc(outputDataLength);

    void* userParam;
    // Read output results
    retCode = ncFifoReadElem(bufferOut, (void*) resultData, &outputDataLength, &userParam);
    if (retCode != NC_OK)
    {
        printf("Error - Could not get result for image %s\n", imageFileName);
        printf("    ncStatus_t from ncFifoReadElem is: %d\n", retCode);
        return false;
    }

    // Successfully got the result.  The inference result is in the buffer pointed to by resultData
    printf("Successfully got the inference result for image %s\n", imageFileName);

    unsigned int numResults = outputDataLength/sizeof(float);
    float maxResult = 0.0;
    int maxIndex = -1;
    for (int index = 0; index < numResults; index++)
    {
        // printf("Category %d is: %f\n", index, resultData[index]);
        if (resultData[index] > maxResult)
        {
            maxResult = resultData[index];
            maxIndex = index;
        }
    }
    printf("Index of top result is: %d\n", maxIndex);
    printf("Probability of top result is: %f\n", resultData[maxIndex]);
    free(resultData);
}

// Main entry point for the program
int main(int argc, char** argv)
{
    int rc = 0;
    unsigned int length;
    struct ncDeviceHandle_t *devHandle1;
    struct ncDeviceHandle_t *devHandle2;
    struct ncGraphHandle_t *graphHandleGoogleNet;
    struct ncGraphHandle_t *graphHandleSqueezeNet;
    int loglevel = 2;
    ncStatus_t retCode = ncGlobalSetOption(NC_RW_LOG_LEVEL, &loglevel, sizeof(loglevel));

    if (!OpenOneNCS(0, &devHandle1))
    {   // couldn't open first NCS device
        exit(-1);
    }
    if (!OpenOneNCS(1, &devHandle2))
    {   // couldn't open second NCS device
        exit(-1);
    }

    if (!LoadGraphToNCS(devHandle1, GOOGLENET_GRAPH_FILE_NAME, &graphHandleGoogleNet))
    {
        ncDeviceClose(devHandle1);
        ncDeviceClose(devHandle2);
        exit(-2);
    }
    if (!LoadGraphToNCS(devHandle2, SQUEEZENET_GRAPH_FILE_NAME, &graphHandleSqueezeNet))
    {
        ncGraphDestroy(&graphHandleGoogleNet);
        graphHandleGoogleNet = NULL;
        ncDeviceClose(devHandle1);
        ncDeviceClose(devHandle2);
        exit(-2);
    }

    // Read tensor descriptors for Googlenet
    struct ncTensorDescriptor_t inputTdGooglenet;
    struct ncTensorDescriptor_t outputTdGooglenet;
    length = sizeof(struct ncTensorDescriptor_t);
    ncGraphGetOption(graphHandleGoogleNet, NC_RO_GRAPH_INPUT_TENSOR_DESCRIPTORS, &inputTdGooglenet,  &length);
    ncGraphGetOption(graphHandleGoogleNet, NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS, &outputTdGooglenet,  &length);

    // Read tensor descriptors for SqueezeNet
    struct ncTensorDescriptor_t inputTdSqueezeNet;
    struct ncTensorDescriptor_t outputTdSqueezeNet;
    length = sizeof(struct ncTensorDescriptor_t);
    ncGraphGetOption(graphHandleSqueezeNet, NC_RO_GRAPH_INPUT_TENSOR_DESCRIPTORS, &inputTdSqueezeNet,  &length);
    ncGraphGetOption(graphHandleSqueezeNet, NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS, &outputTdSqueezeNet,  &length);

    // Init & Create Fifos for Googlenet
    struct ncFifoHandle_t * bufferInGooglenet;
    struct ncFifoHandle_t * bufferOutGooglenet;
    rc = ncFifoCreate("fifoIn0", NC_FIFO_HOST_WO, &bufferInGooglenet);
    rc += ncFifoAllocate(bufferInGooglenet, devHandle1, &inputTdGooglenet, 2);
    rc += ncFifoCreate("fifoOut0", NC_FIFO_HOST_RO, &bufferOutGooglenet);
    rc += ncFifoAllocate(bufferOutGooglenet, devHandle1, &outputTdGooglenet, 2);
    if(rc)
        printf("Fifo allocation failed for Googlenet, rc=%d\n", rc);

    // Init & Create Fifos for SqueezeNet
    struct ncFifoHandle_t * bufferInSqueezeNet;
    struct ncFifoHandle_t * bufferOutSqueezeNet;
    rc = ncFifoCreate("fifoIn1", NC_FIFO_HOST_WO, &bufferInSqueezeNet);
    rc += ncFifoAllocate(bufferInSqueezeNet, devHandle2, &inputTdSqueezeNet, 2);
    rc += ncFifoCreate("fifoOut2", NC_FIFO_HOST_RO, &bufferOutSqueezeNet);
    rc += ncFifoAllocate(bufferOutSqueezeNet, devHandle2, &outputTdSqueezeNet, 2);
    if(rc)
        printf("Fifo allocation failed for SqueezeNet, rc=%d\n", rc);

    printf("\n--- NCS 1 inference ---\n");
    DoInferenceOnImageFile(graphHandleGoogleNet, devHandle1, bufferInGooglenet, bufferOutGooglenet, GOOGLENET_IMAGE_FILE_NAME, networkDimGoogleNet, networkMeanGoogleNet);
    printf("-----------------------\n");

    printf("\n--- NCS 2 inference ---\n");
    DoInferenceOnImageFile(graphHandleSqueezeNet, devHandle2, bufferInSqueezeNet, bufferOutSqueezeNet, SQUEEZENET_IMAGE_FILE_NAME, networkDimSqueezeNet, networkMeanSqueezeNet);
    printf("-----------------------\n");
    retCode = ncFifoDestroy(&bufferInGooglenet);
    retCode = ncFifoDestroy(&bufferOutGooglenet);
    retCode = ncFifoDestroy(&bufferInSqueezeNet);
    retCode = ncFifoDestroy(&bufferOutSqueezeNet);
    retCode = ncGraphDestroy(&graphHandleSqueezeNet);
    graphHandleSqueezeNet = NULL;
    retCode = ncGraphDestroy(&graphHandleGoogleNet);
    graphHandleGoogleNet = NULL;
  
    retCode = ncDeviceClose(devHandle1);
    devHandle1 = NULL;

    retCode = ncDeviceClose(devHandle2);
    devHandle2 = NULL;
}
