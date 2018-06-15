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

#include "XLink.h"

#include "stdio.h"
#include "stdint.h"
#include "string.h"

#include <assert.h>
#include <stdlib.h>
#if (defined(_WIN32) || defined(_WIN64))
#include "win_pthread.h"
#include "win_semaphore.h"
#include "gettime.h"
#else
#include <pthread.h>
#include <semaphore.h>
#endif
#include "mvMacros.h"
#include "UsbLinkPlatform.h"
#include "XLinkDispatcher.h"
#define _USBLINK_ENABLE_PRIVATE_INCLUDE_
#include "XLinkPrivateDefines.h"

#ifdef MVLOG_UNIT_NAME
#undef MVLOG_UNIT_NAME
#define MVLOG_UNIT_NAME xLink
#endif
#include "mvLog.h"

#define USB_DATA_TIMEOUT 2000
#define CIRCULAR_INCREMENT(x,maxVal) \
    { \
         x++; \
         if (x == maxVal) \
             x = 0; \
    }
//avoid problems with unsigned. first compare and then give the nuw value
#define CIRCULAR_DECREMENT(x,maxVal) \
{ \
    if (x == 0) \
        x = maxVal; \
    else \
        x--; \
}
#define EXTRACT_IDS(streamId, linkId) \
{ \
    linkId = (streamId >> 24) & 0XFF; \
    streamId = streamId & 0xFFFFFF; \
}

#define COMBIN_IDS(streamId, linkid) \
     streamId = streamId | ((linkid & 0xFF) << 24);


int dispatcherLocalEventGetResponse(xLinkEvent_t* event, xLinkEvent_t* response);
int dispatcherRemoteEventGetResponse(xLinkEvent_t* event, xLinkEvent_t* response);
//adds a new event with parameters and returns event id
int dispatcherEventSend(xLinkEvent_t* event);
streamDesc_t* getStreamById(void* fd, streamId_t id);
void releaseStream(streamDesc_t*);
int addNewPacketToStream(streamDesc_t* stream, void* buffer, uint32_t size);

struct dispatcherControlFunctions controlFunctionTbl;
XLinkGlobalHandler_t* glHandler; //TODO need to either protect this with semaphor
                                 //or make profiling data per device
linkId_t nextUniqueLinkId = 0; //incremental number, doesn't get decremented.
//streams
typedef struct xLinkDesc_t {
    int nextUniqueStreamId; //incremental number, doesn't get decremented.
                                //Needs to be per link to match remote
    streamDesc_t availableStreams[USB_LINK_MAX_STREAMS];
    xLinkState_t peerState;
    void* fd;
    linkId_t id;
} xLinkDesc_t;

xLinkDesc_t availableXLinks[MAX_LINKS];

sem_t  pingSem; //to b used by myriad


/*#################################################################################
###################################### INTERNAL ###################################
##################################################################################*/

static float timespec_diff(struct timespec *start, struct timespec *stop)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        start->tv_sec = stop->tv_sec - start->tv_sec - 1;
        start->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        start->tv_sec = stop->tv_sec - start->tv_sec;
        start->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return start->tv_nsec/ 1000000000.0 + start->tv_sec;
}

