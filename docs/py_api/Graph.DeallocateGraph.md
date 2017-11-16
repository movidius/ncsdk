# Graph.DeallocateGraph()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also|[Device.AllocateGraph()](Device.AllocateGraph.md)|

## Overview
This function is used to deallocate a graph that was allocated for an Intel® Movidius™ NCS device with the Device.AllocateGraph() method.  This should be called for every graph that is created to free resources associated with the graph. 

## Syntax

```python
graph.DeallocateGraph()
```
## Parameters
None.

## Return
None.

## Known Issues
The Graph class can only be created via the Device.AllocateGraph() function.  

## Example
```python

import mvnc.mvncapi as ncs
deviceNames = ncs.EnumerateDevices()
if len(deviceNames) == 0:
	print("Error - No devices detected.")
	quit()

# Open first NCS device found
device = mvnc.Device(devices[0])

# Allocate the graph 
device.AllocateGraph("my_graph")

# Use device here

graph.DeallocateGraph()

device.CloseDevice()
```
