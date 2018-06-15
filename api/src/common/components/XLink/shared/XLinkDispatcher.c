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
#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"

#include <assert.h>
#include <stdlib.h>
#if (defined(_WIN32) || defined(_WIN64))
#include "win_pthread.h"
#include "win_semaphore.h"
#else
#include <pthread.h>
#include <semaphore.h>
#endif
#include "XLinkDispatcher.h"
#include "XLinkPrivateDefines.h"

#define MVLOG_UNIT_NAME xLink
#include "mvLog.h"

typedef enum {
    EVENT_PENDING,
    EVENT_BLOCKED,
    EVENT_READY,
    EVENT_SERVED,
} xLinkEventState_t;

typedef struct xLinkEventPriv_t {
    xLinkEvent_t packet;
    xLinkEventState_t isServed;
    xLinkEventOrigin_t origin;
    sem_t* sem;
    void* data;
    uint32_t pad;
} xLinkEventPriv_t;

typedef struct {
    sem_t sem;
    pthread_t threadId;
    int refs;
} localSem_t;

typedef struct{
    xLinkEventPriv_t* end;
    xLinkEventPriv_t* base;

    xLinkEventPriv_t* curProc;
    xLinkEventPriv_t* cur;
    __attribute__((aligned(8))) xLinkEventPriv_t q[MAX_EVENTS];

}eventQueueHandler_t;
typedef struct {
    void* xLinkFD; //will be device handler
    int schedulerId;

    sem_t addEventSem;
    sem_t notifyDispatcherSem;
    uint32_t resetXLink;
    uint32_t semaphores;
    pthread_t xLinkThreadId;

    eventQueueHandler_t lQueue; //local queue
    eventQueueHandler_t rQueue; //remote queue
    localSem_t eventSemaphores[MAXIMUM_SEMAPHORES];
} xLinkSchedulerState_t;


#define CIRCULAR_INCREMENT(x, maxVal, base) \
    { \
        x++; \
        if (x == maxVal) \
            x = base; \
    }
//avoid problems with unsigned. first compare and then give the nuw value
#define CIRCULAR_DECREMENT(x, maxVal, base) \
{ \
    if (x == base) \
        x = maxVal - 1; \
    else \
        x--; \
}

char* TypeToStr(int type)
{
    switch(type)
    {
        case USB_WRITE_REQ:     return "USB_WRITE_REQ";
        case USB_READ_REQ:      return "USB_READ_REQ";
        case USB_READ_REL_REQ:  return "USB_READ_REL_REQ";
        case USB_CREATE_STREAM_REQ:return "USB_CREATE_STREAM_REQ";
        case USB_CLOSE_STREAM_REQ: return "USB_CLOSE_STREAM_REQ";
        case USB_PING_REQ:         return "USB_PING_REQ";
        case USB_RESET_REQ:        return "USB_RESET_REQ";
        case USB_REQUEST_LAST:     return "USB_REQUEST_LAST";
        case USB_WRITE_RESP:   return "USB_WRITE_RESP";
        case USB_READ_RESP:     return "USB_READ_RESP";
        case USB_READ_REL_RESP: return "USB_READ_REL_RESP";
        case USB_CREATE_STREAM_RESP: return "USB_CREATE_STREAM_RESP";
        case USB_CLOSE_STREAM_RESP:  return "USB_CLOSE_STREAM_RESP";
        case USB_PING_RESP:  return "USB_PING_RESP";
        case USB_RESET_RESP: return "USB_RESET_RESP";
        case USB_RESP_LAST:  return "USB_RESP_LAST";
        default: break;
    }
    return "";
}
#if (defined(_WIN32) || defined(_WIN64))
static void* __cdecl eventSchedulerRun(void* ctx);
#else
static void* eventSchedulerRun(void*);
#endif
//These will be common for all, Initialized only once
struct dispatcherControlFunctions* glControlFunc;
int numSchedulers;
xLinkSchedulerState_t schedulerState[MAX_SCHEDULERS];
sem_t addSchedulerSem;

//below workaround for "C2088 '==': illegal for struct" error
int pthread_t_compare(pthread_t a, pthread_t b)
{
    return  (a == b);
}