int handleIncomingEvent(xLinkEvent_t* event){
    //this function will be dependent whether this is a client or a Remote
    //specific actions to this peer
    void* buffer ;
    streamDesc_t* stream ;
    switch (event->header.type){
    case USB_WRITE_REQ:
        /*If we got here, we will read the data no matter what happens.
          If we encounter any problems we will still read the data to keep
          the communication working but send a NACK.*/
        stream = getStreamById(event->xLinkFD, event->header.streamId);
        ASSERT_X_LINK(stream);

        stream->localFillLevel += event->header.size;
        mvLog(MVLOG_DEBUG,"Got write of %ld, current local fill level is %ld out of %ld %ld\n",
            event->header.size, stream->localFillLevel, stream->readSize, stream->writeSize);

        buffer = allocateData(ALIGN_UP(event->header.size, __CACHE_LINE_SIZE), __CACHE_LINE_SIZE);
        if (buffer == NULL){
            mvLog(MVLOG_FATAL,"out of memory\n");
            ASSERT_X_LINK(0);
        }
        int sc = USBLinkRead(event->xLinkFD, buffer, event->header.size, USB_DATA_TIMEOUT);
        if(sc < 0){
            mvLog(MVLOG_ERROR,"%s() Read failed %d\n", __func__, (int)sc);
        }

        event->data = buffer;
        if (addNewPacketToStream(stream, buffer, event->header.size)){
            mvLog(MVLOG_WARN,"No more place in stream. release packet\n");
            deallocateData(buffer, ALIGN_UP(event->header.size, __CACHE_LINE_SIZE), __CACHE_LINE_SIZE);
            event->header.flags.bitField.ack = 0;
            event->header.flags.bitField.nack = 1;
            assert(0);
        }
        releaseStream(stream);
        break;
    case USB_READ_REQ:
        break;
    case USB_READ_REL_REQ:
        break;
    case USB_CREATE_STREAM_REQ:
        break;
    case USB_CLOSE_STREAM_REQ:
        break;
    case USB_PING_REQ:
        break;
    case USB_RESET_REQ:
        break;
    case USB_WRITE_RESP:
        break;
    case USB_READ_RESP:
        break;
    case USB_READ_REL_RESP:
        break;
    case USB_CREATE_STREAM_RESP:
        break;
    case USB_CLOSE_STREAM_RESP:
        break;
    case USB_PING_RESP:
        break;
    case USB_RESET_RESP:
        break;
    default:
        ASSERT_X_LINK(0);
    }
    //adding event for the scheduler. We let it know that this is a remote event
    dispatcherAddEvent(EVENT_REMOTE, event);
    return 0;
}
 int dispatcherEventReceive(xLinkEvent_t* event){
    static xLinkEvent_t prevEvent;
    int sc = USBLinkRead(event->xLinkFD, &event->header, sizeof(event->header), 0);

    if(sc < 0 && event->header.type == USB_RESET_RESP) {
		return sc;
    }
    if(sc < 0){
        mvLog(MVLOG_ERROR,"%s() Read failed %d\n", __func__, (int)sc);
        return sc;
    }

    mvLog(MVLOG_DEBUG,"Incoming event %d %d %d %d\n",
                                (int)event->header.type,
                                (int)event->header.id,
                                (int)prevEvent.header.id,
                                (int)prevEvent.header.type);

    if (prevEvent.header.id == event->header.id &&
            prevEvent.header.type == event->header.type &&
            prevEvent.xLinkFD == event->xLinkFD)
    {
        //TODO: Historically this check comes from a bug in the myriad USB stack.
        //Shouldn't be the case anymore
        mvLog(MVLOG_FATAL,"Duplicate id detected. \n");
        ASSERT_X_LINK(0);
    }
    prevEvent = *event;
    handleIncomingEvent(event);

    if(event->header.type == USB_RESET_REQ)
    {
        return -1;
    }

    return 0;
}

int getLinkIndex(void* fd)
{
    int i;
    for (i = 0; i < MAX_LINKS; i++)
        if (availableXLinks[i].fd == fd)
            return i;
    return -1;
}

xLinkDesc_t* getLinkById(linkId_t id)
{
    int i;
    for (i = 0; i < MAX_LINKS; i++)
        if (availableXLinks[i].id == id)
            return &availableXLinks[i];
    return NULL;
}
xLinkDesc_t* getLink(void* fd)
{
    int i;
    for (i = 0; i < MAX_LINKS; i++)
        if (availableXLinks[i].fd == fd)
            return &availableXLinks[i];
    return NULL;
}
int getNextAvailableLinkIndex()
{
    int i;
    for (i = 0; i < MAX_LINKS; i++)
        if (availableXLinks[i].id == INVALID_LINK_ID)
            return i;

    mvLog(MVLOG_ERROR,"%s():- no next available link!\n", __func__);
    return -1;
}
int getNextAvailableStreamIndex(xLinkDesc_t* link)
{
    if (link == NULL)
        return -1;

    int idx;
    for (idx = 0; idx < USB_LINK_MAX_STREAMS; idx++) {
        if (link->availableStreams[idx].id == INVALID_STREAM_ID)
            return idx;
    }

    mvLog(MVLOG_DEBUG,"%s(): - no next available stream!\n", __func__);
    return -1;
}

streamDesc_t* getStreamById(void* fd, streamId_t id)
{
    xLinkDesc_t* link = getLink(fd);
    ASSERT_X_LINK(link != NULL);
    int stream;
    for (stream = 0; stream < USB_LINK_MAX_STREAMS; stream++) {
        if (link->availableStreams[stream].id == id) {
            sem_wait(&link->availableStreams[stream].sem);
            return &link->availableStreams[stream];
        }
    }
    return NULL;
}

streamDesc_t* getStreamByName(xLinkDesc_t* link, const char* name)
{
    ASSERT_X_LINK(link != NULL);
    int stream;
    for (stream = 0; stream < USB_LINK_MAX_STREAMS; stream++) {
        if (link->availableStreams[stream].id != INVALID_STREAM_ID &&
            strcmp(link->availableStreams[stream].name, name) == 0) {
                sem_wait(&link->availableStreams[stream].sem);
                    return &link->availableStreams[stream];
        }
    }
    return NULL;
}

void releaseStream(streamDesc_t* stream)
{
    if (stream && stream->id != INVALID_STREAM_ID) {
        sem_post(&stream->sem);
    }
    else {
        mvLog(MVLOG_DEBUG,"trying to release a semaphore for a released stream\n");
    }
}

streamId_t getStreamIdByName(xLinkDesc_t* link, const char* name)
{
    streamDesc_t* stream = getStreamByName(link, name);
    streamId_t id;
    if (stream) {
        id = stream->id;
        releaseStream(stream);
        return id;
    }
    else
        return INVALID_STREAM_ID;
}

