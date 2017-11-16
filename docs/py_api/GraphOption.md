# GraphOption Enumeration Class

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |  [Graph.SetGraphOption()](Graph.SetGraphOption.md) <br>[Graph.GetGraphOption()](Graph.GetGraphOption.md)|



## Overview
The GraphOption class is an enumeration class that defines the options that are passed to and received from the SetDeviceOption and the GetDeviceOption functions.


enum| option type | possible values|get/set|Description
--- | ----------- | -------------- |-------|-----------
DONTBLOCK |int    |0 or 1|get/set|0: Calls to Graph.LoadTensor() and Graph.GetResult() will block (won't return until the action is completed) (Default)<br>1: Calls to those functions don't block (they will return immediately).  If the action coudn't be completed the return value will indicate why.  Graph.LoadTensor() will return MVNC_BUSY when the NCS isn't able to perform the action because its busy, try again later.  Graph.GetResult() will return MVNC_NO_DATA unless there is an inference that is ready to be returned.  In this case try again later and when there is a completed inference the results will be returned.
TIMETAKEN |string |any|get|Return a NumPy float array [numpy.array()] of inference times per layer in float data type.
DEBUGINFO |string |any|get|Return a string with the error text as returned by the device.
