# Device.\_\_init\_\_()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also||

## Overview
This function is used to initialize a device object.

## Syntax

```python
mvnc.Device("device name here")
```

## Parameters

|Parameter      | Description |
|---------------|---------------|
|deviceName     | The name of the device to initialize. This must come from calling mvncapi module function EnumerateDevices().|

## Return 
None.

## Known Issues

## Example
```python
import mvnc.mvncapi as ncs

# Enumerate devices
deviceNames = ncs.EnumerateDevices()

# Create and init a device instance
ncsDevice = ncs.Device(deviceNames[0])

# Open device, use device, close device

```
