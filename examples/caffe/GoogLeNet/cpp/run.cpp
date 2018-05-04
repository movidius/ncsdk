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

// graph file name - assume we are running in this directory: ncsdk/examples/caffe/GoogLeNet/cpp
#define GRAPH_FILE_NAME "../graph"

// image file name - assume we are running in this directory: ncsdk/examples/caffe/GoogLeNet/cpp
#define IMAGE_FILE_NAME "../../../data/images/nps_electric_guitar.png"

// GoogleNet image dimensions, network mean values for each channel in BGR order.
const int networkDim = 224;
float networkMean[] = {0.40787054*255.0, 0.45752458*255.0, 0.48109378*255.0};

// Load a graph file
// caller must free the buffer returned.
void *LoadFile(const char *path, unsigned int *length)
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


float *LoadImage(const char *path, int reqsize, float *mean, unsigned int* imageSize)
{
	int width, height, cp, i;
	unsigned char *img, *imgresized;
	float *imgfp32;

	img = stbi_load(path, &width, &height, &cp, 3);
	if(!img)
	{
		printf("The picture %s could not be loaded\n", path);
		return 0;
	}
	imgresized = (unsigned char*) malloc(3*reqsize*reqsize);
	if(!imgresized)
	{
		free(img);
		perror("malloc");
		return 0;
	}
	stbir_resize_uint8(img, width, height, 0, imgresized, reqsize, reqsize, 0, 3);
	free(img);
    *imageSize = sizeof(*imgfp32) * reqsize * reqsize * 3;
	imgfp32 = (float*) malloc(*imageSize);
	if(!imgfp32)
	{
		free(imgresized);
		perror("malloc");
		return 0;
	}
	for(i = 0; i < reqsize * reqsize * 3; i++)
		imgfp32[i] = imgresized[i];
	free(imgresized);
	for(i = 0; i < reqsize*reqsize; i++)
	{
		float blue, green, red;
		blue = imgfp32[3*i+2];
		green = imgfp32[3*i+1];
		red = imgfp32[3*i+0];

		imgfp32[3*i+0] = blue-mean[0];
		imgfp32[3*i+1] = green-mean[1];
		imgfp32[3*i+2] = red-mean[2];

		// uncomment to see what values are getting passed to mvncLoadTensor() before conversion to half float
		//printf("Blue: %f, Grean: %f,  Red: %f \n", imgfp32[3*i+0], imgfp32[3*i+1], imgfp32[3*i+2]);
	}
	return imgfp32;
}


