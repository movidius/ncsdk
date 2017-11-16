# mvncGetGraphOption()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncAllocateGraph](mvncAllocateGraph.md), [mvncGraphOptions](mvncGraphOptions.md), [mvncSetGraphOption](mvncSetGraphOption.md)

## Overview
This function gets the current value of an option for a graph. The available options can be found in the GraphOptions enumeration.

## Prototype

```C
mvncStatus mvncGetGraphOption(void *graphHandle, int option, void **data, unsigned int *datalength);
```
## Parameters

Name|Type|Description
----|----|-----------
graphHandle|void\*|Pointer to opaque graph data type that was initialized with the mvncAllocateGraph() function, which represents the neural network. This specifies which graph's option value will be retrieved.
option|int|A value from the GraphOptions enumeration that specifies which option will be retrieved.
data|void\*|Pointer to a buffer where the value of the option will be copied. The type of data this points to will depend on the option that is specified. Check mvncGraphOptions for the data types that each option requires.
dataLength|unsigned int\*|Pointer to an unsigned int, which must point to the size, in bytes, of the buffer allocated to the data parameter when called. Upon successfull return, it will be set to the number of bytes copied to the data buffer.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
```C++
.
.
.
    // Open device to get device handle, 
    // allocate the graph to get graph handle.

    // Set the graph option for blocking calls
    int dontBlockValue;
    unsigned int sizeOfValue;
    retCode = mvncGetGraphOption(graphHandle, MVNC_DONTBLOCK, (void**)(&dontBlockValue), &sizeOfValue);
    if (retCode == MVNC_OK)
    {
        printf("Successfully got graph option, value is: %d\n", dontBlockValue);
    }
    else
    {
        printf("Could not get graph option\n");
        printf("Error returned from mvncGetGraphOption: %d\n", retCode);
    }

    // Use graph, deallocate graph, close device, etc.
    
    .
    .
    .

```
