# mvncGraphOptions enumeration

Info | Value/s
------------ | -------------
Header|mvnc.h
Version|1.0
See also|[mvncGetGraphOption](mvncGetGraphOption.md), [mvncSetGraphOption](mvncSetGraphOption.md), [mvncAllocateGraph](mvncAllocateGraph.md) 

## Overview


This enumeration is used to specify an option on the graph that can be written or read via mvncGetGraphOption() and mvncSetGraphOption(). The table below provides details on the meaning of each of the values in the enumeration.

Constant | Option Type | Possible Values | Get/Set | Description
-------- | ------------| --------------- | ------- | -----------
MVNC_DONT_BLOCK| int |0, 1|get, set|0: Calls to mvncLoadTensor and mvncGetResult will block (won't return until the action is completed) (Default)<br>1: Calls to those functions don't block (they will return immediately).  If the action coudn't be completed the return value will indicate why.  mvncLoadTensor() will return MVNC_BUSY when the NCS isn't able to perform the action because its busy, try again later.  mvncGetResult() will return MVNC_NO_DATA unless there is an inference that is ready to be returned.  In this case try again later and when there is a completed inference the results will be returned.
MVNC_TIME_TAKEN| float\* | any | get |Time taken for the last inference returned by mvncGetResult.
MVNC_DEBUG_INFO| char\* | any | get | A string that provides more details when the result of a function call was MVNC_MYRIADERROR.
