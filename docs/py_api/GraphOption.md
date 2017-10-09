# GraphOption enumeration class

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |  Graph.SetGraphOption()<br>Graph.GetGraphOption()|



## Overview
The GraphOption class is an enumeration class that defines the options that are passed to and received from the SetDeviceOption and the GetDeviceOption functions.


enum| option type | possible values|get/set|Description
--- | ----------- | -------------- |-------|-----------
DONTBLOCK |integer |0 or 1|get/set|0: LoadTensor and GetResult Block<br>1: LoadTensor returns BUSY instead of blocking. GetResult will return NODATA instead of blocking.
TIMETAKEN |string |any|get|Return a NumPy float array [numpy.array()] of inference times per layer in float data type.
DEBUGINFO | string |any|get|Return a string with the error text as returned by the device.
