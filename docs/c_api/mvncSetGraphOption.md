# mvncSetGraphOption()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncAllocateGraph](mvncAllocateGraph.md), [mvncGraphOptions](mvncGraphOptions.md), [mvncGetGraphOption](mvncGetGraphOption.md)

## Overview
This function sets an option of a graph.  The available options can be found in the GraphOptions enumeration.

## Prototype

```C
mvncStatus mvncSetGraphOption(void *graphHandle, int option, const void *data, unsigned int datalength);
```
## Parameters

Name|Type|Description
----|----|-----------
graphHandle|void\*|Pointer to opaque graph data type that was initialized with the mvncAllocateGraph() function that represents the neural network.  This specifies which graph's option will be set.
option|int|A value from the GraphOptions enumeration that specifies which option will be set.
data|const void\*|Pointer to the data for the new value for the option.  The type of data this points to depends on the option that is being set.  Check mvncGraphOptions for the data types that each option requires.
dataLength|unsigned int| An unsigned int that contains the length, in bytes, of the buffer that the data parameter points to.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
```C++
.
.
.
    // Open NCS device to initialize deviceHandle.
    // Read compiled graph file into graphFileBuf and put length of it in graphFileLen

    // Allocate the graph
    void* graphHandle;
    retCode = mvncAllocateGraph(deviceHandle, &graphHandle, graphFileBuf, graphFileLen);
    if (retCode != MVNC_OK)
    {   // Error allocating graph
        printf("Could not allocate graph for file: %s\n", GRAPH_FILE_NAME); 
    }
    else
    {   // Successfully allocated graph. Now graphHandle is ready to go.  
        // Use graphHandle for other API calls and call mvncDeallocateGraph
        // when done with it.
        printf("Successfully allocated graph for %s\n", GRAPH_FILE_NAME);
        
        // Set the graph option for blocking calls
        int dontBlockValue = 0;
        retCode = mvncSetGraphOption(graphHandle, MVNC_DONTBLOCK, &dontBlockValue, sizeof(int));
        if (retCode == MVNC_OK)
        {
            printf("Successfully set graph option\n");
        }
        else
        {
            printf("Could not set graph option\n");
            printf("Error returned from mvncSetGraphOption: %d\n", retCode);
        }

        // Use graphHandle here with the option set above.
        // Then deallocate the graph and close the device.
    }
.
.
.
```
