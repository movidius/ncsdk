# Graph Class

The Graph class is a container for a neural network graph file that is associated with a particular Intel® Movidius™ Neural Compute Stick (Intel® Movidius™ NCS) device. 

## Usage
To use the Graph class, you must create a graph handle by calling AllocateGraph() from the Device class. The location of the graph file will be passed to AllocateGraph(), and it will return an instance of the Graph class. Once you have successfully created an instance of this class, the typical usage is to optionally get/set graph options, then call LoadTensor() and GetResult() to perform inferencing with the graph that was allocated. Finally, call DeallocateGraph() when the neural network is no longer needed. 

## Graph methods
- [DeallocateGraph](Graph.DeallocateGraph.md)
- [SetGraphOption](Graph.SetGraphOption.md)
- [GetGraphOption](Graph.GetGraphOption.md)
- [LoadTensor](Graph.LoadTensor.md)
- [GetResult](Graph.GetResult.md)
