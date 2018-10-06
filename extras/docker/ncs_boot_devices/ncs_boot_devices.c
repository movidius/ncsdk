/*
*
* Copyright (c) 2017-2018 Intel Corporation. All Rights Reserved
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include <stdio.h>
#include <stdlib.h>
#include <mvnc.h>

int help()
{
    fprintf(stderr, "./ncs_boot_devices [-l <loglevel>] [-n <count>]\n");
    fprintf(stderr, "                <loglevel> API log level\n");
    fprintf(stderr, "                <count> is the number of devices to boot, default is all (0). \n");
    return 0;
}

int main(int argc, char** argv)
{
    struct ncDeviceHandle_t *deviceHandle;
    int number_of_devices_to_boot = 0;
    int loglevel = 2;
    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            if (argc <= i+1) {
                printf("Error: missing argument for option %s\n", argv[i]);
                return help();
            }
            if (argv[i][1] == 'l') {
                loglevel = atoi(argv[++i]);
                continue;
            }
            else if (argv[i][1] == 'n') {
                number_of_devices_to_boot  = atoi(argv[++i]);
                continue;
            }
        } 
        else {
            printf("Error: unsupported option %s\n", argv[i]);
            return help();
        }
    }

    ncStatus_t retCode = ncGlobalSetOption(NC_RW_LOG_LEVEL, &loglevel, sizeof(loglevel));

    int idx = 0;
    int successfully_booted = 0;
    while (!number_of_devices_to_boot || idx < number_of_devices_to_boot) {
        retCode = ncDeviceCreate(idx,&deviceHandle);
        if(retCode != NC_OK) {
            printf("No more NCS devices found.\n");
            break;
        }

        retCode = ncDeviceOpen(deviceHandle);
        if(retCode != NC_OK) {
            printf("Warning: ncDeviceOpen of device %d failed (ncStatus %d), skipping\n", retCode, idx);
            idx++;
            continue;
        }
 
        printf("Hello NCS! Device %d opened normally.\n", idx);
    
        retCode = ncDeviceClose(deviceHandle);
        deviceHandle = NULL;
        if (retCode != NC_OK) {
            printf("Error: ncDeviceClose of device %d failed (ncStatus %d), exiting...\n", retCode, idx);
            printf("    ncStatus value: %d\n", retCode);
            exit(-1);
        }
        idx++;
        successfully_booted++;
    }
    if (successfully_booted) {
        printf("Successfully Booted %d devices\n", successfully_booted);
        if (idx < number_of_devices_to_boot)
            printf("Warning: Number of devices connected (%d) is smaller than requested (%d)\n", idx, number_of_devices_to_boot);
    }
    else
        printf("Error: Failed to boot any devices!\n");
}