streamPacketDesc_t* getPacketFromStream(streamDesc_t* stream)
{
    streamPacketDesc_t* ret = NULL;
    if (stream->availablePackets)
    {
        ret = &stream->packets[stream->firstPacketUnused];
        stream->availablePackets--;
        CIRCULAR_INCREMENT(stream->firstPacketUnused,
                            USB_LINK_MAX_PACKETS_PER_STREAM);
        stream->blockedPackets++;
    }
    return ret;
}

void deallocateStream(streamDesc_t* stream)
{
    if (stream && stream->id != INVALID_STREAM_ID)
    {
        if (stream->readSize)
        {
            stream->readSize = 0;
            stream->closeStreamInitiated = 0;
        }
    }
}


int releasePacketFromStream(streamDesc_t* stream, uint32_t* releasedSize)
{
    streamPacketDesc_t* currPack = &stream->packets[stream->firstPacket];
    if(stream->blockedPackets == 0){
        mvLog(MVLOG_ERROR,"There is no packet to release\n");
        return 0; // ignore this, although this is a big problem on application side
    }
    stream->localFillLevel -= currPack->length;
    mvLog(MVLOG_DEBUG,"Got release of %ld , current local fill level is %ld out of %ld %ld\n",
        currPack->length, stream->localFillLevel, stream->readSize, stream->writeSize);

    deallocateData(currPack->data, ALIGN_UP_INT32((int32_t)currPack->length, __CACHE_LINE_SIZE), __CACHE_LINE_SIZE);
    CIRCULAR_INCREMENT(stream->firstPacket, USB_LINK_MAX_PACKETS_PER_STREAM);
    stream->blockedPackets--;
    *releasedSize = currPack->length;
    return 0;
}

int isStreamSpaceEnoughFor(streamDesc_t* stream, uint32_t size)
{
    if(stream->remoteFillPacketLevel >= USB_LINK_MAX_PACKETS_PER_STREAM ||
        stream->remoteFillLevel + size > stream->writeSize){
        return 0;
    }
    else
        return 1;
}

int addNewPacketToStream(streamDesc_t* stream, void* buffer, uint32_t size){
    if (stream->availablePackets + stream->blockedPackets < USB_LINK_MAX_PACKETS_PER_STREAM)
    {
        stream->packets[stream->firstPacketFree].data = buffer;
        stream->packets[stream->firstPacketFree].length = size;
        CIRCULAR_INCREMENT(stream->firstPacketFree, USB_LINK_MAX_PACKETS_PER_STREAM);
        stream->availablePackets++;
        return 0;
    }
    return -1;
}

