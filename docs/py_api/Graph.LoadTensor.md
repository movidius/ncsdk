# Graph.LoadTensor()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |[Graph.GetResult()](Graph.GetResult.md)|

## Overview
This function initiates an inference on the specified graph via the associated Intel® Movidius™ NCS device. After calling this function, use the Graph.GetResult() function to retrieve the inference result.

## Syntax

```python
graph.LoadTensor(inputTensor, userObject)
```
## Parameters

|Parameter      | Description |
|---------------|---------------|
|inputTensor   |  Input data on which an inference will be run. The data must be passed in a NumPy ndarray of half precision floats (float 16). |         |
|userObject    |  A user-defined parameter that is returned by the GetResult function along with the inference result for this tensor.|

## Return
Returns True if the function works, False if not. When the graph is in non-blocking mode (GraphOption.DONTBLOCK), this function will return False if the device is busy. 

## Known Issues

## Example
```python

# Enumerate Device
# Open Device, # Allocate Graph, # Set Graph Option
# Read an image, resize the image and adjust for mean if necessary to match the network expected size

if (graph.LoadTensor(img.astype(numpy.float16), 'user object')):
    print("LoadTensor success")
    output, userobj = graph.GetResult()

# Deallocate the graph and Close the device
```
