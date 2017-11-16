# mvncOpenDevice()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncCloseDevice](mvncCloseDevice.md), [mvncGetDeviceName](mvncGetDeviceName.md) [mvncGetDeviceOption](mvncGetDeviceOption.md), [mvncSetDeviceOption](mvncSetDeviceOption.md)

## Overview
This function is used to initialize the Intel® Movidius™ NCS device and return a device handle that can be passed to other API functions.

## Prototype

```C
mvncStatus mvncOpenDevice(const char *name, void **deviceHandle);
```

## Parameters

Name|Type|Description
----|----|------------
name|const char\*|Pointer to a constant array of chars that contains the name of the device to open. This value is obtained from mvncGetDeviceName.
deviceHandle|void\*\*|Address of a pointer that will be set to point to an opaque structure representing an NCS device.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
In the example below, the code gets the name of the first device and then calls mvncOpenDevice to open the device and set the deviceHandle variable for use to other API calls that expect a device handle for an open device.
```C++
#include <stdio.h>
#include <stdlib.h>

extern "C" 
{
#include <mvnc.h>
}
// Somewhat arbitrary buffer size for the device name.
#define NAME_SIZE 100
int main(int argc, char** argv)
{
    mvncStatus retCode;
    void *deviceHandle;
    char devName[NAME_SIZE];
    retCode = mvncGetDeviceName(0, devName, NAME_SIZE);
    if (retCode != MVNC_OK)
    {   // If failed to get device name, may be none plugged in.
        printf("No NCS devices found\n");
        exit(-1);
    }
    
    // Try to open the NCS device via the device name.
    retCode = mvncOpenDevice(devName, &deviceHandle);
    if (retCode != MVNC_OK)
    {   // Failed to open the device.  
        printf("Could not open NCS device\n");
        exit(-1);
    }
    
    // deviceHandle is ready to use now.  
    // Pass it to other NC API calls as needed, and close it when finished.
    printf("Successfully opened NCS device!\n");
    
    retCode = mvncCloseDevice(deviceHandle);
}

```
