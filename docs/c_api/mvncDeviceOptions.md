# mvncDeviceOptions enumeration

Info | Value/s
------------ | -------------
Header|mvnc.h
Version|1.0
See also|[mvncGetDeviceOption](mvncGetDeviceOption.md), [mvncSetDeviceOption](mvncSetDeviceOption.md), [mvncOpenDevice](mvncOpenDevice.md) 

## Overview

This enumeration is used to specify an option on the Intel® Movidius™ NCS device that can be written or read via mvncGetDeviceOption() and mvncSetDeviceOption(). The table below provides details on the meaning of each of the values in the enumeration.

Constant | Option Type | Possible Values | Get/Set | Description
-------- | ------------| --------------- | ------- | -----------
MVNC_THERMAL_THROTTLING_LEVEL|int|1, 2|get|1: if lower guard temperature threshold of chip sensor is reached. This indicates short throttling time is in action between inferences to protect the device. <br>2: if upper guard temperature of chip sensor is reached. This indicates long throttling time is in action between inferences to protect the device.
