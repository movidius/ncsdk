// Copyright 2017 Intel Corporation. 
// The source code, information and material ("Material") contained herein is  
// owned by Intel Corporation or its suppliers or licensors, and title to such  
// Material remains with Intel Corporation or its suppliers or licensors.  
// The Material contains proprietary information of Intel or its suppliers and  
// licensors. The Material is protected by worldwide copyright laws and treaty  
// provisions.  
// No part of the Material may be used, copied, reproduced, modified, published,  
// uploaded, posted, transmitted, distributed or disclosed in any way without  
// Intel's prior express written permission. No license under any patent,  
// copyright or other intellectual property rights in the Material is granted to  
// or conferred upon you, either expressly, by implication, inducement, estoppel  
// or otherwise.  
// Any license under such intellectual property rights must be express and  
// approved by Intel in writing. 



#include <stdio.h>
#include <stdlib.h>

#include <mvnc.h>

// somewhat arbitrary buffer size for the device name
#define NAME_SIZE 100


int main(int argc, char** argv)
{
    mvncStatus retCode;
    void *deviceHandle;
    char devName[NAME_SIZE];
    retCode = mvncGetDeviceName(0, devName, NAME_SIZE);
    if (retCode != MVNC_OK)
    {   // failed to get device name, maybe none plugged in.
        printf("Error - No NCS devices found.\n");
	printf("    mvncStatus value: %d\n", retCode);
        exit(-1);
    }
    
    // Try to open the NCS device via the device name
    retCode = mvncOpenDevice(devName, &deviceHandle);
    if (retCode != MVNC_OK)
    {   // failed to open the device.  
        printf("Error - Could not open NCS device.\n");
	printf("    mvncStatus value: %d\n", retCode);
        exit(-1);
    }
    
    // deviceHandle is ready to use now.  
    // Pass it to other NC API calls as needed and close it when finished.
    printf("Hello NCS! Device opened normally.\n");

    retCode = mvncCloseDevice(deviceHandle);
    deviceHandle = NULL;
    if (retCode != MVNC_OK)
    {
        printf("Error - Could not close NCS device.\n");
	printf("    mvncStatus value: %d\n", retCode);
	exit(-1);
    }

    printf("Goodbye NCS!  Device Closed normally.\n");
    printf("NCS device working.\n");
}