static int unrefSem(sem_t* sem,  xLinkSchedulerState_t* curr) {
    ASSERT_X_LINK(curr != NULL);
    localSem_t* temp = curr->eventSemaphores;
    while (temp < curr->eventSemaphores + MAXIMUM_SEMAPHORES) {
        if (&temp->sem == sem) {
            temp->refs--;
            //mvLog(MVLOG_WARN,"unrefSem : sem->threadId %x refs %d number of semaphors %d\n", temp->threadId, temp->refs, curr->semaphores);
            if (temp->refs == 0) {
                curr->semaphores--;
                ASSERT_X_LINK(sem_destroy(&temp->sem) != -1);
                temp->refs = -1;
            }
            return 1;
        }
        temp++;
    }
    mvLog(MVLOG_WARN,"unrefSem : sem wasn't found\n");
    return 0;
}
static sem_t* getCurrentSem(pthread_t threadId, xLinkSchedulerState_t* curr, int inc_ref)
{
    ASSERT_X_LINK(curr != NULL);

    localSem_t* sem = curr->eventSemaphores;
    while (sem < curr->eventSemaphores + MAXIMUM_SEMAPHORES) {
        if (pthread_t_compare(sem->threadId, threadId) && sem->refs > 0) {
            sem->refs += inc_ref;
            //mvLog(MVLOG_WARN,"getCurrentSem : sem->threadId %x refs %d inc_ref %d\n", sem->threadId, sem->refs, inc_ref);
            return &sem->sem;
        }
        sem++;
    }
    return NULL;
}

static sem_t* createSem(xLinkSchedulerState_t* curr)
{
    ASSERT_X_LINK(curr != NULL);


    sem_t* sem = getCurrentSem(pthread_self(), curr, 0);
    if (sem) // it already exists, error
        return NULL;
    else
    {
        if (curr->semaphores < MAXIMUM_SEMAPHORES) {
            localSem_t* temp = curr->eventSemaphores;
            while (temp < curr->eventSemaphores + MAXIMUM_SEMAPHORES) {
                if (temp->refs < 0) {
                    sem = &temp->sem;
                    if (temp->refs == -1) {
                        if (sem_init(sem, 0, 0))
                            perror("Can't create semaphore\n");
                    }
                    curr->semaphores++;
                    temp->refs = 1;
                    temp->threadId = pthread_self();

                    break;
                }
                temp++;
            }
            if (!sem)
                return NULL; //shouldn't happen
            //mvLog(MVLOG_WARN,"createSem : sem->threadId %x refs %d  number of semaphors %d \n", temp->threadId, temp->refs, curr->semaphores);
        }
        else
            return NULL;
       return sem;
    }
}

#if (defined(_WIN32) || defined(_WIN64))
static void* __cdecl eventReader(void* ctx)
#else
static void* eventReader(void* ctx)
#endif
{
    int schedulerId = *(int *)ctx;
    ASSERT_X_LINK(schedulerId < MAX_SCHEDULERS);

    xLinkEvent_t event = { 0 };// to fix error C4700 in win
    event.header.id = -1;
    event.xLinkFD = schedulerState[schedulerId].xLinkFD;

    while (!schedulerState[schedulerId].resetXLink) {
        mvLog(MVLOG_DEBUG,"Reading %s (scheduler Id %d, fd %p)\n", TypeToStr(event.header.type),
            schedulerId, event.xLinkFD);
        int sc = glControlFunc->eventReceive(&event);
        if (sc) {
            break;
        }
    }
    return 0;
}



static int isEventTypeRequest(xLinkEventPriv_t* event)
{
    if (event->packet.header.type < USB_REQUEST_LAST)
        return 1;
    else
        return 0;
}

static void markEventBlocked(xLinkEventPriv_t* event)
{
    event->isServed = EVENT_BLOCKED;
}

static void markEventReady(xLinkEventPriv_t* event)
{
    event->isServed = EVENT_READY;
}

static void markEventServed(xLinkEventPriv_t* event)
{
    if(event->sem){
        if (sem_post(event->sem)) {
            mvLog(MVLOG_ERROR,"can't post semaphore\n");
        }
    }
    event->isServed = EVENT_SERVED;
}


