# DeviceOption enumeration class

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |Device.SetDeviceOption()<br>Device.GetDeviceOption()|



## Overview
The DeviceOption class is an enumeration class that defines the options that are passed to and received from the SetDeviceOption and the GetDeviceOption functions.


Enumerator Values|Description
------------ | -------------
THERMAL_THROTTLING_LEVEL |Returns 1 if lower guard temperature threshold of chip sensor is reached. This indicates short throttling time is in action between inferences to protect the device. Returns 2 if upper guard temperature of chip sensor is reached. This indicates long throttling time is in action between inferences to protect the device. 
