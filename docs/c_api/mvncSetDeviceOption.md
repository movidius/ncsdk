# mvncSetDeviceOption()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncOpenDevice](mvncOpenDevice.md), [mvncDeviceOptions](mvncDeviceOptions.md), [mvncGetDeviceOption](mvncGetDeviceOption.md)

## Overview
This function sets an option for a specific Intel® Movidius™ Neural Compute Stick (Intel® Movidius™ NCS) device. The available options can be found in the [DeviceOptions](mvncDeviceOptions.md) enumeration.

## Prototype

```C
mvncStatus mvncSetDeviceOption(void *deviceHandle, int option, const void *data, unsigned int datalength);
```
## Parameters

Name|Type|Description
----|----|-----------
deviceHandle|void\*|Pointer to opaque device data type that was initialized with the mvncOpenDevice() function. This specifies which device's option will be set.
option|int|A value from the DeviceOptions enumeration that specifies which option will be set.
data|const void\*|Pointer to the data for the new value for the option. The type of data this points to depends on the option that is being set. Check mvncDeviceOptions for the data types that each option requires.
dataLength|unsigned int|An unsigned int that contains the length, in bytes, of the buffer that the data parameter points to.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
```C
TBD
```