static int dispatcherRequestServe(xLinkEventPriv_t * event, xLinkSchedulerState_t* curr){
    ASSERT_X_LINK(curr != NULL);
    ASSERT_X_LINK(isEventTypeRequest(event));
    xLinkEventHeader_t *header = &event->packet.header;
    if (header->flags.bitField.block){ //block is requested
        markEventBlocked(event);
    }else if(header->flags.bitField.localServe == 1 ||
             (header->flags.bitField.ack == 0
             && header->flags.bitField.nack == 1)){ //this event is served locally, or it is failed
        markEventServed(event);
    }else if (header->flags.bitField.ack == 1
              && header->flags.bitField.nack == 0){
        event->isServed = EVENT_PENDING;
        mvLog(MVLOG_DEBUG,"------------------------UNserved %s\n",
              TypeToStr(event->packet.header.type));
    }else{
        ASSERT_X_LINK(0);
    }
    return 0;
}


static int dispatcherResponseServe(xLinkEventPriv_t * event, xLinkSchedulerState_t* curr)
{
    int i = 0;
    ASSERT_X_LINK(curr != NULL);
    ASSERT_X_LINK(!isEventTypeRequest(event));
    for (i = 0; i < MAX_EVENTS; i++)
    {
        xLinkEventHeader_t *header = &curr->lQueue.q[i].packet.header;
        xLinkEventHeader_t *evHeader = &event->packet.header;

        if (curr->lQueue.q[i].isServed == EVENT_PENDING &&
                        header->id == evHeader->id &&
                        header->type == evHeader->type - USB_REQUEST_LAST -1)
        {
            mvLog(MVLOG_DEBUG,"----------------------ISserved %s\n",
                    TypeToStr(header->type));
            //propagate back flags
            header->flags = evHeader->flags;
            markEventServed(&curr->lQueue.q[i]);
            break;
        }
    }
    if (i == MAX_EVENTS) {
        mvLog(MVLOG_FATAL,"no request for this response: %s %d\n", TypeToStr(event->packet.header.type), event->origin);
        ASSERT_X_LINK(0);
    }
    return 0;
}

static inline xLinkEventPriv_t* getNextElementWithState(xLinkEventPriv_t* base, xLinkEventPriv_t* end,
                                                        xLinkEventPriv_t* start, xLinkEventState_t state){
    xLinkEventPriv_t* tmp = start;
    while (start->isServed != state){
        CIRCULAR_INCREMENT(start, end, base);
        if(tmp == start){
            break;
        }
    }
    if(start->isServed == state){
        return start;
    }else{
        return NULL;
    }
}

static xLinkEventPriv_t* searchForReadyEvent(xLinkSchedulerState_t* curr)
{
    ASSERT_X_LINK(curr != NULL);
    xLinkEventPriv_t* ev = NULL;

    ev = getNextElementWithState(curr->lQueue.base, curr->lQueue.end, curr->lQueue.base, EVENT_READY);
    if(ev){
        mvLog(MVLOG_DEBUG,"ready %s %d \n",
              TypeToStr((int)ev->packet.header.type),
              (int)ev->packet.header.id);
    }
    return ev;
}

static xLinkEventPriv_t* getNextQueueElemToProc(eventQueueHandler_t *q ){
    xLinkEventPriv_t* event = NULL;
    if (q->cur != q->curProc) {
        event = getNextElementWithState(q->base, q->end, q->curProc, EVENT_SERVED);
        q->curProc = event;
        CIRCULAR_INCREMENT(q->curProc, q->end, q->base);
    }
    return event;
}


static xLinkEvent_t* addNextQueueElemToProc(xLinkSchedulerState_t* curr,
                                            eventQueueHandler_t *q, xLinkEvent_t* event,
                                            sem_t* sem, xLinkEventOrigin_t o){
    xLinkEvent_t* ev;
    xLinkEventPriv_t* eventP = getNextElementWithState(q->base, q->end, q->cur, EVENT_SERVED);
    mvLog(MVLOG_DEBUG,"received event %s %d\n",TypeToStr(event->header.type), o);
    ev = &eventP->packet;
    if (eventP->sem)
        unrefSem(eventP->sem,  curr);
    eventP->sem = sem;
    eventP->packet = *event;
    eventP->origin = o;
    q->cur = eventP;
    CIRCULAR_INCREMENT(q->cur, q->end, q->base);
    return ev;
}