streamId_t allocateNewStream(void* fd,
                             const char* name,
                             uint32_t writeSize,
                             uint32_t readSize,
                             streamId_t forcedId)
{
    streamId_t streamId;
    streamDesc_t* stream;
    xLinkDesc_t* link = getLink(fd);
    ASSERT_X_LINK(link != NULL);

    stream = getStreamByName(link, name);

    if (stream != NULL)
    {
        /*the stream already exists*/
        if ((writeSize > stream->writeSize && stream->writeSize != 0) ||
            (readSize > stream->readSize && stream->readSize != 0))
        {
            return INVALID_STREAM_ID;
        }
        mvLog(MVLOG_DEBUG,"%s(): streamName Exists %d\n", __func__, (int)stream->id);
    }
    else
    {
        int idx = getNextAvailableStreamIndex(link);

        if (idx == -1)
        {
            return INVALID_STREAM_ID;
        }
        stream = &link->availableStreams[idx];
        if (forcedId == INVALID_STREAM_ID)
            stream->id = link->nextUniqueStreamId;
        else
            stream->id = forcedId;
        link->nextUniqueStreamId++; //even if we didnt use a new one, we need to align with total number of  unique streams
        int sem_initiated = strlen(stream->name) != 0;
        strncpy(stream->name, name, MAX_NAME_LENGTH);
        stream->readSize = 0;
        stream->writeSize = 0;
        stream->remoteFillLevel = 0;
        stream->remoteFillPacketLevel = 0;

        stream->localFillLevel = 0;
        stream->closeStreamInitiated = 0;
        if (!sem_initiated) //if sem_init is called for already initiated sem, behavior is undefined
            sem_init(&stream->sem, 0, 0);
    }
    if (readSize && !stream->readSize)
    {
        stream->readSize = readSize;
    }
    if (writeSize && !stream->writeSize)
    {
        stream->writeSize = writeSize;
    }
    streamId = stream->id;
    releaseStream(stream);
    return streamId;
}
//this function should be called only for remote requests
int dispatcherLocalEventGetResponse(xLinkEvent_t* event, xLinkEvent_t* response)
{
    streamDesc_t* stream;
    response->header.id = event->header.id;
    switch (event->header.type){
    case USB_WRITE_REQ:
        //in case local tries to write after it issues close (writeSize is zero)
        stream = getStreamById(event->xLinkFD, event->header.streamId);
        ASSERT_X_LINK(stream);
        if (stream->writeSize == 0)
        {
            event->header.flags.bitField.nack = 1;
            event->header.flags.bitField.ack = 0;
            // return -1 to don't even send it to the remote
            releaseStream(stream);
            return -1;
        }
        event->header.flags.bitField.ack = 1;
        event->header.flags.bitField.nack = 0;
        event->header.flags.bitField.localServe = 0;

        if(!isStreamSpaceEnoughFor(stream, event->header.size)){
            mvLog(MVLOG_DEBUG,"local NACK RTS. stream is full\n");
            event->header.flags.bitField.block = 1;
            event->header.flags.bitField.localServe = 1;
            // TODO: easy to implement non-blocking read here, just return nack
        }else{
            event->header.flags.bitField.block = 0;
            stream->remoteFillLevel += event->header.size;
            stream->remoteFillPacketLevel++;

            mvLog(MVLOG_DEBUG,"Got local write of %ld , remote fill level %ld out of %ld %ld\n",
                event->header.size, stream->remoteFillLevel, stream->writeSize, stream->readSize);
        }
        releaseStream(stream);
        break;
    case USB_READ_REQ:
        stream = getStreamById(event->xLinkFD, event->header.streamId);
        ASSERT_X_LINK(stream);
        streamPacketDesc_t* packet = getPacketFromStream(stream);
        if (packet){
            //the read can be served with this packet
            streamPacketDesc_t** pack = (streamPacketDesc_t**)event->data;
            *pack = packet;
            event->header.flags.bitField.ack = 1;
            event->header.flags.bitField.nack = 0;
            event->header.flags.bitField.block = 0;
        }
        else{
            event->header.flags.bitField.block = 1;
            // TODO: easy to implement non-blocking read here, just return nack
        }
        releaseStream(stream);
        event->header.flags.bitField.localServe = 1;
        break;
    case USB_READ_REL_REQ:
        stream = getStreamById(event->xLinkFD, event->header.streamId);
        ASSERT_X_LINK(stream);
        uint32_t releasedSize;
        releasePacketFromStream(stream, &releasedSize);
        event->header.size = releasedSize;
        releaseStream(stream);
        break;
    case USB_CREATE_STREAM_REQ:
        break;
    case USB_CLOSE_STREAM_REQ:
        stream = getStreamById(event->xLinkFD, event->header.streamId);

        ASSERT_X_LINK(stream);
        if (stream->remoteFillLevel != 0){
            stream->closeStreamInitiated = 1;
            event->header.flags.bitField.block = 1;
            event->header.flags.bitField.localServe = 1;
        }else{
            event->header.flags.bitField.block = 0;
            event->header.flags.bitField.localServe = 0;
        }
        releaseStream(stream);
        break;
    case USB_PING_REQ:
    case USB_RESET_REQ:
    case USB_WRITE_RESP:
    case USB_READ_RESP:
    case USB_READ_REL_RESP:
    case USB_CREATE_STREAM_RESP:
    case USB_CLOSE_STREAM_RESP:
    case USB_PING_RESP:
        break;
    case USB_RESET_RESP:
        //should not happen
        event->header.flags.bitField.localServe = 1;
        break;
    default:
        ASSERT_X_LINK(0);
    }
    return 0;
}

