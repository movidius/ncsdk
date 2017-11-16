# Graph.SetGraphOption()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also|[Graph.GetGraphOption()](Graph.GetGraphOption.md) <br>[GraphOption](GraphOption.md)|

## Overview
This function is used to set a graph option. The available options can be found in the [GraphOption](GraphOption.md) enumeration class. 

## Syntax
```python
graph.SetGraphOption(option, value)
```

## Parameters

Parameter      | Description
---------------|---------------
option         | Member of the GraphOption enumeration specifying which option's value will be set.
value          | The new value to which the specified graph option will be set. See the [GraphOption](GraphOption.md) enumeration class for the type of value for each option.


## Return
None.

## Known Issues

## Example
```python
import mvnc.mvncapi as ncs
deviceNames = ncs.EnumerateDevices()
ncsDevice = ncs.Device(deviceNames[0])
ncsDevice.OpenDevice()
graph = ncsDevice.AllocateGraph("../networks/myNetwork/graph")

graph.SetGraphOption(mvnc.GraphOption.DONTBLOCK, 1)

# Use device here

graph.DeallocateGraph()
ncsDevice.CloseDevice()    
```
