# DeviceOption Enumeration Class

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |[Device.SetDeviceOption()](Device.SetDeviceOption.md) <br>[Device.GetDeviceOption()](Device.GetDeviceOption.md)|



## Overview
The DeviceOption class is an enumeration class that defines the options that are passed to and received from the SetDeviceOption and the GetDeviceOption functions.


enum                     | option type | possible values|get/set|Description
------------------------ | ----------- | -------------- |-------|-----------
THERMAL_THROTTLING_LEVEL | int         | 1, 2           | get   |1: if lower guard temperature threshold of chip sensor is reached. This indicates short throttling time is in action between inferences to protect the device. <br>2: if upper guard temperature of chip sensor is reached. This indicates long throttling time is in action between inferences to protect the device.