//this function should be called only for remote requests
int dispatcherRemoteEventGetResponse(xLinkEvent_t* event, xLinkEvent_t* response)
{
    streamDesc_t* stream;
    response->header.id = event->header.id;
    response->header.flags.raw = 0;
    switch (event->header.type)
    {
        case USB_WRITE_REQ:
            //let remote write immediately as we have a local buffer for the data
            response->header.type = USB_WRITE_RESP;
            response->header.size = event->header.size;
            response->header.streamId = event->header.streamId;
            response->header.flags.bitField.ack = 1;
            response->xLinkFD = event->xLinkFD;

            // we got some data. We should unblock a blocked read
            int xxx = dispatcherUnblockEvent(-1,
                                             USB_READ_REQ,
                                             response->header.streamId,
                                             event->xLinkFD);
            (void) xxx;
            mvLog(MVLOG_DEBUG,"unblocked from stream %d %d\n",
                  (int)response->header.streamId, (int)xxx);
            break;
        case USB_READ_REQ:
            break;
        case USB_READ_REL_REQ:
            response->header.flags.bitField.ack = 1;
            response->header.flags.bitField.nack = 0;
            response->header.type = USB_READ_REL_RESP;
            response->xLinkFD = event->xLinkFD;
            stream = getStreamById(event->xLinkFD,
                                   event->header.streamId);
            ASSERT_X_LINK(stream);
            stream->remoteFillLevel -= event->header.size;
            stream->remoteFillPacketLevel--;

            mvLog(MVLOG_DEBUG,"Got remote release of %ld, remote fill level %ld out of %ld %ld\n",
                event->header.size, stream->remoteFillLevel, stream->writeSize, stream->readSize);
            releaseStream(stream);

            dispatcherUnblockEvent(-1, USB_WRITE_REQ, event->header.streamId,
                                    event->xLinkFD);
            //with every released packet check if the stream is already marked for close
            if (stream->closeStreamInitiated && stream->localFillLevel == 0)
            {
                mvLog(MVLOG_DEBUG,"%s() Unblock close STREAM\n", __func__);
                int xxx = dispatcherUnblockEvent(-1,
                                                 USB_CLOSE_STREAM_REQ,
                                                 event->header.streamId,
                                                 event->xLinkFD);
                (void) xxx;
            }
            break;
        case USB_CREATE_STREAM_REQ:
            response->header.flags.bitField.ack = 1;
            response->header.type = USB_CREATE_STREAM_RESP;
            //write size from remote means read size for this peer
            response->header.streamId = allocateNewStream(event->xLinkFD,
                                                          event->header.streamName,
                                                          0, event->header.size,
                                                          INVALID_STREAM_ID);
            response->xLinkFD = event->xLinkFD;
            strncpy(response->header.streamName, event->header.streamName, MAX_NAME_LENGTH);
            response->header.size = event->header.size;
            //TODO check streamid is valid
            mvLog(MVLOG_DEBUG,"creating stream %x\n", (int)response->header.streamId);
            break;
        case USB_CLOSE_STREAM_REQ:
            {
                response->header.type = USB_CLOSE_STREAM_RESP;
                response->header.streamId = event->header.streamId;
                response->xLinkFD = event->xLinkFD;

                streamDesc_t* stream = getStreamById(event->xLinkFD,
                                                     event->header.streamId);
                if (!stream) {
                    //if we have sent a NACK before, when the event gets unblocked
                    //the stream might already be unavailable
                    response->header.flags.bitField.ack = 1; //All is good, we are done
                    response->header.flags.bitField.nack = 0;
                    mvLog(MVLOG_DEBUG,"%s() got a close stream on aready closed stream\n", __func__);
                } else {
                    if (stream->localFillLevel == 0)
                    {
                        response->header.flags.bitField.ack = 1;
                        response->header.flags.bitField.nack = 0;

                        deallocateStream(stream);
                        if (!stream->writeSize) {
                            stream->id = INVALID_STREAM_ID;
                        }
                    }
                    else
                    {
                        mvLog(MVLOG_DEBUG,"%s():fifo is NOT empty returning NACK \n", __func__);
                        response->header.flags.bitField.nack = 1;
                        stream->closeStreamInitiated = 1;
                    }

                    releaseStream(stream);
                }
                break;
            }
        case USB_PING_REQ:
            response->header.type = USB_PING_RESP;
            response->header.flags.bitField.ack = 1;
            response->xLinkFD = event->xLinkFD;
            sem_post(&pingSem);
            break;
        case USB_RESET_REQ:
            mvLog(MVLOG_DEBUG,"reset request\n");
            response->header.flags.bitField.ack = 1;
            response->header.flags.bitField.nack = 0;
            response->header.type = USB_RESET_RESP;
            response->xLinkFD = event->xLinkFD;
            // need to send the response, serve the event and then reset
            break;
        case USB_WRITE_RESP:
            break;
        case USB_READ_RESP:
            break;
        case USB_READ_REL_RESP:
            break;
        case USB_CREATE_STREAM_RESP:
        {
            // write_size from the response the size of the buffer from the remote
            response->header.streamId = allocateNewStream(event->xLinkFD,
                                                          event->header.streamName,
                                                          event->header.size,0,
                                                          event->header.streamId);
            response->xLinkFD = event->xLinkFD;
            break;
        }
        case USB_CLOSE_STREAM_RESP:
        {
            streamDesc_t* stream = getStreamById(event->xLinkFD,
                                                 event->header.streamId);

            if (!stream){
                response->header.flags.bitField.nack = 1;
                response->header.flags.bitField.ack = 0;
                break;
            }
            stream->writeSize = 0;
            if (!stream->readSize) {
                response->header.flags.bitField.nack = 1;
                response->header.flags.bitField.ack = 0;
                stream->id = INVALID_STREAM_ID;
                break;
            }
            releaseStream(stream);
            break;
        }

        case USB_PING_RESP:
            break;
        case USB_RESET_RESP:
            break;
        default:
            ASSERT_X_LINK(0);
    }
    return 0;
}
//adds a new event with parameters and returns event id
int dispatcherEventSend(xLinkEvent_t *event)
{
    mvLog(MVLOG_DEBUG,"sending %d %d\n", (int)event->header.type,  (int)event->header.id);
    int rc = USBLinkWrite(event->xLinkFD, &event->header, sizeof(event->header), 0);
    if(rc < 0)
    {
        mvLog(MVLOG_ERROR,"Write failed %d\n", rc);
    }
    if (event->header.type == USB_WRITE_REQ)
    {
        //write requested data
        rc = USBLinkWrite(event->xLinkFD, event->data,
                          event->header.size, USB_DATA_TIMEOUT);
        if(rc < 0) {
            mvLog(MVLOG_ERROR,"Write failed %d\n", rc);
        }
    }
    // this function will send events to the remote node
    return 0;
}

static xLinkState_t getXLinkState(xLinkDesc_t* link)
{
    ASSERT_X_LINK(link != NULL);
    mvLog(MVLOG_DEBUG,"%s() link %p link->peerState %d\n", __func__,link, link->peerState);
    return link->peerState;
}

