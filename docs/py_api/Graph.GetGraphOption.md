# Graph.GetGraphOption()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also|[Graph.SetGraphOption()](Graph.SetGraphOption.md) <br>[GraphOption](GraphOption.md)|

## Overview
This function is used to get a graph option. The available options can be found in [GraphOption](GraphOption.md) enumeration class. 

## Syntax
```python
value = graph.GetGraphOption(option)
```

## Parameters

|Parameter      | Description |
|---------------|---------------|
|option   |  A value from the GraphOption enumeration to specify which option's value should be returned. |

## Return
The value for the specified GraphOption is returned. The type of the returned value depends on the option specified. See the [GraphOption](GraphOption.md) class for the value types for each option.

## Known Issues

## Example
```python
import mvnc.mvncapi as ncs
deviceNames = ncs.EnumerateDevices()
ncsDevice = ncs.Device(deviceNames[0])
ncsDevice.OpenDevice()
graph = ncsDevice.AllocateGraph("../networks/myNetwork/graph")

# Get the graph option
optionValue = graph.GetGraphOption(mvnc.GraphOption.DONTBLOCK)

# Use device here

graph.DeallocateGraph()
ncsDevice.CloseDevice()    



```
