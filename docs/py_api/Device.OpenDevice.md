# Device.OpenDevice()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also|[Device.\_\_init\_\_()](Device.__init__.md) <br>[Device.CloseDevice()](Device.CloseDevice.md)|

## Overview
This function is used to initialize the device.  

## Syntax
```python
OpenDevice()
```

## Parameters
None.

## Return
None.

## Known Issues

## Example
```python
#############################################
# Open first device only
import mvnc.mvncapi as ncs
deviceNames = ncs.EnumerateDevices()
firstDevice = ncs.Device(deviceNames[0])
firstDevice.OpenDevice()
# Use device
firstDevice.CloseDevice()
#############################################


#############################################
# Open all devices
import mvnc.mvncapi as ncs
devices = ncs.EnumerateDevices()
devlist = list()
for devnum in range(len(devices)):
    devlist.append(ncs.Device(devices[devnum]))
    devlist[devnum].OpenDevice()
#
# Use devices in devlist
#
for dev in devList:
    dev.CloseDevice()
#############################################
    
```