void dispatcherCloseUsbLink(void*fd)
{
    xLinkDesc_t* link = getLink(fd);
    ASSERT_X_LINK(link != NULL);
    link->peerState = X_LINK_COMMUNICATION_NOT_OPEN;
    link->id = INVALID_LINK_ID;
    link->fd = NULL;
    link->nextUniqueStreamId = 0;

    int stream;
    for (stream = 0; stream < USB_LINK_MAX_STREAMS; stream++)
        link->availableStreams[stream].id = INVALID_STREAM_ID;
}

void dispatcherResetDevice(void* fd)
{
    USBLinkPlatformResetRemote(fd);
}


/*#################################################################################
###################################### EXTERNAL ###################################
##################################################################################*/
//Called only from app - per device
XLinkError_t XLinkConnect(XLinkHandler_t* handler)
{
    int index = getNextAvailableLinkIndex();
    ASSERT_X_LINK(index != -1);

    xLinkDesc_t* link = &availableXLinks[index];
    mvLog(MVLOG_DEBUG,"%s() device name %s \n", __func__, handler->devicePath);

    if (UsbLinkPlatformConnect(handler->devicePath2, handler->devicePath, &link->fd) == -1)
    {
        return X_LINK_ERROR;
    }

    dispatcherStart(link->fd);

    xLinkEvent_t event = {0};
    event.header.type = USB_PING_REQ;
    event.xLinkFD = link->fd;
    dispatcherAddEvent(EVENT_LOCAL, &event);
    dispatcherWaitEventComplete(link->fd);

    link->id = nextUniqueLinkId++;
    link->peerState = USB_LINK_UP;
    handler->linkId = link->id;

    return 0;
}

XLinkError_t XLinkInitialize(XLinkGlobalHandler_t* handler)
{
    ASSERT_X_LINK(USB_LINK_MAX_STREAMS <= MAX_POOLS_ALLOC);
    glHandler = handler;
    sem_init(&pingSem,0,0);
    int i;
    int sc = UsbLinkPlatformInit(handler->loglevel);
    if (sc)
    {
       return X_LINK_COMMUNICATION_NOT_OPEN;
    }

    //initialize availableStreams
    xLinkDesc_t* link;
    for (i = 0; i < MAX_LINKS; i++) {
        link = &availableXLinks[i];
        link->id = INVALID_LINK_ID;
        link->fd = NULL;
        link->peerState = USB_LINK_NOT_INIT;
        int stream;
        for (stream = 0; stream < USB_LINK_MAX_STREAMS; stream++)
            link->availableStreams[stream].id = INVALID_STREAM_ID;
    }

    controlFunctionTbl.eventReceive = &dispatcherEventReceive;
    controlFunctionTbl.eventSend = &dispatcherEventSend;
    controlFunctionTbl.localGetResponse = &dispatcherLocalEventGetResponse;
    controlFunctionTbl.remoteGetResponse = &dispatcherRemoteEventGetResponse;
    controlFunctionTbl.closeLink = &dispatcherCloseUsbLink;
    controlFunctionTbl.resetDevice = &dispatcherResetDevice;
    dispatcherInitialize(&controlFunctionTbl);

#ifndef __PC__
    int index = getNextAvailableLinkIndex();
    if (index == -1)
        return X_LINK_COMMUNICATION_NOT_OPEN;

    link = &availableXLinks[index];
    link->fd = NULL;
    link->id = nextUniqueLinkId++;
    link->peerState = USB_LINK_UP;

    sem_wait(&pingSem);
#endif
    return X_LINK_SUCCESS;
}


XLinkError_t XLinkGetFillLevel(streamId_t streamId, int isRemote, int* fillLevel)
{
    linkId_t id;
    EXTRACT_IDS(streamId,id);
    xLinkDesc_t* link = getLinkById(id);
    streamDesc_t* stream;

    ASSERT_X_LINK(link != NULL);
    if (getXLinkState(link) != USB_LINK_UP)
    {
        return X_LINK_COMMUNICATION_NOT_OPEN;
    }
    stream = getStreamById(link->fd, streamId);
    ASSERT_X_LINK(stream);

    if (isRemote)
        *fillLevel = stream->remoteFillLevel;
    else
        *fillLevel = stream->localFillLevel;
    releaseStream(stream);
    return X_LINK_SUCCESS;
}

