# Device.AllocateGraph()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also|[Graph](Graph.md)<br>[Graph.DeallocateGraph()](Graph.DeallocateGraph.md)<br>|

## Overview
This function is used to create an instance of a Graph that represents a neural network, which can be used to infer results via methods Graph.LoadTensor() and Graph.GetResult().

## Syntax

```python
dev.AllocateGraph(graphPath)
```

## Parameters

|Parameter      | Description |
|---------------|---------------|
|graphPath      | A string that is the path to the graph file. The graph file must have been created with the NC SDK graph compiler.|

## Return
Returns an instance of a Graph object that is ready to use.  

## Known Issues
After the Graph that is created is no longer needed, Graph.DeallocateGraph() must be called to free the graph resources. 

## Example
```python

from mvnc import mvncapi as ncs
# Enumerate Devices
device_List = ncs.Enumerate()

# Initialize and open the first device
device = ncs.Device(device_List[0])
device.OpenDevice()

# Allocate a graph on the device by specifying the path to a graph file 
graph = device.AllocateGraph("../networks/myNetwork/graph")

# Use graph here

# Deallocate the graph to free resources
graph.DeallocateGraph()

#close device
```
