# mvncCloseDevice()

Type|Function
------------ | -------------
Header|mvnc.h
Library| libmvnc.so
Return|[mvncStatus](mvncStatus.md)
Version|1.0
See also|[mvncOpenDevice](mvncOpenDevice.md), [mvncGetDeviceOption](mvncGetDeviceOption.md), [mvncSetDeviceOption](mvncSetDeviceOption.md)

## Overview
This function is used to cease communication and reset the device.

## Prototype

```C
mvncStatus mvncCloseDevice(void *deviceHandle);
```
## Parameters

Name|Type|Description
----|----|-----------
deviceHandle|void*|Pointer to the opaque NCS device structure that was allocated and returned from the mvncOpenDevice function.

## Return
This function returns an appropriate value from the [mvncStatus](mvncStatus.md) enumeration.

## Known Issues

## Example
```C
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
    {   // failed to get device name, maybe none plugged in.
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
    // Pass it to other NC API calls as needed and close it when finished.
    printf("Successfully opened NCS device!\n");
    
    // Close the device previously opened by mvncOpenDevice().
    retCode = mvncCloseDevice(deviceHandle);
}
```
