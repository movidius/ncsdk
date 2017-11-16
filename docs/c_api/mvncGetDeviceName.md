# mvncGetDeviceName()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncOpenDevice](mvncOpenDevice.md) 

## Overview
This function is used to get the name of a particular Intel® Movidius™ NCS device. Typical usage is to call the function repeatedly, starting with index = 0, and incrementing the index each time until an error is returned. These successive calls will give you the names of all the devices in the system.

## Prototype

```C
mvncStatus mvncGetDeviceName(int index, char* name, unsigned int nameSize);
```
## Parameters

Name|Type|Description
----|----|-----------
index|int|Index of the device for which the name should be retrieved.
name|char\*|Pointer to a character buffer into which the name will be copied. This buffer should be allocated by the caller.
nameSize|unsigned int|The number of characters allocated to the buffer pointed to by the name parameter.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
The following example shows how to get the name of all Intel Movidius NCS devices attached to the system. mvncGetDeviceName is called repeatedly until it returns MVNC_DEVICE_NOT_FOUND.

```C++
#include <stdio.h>
extern "C" 
{
#include <mvnc.h>
}
// somewhat arbitrary buffer size for the device name
#define NAME_SIZE 100
int main(int argc, char** argv)
{
    mvncStatus retCode;
    int deviceCount = 0;
    char devName[NAME_SIZE];
    while ((retCode = mvncGetDeviceName(deviceCount, devName, NAME_SIZE)) != MVNC_DEVICE_NOT_FOUND)
    {
        printf("Found NCS device named: \"%s\"\n", devName); 
        deviceCount++;
    }
    printf("Total number of NCS devices found: %d\n", deviceCount);
}
```
Output from the example code above with two Intel Movidius NCS devices in the system.

```
Found NCS device named: "2.3"
Found NCS device named: "1"
Total number of NCS devices found: 2
```
