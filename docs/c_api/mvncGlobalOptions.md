# mvncGlobalOptions enumeration

Info | Value/s
------------ | -------------
Header|mvnc.h
Version|1.09
See also|[mvncGetGlobalOption](mvncGetGlobalOption.md), [mvncSetGlobalOption](mvncSetGlobalOption.md)

## Overview

This enumeration is used to specify a global option that can be written or read via mvncGetDeviceOption() and mvncSetDeviceOption().  The table below provides details on the meaning of each of the values in the enumeration.

constant | Option Type | Possible Values | get/set | Description
-------- | ------------| --------------- | ------- | -----------
MVNC_LOGLEVEL | int |  0, 1, 2 |get, set|The logging level for application  Value meanings are: 0 = log nothing (default), 1 = log errors only, 2 = log all, verbose logging. 
