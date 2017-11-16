# Device.GetDeviceOption()

|Info      | Value |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |[Device.SetDeviceOption()](Device.SetDeviceOption.md) <br>[DeviceOption](DeviceOption.md)|

## Overview
This function is used to get an option for the device. The options can be found in the [DeviceOption](DeviceOption.md) enumeration table.  

## Syntax
```python
GetDeviceOption(option)
```

## Parameters

|Parameter      | Description |
|---------------|---------------|
| option        | Member of the DeviceOption enumeration that specifies which option to get.|

## Return
The value for the specified device option. The type of the returned value depends on the option specified. See the [DeviceOption](DeviceOption.md) enumeration for the type that will be returned for each option.

## Known Issues

## Example
```python
import mvnc.mvncapi as ncs

# Initialize and open device

# Get the device option THERMAL_THROTTLING_LEVEL
optionValue = device.GetDeviceOption(mvnc.DeviceOption.THERMAL_THROTTLING_LEVEL)

```
