# EnumerateDevices()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |  [Device](Device.md)       |

## Overview
This function is used to get a list of the names of the devices present in the system. Each of the names returned can be used to create an instance of the Device class. 

## Syntax

```python
deviceNames = EnumerateDevices()
```

## Parameters
None.

## Return
An array of device names, each of which can be used to create a new instance of the Device class.

## Known Issues

## Example
```Python
import mvnc.mvncapi as ncs
deviceNames = ncs.EnumerateDevices()
if len(deviceNames) == 0:
	print("Error - No devices detected.")
	quit()

# Open first NCS device found
device = ncs.Device(deviceNames[0])	
	
# Allocate graph / otherwise use device as needed

# Deallocate graph if allocated

device.CloseDevice()
```
