# mvncGetResult()

Type|Function
------------ | -------------
Header|mvnc.h
Library|libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncOpenDevice](mvncOpenDevice.md), [mvncAllocateGraph](mvncAllocateGraph.md), [mvncLoadTensor](mvncLoadTensor.md)

## Overview
This function retrieves the result of an inference that was initiated via LoadTensor() on the specified graph.  

## Prototype

```C
mvncStatus mvncGetResult(void *graphHandle, void **outputData, unsigned int *outputDataLength, void **userParam);
```
## Parameters

Name|Type|Description
----|----|-----------
graphHandle|void\*|Pointer to opaque graph data type that was initialized with the mvncAllocateGraph() function that represents the neural network for which an inference was initiated.
outputData|void\*\*|Address of the pointer that will be set to a buffer of 16-bit floats, which contain the result of the inference. The buffer will contain one 16-bit float for each network category, the values of which are the results of the output node. Typically these values are the probabilities that an image belongs to the category.
outputDataLength|unsigned int\*|Pointer to an unsigned int that will be set to the number of bytes in the outputData buffer.
userParam|void\*\*|Address of a pointer that will be set to the user parameter for this inference. This corresponds to the userParam that was passed to the LoadTensor() function, which initiated the inference.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
```C++
.
.
.

// Use a 16-bit unsigned type to represent half precision floats since C++ has no 
// built-in support for 16-bit floats.
typedef unsigned short half;

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
    // Load an image.png from disk and preprocess it to match network 
    // requirements so that imageBufFp16 list to 16-bit floats.
    //
    
    // Start the inference with call to mvncLoadTensor().
    retCode = mvncLoadTensor(graphHandle, imageBufFp16, lenBufFp16, NULL);
    if (retCode == MVNC_OK)
    {   // The inference has been started, now call mvncGetResult() for the
        // inference result. 
        printf("Successfully loaded the tensor for image %s\n", "image.png");
            
        void* resultData16;
        void* userParam;
        unsigned int lenResultData;
        retCode = mvncGetResult(graphHandle, &resultData16, &lenResultData, &userParam);
        if (retCode == MVNC_OK)
        {   // Successfully got the result. The inference result is in the buffer pointed to by resultData.
            printf("Successfully got the inference result for image %s\n", IMAGE_FILE_NAME);
            printf("resultData is %d bytes which is %d 16-bit floats.\n", lenResultData, lenResultData/(int)sizeof(half));
                
            // convert half precision floats to full floats
            int numResults = lenResultData / sizeof(half);
            float* resultData32;
            resultData32 = (float*)malloc(numResults * sizeof(*resultData32));
            fp16tofloat(resultData32, (unsigned char*)resultData16, numResults);

            float maxResult = 0.0;
            int maxIndex = -1;
            for (int index = 0; index < numResults; index++)
            {
                printf("Category %d is: %f\n", index, resultData32[index]);
                if (resultData32[index] > maxResult)
                {
                    maxResult = resultData32[index];
                    maxIndex = index;
                }
            }
            printf("index of top result is: %d - probability of top result is: %f\n", maxIndex, resultData32[maxIndex]);
            free(resultData32);
        } 
    }

    // 
    // Call mvncDeallocateGraph to free the resources tied to graphHandle.
    // Close the device with mvncCloseDevice().
    // 
}

```
