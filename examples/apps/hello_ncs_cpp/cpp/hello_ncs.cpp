// Copyright 2018 Intel Corporation.
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


int main(int argc, char** argv)
{
    struct ncDeviceHandle_t *deviceHandle;
    int loglevel = 2;
    ncStatus_t retCode = ncGlobalSetOption(NC_RW_LOG_LEVEL, &loglevel, sizeof(loglevel));

    // Initialize device handle
    retCode = ncDeviceCreate(0,&deviceHandle);
    if(retCode != NC_OK)
    {
        printf("Error - No NCS devices found.\n");
        printf("    ncStatus value: %d\n", retCode);
        exit(-1);
    }

    // Open device
    retCode = ncDeviceOpen(deviceHandle);
    if(retCode != NC_OK)
    {
        printf("Error- ncDeviceOpen failed\n");
        printf("    ncStatus value: %d\n", retCode);
        exit(-1);
    }
    
    // deviceHandle is ready to use now.  
    // Pass it to other NC API calls as needed and close it when finished.
    printf("Hello NCS! Device opened normally.\n");

    retCode = ncDeviceClose(deviceHandle);
    deviceHandle = NULL;
    if (retCode != NC_OK)
    {
        printf("Error - Could not close NCS device.\n");
        printf("    ncStatus value: %d\n", retCode);
        exit(-1);
    }

    printf("Goodbye NCS!  Device Closed normally.\n");
    printf("NCS device working.\n");
}