static xLinkEventPriv_t* dispatcherGetNextEvent(xLinkSchedulerState_t* curr)
{
    ASSERT_X_LINK(curr != NULL);

    xLinkEventPriv_t* event = NULL;
    event = searchForReadyEvent(curr);
    if (event) {
        return event;
    }
    if (sem_wait(&curr->notifyDispatcherSem)) {
        mvLog(MVLOG_ERROR,"can't post semaphore\n");
    }
    event = getNextQueueElemToProc(&curr->lQueue);
    if (event) {
        return event;
    }
    event = getNextQueueElemToProc(&curr->rQueue);
    return event;
}

static void dispatcherReset(xLinkSchedulerState_t* curr)
{
    ASSERT_X_LINK(curr != NULL);

    glControlFunc->closeLink(curr->xLinkFD);
    if (sem_post(&curr->notifyDispatcherSem)) {
        mvLog(MVLOG_ERROR,"can't post semaphore\n"); //to allow us to get a NULL event
    }
    xLinkEventPriv_t* event = dispatcherGetNextEvent(curr);
    while (event != NULL) {
       event = dispatcherGetNextEvent(curr);
    }
    event = getNextElementWithState(curr->lQueue.base, curr->lQueue.end, curr->lQueue.base, EVENT_PENDING);
    while (event != NULL) {
        markEventServed(event);
        event = getNextElementWithState(curr->lQueue.base, curr->lQueue.end, curr->lQueue.base, EVENT_PENDING);
    }
    glControlFunc->resetDevice(curr->xLinkFD);
    curr->schedulerId = -1;
    numSchedulers--;
    mvLog(MVLOG_INFO,"Reset Successfully\n");
}
#if (defined(_WIN32) || defined(_WIN64))
static void* __cdecl eventSchedulerRun(void* ctx)
#else
static void* eventSchedulerRun(void* ctx)
#endif
{
    int schedulerId = *((int*) ctx);
    mvLog(MVLOG_DEBUG,"%s() schedulerId %d\n", __func__, schedulerId);
    ASSERT_X_LINK(schedulerId < MAX_SCHEDULERS);

    xLinkSchedulerState_t* curr = &schedulerState[schedulerId];
    pthread_t readerThreadId;        /* Create thread for reader.
                        This thread will notify the dispatcher of any incoming packets*/
    pthread_attr_t attr;
    int sc;
    int res;
    if (pthread_attr_init(&attr) !=0) {
        mvLog(MVLOG_ERROR,"pthread_attr_init error");
    }
#ifndef __PC__
    if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0) {
        mvLog(MVLOG_ERROR,"pthread_attr_setinheritsched error");
    }
    if (pthread_attr_setschedpolicy(&attr, SCHED_RR) != 0) {
        mvLog(MVLOG_ERROR,"pthread_attr_setschedpolicy error");
    }
#endif
    sc = pthread_create(&readerThreadId, &attr, eventReader, &schedulerId);
    if (sc) {
        perror("Thread creation failed");
    }
    xLinkEventPriv_t* event;
    xLinkEventPriv_t response;

    while (!curr->resetXLink) {
        event = dispatcherGetNextEvent(curr);
        ASSERT_X_LINK(event->packet.xLinkFD == curr->xLinkFD);
        getRespFunction getResp;
        xLinkEvent_t* toSend;

        if (event->origin == EVENT_LOCAL){
            getResp = glControlFunc->localGetResponse;
            toSend = &event->packet;
        }else{
            getResp = glControlFunc->remoteGetResponse;
            toSend = &response.packet;
        }

        res = getResp(&event->packet, &response.packet);
        if (isEventTypeRequest(event)){
            if (event->origin == EVENT_LOCAL){ //we need to do this for locals only
                dispatcherRequestServe(event, curr);
            }
            if (res == 0 && event->packet.header.flags.bitField.localServe == 0){
                glControlFunc->eventSend(toSend);
            }
        }else{
            if (event->origin == EVENT_REMOTE){ // match remote response with the local request
                dispatcherResponseServe(event, curr);
            }
        }

        //TODO: dispatcher shouldn't know about this packet. Seems to be easily move-able to protocol
        if (event->origin == EVENT_REMOTE){
            if (event->packet.header.type == USB_RESET_REQ) {
                curr->resetXLink = 1;
            }
        }
    }
    pthread_join(readerThreadId, NULL);
    dispatcherReset(curr);
    return NULL;
}

