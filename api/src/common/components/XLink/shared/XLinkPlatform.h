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

///
/// @brief     Application configuration Leon header
///

#ifndef _XLINK_USBLINKPLATFORM_H
#define _XLINK_USBLINKPLATFORM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_POOLS_ALLOC 32
#define PACKET_LENGTH (64*1024)
//PCIe
#define PCIE_MAX_BUFFER_SIZE (255 * 4096)
#ifdef __PC__
#define MAX_LINKS 16
#else
#define MAX_LINKS 1
#endif

/*
heep this struc tduplicate for now. think of a different solution*/
typedef enum{
    UsbVSC = 0,
    UsbCDC,
    Pcie,
    Ipc,
    protocols
} protocol_t;

int XLinkWrite(void* fd, void* data, int size, unsigned int timeout);
int XLinkRead(void* fd, void* data, int size, unsigned int timeout);
int XLinkPlatformConnect(const char* devPathRead,
                           const char* devPathWrite, void** fd);
int XLinkPlatformInit(protocol_t protocol, int loglevel);

int XLinkPlatformGetDeviceName(int index,
                                char* name,
                                int nameSize);
int XLinkPlatformGetDeviceNameExtended(int index,
                                char* name,
                                int nameSize,
                                int pid);

int XLinkPlatformBootRemote(const char* deviceName,
							const char* binaryPath);
int XLinkPlatformResetRemote(void* fd);

void* allocateData(uint32_t size, uint32_t alignment);
void deallocateData(void* ptr,uint32_t size, uint32_t alignment);

typedef enum xLinkPlatformErrorCode {
    X_LINK_PLATFORM_SUCCESS = 0,
    X_LINK_PLATFORM_DEVICE_NOT_FOUND = -1,
    X_LINK_PLATFORM_ERROR = -2,
    X_LINK_PLATFORM_TIMEOUT = -3,
    X_LINK_PLATFORM_DRIVER_NOT_LOADED = -4
} xLinkPlatformErrorCode_t;

#ifdef __cplusplus
}
#endif

#endif

/* end of include file */
