# mvncDeallocateGraph()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncOpenDevice](mvncOpenDevice.md), [mvncAllocateGraph](mvncAllocateGraph.md)

## Overview
This function deallocates a graph that was previously allocated with mvncAllocateGraph(). After successful return from this function, the passed graphHandle will be invalid and should not be used.

## Prototype

```C
mvncStatus mvncDeallocateGraph(void *graphHandle);
```
## Parameters

Name|Type|Description
----|----|-----------
graphHandle|void\*\*|Pointer to opaque graph data type that was initialized with the mvncAllocateGraph() function.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues
Using a deallocated graph handle can lead to hard-to-find bugs. To prevent this, it's good practice to set the handle to NULL (or nullptr for C++ 11) after deallocating, as shown in this code snippet:
```C++
mvncDeallocateGraph(graphHandle);
graphHandle = NULL;
```

## Example
```C
// Graph file name
#define GRAPH_FILE_NAME "graphfile"

int main(int argc, char** argv)
{
    void* deviceHandle; 
    
    .
    .
    .
    
    //
    // Assume NCS device opened here and deviceHandle is valid now
    // and the graph file is in graphFileBuf and length in bytes  
    // is in graphFileLen.
    //

    // Allocate the graph
    void* graphHandle;
    retCode = mvncAllocateGraph(deviceHandle, &graphHandle, graphFileBuf, graphFileLen);
    if (retCode != MVNC_OK)
    {   // Error allocating graph
        printf("Could not allocate graph for file: %s\n", GRAPH_FILE_NAME); 
    }
    else
    {   // Successfully allocated graph. Now graphHandle is ready to go.  
        // Use graphHandle for other API calls, and call mvncDeallocateGraph
        // when done with it.
        printf("Successfully allocated graph for %s\n", GRAPH_FILE_NAME);
        
        retCode = mvncDeallocateGraph(graphHandle);
        graphHandle = NULL;
    }
.
.
.

}
```
