// Copyright (c) 2017-2018 Intel Corporation. All Rights Reserved
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.



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