int main(int argc, char** argv)
{
    struct ncDeviceHandle_t *deviceHandle;
    int loglevel = 2;
    ncStatus_t retCode = ncGlobalSetOption(NC_RW_LOG_LEVEL, &loglevel, sizeof(loglevel));

    // Initialize device handle
    retCode = ncDeviceCreate(0, &deviceHandle);
    if(retCode != NC_OK)
    {
        printf("Error - No NCS devices found.\n");
        printf("    ncStatus value: %d\n", retCode);
        exit(-1);
    }

    // Try to open the NCS device via the device name
    retCode = ncDeviceOpen(deviceHandle);
    if (retCode != NC_OK)
    {   // failed to open the device.  
        printf("Could not open NCS device\n");
        exit(-1);
    }
    
    // deviceHandle is ready to use now.  
    // Pass it to other NC API calls as needed and close it when finished.
    printf("Successfully opened NCS device!\n");

    // Now read in a graph file
    struct ncGraphHandle_t *graphHandle;
    unsigned int graphFileLen;
    void* graphFileBuf = LoadFile(GRAPH_FILE_NAME, &graphFileLen);

    // Init graph handle
    retCode = ncGraphCreate("graph", &graphHandle);
    if (retCode != NC_OK)
    {
        printf("Error - ncGraphCreate failed\n");
        exit(-1);
    }

    // Send graph to device
    retCode = ncGraphAllocate(deviceHandle, graphHandle, graphFileBuf, graphFileLen);
    if (retCode != NC_OK)
    {   // error allocating graph
        printf("Could not allocate graph for file: %s\n", GRAPH_FILE_NAME); 
        printf("Error from ncGraphAllocate is: %d\n", retCode);
    }
    else
    {   // successfully allocated graph.  Now graphHandle is ready to go.  
        // use graphHandle for other API calls and call mvncDeallocateGraph
        // when done with it.
        printf("Successfully allocated graph for %s\n", GRAPH_FILE_NAME);

        // Read tensor descriptors
        struct ncTensorDescriptor_t inputTensorDesc;
        struct ncTensorDescriptor_t outputTensorDesc;
        unsigned int length = sizeof(struct ncTensorDescriptor_t);
        ncGraphGetOption(graphHandle, NC_RO_GRAPH_INPUT_TENSOR_DESCRIPTORS, &inputTensorDesc,  &length);
        ncGraphGetOption(graphHandle, NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS, &outputTensorDesc,  &length);
        int dataTypeSize = outputTensorDesc.totalSize/(outputTensorDesc.w* outputTensorDesc.h*outputTensorDesc.c*outputTensorDesc.n);
        //printf("output data type size %d \n", dataTypeSize);

        // Init & Create Fifos
        struct ncFifoHandle_t * bufferIn;
        struct ncFifoHandle_t * bufferOut;
        retCode = ncFifoCreate("FifoIn0",NC_FIFO_HOST_WO, &bufferIn);
        if (retCode != NC_OK)
        {
            printf("Error - Input Fifo Initialization failed!");
            exit(-1);
        }
        retCode = ncFifoAllocate(bufferIn, deviceHandle, &inputTensorDesc, 2);
        if (retCode != NC_OK)
        {
            printf("Error - Input Fifo allocation failed!");
            exit(-1);
        }
        retCode = ncFifoCreate("FifoOut0",NC_FIFO_HOST_RO, &bufferOut);
        if (retCode != NC_OK)
        {
            printf("Error - Output Fifo Initialization failed!");
            exit(-1);
        }
        retCode = ncFifoAllocate(bufferOut, deviceHandle, &outputTensorDesc, 2);
        if (retCode != NC_OK)
        {
            printf("Error - Output Fifo allocation failed!");
            exit(-1);
        }

        // LoadImage will read image from disk, convert channels to floats
        // subtract network mean for each value in each channel.
        unsigned int imageSize;
        float* image = LoadImage(IMAGE_FILE_NAME, networkDim, networkMean, &imageSize);

        // Write tensor to input fifo
        retCode = ncFifoWriteElem(bufferIn, image, &imageSize, 0);
        if (retCode != NC_OK)
        {
            printf("Error - Failed to write element to Fifo!\n");
            exit(-1);
        }
        else
        {   // the inference has been started, now call mvncGetResult() for the
            // inference result
            printf("Successfully loaded the tensor for image %s\n", IMAGE_FILE_NAME);

            // queue inference
            retCode = ncGraphQueueInference(graphHandle, &bufferIn, 1, &bufferOut, 1);
            if (retCode != NC_OK)
            {
                printf("Error - Failed to queue Inference!");
                exit(-1);
            }
            free(image);
            // Read output results
            unsigned int outputDataLength;
            length = sizeof(unsigned int);
            retCode = ncFifoGetOption(bufferOut, NC_RO_FIFO_ELEMENT_DATA_SIZE, &outputDataLength, &length);
            if (retCode || length != sizeof(unsigned int)){
                printf("ncFifoGetOption failed, rc=%d\n", retCode);
                exit(-1);
            }
            void *result = malloc(outputDataLength);
            if (!result) {
                printf("malloc failed!\n");
                exit(-1);
            }
            void *userParam;
            retCode = ncFifoReadElem(bufferOut, result, &outputDataLength, &userParam);
            if (retCode != NC_OK)
            {
                printf("Error - Read Inference result failed!");
                exit(-1);
            }
            else
            {   // Successfully got the result.  The inference result is in the buffer pointed to by resultData
                printf("Successfully got the inference result for image %s\n", IMAGE_FILE_NAME);
                unsigned int numResults =  outputDataLength / sizeof(float);
                printf("resultData length is %d \n", numResults);
                float *fresult = (float*) result;

                float maxResult = 0.0;
                int maxIndex = -1;
                for (int index = 0; index < numResults; index++)
                {
                    // printf("Category %d is: %f\n", index, resultData32[index]);
                    if (fresult[index] > maxResult)
                    {
                        maxResult = fresult[index];
                        maxIndex = index;
                    }
                }
                printf("Index of top result is: %d\n", maxIndex);
                printf("Probability of top result is: %f\n", fresult[maxIndex]);
            }
            free(result);
        }

        retCode = ncGraphDestroy(&graphHandle);
        if (retCode != NC_OK)
        {
            printf("Error - Failed to deallocate graph!");
            exit(-1);
        }
        retCode = ncFifoDestroy(&bufferOut);
        if (retCode != NC_OK)
        {
            printf("Error - Failed to deallocate fifo!");
            exit(-1);
        }
        retCode = ncFifoDestroy(&bufferIn);
        if (retCode != NC_OK)
        {
            printf("Error - Failed to deallocate fifo!");
            exit(-1);
        }
        graphHandle = NULL;
    }

    free(graphFileBuf);
    retCode = ncDeviceClose(deviceHandle);
    deviceHandle = NULL;
}