streamId_t XLinkOpenStream(linkId_t id, const char* name, int stream_write_size)
{
    xLinkEvent_t event = {0};
    xLinkDesc_t* link = getLinkById(id);
    mvLog(MVLOG_DEBUG,"%s() id %d link %p\n", __func__, id, link);
    ASSERT_X_LINK(link != NULL);
    if (getXLinkState(link) != USB_LINK_UP)
    {
        /*no link*/
        mvLog(MVLOG_DEBUG,"%s() no link up\n", __func__);
        return INVALID_STREAM_ID;
    }

    if(strlen(name) > MAX_NAME_LENGTH)
    {
        mvLog(MVLOG_WARN,"name too long\n");
        return INVALID_STREAM_ID;
    }

    if(stream_write_size > 0)
    {
        stream_write_size = ALIGN_UP(stream_write_size, __CACHE_LINE_SIZE);
        event.header.type = USB_CREATE_STREAM_REQ;
        strncpy(event.header.streamName, name, MAX_NAME_LENGTH);
        event.header.size = stream_write_size;
        event.header.streamId = INVALID_STREAM_ID;
        event.xLinkFD = link->fd;

        dispatcherAddEvent(EVENT_LOCAL, &event);
        dispatcherWaitEventComplete(link->fd);

    }
    streamId_t streamId = getStreamIdByName(link, name);
    if (streamId > 0xFFFFFFF) {
        mvLog(MVLOG_ERROR,"Max streamId reached %x!", streamId);
        return INVALID_STREAM_ID;
    }
    COMBIN_IDS(streamId, id);
    return streamId;
}


// Just like open stream, when closeStream is called
// on the local size we are resetting the writeSize
// and on the remote side we are freeing the read buffer
XLinkError_t XLinkCloseStream(streamId_t streamId)
{
    linkId_t id;
    EXTRACT_IDS(streamId,id);
    xLinkDesc_t* link = getLinkById(id);
    ASSERT_X_LINK(link != NULL);

    mvLog(MVLOG_DEBUG,"%s(): streamId %d\n", __func__, (int)streamId);
    if (getXLinkState(link) != USB_LINK_UP)
        return X_LINK_COMMUNICATION_NOT_OPEN;

    xLinkEvent_t event = {0};
    event.header.type = USB_CLOSE_STREAM_REQ;
    event.header.streamId = streamId;
    event.xLinkFD = link->fd;
    xLinkEvent_t* ev = dispatcherAddEvent(EVENT_LOCAL, &event);
    dispatcherWaitEventComplete(link->fd);

    if (ev->header.flags.bitField.ack == 1)
        return X_LINK_SUCCESS;
    else
        return X_LINK_COMMUNICATION_FAIL;

    return X_LINK_SUCCESS;
}


XLinkError_t XLinkGetAvailableStreams(linkId_t id)
{
    xLinkDesc_t* link = getLinkById(id);
    ASSERT_X_LINK(link != NULL);
    if (getXLinkState(link) != USB_LINK_UP)
    {
        return X_LINK_COMMUNICATION_NOT_OPEN;
    }
    /*...get other statuses*/
    return X_LINK_SUCCESS;
}

XLinkError_t XLinkGetDeviceName(int index, char* name, int nameSize)
{
    int rc = UsbLinkPlatformGetDeviceName(index, name, nameSize);
    switch(rc) {
        case USB_LINK_PLATFORM_SUCCESS:
            return X_LINK_SUCCESS;
        case USB_LINK_PLATFORM_DEVICE_NOT_FOUND:
            return X_LINK_DEVICE_NOT_FOUND;
        case USB_LINK_PLATFORM_TIMEOUT:
            return X_LINK_TIMEOUT;
        default:
            return X_LINK_ERROR;

    }
}

XLinkError_t XLinkWriteData(streamId_t streamId, const uint8_t* buffer,
                            int size)
{
    linkId_t id;
    EXTRACT_IDS(streamId,id);
    xLinkDesc_t* link = getLinkById(id);
    ASSERT_X_LINK(link != NULL);
    if (getXLinkState(link) != USB_LINK_UP)
    {
        return X_LINK_COMMUNICATION_NOT_OPEN;
    }
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    xLinkEvent_t event = {0};
    event.header.type = USB_WRITE_REQ;
    event.header.size = size;
    event.header.streamId = streamId;
    event.xLinkFD = link->fd;
    event.data = (void*)buffer;

    xLinkEvent_t* ev = dispatcherAddEvent(EVENT_LOCAL, &event);
    dispatcherWaitEventComplete(link->fd);
    clock_gettime(CLOCK_REALTIME, &end);


    if (ev->header.flags.bitField.ack == 1)
    {
         //profile only on success
        if( glHandler->profEnable)
        {
            glHandler->profilingData.totalWriteBytes += size;
            glHandler->profilingData.totalWriteTime += timespec_diff(&start, &end);
        }
        return X_LINK_SUCCESS;
    }
    else
        return X_LINK_COMMUNICATION_FAIL;
}

XLinkError_t XLinkAsyncWriteData()
{
    if (getXLinkState(NULL) != USB_LINK_UP)
    {
        return X_LINK_COMMUNICATION_NOT_OPEN;
    }
    return X_LINK_SUCCESS;
}

