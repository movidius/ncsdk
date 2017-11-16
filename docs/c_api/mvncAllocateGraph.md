# mvncAllocateGraph()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncOpenDevice](mvncOpenDevice.md), [mvncDeallocateGraph](mvncDeallocateGraph.md)

## Overview
This function allocates a graph on the specified device and creates a handle to the graph that can be passed to other API functions such as mvncLoadTensor() and mvncGetResult(). When the caller is done with the graph, the mvncDeallocateGraph() function should be called to free the resources associated with the graph.

## Prototype

```C
mvncStatus mvncAllocateGraph(void *deviceHandle, void **graphHandle, const void *graphFile, unsigned int graphFileLength);
```
## Parameters

Name|Type|Description
----|----|-----------
deviceHandle|void\*|The deviceHandle pointer to the opaque device datatype (which was created via mvncOpenDevice()) on which the graph should be allocated.
graphHandle|void\*\*|Address of a pointer that will be set to point to an opaque graph datatype. Upon successful return, this graphHandle can be passed to other API funtions.
graphFile|const void\* | Pointer to a buffer that contains the contents of a graph file. The graph file is a compiled neural network file that gets created by the MvNCCompile SDK tool.  
graphFileLength|unsigned int|The number of bytes allocated for the buffer that graphFile points to.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues
- Be sure to call mvncDeallocateGraph() for the graphHandle returned from this function.

## Example
```C

// Graph file name
#define GRAPH_FILE_NAME "graphfile"

int main(int argc, char** argv)
{
    void* deviceHandle; 
    
    //
    // Assume NCS device opened here and deviceHandle is valid now.
    //
    
    // Now read in a graph file so graphFileBuf will point to the 
    // bytes of the file in a memory buffer and graphFileLen will be
    // set to the number of bytes in the graph file and memory buffer.
    unsigned int graphFileLen;
    void* graphFileBuf = LoadFile(GRAPH_FILE_NAME, &graphFileLen);

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
        
        retCode = mvncDeallocateGraph(graphHandle);
        graphHandle = NULL;
    }
    
    free(graphFileBuf);
    retCode = mvncCloseDevice(deviceHandle);
    deviceHandle = NULL;
}

```