static int createUniqueID()
{
    static int id = 0xa;
    return id++;
}

static xLinkSchedulerState_t* findCorrespondingScheduler(void* xLinkFD)
{
    int i;
    if (xLinkFD == NULL) { //in case of myriad there should be one scheduler
        if (numSchedulers == 1)
            return &schedulerState[0];
        else
            NULL;
    }
    for (i=0; i < MAX_SCHEDULERS; i++)
        if (schedulerState[i].schedulerId != -1 &&
            schedulerState[i].xLinkFD == xLinkFD)
            return &schedulerState[i];

    return NULL;
}
///////////////// External Interface //////////////////////////
/*Adds a new event with parameters and returns event id*/
xLinkEvent_t* dispatcherAddEvent(xLinkEventOrigin_t origin, xLinkEvent_t *event)
{
    xLinkSchedulerState_t* curr = findCorrespondingScheduler(event->xLinkFD);
    ASSERT_X_LINK(curr != NULL);

    if(curr->resetXLink) {
        return NULL;
    }
    mvLog(MVLOG_DEBUG,"receiving event %s %d\n",TypeToStr(event->header.type), origin);
    if (sem_wait(&curr->addEventSem)) {
        mvLog(MVLOG_ERROR,"can't wait semaphore\n");
    }
    sem_t *sem = NULL;
    xLinkEvent_t* ev;
    if (origin == EVENT_LOCAL) {
        event->header.id = createUniqueID();
        sem = getCurrentSem(pthread_self(), curr, 1);
        if (!sem) {

            sem = createSem(curr);
        }
        if (!sem) {
            mvLog(MVLOG_WARN,"No more semaphores. Increase XLink or OS resources\n");
            return NULL;
        }
        event->header.flags.raw = 0;
        event->header.flags.bitField.ack = 1;
        ev = addNextQueueElemToProc(curr, &curr->lQueue, event, sem, origin);
    } else {
        ev = addNextQueueElemToProc(curr, &curr->rQueue, event, NULL, origin);
    }
    if (sem_post(&curr->addEventSem)) {
        mvLog(MVLOG_ERROR,"can't post semaphore\n");
    }
    if (sem_post(&curr->notifyDispatcherSem)) {
        mvLog(MVLOG_ERROR, "can't post semaphore\n");
    }
    return ev;
}

int dispatcherWaitEventComplete(void* xLinkFD)
{
    xLinkSchedulerState_t* curr = findCorrespondingScheduler(xLinkFD);
    ASSERT_X_LINK(curr != NULL);

    sem_t* id = getCurrentSem(pthread_self(), curr, 0);
    if (id == NULL) {
        return -1;
    }

    return sem_wait(id);
}

int dispatcherUnblockEvent(eventId_t id, xLinkEventType_t type, streamId_t stream, void* xLinkFD)
{
    xLinkSchedulerState_t* curr = findCorrespondingScheduler(xLinkFD);
    ASSERT_X_LINK(curr != NULL);

    mvLog(MVLOG_DEBUG,"unblock\n");
    xLinkEventPriv_t* blockedEvent;
    for (blockedEvent = curr->lQueue.q;
         blockedEvent < curr->lQueue.q + MAX_EVENTS;
         blockedEvent++)
    {
        if (blockedEvent->isServed == EVENT_BLOCKED &&
            ((blockedEvent->packet.header.id == id || id == -1)
            && blockedEvent->packet.header.type == type
            && blockedEvent->packet.header.streamId == stream))
        {
            mvLog(MVLOG_DEBUG,"unblocked**************** %d %s\n",
                  (int)blockedEvent->packet.header.id,
                  TypeToStr((int)blockedEvent->packet.header.type));
            markEventReady(blockedEvent);
            return 1;
        } else {
            mvLog(MVLOG_DEBUG,"%d %s\n",
                  (int)blockedEvent->packet.header.id,
                  TypeToStr((int)blockedEvent->packet.header.type));
        }
    }
    return 0;
}

