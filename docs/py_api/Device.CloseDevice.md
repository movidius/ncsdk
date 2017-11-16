# Device.CloseDevice()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also|[Device.\_\_init\_\_()](Device.__init__.md) <br>[Device.OpenDevice()](Device.OpenDevice.md)|

## Overview
This function is used to cease communication and reset the device.

## Syntax
```python
dev.CloseDevice()
```

## Parameters
None.

## Return
None.

## Known Issues

## Example
```Python
# Open and close the first NCS device
import mvnc.mvncapi as ncs

deviceNames = ncs.EnumerateDevices()
firstDevice = ncs.Device(deviceNames[0])

# Open the device
firstDevice.OpenDevice()

# Use device here

# Close the device 
firstDevice.CloseDevice()
```
