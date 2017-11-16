
# mvncGetGlobalOption()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.09
See also|[mvncGlobalOptions](mvncGlobalOptions.md), [mvncSetGlobalOption](mvncSetGlobalOption.md)

## Overview
This function gets the current value of an option that is global to the SDK. The available options and their data types can be found in the [mvncGlobalOptions](mvncGlobalOptions.md) enumeration documentation.

## Prototype

```C
mvncStatus mvncGetGlobalOption(int option, void *data, unsigned int *datalength);
```
## Parameters

Name|Type|Description
----|----|-----------
option|int|A value from the GlobalOptions enumeration that specifies which option will be retrieved.
data|void\*|Pointer to a buffer where the value of the option will be copied. The type of data this points to will depend on the option that is specified. Check mvncGlobalOptions for the data types that each option requires.
dataLength|unsigned int\*|Pointer to an unsigned int which must point to the size, in bytes, of the buffer allocated to the data parameter when called. Upon successfull return, it will be set to the number of bytes copied to the data buffer.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
```C
int len;
int loggingLevel;
status = mvncGetGlobalOption(MVNC_LOGLEVEL, &loggingLevel, &len);
// loggingLevel has the option value now.
```
