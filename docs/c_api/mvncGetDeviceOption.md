# mvncGetDeviceOption()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncOpenDevice](mvncOpenDevice.md), [mvncDeviceOptions](mvncDeviceOptions.md), [mvncSetDeviceOption](mvncSetDeviceOption.md)

## Overview
This function gets the current value of an option for an Intel® Movidius™ Neural Compute Stick (Intel® Movidius™ NCS) device. The available options and their data types can be found in the [DeviceOptions](mvncDeviceOptions.md) enumeration documentation.

## Prototype

```C
mvncStatus mvncGetDeviceOption(void *deviceHandle, int option, void *data, unsigned int *datalength);
```
## Parameters

Name|Type|Description
----|----|-----------
deviceHandle|void\*|Pointer to opaque device data type that was initialized with the mvncOpenDevice() function. This specifies which NCS device's option will be retrieved.
option|int|A value from the DeviceOptions enumeration that specifies which option will be retrieved.
data|void\*|Pointer to a buffer where the value of the option will be copied. The type of data this points to will depend on the option that is specified. Check mvncDeviceOptions for the data types that each option requires.
dataLength|unsigned int\*|Pointer to an unsigned int which must point to the size, in bytes, of the buffer allocated to the data parameter when called. Upon successfull return, it will be set to the number of bytes copied to the data buffer.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
```C
TBD
```
