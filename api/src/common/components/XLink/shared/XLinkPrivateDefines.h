/*
* Copyright 2018 Intel Corporation.
* The source code, information and material ("Material") contained herein is
* owned by Intel Corporation or its suppliers or licensors, and title to such
* Material remains with Intel Corporation or its suppliers or licensors.
* The Material contains proprietary information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright laws and treaty
* provisions.
* No part of the Material may be used, copied, reproduced, modified, published,
* uploaded, posted, transmitted, distributed or disclosed in any way without
* Intel's prior express written permission. No license under any patent,
* copyright or other intellectual property rights in the Material is granted to
* or conferred upon you, either expressly, by implication, inducement, estoppel
* or otherwise.
* Any license under such intellectual property rights must be express and
* approved by Intel in writing.
*/

///
/// @file
///
/// @brief     Application configuration Leon header
///
#ifndef _XLINKPRIVATEDEFINES_H
#define _XLINKPRIVATEDEFINES_H

#ifdef _USBLINK_ENABLE_PRIVATE_INCLUDE_

#include <stdint.h>
#if (defined(_WIN32) || defined(_WIN64))
#include "win_semaphore.h"
#else
#include <semaphore.h>
#endif
#include <XLinkPublicDefines.h>
#include "UsbLinkPlatform.h"

#ifdef __cplusplus
extern "C"
{
#endif
#define MAX_NAME_LENGTH 16

#ifdef USE_USB_VSC
#define HEADER_SIZE (64-12 -8)
#else
#define HEADER_SIZE (64-12 -8)
#endif

#define MAXIMUM_SEMAPHORES 32
#define __CACHE_LINE_SIZE 64

#define ASSERT_X_LINK(x)   if(!(x)) { fprintf(stderr, "info: %s:%d: ", __FILE__, __LINE__); abort(); }

typedef int32_t eventId_t;

typedef enum {
    USB_LINK_NOT_INIT,
    USB_LINK_UP,
    USB_LINK_DOWN,
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
    /*Add PCI-E related events at tail*/

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

#endif  /*_USBLINK_ENABLE_PRIVATE_INCLUDE_ end*/
#endif

/* end of include file */