int findAvailableScheduler()
{
    int i;
    for (i = 0; i < MAX_SCHEDULERS; i++)
        if (schedulerState[i].schedulerId == -1)
            return i;
    return -1;
}

int dispatcherStart(void* fd)
{
    pthread_attr_t attr;
    int eventIdx;
    if (numSchedulers >= MAX_SCHEDULERS)
    {
        mvLog(MVLOG_ERROR,"Max number Schedulers reached!\n");
        return -1;
    }
    int idx = findAvailableScheduler();

    schedulerState[idx].semaphores = 0;

    schedulerState[idx].resetXLink = 0;
    schedulerState[idx].xLinkFD = fd;
    schedulerState[idx].schedulerId = idx;

    schedulerState[idx].lQueue.cur = schedulerState[idx].lQueue.q;
    schedulerState[idx].lQueue.curProc = schedulerState[idx].lQueue.q;
    schedulerState[idx].lQueue.base = schedulerState[idx].lQueue.q;
    schedulerState[idx].lQueue.end = &schedulerState[idx].lQueue.q[MAX_EVENTS];

    schedulerState[idx].rQueue.cur = schedulerState[idx].rQueue.q;
    schedulerState[idx].rQueue.curProc = schedulerState[idx].rQueue.q;
    schedulerState[idx].rQueue.base = schedulerState[idx].rQueue.q;
    schedulerState[idx].rQueue.end = &schedulerState[idx].rQueue.q[MAX_EVENTS];

    for (eventIdx = 0 ; eventIdx < MAX_EVENTS; eventIdx++)
    {
        schedulerState[idx].rQueue.q[eventIdx].isServed = EVENT_SERVED;
        schedulerState[idx].lQueue.q[eventIdx].isServed = EVENT_SERVED;
    }

    if (sem_init(&schedulerState[idx].addEventSem, 0, 1)) {
        perror("Can't create semaphore\n");
        return -1;
    }
    if (sem_init(&schedulerState[idx].notifyDispatcherSem, 0, 0)) {
        perror("Can't create semaphore\n");
    }
    localSem_t* temp = schedulerState[idx].eventSemaphores;
    while (temp < schedulerState[idx].eventSemaphores + MAXIMUM_SEMAPHORES) {
        temp->refs = -1;
        temp++;
    }
    if (pthread_attr_init(&attr) != 0) {
        mvLog(MVLOG_ERROR,"pthread_attr_init error");
    }
#ifndef __PC__
    if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0) {
        mvLog(MVLOG_ERROR,"pthread_attr_setinheritsched error");
    }
    if (pthread_attr_setschedpolicy(&attr, SCHED_RR) != 0) {
        mvLog(MVLOG_ERROR,"pthread_attr_setschedpolicy error");
    }

#endif
    sem_wait(&addSchedulerSem);
    mvLog(MVLOG_DEBUG,"%s() starting a new thread - schedulerId %d \n", __func__, idx);
    int sc = pthread_create(&schedulerState[idx].xLinkThreadId,
                            &attr,
                            eventSchedulerRun,
                            (void*)&schedulerState[idx].schedulerId);
    if (sc) {
        perror("Thread creation failed");
    }
    numSchedulers++;
    sem_post(&addSchedulerSem);

    return 0;
}

int dispatcherInitialize(struct dispatcherControlFunctions* controlFunc) {
    // create thread which will communicate with the pc

    int i;
    if (!controlFunc ||
        !controlFunc->eventReceive ||
        !controlFunc->eventSend ||
        !controlFunc->localGetResponse ||
        !controlFunc->remoteGetResponse)
    {
        return -1;
    }

    glControlFunc = controlFunc;
    if (sem_init(&addSchedulerSem, 0, 1)) {
        perror("Can't create semaphore\n");
    }
    numSchedulers = 0;
    for (i = 0; i < MAX_SCHEDULERS; i++){
        schedulerState[i].schedulerId = -1;
    }
#ifndef __PC__
    return dispatcherStart(NULL); //myriad has one
#else
    return 0;
#endif
}

/* end of file */
