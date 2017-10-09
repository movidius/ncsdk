# mvncDeviceOptions enumeration

Info | Value/s
------------ | -------------
Header|mvnc.h
Version|1.0
See also|[mvncGetDeviceOption](mvncGetDeviceOption.md), [mvncSetDeviceOption](mvncSetDeviceOption.md), [mvncOpenDevice](mvncOpenDevice.md) 

## Overview

This enumeration is used to specify an option on the NCS device that can be written or read via mvncGetDeviceOption() and mvncSetDeviceOption().  The table below provides details on the meaning of each of the values in the enumeration.

constant | Option Type | Possible Values | get/set | Description
-------- | ------------| --------------- | ------- | -----------
MVNC_THERMAL_THROTTLING_LEVEL|int|1, or 2|get|Returns 1 if lower guard temperature threshold of chip sensor is reached. This indicates short throttling time is in action between inferences to protect the device. Returns 2 if upper guard temperature of chip sensor is reached.  This indicates long throttling time is in action between inferences to protect the device.