XLinkError_t XLinkReadData(streamId_t streamId, streamPacketDesc_t** packet)
{
    linkId_t id;
    EXTRACT_IDS(streamId,id);
    xLinkDesc_t* link = getLinkById(id);
    ASSERT_X_LINK(link != NULL);
    if (getXLinkState(link) != USB_LINK_UP)
    {
        return X_LINK_COMMUNICATION_NOT_OPEN;
    }

    xLinkEvent_t event = {0};
    struct timespec start, end;

    event.header.type = USB_READ_REQ;
    event.header.size = 0;
    event.header.streamId = streamId;
    event.xLinkFD = link->fd;
    event.data = (void*)packet;

    clock_gettime(CLOCK_REALTIME, &start);
    xLinkEvent_t* ev = dispatcherAddEvent(EVENT_LOCAL, &event);
    dispatcherWaitEventComplete(link->fd);
    clock_gettime(CLOCK_REALTIME, &end);

    if( glHandler->profEnable)
    {
        glHandler->profilingData.totalReadBytes += (*packet)->length;
        glHandler->profilingData.totalReadTime += timespec_diff(&start, &end);
    }

    if (ev->header.flags.bitField.ack == 1)
        return X_LINK_SUCCESS;
    else
        return X_LINK_COMMUNICATION_FAIL;
}

XLinkError_t XLinkReleaseData(streamId_t streamId)
{
    linkId_t id;
    EXTRACT_IDS(streamId,id);
    xLinkDesc_t* link = getLinkById(id);
    ASSERT_X_LINK(link != NULL);
    if (getXLinkState(link) != USB_LINK_UP)
    {
        return X_LINK_COMMUNICATION_NOT_OPEN;
    }

    xLinkEvent_t event = {0};
    event.header.type = USB_READ_REL_REQ;
    event.header.streamId = streamId;
    event.xLinkFD = link->fd;

    xLinkEvent_t* ev = dispatcherAddEvent(EVENT_LOCAL, &event);
    dispatcherWaitEventComplete(link->fd);

    if (ev->header.flags.bitField.ack == 1)
        return X_LINK_SUCCESS;
    else
        return X_LINK_COMMUNICATION_FAIL;
}

XLinkError_t XLinkBootRemote(const char* deviceName, const char* binaryPath)
{
    if (UsbLinkPlatformBootRemote(deviceName, binaryPath) == 0)
        return X_LINK_SUCCESS;
    else
        return X_LINK_COMMUNICATION_FAIL;
}

XLinkError_t XLinkResetRemote(linkId_t id)
{
    xLinkDesc_t* link = getLinkById(id);
    ASSERT_X_LINK(link != NULL);
    if (getXLinkState(link) != USB_LINK_UP)
    {
        USBLinkPlatformResetRemote(link->fd);
        return X_LINK_COMMUNICATION_NOT_OPEN;
    }
    xLinkEvent_t event = {0};
    event.header.type = USB_RESET_REQ;
    event.xLinkFD = link->fd;
    mvLog(MVLOG_DEBUG,"sending reset remote event\n");
    dispatcherAddEvent(EVENT_LOCAL, &event);
    dispatcherWaitEventComplete(link->fd);

    return X_LINK_SUCCESS;
}

XLinkError_t XLinkResetAll()
{
    int i;
    for (i = 0; i < MAX_LINKS; i++) {
        if (availableXLinks[i].id != INVALID_LINK_ID) {
            xLinkDesc_t* link = &availableXLinks[i];
            int stream;
            for (stream = 0; stream < USB_LINK_MAX_STREAMS; stream++) {
                if (link->availableStreams[stream].id != INVALID_STREAM_ID) {
                    streamId_t streamId = link->availableStreams[stream].id;
                    mvLog(MVLOG_DEBUG,"%s() Closing stream (stream = %d) %d on link %d\n",
                          __func__, stream, (int) streamId, (int) link->id);
                    COMBIN_IDS(streamId, link->id);
                    XLinkCloseStream(streamId);
                }
            }
            XLinkResetRemote(link->id);
        }
    }
    return X_LINK_SUCCESS;
}

XLinkError_t XLinkProfStart()
{
    glHandler->profEnable = 1;
    glHandler->profilingData.totalReadBytes = 0;
    glHandler->profilingData.totalWriteBytes = 0;
    glHandler->profilingData.totalWriteTime = 0;
    glHandler->profilingData.totalReadTime = 0;
    glHandler->profilingData.totalBootCount = 0;
    glHandler->profilingData.totalBootTime = 0;

    return X_LINK_SUCCESS;
}

XLinkError_t XLinkProfStop()
{
    glHandler->profEnable = 0;
    return X_LINK_SUCCESS;
}

XLinkError_t XLinkProfPrint()
{
    printf("XLink profiling results:\n");
    if (glHandler->profilingData.totalWriteTime)
    {
        printf("Average write speed: %f MB/Sec\n",
               glHandler->profilingData.totalWriteBytes /
               glHandler->profilingData.totalWriteTime /
               1024.0 /
               1024.0 );
    }
    if (glHandler->profilingData.totalReadTime)
    {
        printf("Average read speed: %f MB/Sec\n",
               glHandler->profilingData.totalReadBytes /
               glHandler->profilingData.totalReadTime /
               1024.0 /
               1024.0);
    }
    if (glHandler->profilingData.totalBootCount)
    {
        printf("Average boot speed: %f sec\n",
               glHandler->profilingData.totalBootTime /
               glHandler->profilingData.totalBootCount);
    }
    return X_LINK_SUCCESS;
}
/* end of file */
