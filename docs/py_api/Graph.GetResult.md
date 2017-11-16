# Graph.GetResult()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |[Graph.LoadTensor()](Graph.LoadTensor.md)|

## Overview
This function retrieves the result of an inference that was initiated via Graph.LoadTensor() on the specified graph.  

## Syntax
```python
inferenceResult, userObj = graph.GetResult()
```

## Parameters
None.

## Return
The inference result and user object that was specified in the Graph.LoadTensor() call that initiated this inference.

## Known Issues

## Example
```python

# Open NCS device and allocate a graph

graph.LoadTensor(img.astype(numpy.float16), "user object for this inference")
inferenceResult, userObj = graph.GetResult()

# Deallocate graph
# Close device

```
