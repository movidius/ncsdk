# mvncLoadTensor()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncOpenDevice](mvncOpenDevice.md), [mvncAllocateGraph](mvncAllocateGraph.md), [mvncGetResult](mvncGetResult.md)

## Overview
This function initiates an inference on the specified graph via the associated Intel® Movidius™ Neural Compute Stick (Intel® Movidius™ NCS) device. After calling this function, use the mvncGetResult() function to retrieve the inference result.

## Prototype

```C
mvncStatus mvncLoadTensor(void *graphHandle, const void *inputTensor, unsigned int inputTensorLength, void *userParam);
```
## Parameters

Name|Type|Description
----|----|-----------
graphHandle|void\*|Pointer to opaque graph data type that was initialized with the mvncAllocateGraph() function that represents the neural network for which an inference will be initiated.
inputTensor|const void\*|Pointer to tensor data buffer, which contains 16-bit half precision floats (per IEEE 754 half precision binary floating-point format: binary16). The values in the buffer are dependent on the neural network (graph), but are typically representations of each color channel of each pixel of an image.
inputTensorLength|unsigned int|The length, in bytes, of the buffer pointed to by the inputTensor parameter.
userParam|void\*|Pointer to user data that will be returned along with the inference result from the GetResult() function.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
```C+
.
.
.

// Use a 16-bit unsigned type to represent half precision floats, since C++ has no 
// built-in support for 16-bit floats.
typedef unsigned short half;

// GoogleNet image dimensions and network mean values for each channel. This information is specific 
// for each network, and usually available from network creators.
const int networkDim = 224;
float networkMean[] = {0.40787054*255.0, 0.45752458*255.0, 0.48109378*255.0};

int main(int argc, char** argv)
{
.
.
.
    //
    // Open NCS device and set deviceHandle to the valid handle.
    //
    
    //
    // Read graph from disk and call mvncAllocateGraph to set graphHandle appropriately.
    //

    //
    // Load an image from disk.
    // LoadImage will read image from disk, convert channels to floats.
    // Subtract network mean for each value in each channel. Then convert
    // floats to half precision floats.
    // Return pointer to the buffer of half precision floats. 
    half* imageBufFp16 = LoadImage("image.png", networkDim, networkMean);
        
    // Calculate the length of the buffer that contains the half precision floats.
    // 3 channels * width * height * sizeof a 16-bit float 
    unsigned int lenBufFp16 = 3*networkDim*networkDim*sizeof(*imageBufFp16);

    // Start the inference with mvncLoadTensor()
    retCode = mvncLoadTensor(graphHandle, imageBufFp16, lenBufFp16, NULL);
    if (retCode == MVNC_OK)
    {   // The inference has been started, now call mvncGetResult() for the
        // inference result. 
        printf("Successfully loaded the tensor for image %s\n", "image.png");
     
        // Here mvncGetResult() can be called to get the result of the inference
        // that was started with mvncLoadTensor() above.
    }

    // 
    // Call mvncDeallocateGraph to free the resources tied to graphHandle.
    // Close the device with mvncCloseDevice().
    // 
}

```
