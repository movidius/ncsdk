# mvncGraphOptions enumeration

Info | Value/s
------------ | -------------
Header|mvnc.h
Version|1.0
See also|[mvncGetGraphOption](mvncGetGraphOption.md), [mvncSetGraphOption](mvncSetGraphOption.md), [mvncAllocateGraph](mvncAllocateGraph.md) 

## Overview


This enumeration is used to specify an option on the a graph that can be written or read via mvncGetGraphOption() and mvncSetGraphOption().  The table below provides details on the meaning of each of the values in the enumeration.

constant | Option Type | Possible Values | get/set | Description
-------- | ------------| --------------- | ------- | -----------
MVNC_DONT_BLOCK| int |0, 1|get, set|0: Calls to mvncLoadTensor and mvncGetResult block, 1: calls to those functions don't block.
MVNC_TIME_TAKEN| float\* | any | get |Time taken for the last inference returned by mvncGetResult.
MVNC_DEBUG_INFO| char\* | any | get | A string that provides more details when the result of a function call was MVNC_MYRIADERROR.
