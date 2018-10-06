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
/// @file
///
/// @brief     Application configuration Leon header
///
#ifndef _XLINKPRIVATEDEFINES_H
#define _XLINKPRIVATEDEFINES_H

#ifdef _XLINK_ENABLE_PRIVATE_INCLUDE_

#include <stdint.h>
#if (defined(_WIN32) || defined(_WIN64))
#include "win_semaphore.h"
#else
#include <semaphore.h>
#endif

#include <XLinkPublicDefines.h>
#include "XLinkPlatform.h"

#ifdef __cplusplus
extern "C"
{
#endif
#define MAX_NAME_LENGTH 16

#define HEADER_SIZE (64-12 -8)

#define MAXIMUM_SEMAPHORES 32
#define __CACHE_LINE_SIZE 64

#define ASSERT_X_LINK(x)   if(!(x)) { fprintf(stderr, "info: %s:%d: ", __FILE__, __LINE__); abort(); }

typedef int32_t eventId_t;

typedef enum {
    XLINK_NOT_INIT,
    XLINK_UP,
    XLINK_DOWN,
}xLinkState_t;

typedef struct{
    char name[MAX_NAME_LENGTH];
    streamId_t id;
    void* fd;
    uint32_t writeSize;
    uint32_t readSize;  /*No need of read buffer. It's on remote,
    will read it directly to the requested buffer*/
    streamPacketDesc_t packets[USB_LINK_MAX_PACKETS_PER_STREAM];
    uint32_t availablePackets;
    uint32_t blockedPackets;

    uint32_t firstPacket;
    uint32_t firstPacketUnused;
    uint32_t firstPacketFree;
    uint32_t remoteFillLevel;
    uint32_t localFillLevel;
    uint32_t remoteFillPacketLevel;

    uint32_t closeStreamInitiated;

    sem_t sem;
}streamDesc_t;

//events which are coming from remote
typedef enum
{
    /*USB related events*/
    USB_WRITE_REQ,
    USB_READ_REQ,
    USB_READ_REL_REQ,
    USB_CREATE_STREAM_REQ,
    USB_CLOSE_STREAM_REQ,
    USB_PING_REQ,
    USB_RESET_REQ,
    USB_REQUEST_LAST,
    //note that is important to separate request and response
    USB_WRITE_RESP,
    USB_READ_RESP,
    USB_READ_REL_RESP,
    USB_CREATE_STREAM_RESP,
    USB_CLOSE_STREAM_RESP,
    USB_PING_RESP,
    USB_RESET_RESP,
    USB_RESP_LAST,

    /*PCI-E related events*/
    PCIE_WRITE_REQ,
    PCIE_READ_REQ,
    PCIE_CREATE_STREAM_REQ,
    PCIE_CLOSE_STREAM_REQ,
    //
    PCIE_WRITE_RESP,
    PCIE_READ_RESP,
    PCIE_CREATE_STREAM_RESP,
    PCIE_CLOSE_STREAM_RESP,

    /*IPC related events*/
    IPC_WRITE_REQ,
    IPC_READ_REQ,
    IPC_CREATE_STREAM_REQ,
    IPC_CLOSE_STREAM_REQ,
    //
    IPC_WRITE_RESP,
    IPC_READ_RESP,
    IPC_CREATE_STREAM_RESP,
    IPC_CLOSE_STREAM_RESP,
} xLinkEventType_t;

typedef enum
{
    EVENT_LOCAL,
    EVENT_REMOTE,
} xLinkEventOrigin_t;

#define MAX_EVENTS 64

#define MAX_SCHEDULERS MAX_LINKS
//static int eventCount;
//static dispEventType_t eventQueue[MAX_EVENTS];

typedef struct xLinkEventHeader_t{
    eventId_t           id;
    xLinkEventType_t    type;
    char                streamName[MAX_NAME_LENGTH];
    streamId_t          streamId;
    uint32_t            size;
    union{
        uint32_t raw;
        struct{
            uint32_t ack : 1;
            uint32_t nack : 1;
            uint32_t block : 1;
            uint32_t localServe : 1;
            uint32_t terminate : 1;
            uint32_t bufferFull : 1;
            uint32_t sizeTooBig : 1;
            uint32_t noSuchStream : 1;
        }bitField;
    }flags;
}xLinkEventHeader_t;

typedef struct xLinkEvent_t {
    xLinkEventHeader_t header;
    void* xLinkFD;
    void* data;
}xLinkEvent_t;

#ifdef __cplusplus
}
#endif

#endif  /*_XLINK_ENABLE_PRIVATE_INCLUDE_ end*/
#endif

/* end of include file */
