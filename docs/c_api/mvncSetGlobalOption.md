# mvncSetGlobalOption()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.09
See also|[mvncGlobalOptions](mvncGlobalOptions.md), [mvncGetGlobalOption](mvncGetGlobalOption.md)

## Overview
This function sets an option that is global for an application using the SDK.  The available options can be found in the [mvncGlobalOptions](mvncGlobalOptions.md) enumeration.

## Prototype

```C
mvncStatus mvncSetGlobalOption(int option, const void *data, unsigned int datalength);
```
## Parameters

Name|Type|Description
----|----|-----------
option|int|A value from the mvncGlobalOptions enumeration that specifies which option will be set.
data|const void\*|Pointer to the data for the new value for the option.  The type of data this points to depends on the option that is being set.  Check mvncGlobalOptions for the data types that each option requires.
dataLength|unsigned int| An unsigned int that contains the length, in bytes, of the buffer that the data parameter points to.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
```C
int logLevel = 3;
retCode = mvncSetGlobalOption(MVNC_LOGLEVEL, &logLevel, sizeof(int));

```
