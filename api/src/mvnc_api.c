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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#if (defined(_WIN32) || defined(_WIN64) )
#include "gettime.h"
#else
#include <dlfcn.h>  // For dladdr
#include <unistd.h>
#include <dirent.h>
#endif

#include "mvnc.h"
#include "fp16.h"

#include "XLink.h"
#include "ncCommPrivate.h"
#include "ncPrivateTypes.h"

#define MVLOG_UNIT_NAME ncAPI
#include "mvLog.h"
#include "mvMacros.h"

#define BLOB_STREAM_SIZE 4096
#define NC_MAX_OPTIMISATIONS       40
#define NC_OPTIMISATION_NAME_LEN   50
#define NC_OPTIMISATION_LIST_BUFFER_SIZE (NC_MAX_OPTIMISATIONS * NC_OPTIMISATION_NAME_LEN)

#define CONFIG_STREAM_SIZE NC_OPTIMISATION_LIST_BUFFER_SIZE

#define MAX_PATH_LENGTH         255
#define STATUS_WAIT_TIMEOUT     15

#define SLEEP_MS        250
#define MAX_ITERATIONS  20

#define GRAPH_CLASS0_BASE   1000
#define DEVICE_CLASS0_BASE  2000
#define OPTION_CLASS_SIZE   100

#define FP16_DATA_SIZE 2

static int initialized = 0;
static int loglevel_initialized = 0;
static unsigned int api_version[4] = { 0 };

static pthread_mutex_t globalMutex = PTHREAD_MUTEX_INITIALIZER;
static XLinkGlobalHandler_t ghandler;

#ifndef EXCLUDE_HIGHCLASS
extern ncStatus_t setDeviceOptionClass2(struct _devicePrivate_t *d,
                                        int option, const void *data,
                                        unsigned int dataLength);

extern ncStatus_t setDeviceOptionClass3(struct _devicePrivate_t *d,
                                        int option, const void *data,
                                        unsigned int dataLength);

extern ncStatus_t getDeviceOptionClass2(struct _devicePrivate_t *d,
                                        int option, void *data,
                                        unsigned int *dataLength);

extern ncStatus_t getDeviceOptionClass3(struct _devicePrivate_t *d,
                                        int option, void *data,
                                        unsigned int *dataLength);

extern ncStatus_t getGraphOptionClass2(struct _graphPrivate_t *g,
                                       int option, void *data,
                                       unsigned int *dataLength);

extern ncStatus_t getGraphOptionClass3(struct _graphPrivate_t *g,
                                       int option, void *data,
                                       unsigned int *dataLength);

extern ncStatus_t setGraphOptionClass2(struct _graphPrivate_t *g,
                                       int option, const void *data,
                                       unsigned int dataLength);

extern ncStatus_t setGraphOptionClass3(struct _graphPrivate_t *g,
                                       int option, const void *data,
                                       unsigned int dataLength);
#endif

static double timeInSeconds()
{
    static double s;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (!s)
        s = ts.tv_sec + ts.tv_nsec * 1e-9;
    return ts.tv_sec + ts.tv_nsec * 1e-9 - s;
}

static char *getProductName(const char *name)
{
    char *p = strchr(name, '-');
    if (p == NULL)
        return "";
    return p;
}

static ncOptionClass_t getOptionClass(int option, int base)
{
    return (int) ((option - base) / OPTION_CLASS_SIZE);
}

#if (defined(_WIN32) || defined(_WIN64) )
#define MAX_2(a,b)		((a) > (b) ? (a) : (b))
#define MAX_3(a,b,c)	((a) > (b) ? MAX_2((a), (c)) : MAX_2((b), (c)))
#ifdef MAX
#undef MAX
#define MAX MAX_2
#endif
#else
#define MAX_3(a,b,c)                            \
    ({ __typeof__ (a) _a = (a);                 \
        __typeof__ (b) _b = (b);                \
        __typeof__ (c) _c = (c);                \
        (_a > _b && _a > _c) ? _a : ((_b > _c && _b > _a) ? _b : _c); })
#endif

static ncFifoLayout_t getLayout(struct ncTensorDescriptor_t* td) {
    unsigned int max = MAX_3(td->hStride, td->wStride, td->cStride);
    if (max == td->hStride) {
        if (MAX(td->wStride, td->cStride) == td->wStride)
            return NC_FIFO_HWC;
        else
            return NC_FIFO_HCW;
    } else if (max == td->cStride) {
        if (MAX(td->wStride, td->hStride) == td->hStride)
            return NC_FIFO_CHW;
        else
            return NC_FIFO_CWH;
    } else { //W is major
        if (MAX(td->hStride, td->cStride) == td->hStride)
            return NC_FIFO_WHC;
        else
            return NC_FIFO_WCH;
    }
}

static void convertDataTypeAndLayout(const unsigned char* src, unsigned char* dst,
                          const struct ncTensorDescriptor_t* srcTd,
                          const struct ncTensorDescriptor_t* dstTd,
                          ncFifoDataType_t srcType,
                          ncFifoDataType_t dstType) {
    mvLog(MVLOG_DEBUG, "src data type %d dst data type %d\n", srcType, dstType);
    mvLog(MVLOG_DEBUG, "SRC: w %d h %d c %d w_s %d h_s %d c_s %d\n", srcTd->w,
        srcTd->h, srcTd->c, srcTd->wStride, srcTd->hStride, srcTd->cStride);
    mvLog(MVLOG_DEBUG, "DST: w %d h %d c %d w_s %d h_s %d c_s %d\n", dstTd->w,
        dstTd->h, dstTd->c, dstTd->wStride, dstTd->hStride, dstTd->cStride);
    int dataTypeSize =  dstType == NC_FIFO_FP16 ? FP16_DATA_SIZE : sizeof(float);
    for (int row = 0; row < srcTd->h; row++) { //row
        for (int col = 0; col < srcTd->w; col++) {
            for (int c = 0; c < srcTd->c; c++) {
                if (srcType == dstType) {
                    memcpy(&dst[dstTd->wStride*col + row * dstTd->hStride + c * dstTd->cStride],
                        &src[srcTd->wStride*col + row * srcTd->hStride + c * srcTd->cStride], dataTypeSize);
                } else if (srcType == NC_FIFO_FP16) {
                    //converting to FP32
                    unsigned short input = *((unsigned short*) &src[srcTd->wStride*col + row * srcTd->hStride + c * srcTd->cStride]);
                    unsigned *_dst = (unsigned *) &dst[dstTd->wStride*col + row * dstTd->hStride + c * dstTd->cStride];
                    *_dst = half2float(input);
                } else {
                    //converting to FP16
                    unsigned int input = * ((unsigned *)&src[srcTd->wStride*col + row * srcTd->hStride + c * srcTd->cStride]);
                    unsigned short *_dst = (unsigned short*) &dst[dstTd->wStride*col + row * dstTd->hStride + c * dstTd->cStride];
                    *_dst = float2half(input);
                }
            }
        }
    }
}

void printImg(unsigned char* inputTensor, struct ncTensorDescriptor_t* inputDesc) {
    for (int c = 0; c < inputDesc->c; c++) {
        for (int row = 0; row < inputDesc->h; row++) { //row
            for (int col = 0; col < inputDesc->w; col++) {
                printf("%x ", inputTensor[col + row * inputDesc->hStride +
                        c * inputDesc->cStride]);
            }
            printf(" ===== ROW %d (channel %d) Done === \n", row, c);
        }
        printf("\n");
    }
}

static void resetAll()
{
    int index = 0;
    int stalled_count = 0;
    int iters = 0;
    int bootrom_count = 0;
    int after_reset_count = 0;
    char name[NC_MAX_NAME_SIZE] = "";
    XLinkError_t rc;

    while (1) {
        rc = XLinkGetDeviceName(index, name, NC_MAX_NAME_SIZE);
        if (rc != X_LINK_SUCCESS)
            break;  //no more devices found

        if (strlen(getProductName(name)) == 1) {    //name doesn't have product number
            //device is already booted, need to reset
            mvLog(MVLOG_DEBUG, "Found stalled device %s\n", name);
            XLinkHandler_t *handler = calloc(1, sizeof(XLinkHandler_t));

            if (!handler) {
                mvLog(MVLOG_ERROR, "Memory allocation failed");
                break;
            }
            handler->devicePath = (char *) name;
            if (XLinkConnect(handler) != X_LINK_SUCCESS) {
                mvLog(MVLOG_ERROR, "Failed to connect to stalled device\n");
            }
            else {
                stalled_count++;
            }
            free(handler);

        } else {
            bootrom_count++;
        }
        index++;
    }

    if (stalled_count) {
        mvLog(MVLOG_INFO, "Stalled devices found, Reseting...");
        XLinkResetAll();
#if (!defined(_WIN32) && !defined(_WIN64) )
        usleep(500000); //device takes some time to re-enumrate, wait till it's back
#endif
        iters = 0;

        while ((after_reset_count < bootrom_count + stalled_count) &&
               iters < MAX_ITERATIONS) {
            usleep(SLEEP_MS * 1000);
            after_reset_count = 0;
            index = 0;
            while (1) {
                XLinkError_t rc = XLinkGetDeviceName(index, name,
                                                     NC_MAX_NAME_SIZE);
                if (rc != X_LINK_SUCCESS)
                    break;  //no more devices found

                if (strlen(getProductName(name)) > 1) { //name has product number
                    after_reset_count++;
                }
                index++;
            }
            iters++;
            mvLog(MVLOG_INFO, "...");
        }
        usleep(SLEEP_MS * 1000);
    }
}

static void initialize_loglevel()
{
    if (!loglevel_initialized) {
        mvLogLevelSet(MVLOG_WARN);
        mvLogDefaultLevelSet(MVLOG_WARN);   //Allow turning off warnings and errors
    }
    loglevel_initialized = 1;
}

static void initialize()
{
    if (!loglevel_initialized)
        initialize_loglevel();

    initialized = 1;
    int sc = XLinkInitialize(&ghandler);    //need to be called once
    if (sc != X_LINK_SUCCESS) {
        mvLog(MVLOG_ERROR, "Initialization failed\n");
    }
#ifndef XLINK_NO_BOOT
    resetAll();
#endif
}

ncStatus_t ncDeviceCreate(int index, struct ncDeviceHandle_t **deviceHandle)
{
    mvLog(MVLOG_DEBUG, "ncDeviceCreate index %d\n", index);
    if (index < 0) {
        mvLog(MVLOG_ERROR, "Invalid index");
        return NC_INVALID_PARAMETERS;
    }
    if (!deviceHandle) {
        mvLog(MVLOG_ERROR, "Device Handle is NULL");
        return NC_INVALID_HANDLE;
    }

    char name[NC_MAX_NAME_SIZE] = "";

    pthread_mutex_lock(&globalMutex);
    if (!initialized)
        initialize();

    XLinkError_t rc = XLinkGetDeviceName(index, name, NC_MAX_NAME_SIZE);
    pthread_mutex_unlock(&globalMutex);

    if (rc == X_LINK_SUCCESS) {
        struct ncDeviceHandle_t *dH = calloc(1, sizeof(*dH));
        struct _devicePrivate_t *d = calloc(1, sizeof(*d));
        dH->private_data = d;
        d->state = NC_DEVICE_CREATED;
        d->dev_addr = strdup(name);
        d->dev_addr2 = NULL;
        d->device_mon_stream_id = INVALID_LINK_ID;
        d->graph_monitor_stream_id = INVALID_LINK_ID;
        *deviceHandle = dH;
    }
    switch (rc) {
    case X_LINK_SUCCESS:
        return NC_OK;
    case X_LINK_DEVICE_NOT_FOUND:
        return NC_DEVICE_NOT_FOUND;
    case X_LINK_TIMEOUT:
        return NC_TIMEOUT;
    default:
        return NC_ERROR;
    }
}

static ncStatus_t getDevAttributes(struct _devicePrivate_t *d)
{
    pthread_mutex_lock(&d->dev_stream_m);
    deviceCommand_t config;
    config.type.c0 = CLASS0_DEVICE_CAPABILITIES;
    config.optionClass = NC_OPTION_CLASS0;
    if (XLinkWriteData(d->device_mon_stream_id, (const uint8_t *) &config,
                       sizeof(config)) != 0) {
        pthread_mutex_unlock(&d->dev_stream_m);
        return NC_ERROR;
    }
    streamPacketDesc_t *packet;
    if (XLinkReadData(d->device_mon_stream_id, &packet) != 0) {
        pthread_mutex_unlock(&d->dev_stream_m);
        return NC_ERROR;
    }
    pthread_mutex_unlock(&d->dev_stream_m);
    if (packet->length != sizeof(d->dev_attr)) {
        mvLog(MVLOG_WARN, "Broken protocol. DevData can't be read\n");
        XLinkReleaseData(d->device_mon_stream_id);
        return NC_ERROR;
    }
    d->dev_attr = *(deviceCapabilities_t *) packet->data;
    XLinkReleaseData(d->device_mon_stream_id);
    mvLog(MVLOG_INFO, "Device attributes\n");
    mvLog(MVLOG_INFO, "Device FW version: %x.%x.%x.%x\n",
          d->dev_attr.fw_version[0], d->dev_attr.fw_version[1],
          d->dev_attr.fw_version[2], d->dev_attr.fw_version[3]);
    mvLog(MVLOG_INFO, "mvTensorVersion %d.%d \n",
          d->dev_attr.mv_tensor_version[0], d->dev_attr.mv_tensor_version[1]);
    mvLog(MVLOG_INFO, "Maximum graphs: %d\n", d->dev_attr.max_graphs);
    mvLog(MVLOG_INFO, "Maximum fifos: %d\n", d->dev_attr.max_fifos);
    mvLog(MVLOG_INFO, "Maximum graph option class: %d\n",
          d->dev_attr.max_graph_opt_class);
    mvLog(MVLOG_INFO, "Maximum device option class: %d\n",
          d->dev_attr.max_device_opt_class);
    mvLog(MVLOG_INFO, "Device memory capacity: %d\n", d->dev_attr.max_memory);
    return NC_OK;
}


static ncStatus_t getThermalStats(struct _devicePrivate_t *d)
{
    if (!d->thermal_stats) {
        d->thermal_stats = calloc(NC_THERMAL_BUFFER_SIZE + sizeof(float), 1);   //extra space for throttling level
        if (!d->thermal_stats)
            return NC_OUT_OF_MEMORY;
    }
    deviceCommand_t config;
    config.type.c0 = CLASS0_THERMAL_STATS;
    config.optionClass = NC_OPTION_CLASS0;
    pthread_mutex_lock(&d->dev_stream_m);
    if (XLinkWriteData(d->device_mon_stream_id, (const uint8_t *) &config,
                       sizeof(config)) != 0) {
        pthread_mutex_unlock(&d->dev_stream_m);
        return NC_ERROR;
    }
    streamPacketDesc_t *packet;

    if (XLinkReadData(d->device_mon_stream_id, &packet) != 0) {
        pthread_mutex_unlock(&d->dev_stream_m);
        return NC_ERROR;
    }
    pthread_mutex_unlock(&d->dev_stream_m);
    if (packet->length != (NC_THERMAL_BUFFER_SIZE + sizeof(float))) {
        return NC_ERROR;
    }
    memcpy(d->thermal_stats, packet->data, packet->length);
    XLinkReleaseData(d->device_mon_stream_id);
    return NC_OK;
}

static ncStatus_t deviceGetDeviceMemory(struct _devicePrivate_t *d,
                                        uint32_t * mem)
{
    deviceCommand_t config;
    config.type.c0 = CLASS0_DEVICE_USED_MEMORY;
    config.optionClass = NC_OPTION_CLASS0;
    pthread_mutex_lock(&d->dev_stream_m);
    if (XLinkWriteData(d->device_mon_stream_id, (const uint8_t *) &config,
                       sizeof(config)) != 0) {
        pthread_mutex_unlock(&d->dev_stream_m);
        return NC_ERROR;
    }
    streamPacketDesc_t *packet;

    if (XLinkReadData(d->device_mon_stream_id, &packet) != 0) {
        pthread_mutex_unlock(&d->dev_stream_m);
        return NC_ERROR;
    }
    pthread_mutex_unlock(&d->dev_stream_m);
    if (packet->length != (sizeof(uint32_t))) {
        return NC_ERROR;
    }
    memcpy(mem, packet->data, packet->length);
    XLinkReleaseData(d->device_mon_stream_id);
    return NC_OK;
}

static int isDeviceOpened(const char *name)
{
    struct _devicePrivate_t *d = devices;
    while (d) {
        if (strcmp(d->dev_addr, name) == 0 ||
            (d->dev_addr2 != NULL && strcmp(d->dev_addr2, name) == 0))
            return 0;
        d = d->next;
    }
    return -1;
}

ncStatus_t ncDeviceOpen(struct ncDeviceHandle_t * deviceHandle)
{
    char mv_cmd_file_path[MAX_PATH_LENGTH], *p;
    char mv_cmd_file_name[40] = "mvnc/MvNCAPI-maXXXX.mvcmd";
    char name2[NC_MAX_NAME_SIZE] = "";

    if (!deviceHandle || !deviceHandle->private_data) {
        mvLog(MVLOG_ERROR, "Handle is NULL or has been destroyed");
        return NC_INVALID_HANDLE;
    }
    struct _devicePrivate_t *d = deviceHandle->private_data;

    pthread_mutex_lock(&globalMutex);
    if (!initialized)
        initialize();

    // Search the mvnc executable in the same directory of this library, under mvnc
    // in the future there will ideally be one FW file for all, for now they are seperate
    sprintf(mv_cmd_file_name, "mvnc/MvNCAPI%s.mvcmd",
            getProductName(d->dev_addr));
#if (defined(_WIN32) || defined(_WIN64) )
    HMODULE hm = NULL;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR) "ncDeviceOpen", &hm)) {
        int ret = GetLastError();
        fprintf(stderr, "GetModuleHandle returned %d", ret);
    }
    GetModuleFileNameA(hm, mv_cmd_file_path, sizeof(mv_cmd_file_path));
    p = strrchr(mv_cmd_file_path, '\\');
#else
    Dl_info info;
    dladdr(ncDeviceOpen, &info);
    strncpy(mv_cmd_file_path, info.dli_fname, sizeof(mv_cmd_file_path) - 40);
    p = strrchr(mv_cmd_file_path, '/');
#endif
    if (p)
        strcpy(p + 1, mv_cmd_file_name);
    else
        strcpy(mv_cmd_file_path, mv_cmd_file_name);

    mvLog(MVLOG_DEBUG, "File path %s\n", mv_cmd_file_path);

    int rc = XLinkBootRemote(d->dev_addr, mv_cmd_file_path);
    if (rc)
        mvLog(MVLOG_WARN, "%s() XLinkBootRemote returned error %d\n", __func__, rc);
    else
        mvLog(MVLOG_INFO, "%s() XLinkBootRemote returned success %d\n", __func__, rc);

    double waittm = timeInSeconds() + STATUS_WAIT_TIMEOUT;
    while (timeInSeconds() < waittm && rc == 0) {
        XLinkHandler_t *handler = calloc(1, sizeof(XLinkHandler_t));

        handler->devicePath = (char *) d->dev_addr;
        rc = XLinkConnect(handler);
        if (rc != X_LINK_SUCCESS) {
            //we might fail in case name changed after boot
            int count = 0;
            while (1) {
                name2[0] = '\0';
                rc = XLinkGetDeviceName(count, name2, NC_MAX_NAME_SIZE);
                if (rc != X_LINK_SUCCESS)
                    break;
                handler->devicePath = (char *) name2;
                rc = XLinkConnect(handler);
                if (isDeviceOpened(name2) < 0 && rc == X_LINK_SUCCESS) {
                    break;
                }
                count++;
            }
        }

        if (rc != X_LINK_SUCCESS) {
            mvLog(MVLOG_WARN, "failed to find device\n");
            return NC_ERROR;
        }
        mvLog(MVLOG_INFO, "XLinkConnect done - link Id %d\n", handler->linkId);

        if (strlen(name2) > 0) {
            d->dev_addr2 = strdup(name2);
        }

        d->usb_link = handler;
        d->next = devices;
        pthread_mutex_init(&d->dev_data_m, 0);
        pthread_mutex_init(&d->dev_stream_m, 0);
        pthread_mutex_init(&d->graph_streamm, 0);

        devices = d;

        mvLog(MVLOG_DEBUG, "done\n");
        mvLog(MVLOG_INFO, "Booted %s -> %s\n",
              d->dev_addr, d->dev_file ? d->dev_file : "VSC");
        pthread_mutex_unlock(&globalMutex);
        sleep(1);   //Allow device to initialize the XLink
        streamId_t streamId = XLinkOpenStream(d->usb_link->linkId, "deviceMonitor",
                                                CONFIG_STREAM_SIZE);
        if (streamId == INVALID_STREAM_ID) {
            mvLog(MVLOG_WARN, "can't open stream\n");
            return NC_ERROR;
        }
        d->device_mon_stream_id = streamId;
        getDevAttributes(d);


        streamId = XLinkOpenStream(d->usb_link->linkId, "graphMonitor",
                                    BLOB_STREAM_SIZE);
        if (streamId == INVALID_STREAM_ID) {
            mvLog(MVLOG_WARN, "can't open stream\n");
            return NC_ERROR;
        }
        d->graph_monitor_stream_id = streamId;
        d->state = NC_DEVICE_OPENED;
        return NC_OK;
    }

    pthread_mutex_unlock(&globalMutex);
    return NC_ERROR;
}

static int findDevice(struct _devicePrivate_t *deviceHandle)
{

    struct _devicePrivate_t *d = devices;

    while (d) {
        if (d == deviceHandle)
            return 0;
        d = d->next;
    }

    return -1;
}

static int deviceGetNumberOfGraphs(struct _devicePrivate_t *deviceHandle)
{
    if (deviceHandle == NULL)
        return 0;
    int num = 0;
    struct _graphPrivate_t *g = deviceHandle->graphs;
    while (g) {
        num++;
        g = g->next;
    }
    return num;
}

static int deviceGetNumberOfFifos(struct _devicePrivate_t *deviceHandle)
{
    if (deviceHandle == NULL)
        return 0;
    int num = 0;
    struct _fifoPrivate_t *f = deviceHandle->fifos;
    while (f) {
        num++;
        f = f->next;
    }
    return num;
}

static int findGraph(struct _graphPrivate_t *graphHandle)
{
    struct _devicePrivate_t *d = devices;

    while (d) {
        struct _graphPrivate_t *g = d->graphs;
        while (g) {
            if (g == graphHandle)
                return 0;
            g = g->next;
        }
        d = d->next;
    }

    return -1;
}

// Defined here as it will be used twice
static int deallocateGraph(struct _graphPrivate_t *g)
{
    int found = 0;

    // Remove it from the list of the associated device
    if (g->dev->graphs == g) {
        g->dev->graphs = g->next;
        found = 1;
    } else {
        struct _graphPrivate_t *gp = g->dev->graphs;
        while (gp->next) {
            if (gp->next == g) {
                found = 1;
                gp->next = gp->next->next;
                break;
            }
            gp = gp->next;
        }
    }

    // Free it with all its data
    if (found) {
        if (g->dev->thermal_stats)
            free(g->dev->thermal_stats);
        g->dev->thermal_stats = 0;
        free(g);
    }

    return -!found;
}

static int findFifo(struct _fifoPrivate_t *f)
{
    if (!f || !f->dev)
        return 0;

    if (f->dev->fifos == f) {
        return 1;
    } else {
        struct _fifoPrivate_t *fp = f->dev->fifos;
        while (fp->next) {
            if (fp->next == f) {
                return 1;
            }
            fp = fp->next;
        }
    }
    return 0;
}

static int deallocateFifo(struct _fifoPrivate_t *f)
{
    int found = 0;
    // Remove it from the list of the associated device
    if (f->dev->fifos == f) {
        f->dev->fifos = f->next;
        found = 1;
    } else {
        struct _fifoPrivate_t *fp = f->dev->fifos;
        while (fp->next) {
            if (fp->next == f) {
                found = 1;
                fp->next = fp->next->next;
                break;
            }
            fp = fp->next;
        }
    }

    // Free it with all its data
    if (found) {
        //deallocate on device
        XLinkCloseStream(f->streamId);
        struct _userParamPrivate_t *temp;
        while (f->user_param_in) {
            temp = f->user_param_in;
            f->user_param_in = f->user_param_in->next;
            free(temp);
        }
        while (f->user_param_out) {
            temp = f->user_param_out;
            f->user_param_out = f->user_param_out->next;
            free(temp);
        }
    }

    return -!found;
}

ncStatus_t ncDeviceClose(struct ncDeviceHandle_t * deviceHandle)
{
    int found = 0;

    if (!deviceHandle) {
        mvLog(MVLOG_ERROR, "handle is NULL");
        return NC_INVALID_HANDLE;
    }

    pthread_mutex_lock(&globalMutex);
    if (findDevice(deviceHandle->private_data)) {
        mvLog(MVLOG_ERROR, "handle is corrupt or has been destroyed");
        pthread_mutex_unlock(&globalMutex);
        return NC_INVALID_HANDLE;
    }
    mvLog(MVLOG_INFO, "closing device\n");

    struct _devicePrivate_t *d = deviceHandle->private_data;
    // Remove it from our list
    if (devices == d) {
        devices = d->next;
        found = 1;
    } else {
        struct _devicePrivate_t *dp = devices;
        while (dp->next) {
            if (dp->next == d) {
                found = 1;
                dp->next = dp->next->next;
                break;
            }
            dp = dp->next;
        }
    }

    if (!found) {
        pthread_mutex_unlock(&globalMutex);
        return NC_INVALID_PARAMETERS;
    }
    // Deallocate all associated graphs
    pthread_mutex_lock(&d->dev_data_m);
    if (d->graphs) {
        mvLog(MVLOG_WARN,
              "Graphs on the device hasn't been destroyed! Graphs will be deallocated");
        while (d->graphs) {
            deallocateGraph(d->graphs);
        }
    }
    // Deallocate all associated fifos
    if (d->fifos) {
        mvLog(MVLOG_WARN,
              "Fifos on the device hasn't been destroyed! Fifos will be deallocated");
        while (d->fifos) {
            deallocateFifo(d->fifos);
        }
    }
    if (d->device_mon_stream_id != INVALID_LINK_ID)
        XLinkCloseStream(d->device_mon_stream_id);
    if (d->graph_monitor_stream_id != INVALID_LINK_ID)
        XLinkCloseStream(d->graph_monitor_stream_id);
    d->device_mon_stream_id = INVALID_LINK_ID;
    d->graph_monitor_stream_id = INVALID_LINK_ID;

    // Reset
    XLinkResetRemote(d->usb_link->linkId);

//  usblink_resetmyriad(d->usb_link);
//  usblink_close(d->usb_link);

    pthread_mutex_destroy(&d->dev_stream_m);
    pthread_mutex_destroy(&d->graph_streamm);
    pthread_mutex_destroy(&d->dev_data_m);
    if (d->optimisation_list)
        free(d->optimisation_list);
    d->state = NC_DEVICE_CLOSED;
    free(d->dev_file);
    free(d->dev_addr2);
    d->dev_addr2 = NULL;
    pthread_mutex_unlock(&globalMutex);

#if (!defined(_WIN32) && !defined(_WIN64) )
    usleep(500000); //device takes some time to re-enumrate, wait till it's back
#endif
    return NC_OK;
}

ncStatus_t ncDeviceDestroy(struct ncDeviceHandle_t ** deviceHandle)
{
    if (!deviceHandle) {
        mvLog(MVLOG_ERROR, "handle is NULL");
        return NC_INVALID_HANDLE;
    }
    if (!(*deviceHandle)) {
        mvLog(MVLOG_INFO, "handle already destroyed");
        return NC_OK;
    }

    struct _devicePrivate_t *d = (*deviceHandle)->private_data;
    if (!d) {
        mvLog(MVLOG_ERROR, "device has been destroyed");
        return NC_INVALID_HANDLE;
    }
    //If only created we still delete everything below
    mvLog(MVLOG_INFO, "Destroying Device\n");
    if (d->state == NC_DEVICE_OPENED) {
        ncStatus_t rc = ncDeviceClose((*deviceHandle));
        if (rc)
            return rc;
    }
    pthread_mutex_lock(&globalMutex);
    free(d->dev_addr);
    free(d);
    (*deviceHandle)->private_data = NULL;
    free((*deviceHandle));
    *deviceHandle = NULL;
    pthread_mutex_unlock(&globalMutex);
    return NC_OK;
}

ncStatus_t ncGraphCreate(const char *name,
                         struct ncGraphHandle_t ** graphHandle)
{
    if (!loglevel_initialized)
        initialize_loglevel();

    if (!graphHandle) {
        mvLog(MVLOG_ERROR, "Graph handle is NULL");
        return NC_INVALID_HANDLE;
    }
    if (!name) {
        mvLog(MVLOG_ERROR, "name is NULL");
        return NC_INVALID_PARAMETERS;
    }
    struct ncGraphHandle_t *gH = calloc(1, sizeof(*gH));
    struct _graphPrivate_t *g = calloc(1, sizeof(*g));
    gH->private_data = g;
    strncpy(g->name, name, NC_MAX_NAME_SIZE);
    g->batch_size = 1;
    g->dev = NULL;
    g->executors_number = 1;
    g->started = 0;
    g->state = NC_GRAPH_CREATED;
    *graphHandle = gH;
    return NC_OK;
}

ncStatus_t sendGraphMonitorRequest(streamId_t graphMonStream,
                                   graphMonCommand_t * cmd)
{
    if (XLinkWriteData(graphMonStream, (uint8_t *) cmd, sizeof(*cmd)) != 0) {
        return NC_ERROR;
    }
    return NC_OK;
}

ncStatus_t checkGraphMonitorResponse(streamId_t graphMonStream)
{
    streamPacketDesc_t *ack;
    if (XLinkReadData(graphMonStream, &ack) != 0) {
        mvLog(MVLOG_ERROR, "XLink error");
        return NC_ERROR;
    }
    int value = *((int *) ack->data);
    if (XLinkReleaseData(graphMonStream) != 0) {
        mvLog(MVLOG_ERROR, "XLink error");
        return NC_ERROR;
    }
    if (value != 0) {
        mvLog(MVLOG_WARN, "Graph monitor request returned error");

        return NC_MYRIAD_ERROR;
    }

    return NC_OK;
}

static void copyTensorDescriptorDetails(const struct tensorDescriptor_t* src,
    struct ncTensorDescriptor_t* dest) {
    if (!dest || !src)
        return;
    dest->n = src->n;
    dest->w = src->w;
    dest->h = src->h;
    dest->c = src->c;
    dest->totalSize = src->totalSize;
    dest->cStride = src->channelsStride;
    dest->wStride = src->widthStride;
    dest->hStride = src->heightStride;
    dest->dataType = NC_FIFO_FP16; //graph data type is FP16
}

ncStatus_t ncGraphAllocate(struct ncDeviceHandle_t * deviceHandle,
                           struct ncGraphHandle_t * graphHandle,
                           const void *graphBuffer,
                           unsigned int graphBufferLength)
{
    ncStatus_t rc = NC_OK;
    mvLog(MVLOG_INFO, "Starting Graph allocation sequence\n");
    if (!graphHandle || !deviceHandle) {
        mvLog(MVLOG_ERROR, "Graph or device handle is NULL");
        return NC_INVALID_HANDLE;
    }
    if (!graphBuffer) {
        mvLog(MVLOG_ERROR, "graphBuffer is NULL");
        return NC_INVALID_PARAMETERS;
    }
    static int graphIdCount = 0;
    struct _graphPrivate_t *g = graphHandle->private_data;

    struct _devicePrivate_t *d = devices;
    if (graphBufferLength > d->dev_attr.max_memory) {
        mvLog(MVLOG_ERROR, "The graph file is bigger than the device memory");
        return NC_OUT_OF_MEMORY;
    }

    pthread_mutex_lock(&globalMutex);
    while (d) {
        if (d == deviceHandle->private_data)
            break;
        d = d->next;
    }

    if (!d) {
        pthread_mutex_unlock(&globalMutex);
        mvLog(MVLOG_ERROR, "Device not found!");
        return NC_INVALID_PARAMETERS;
    }
    pthread_mutex_unlock(&globalMutex);

    g->id = graphIdCount++;
    streamId_t streamId;

    if (g->executors_number > d->dev_attr.max_executors) {
        mvLog(MVLOG_ERROR, "Executors number is greater than max allowed!");
        return NC_INVALID_PARAMETERS;
    }

    graphMonCommand_t cmd;
    cmd.cmdClass = GRAPH_MON_CLASS_GRAPH_CMD;
    cmd.cmd.graphCmd.type = GRAPH_ALLOCATE_CMD;
    snprintf(cmd.cmd.graphCmd.streamName, 16, "graphBuffer%d", g->id);
    streamId = XLinkOpenStream(d->usb_link->linkId, cmd.cmd.graphCmd.streamName,
                                graphBufferLength);
    if (streamId == INVALID_STREAM_ID) {
        mvLog(MVLOG_WARN, "can't open stream for graphBuffer transmission");
        return NC_ERROR;
    }
    cmd.cmd.graphCmd.id = g->id;
    cmd.cmd.graphCmd.executors_number = g->executors_number;

    pthread_mutex_lock(&d->graph_streamm);

    if (sendGraphMonitorRequest(d->graph_monitor_stream_id, &cmd)) {
        mvLog(MVLOG_WARN, "can't send graph allocation command");
        pthread_mutex_lock(&d->graph_streamm);
        return NC_ERROR;
    }
    if (XLinkWriteData(streamId, graphBuffer, graphBufferLength) != 0) {
        mvLog(MVLOG_WARN, "can't send graph data to device");
        pthread_mutex_unlock(&d->graph_streamm);
        return NC_ERROR;
    }
    mvLog(MVLOG_INFO, "Sent graph");
    streamPacketDesc_t *tensorDescIn;
    streamPacketDesc_t *tensorDescOut;
    streamPacketDesc_t *nstages;
    streamPacketDesc_t *blob_version;

    streamPacketDesc_t *status;
    int graphStatus = -1;
    //Check for graph error
    if (XLinkReadData(streamId, &status) != 0) {
        mvLog(MVLOG_WARN, "can't read graph status");
        pthread_mutex_unlock(&d->graph_streamm);
        return NC_ERROR;
    }
    if (!status) {
        mvLog(MVLOG_ERROR, "Couldn't read graph status!\n");
        rc = NC_MYRIAD_ERROR;
    } else {
        graphStatus = *(unsigned int *) status->data;
        if (graphStatus != NC_GRAPH_OK) {
            rc = NC_MYRIAD_ERROR;
        }
    }

    XLinkReleaseData(streamId);
    mvLog(MVLOG_DEBUG, "Graph Status %d rc %d \n", graphStatus, rc);

    if (!rc) {
        XLinkReadData(streamId, &tensorDescIn);
        XLinkReadData(streamId, &tensorDescOut);
        XLinkReadData(streamId, &nstages);
        XLinkReadData(streamId, &blob_version);


        //for now, supoprt only count 1
        if (!tensorDescIn ||
            tensorDescIn->length % sizeof(struct tensorDescriptor_t) ||
            tensorDescIn->length / sizeof(struct tensorDescriptor_t) > 1) {
            mvLog(MVLOG_ERROR,
                  "Input tensor descriptors of the graph are invalid\n");
            mvLog(MVLOG_ERROR, "Received data from graph %d\n",
                  *(int *) tensorDescIn->data);
            rc = NC_MYRIAD_ERROR;
        }
        //for now, supoprt only count 1
        if (!tensorDescOut ||
            tensorDescOut->length % sizeof(struct tensorDescriptor_t) ||
            tensorDescOut->length / sizeof(struct tensorDescriptor_t) > 1) {
            mvLog(MVLOG_ERROR,
                  "Output tensor descriptors of the graph are invalid\n");
            rc = NC_MYRIAD_ERROR;
        }
        if (!blob_version || blob_version->length != sizeof(g->blob_version)) {
            mvLog(MVLOG_ERROR, "graph version is invalid blob_version \n");
            if (blob_version)
                mvLog(MVLOG_ERROR, "blob_version->length %d expecting %d\n",
                      blob_version->length, sizeof(g->blob_version));

            rc = NC_MYRIAD_ERROR;
        }
        if (rc == NC_OK) {
            g->input_count =
                tensorDescIn->length / sizeof(struct tensorDescriptor_t);
            struct tensorDescriptor_t* inDesc = (struct tensorDescriptor_t* )tensorDescIn->data;
            copyTensorDescriptorDetails(inDesc, &g->input_tensor_desc);
            mvLog(MVLOG_DEBUG,
                  "Input tensor w %d h %d c %d n %d totalSize %d wstide %d hstride %d cstride %d layout %d\n",
                   g->input_tensor_desc.w, g->input_tensor_desc.h,
                   g->input_tensor_desc.c,
                   g->input_tensor_desc.n, g->input_tensor_desc.totalSize,
                   inDesc->widthStride, inDesc->heightStride,
                   inDesc->channelsStride, getLayout(&g->input_tensor_desc));
            g->output_count =
                tensorDescOut->length / sizeof(struct tensorDescriptor_t);

            struct tensorDescriptor_t* outDesc = (struct tensorDescriptor_t* )tensorDescOut->data;
            copyTensorDescriptorDetails(outDesc, &g->output_tensor_desc);

            mvLog(MVLOG_DEBUG,
                  "output tensor w %d h %d c %d n %d totalSize %d wstide %d hstride %d cstride %d layout %d\n",
                  g->output_tensor_desc.w, g->output_tensor_desc.h,
                  g->output_tensor_desc.c, g->output_tensor_desc.n,
                  g->output_tensor_desc.totalSize,
                  outDesc->widthStride, outDesc->heightStride,
                  outDesc->channelsStride, getLayout(&g->output_tensor_desc));

            g->nstages = *(uint32_t *) nstages->data;
            memcpy(&g->blob_version, blob_version->data,
                   sizeof(g->blob_version));
        }

        XLinkReleaseData(streamId);
        XLinkReleaseData(streamId);
        XLinkReleaseData(streamId);
        XLinkReleaseData(streamId);

        g->graph_stream_id = streamId;
    }
    if (checkGraphMonitorResponse(d->graph_monitor_stream_id)) {
        mvLog(MVLOG_WARN, "The device didn't accept the graph\n");
        if (graphStatus != NC_GRAPH_OK) {
            if (graphStatus == NC_GRAPH_WRONG_INPUT_FORMAT) {
                mvLog(MVLOG_WARN, "graph file version is incompatible\n");
                pthread_mutex_unlock(&d->graph_streamm);
                return NC_UNSUPPORTED_GRAPH_FILE;
            } else {
                rc = NC_MYRIAD_ERROR;
            }
        }
        uint32_t memory_used;
        uint32_t length;
        ncDeviceGetOption(deviceHandle, NC_RO_DEVICE_CURRENT_MEMORY_USED,
                          &memory_used, &length);
        uint32_t remaining_memory = d->dev_attr.max_memory - memory_used;
        mvLog(MVLOG_INFO, "Remaining device memory %d\n", remaining_memory);

        if (remaining_memory < 2 * graphBufferLength) {
            mvLog(MVLOG_WARN,
                  "Remaining device memory (%d) is not enough for graph file (%d)\n",
                  remaining_memory, graphBufferLength);
        }
        pthread_mutex_unlock(&d->graph_streamm);
        return NC_ERROR;
    }
    if (rc) {
        return rc;
    }

    pthread_mutex_unlock(&d->graph_streamm);

    pthread_mutex_lock(&globalMutex);
    g->dev = d;

    if (d->graphs)
        g->next = d->graphs;
    d->graphs = g;
    g->state = NC_GRAPH_ALLOCATED;
    pthread_mutex_unlock(&globalMutex);
    mvLog(MVLOG_INFO, "Graph allocation completed successfully\n");

    return NC_OK;
}

ncStatus_t ncGraphDestroy(struct ncGraphHandle_t ** graphHandle)
{
    if (!graphHandle) {
        mvLog(MVLOG_ERROR, "graph handle is NULL");
        return NC_INVALID_HANDLE;
    }
    struct ncGraphHandle_t *gh = *graphHandle;
    if (!gh) {
        mvLog(MVLOG_INFO, "handle is already destroyed");
        return NC_OK;
    }
    struct _graphPrivate_t *g = gh->private_data;
    if (!g) {
        mvLog(MVLOG_ERROR, "graph handle is corrupt or has been destroyed");
        return NC_INVALID_HANDLE;
    }
    if (g->state == NC_GRAPH_CREATED) {
        free(g);
        gh->private_data = NULL;
        free(gh);
        *graphHandle = NULL;
        return NC_OK;
    }
    pthread_mutex_lock(&globalMutex);
    if (findGraph(g)) {
        pthread_mutex_unlock(&globalMutex);
        mvLog(MVLOG_ERROR, "This graph is corrupt or has been destroyed");
        return NC_INVALID_HANDLE;
    }

    pthread_mutex_unlock(&globalMutex);
    struct _devicePrivate_t *d = (gh->private_data)->dev;

    graphMonCommand_t cmd;
    cmd.cmdClass = GRAPH_MON_CLASS_GRAPH_CMD;
    cmd.cmd.graphCmd.type = GRAPH_DEALLOCATE_CMD;
    cmd.cmd.graphCmd.id = g->id;
    pthread_mutex_lock(&d->graph_streamm);
    if (sendGraphMonitorRequest(d->graph_monitor_stream_id, &cmd)) {
        return NC_ERROR;
    }
    if (checkGraphMonitorResponse(d->graph_monitor_stream_id)) {
        return NC_ERROR;
    }
    XLinkCloseStream(g->graph_stream_id);
    pthread_mutex_unlock(&d->graph_streamm);
    pthread_mutex_lock(&d->dev_data_m);
    if (deallocateGraph(gh->private_data)) {
        mvLog(MVLOG_ERROR, "This graph has already been destroyed");
        pthread_mutex_unlock(&d->dev_data_m);
        return NC_INVALID_PARAMETERS;
    }
    pthread_mutex_unlock(&d->dev_data_m);
    gh->private_data = NULL;
    free(gh);
    *graphHandle = NULL;
    return NC_OK;
}

static ncStatus_t setGraphOptionClass1(struct _graphPrivate_t *g,
                                       ncGraphOption_t option,
                                       const void *data,
                                       unsigned int dataLength)
{
    if (dataLength < sizeof(int)) {
        mvLog(MVLOG_ERROR, "The dataLength is smaller that required %d",
              sizeof(int));
        return NC_INVALID_DATA_LENGTH;
    }
    switch (option) {
    case NC_RW_GRAPH_EXECUTORS_NUM:
        if (g->state != NC_GRAPH_CREATED) {
            mvLog(MVLOG_ERROR, "Can't set NCE number after graph allocation");
            return NC_UNAUTHORIZED;
        }
        g->executors_number = *(int *) data;;
        break;
    default:
        mvLog(MVLOG_ERROR, "There is no such option in class 1");
        return NC_INVALID_PARAMETERS;
    }
    return NC_OK;
}

static int isGraphPreAllocateOption(int option)
{
    switch (option) {
    case NC_RO_GRAPH_NAME:
    case NC_RO_GRAPH_STATE:
    case NC_RW_GRAPH_EXECUTORS_NUM:
        return 1;
    default:
        return 0;
    }
}

ncStatus_t ncGraphSetOption(struct ncGraphHandle_t * graphHandle,
                            int option, const void *data,
                            unsigned int dataLength)
{
    if (!data) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL");
        return NC_INVALID_PARAMETERS;
    }
    if (!graphHandle || !graphHandle->private_data) {
        mvLog(MVLOG_ERROR, "graph handle is NULL or has been destroyed");
        return NC_INVALID_HANDLE;
    }
    if (option < GRAPH_CLASS0_BASE ||
        option > (GRAPH_CLASS0_BASE + OPTION_CLASS_SIZE * NC_OPTION_CLASS3)) {
        mvLog(MVLOG_ERROR, "Option %d is invalid", option);
        return NC_INVALID_PARAMETERS;
    }
    if (option >= GRAPH_CLASS0_BASE &&
        option <= (GRAPH_CLASS0_BASE + OPTION_CLASS_SIZE)) {
        mvLog(MVLOG_ERROR, "Option %d is read only", option);
        return NC_UNAUTHORIZED;
    }
    struct _graphPrivate_t *g = graphHandle->private_data;
    pthread_mutex_lock(&globalMutex);
    if (isGraphPreAllocateOption(option) && g->state != NC_GRAPH_CREATED) {
        mvLog(MVLOG_ERROR,
              "This graph has already been alocated - cannot set option");
        pthread_mutex_unlock(&globalMutex);
        return NC_UNAUTHORIZED;
    }
    if (!isGraphPreAllocateOption(option) && g->state == NC_GRAPH_CREATED) {
        mvLog(MVLOG_ERROR,
              "This graph hasn't been allocated - cannot set option");
        pthread_mutex_unlock(&globalMutex);
        return NC_UNAUTHORIZED;
    }
    if (!isGraphPreAllocateOption(option) && findGraph(g)) {
        mvLog(MVLOG_ERROR, "This graph is corrupt or has been destroyed");
        pthread_mutex_unlock(&globalMutex);
        return NC_INVALID_HANDLE;
    }
    pthread_mutex_unlock(&globalMutex);
    //we check what we can at this point, later we might fail if
    //user set a class that was not permitted
    ncOptionClass_t opClass = getOptionClass(option, GRAPH_CLASS0_BASE);
    if (g->dev != NULL && opClass > g->dev->dev_attr.max_graph_opt_class) {
        mvLog(MVLOG_ERROR, "This device FW does not support NC_OPTION_CLASS%d",
              opClass);
        return NC_UNAUTHORIZED;
    }
    ncStatus_t rc;
    switch (opClass) {
    case NC_OPTION_CLASS0:
        mvLog(MVLOG_ERROR, "Option is read-only");
        rc = NC_UNAUTHORIZED;   // option class 0 consists of read-only value
        break;
    case NC_OPTION_CLASS1:
        rc = setGraphOptionClass1(g, option, data, dataLength);
        break;
#ifndef EXCLUDE_HIGHCLASS
    case NC_OPTION_CLASS2:
        rc = setGraphOptionClass2(g, option, data, dataLength);
        break;
    case NC_OPTION_CLASS3:
        rc = setGraphOptionClass3(g, option, data, dataLength);
        break;
#endif
    default:
        mvLog(MVLOG_ERROR, "There is no such option class");
        rc = NC_INVALID_PARAMETERS;
        break;
    }
    return rc;
}

static ncStatus_t getGraphOptionClass0(struct _graphPrivate_t *g,
                                       ncGraphOption_t option,
                                       void *data, unsigned int *dataLength)
{
    if ((option == NC_RO_GRAPH_STATE ||
         option == NC_RO_GRAPH_INPUT_COUNT ||
         option == NC_RO_GRAPH_OUTPUT_COUNT ||
         option == NC_RO_GRAPH_OPTION_CLASS_LIMIT ||
         option == NC_RW_GRAPH_EXECUTORS_NUM) && *dataLength < sizeof(int)) {
        mvLog(MVLOG_ERROR,
              "data length of data (%d) is smaller that required (%d)!\n",
              *dataLength, sizeof(int));
        *dataLength = sizeof(int);
        return NC_INVALID_DATA_LENGTH;
    }

    graphMonCommand_t cmd;
    streamPacketDesc_t *pack;
    cmd.cmdClass = GRAPH_MON_CLASS_GET_CLASS0;

    switch (option) {
    case NC_RO_GRAPH_STATE:
        if (g->state == NC_GRAPH_CREATED ||
            (g->state == NC_GRAPH_ALLOCATED && !g->started)) {
            *(int *) data = g->state;
        } else {
            //it has been started we must read from graph
            cmd.cmd.optionCmd.type.c0 = CLASS0_STATE;
            cmd.cmd.optionCmd.id = g->id;
            pthread_mutex_lock(&g->dev->graph_streamm);
            if (XLinkWriteData(g->dev->graph_monitor_stream_id,
                               (const uint8_t *) &cmd, sizeof(cmd)) != 0) {
                pthread_mutex_unlock(&g->dev->graph_streamm);
                return NC_ERROR;
            }

            if (XLinkReadData(g->dev->graph_monitor_stream_id, &pack)) {
                pthread_mutex_unlock(&g->dev->graph_streamm);
                return NC_ERROR;
            }

            if (pack->length != sizeof(graphState_t)) {
                pthread_mutex_unlock(&g->dev->graph_streamm);
                XLinkReleaseData(g->dev->graph_monitor_stream_id);
                return NC_ERROR;
            }
            int state = *(int *) pack->data;
            XLinkReleaseData(g->dev->graph_monitor_stream_id);
            if (checkGraphMonitorResponse(g->dev->graph_monitor_stream_id)) {
                pthread_mutex_unlock(&g->dev->graph_streamm);
                return NC_ERROR;
            }
            pthread_mutex_unlock(&g->dev->graph_streamm);
            if (state == GRAPH_RUNNING)
                g->state = NC_GRAPH_RUNNING;
            else
                g->state = NC_GRAPH_WAITING_FOR_BUFFERS;
            *(int *) data = g->state;
        }
        *dataLength = sizeof(ncGraphState_t);
        break;
    case NC_RO_GRAPH_INPUT_COUNT:
        *(int *) data = g->input_count;
        *dataLength = sizeof(int);
        break;
    case NC_RO_GRAPH_OUTPUT_COUNT:
        *(int *) data = g->output_count;
        *dataLength = sizeof(int);
        break;
    case NC_RO_GRAPH_TIME_TAKEN_ARRAY_SIZE:
        *(int *) data = sizeof(float) * g->nstages;
        *dataLength = sizeof(int);
        break;
    case NC_RO_GRAPH_TIME_TAKEN:
        if (*dataLength < sizeof(float) * g->nstages) {
            mvLog(MVLOG_ERROR,
                  "data length of output buffer (%d) is smaller that required (%d)!\n",
                  *dataLength, sizeof(float) * g->nstages);
            *dataLength = sizeof(float) * g->nstages;
            return NC_INVALID_DATA_LENGTH;
        }
        cmd.cmd.optionCmd.id = g->id;
        cmd.cmd.optionCmd.type.c0 = CLASS0_TIMING_DATA;
        pthread_mutex_lock(&g->dev->graph_streamm);
        if (sendGraphMonitorRequest(g->dev->graph_monitor_stream_id, &cmd)) {
            pthread_mutex_unlock(&g->dev->graph_streamm);
            return NC_ERROR;
        }
        if (XLinkReadData(g->dev->graph_monitor_stream_id, &pack)) {
            pthread_mutex_unlock(&g->dev->graph_streamm);
            return NC_ERROR;
        }
        if (pack->length != sizeof(float) * g->nstages) {
            pthread_mutex_unlock(&g->dev->graph_streamm);

            XLinkReleaseData(g->dev->graph_monitor_stream_id);
            return NC_ERROR;
        }
        //Need to copy data before we check the response, since checkGraphMonitorResponse
        //calls releaseData
        memcpy((float *) data, pack->data, pack->length);
        XLinkReleaseData(g->dev->graph_monitor_stream_id);

        if (checkGraphMonitorResponse(g->dev->graph_monitor_stream_id)) {
            pthread_mutex_unlock(&g->dev->graph_streamm);
            return NC_ERROR;
        }

        pthread_mutex_unlock(&g->dev->graph_streamm);
        *dataLength = sizeof(float) * g->nstages;
        break;
    case NC_RO_GRAPH_DEBUG_INFO:
        if (*dataLength < NC_DEBUG_BUFFER_SIZE) {
            mvLog(MVLOG_ERROR,
                  "data length of output buffer (%d) is smaller that required (%d)!\n",
                  *dataLength, NC_DEBUG_BUFFER_SIZE);
            *dataLength = NC_DEBUG_BUFFER_SIZE;
            return NC_INVALID_DATA_LENGTH;
        }

        cmd.cmd.optionCmd.type.c0 = CLASS0_DEBUG_DATA;
        cmd.cmd.optionCmd.id = g->id;
        pthread_mutex_lock(&g->dev->graph_streamm);
        if (XLinkWriteData(g->dev->graph_monitor_stream_id, (const uint8_t *) &cmd,
             sizeof(cmd)) != 0) {
            pthread_mutex_unlock(&g->dev->graph_streamm);
            return NC_ERROR;
        }

        if (XLinkReadData(g->dev->graph_monitor_stream_id, &pack)) {
            pthread_mutex_unlock(&g->dev->graph_streamm);
            return NC_ERROR;
        }

        if (pack->length != NC_DEBUG_BUFFER_SIZE) {
            pthread_mutex_unlock(&g->dev->graph_streamm);
            XLinkReleaseData(g->dev->graph_monitor_stream_id);
            return NC_ERROR;
        }

        memcpy((char *) data, pack->data, pack->length);
        XLinkReleaseData(g->dev->graph_monitor_stream_id);
        if (checkGraphMonitorResponse(g->dev->graph_monitor_stream_id)) {
            pthread_mutex_unlock(&g->dev->graph_streamm);
            return NC_ERROR;
        }
        pthread_mutex_unlock(&g->dev->graph_streamm);

        *dataLength = NC_DEBUG_BUFFER_SIZE;
        break;
    case NC_RO_GRAPH_INPUT_TENSOR_DESCRIPTORS:{
            unsigned int size =
                sizeof(struct ncTensorDescriptor_t) * g->input_count;
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            memcpy((struct ncTensorDescriptor_t *) data, &g->input_tensor_desc,
                   size);
            *dataLength = size;
            break;
        }
    case NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS:{
            unsigned int size =
                sizeof(struct ncTensorDescriptor_t) * g->output_count;
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            memcpy((struct ncTensorDescriptor_t *) data, &g->output_tensor_desc,
                   size);
            *dataLength = size;
            break;
        }
    case NC_RO_GRAPH_NAME:
        if (*dataLength < strlen(g->name) + 1) {
            mvLog(MVLOG_ERROR,
                  "data length of output buffer (%d) is smaller that required (%d)!\n",
                  *dataLength, strlen(g->name) + 1);
            *dataLength = strlen(g->name) + 1;
            return NC_INVALID_DATA_LENGTH;
        }
        *dataLength = strlen(g->name) + 1;
        strncpy((char *) data, g->name, *dataLength);
        break;
    case NC_RO_GRAPH_OPTION_CLASS_LIMIT:
        *(int *) data = g->dev->dev_attr.max_graph_opt_class;
        *dataLength = sizeof(int);
        break;
    case NC_RO_GRAPH_VERSION:{
            unsigned int size = sizeof(g->blob_version);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            memcpy((int *) data, g->blob_version, size);
            *dataLength = size;
            break;
        }
    default:
        mvLog(MVLOG_ERROR, "There is no such option in class 0");
        return NC_INVALID_PARAMETERS;
    }
    return NC_OK;
}

static ncStatus_t getGraphOptionClass1(struct _graphPrivate_t *g,
                                       ncGraphOption_t option,
                                       void *data, unsigned int *dataLength)
{
    switch (option) {
    case NC_RW_GRAPH_EXECUTORS_NUM:{
            int size = sizeof(int);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of data (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            *(int *) data = g->executors_number;
            *dataLength = size;
            break;
        }
    default:
        mvLog(MVLOG_ERROR, "There is no such option in class 1");
        return NC_INVALID_PARAMETERS;
    }
    return NC_OK;
}

ncStatus_t ncGraphGetOption(struct ncGraphHandle_t * graphHandle,
                            int option, void *data, unsigned int *dataLength)
{
    if (!graphHandle || !graphHandle->private_data) {
        mvLog(MVLOG_ERROR, "The graph handle is NULL or have been destroyed");
        return NC_INVALID_HANDLE;
    }
    if (!dataLength || (*dataLength != 0 && !data)) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL");
        return NC_INVALID_PARAMETERS;
    }

    if (option < GRAPH_CLASS0_BASE ||
        option > (GRAPH_CLASS0_BASE + OPTION_CLASS_SIZE * NC_OPTION_CLASS3)) {
        mvLog(MVLOG_ERROR, "Option %d is invalid", option);
        return NC_INVALID_PARAMETERS;
    }

    struct _graphPrivate_t *g = graphHandle->private_data;

    pthread_mutex_lock(&globalMutex);
    if (!isGraphPreAllocateOption(option) && g->state == NC_GRAPH_CREATED) {
        mvLog(MVLOG_ERROR, "This graph hasn't been allocated");
        pthread_mutex_unlock(&globalMutex);
        return NC_NOT_ALLOCATED;
    }
    ncOptionClass_t class = getOptionClass(option, GRAPH_CLASS0_BASE);
    if (g->dev != NULL && class > g->dev->dev_attr.max_graph_opt_class) {
        mvLog(MVLOG_ERROR, "This device FW does not support NC_OPTION_CLASS%d",
              class);
        return NC_UNAUTHORIZED;
    }
    pthread_mutex_unlock(&globalMutex);
    ncStatus_t rc;
    switch (class) {
    case NC_OPTION_CLASS0:
        rc = getGraphOptionClass0(g, option, data, dataLength);
        break;
    case NC_OPTION_CLASS1:
        rc = getGraphOptionClass1(g, option, data, dataLength);
        break;
#ifndef EXCLUDE_HIGHCLASS
    case NC_OPTION_CLASS2:
        rc = getGraphOptionClass2(g, option, data, dataLength);
        break;
    case NC_OPTION_CLASS3:
        rc = getGraphOptionClass3(g, option, data, dataLength);
        break;
#endif
    default:
        mvLog(MVLOG_ERROR, "There is no such option class");
        rc = NC_INVALID_PARAMETERS;
        break;
    }
    return rc;
}

ncStatus_t ncGraphAllocateWithFifos(struct ncDeviceHandle_t * deviceHandle,
                                    struct ncGraphHandle_t * graphHandle,
                                    const void *graphBuffer,
                                    unsigned int graphBufferLength,
                                    struct ncFifoHandle_t ** inFifoHandle,
                                    struct ncFifoHandle_t ** outFifoHandle)
{
    return ncGraphAllocateWithFifosEx(deviceHandle,
                                      graphHandle, graphBuffer,
                                      graphBufferLength, inFifoHandle,
                                      NC_FIFO_HOST_WO, 2, NC_FIFO_FP32,
                                      outFifoHandle, NC_FIFO_HOST_RO, 2,
                                      NC_FIFO_FP32);
}

ncStatus_t ncGraphAllocateWithFifosEx(struct ncDeviceHandle_t * deviceHandle,
                                      struct ncGraphHandle_t * graphHandle,
                                      const void *graphBuffer,
                                      unsigned int graphBufferLength,
                                      struct ncFifoHandle_t ** inFifoHandle,
                                      ncFifoType_t inFifoType, int inNumElem,
                                      ncFifoDataType_t inDataType,
                                      struct ncFifoHandle_t ** outFifoHandle,
                                      ncFifoType_t outFifoType, int outNumElem,
                                      ncFifoDataType_t outDataType)
{
    if (!inFifoHandle || !inNumElem || !outFifoHandle || !outNumElem ||
        !graphHandle || !deviceHandle || !graphBuffer) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL or Zero!");
        return NC_INVALID_PARAMETERS;
    }
    ncStatus_t rc = ncGraphAllocate(deviceHandle, graphHandle, graphBuffer,
                                    graphBufferLength);
    if (rc != NC_OK)
        return rc;

    if (inFifoType == NC_FIFO_HOST_RO) {
        mvLog(MVLOG_ERROR, "input fifo cannot be read-only");
        return NC_INVALID_PARAMETERS;
    }
    if (outFifoType == NC_FIFO_HOST_WO) {
        mvLog(MVLOG_ERROR, "output fifo cannot be write-only");
        return NC_INVALID_PARAMETERS;
    }
    // Read tensor descriptors
    struct ncTensorDescriptor_t inputTensorDesc;
    struct ncTensorDescriptor_t outputTensorDesc;
    unsigned int length = sizeof(struct ncTensorDescriptor_t);
    rc = ncGraphGetOption(graphHandle,
                          NC_RO_GRAPH_INPUT_TENSOR_DESCRIPTORS,
                          &inputTensorDesc, &length);
    if (rc != NC_OK) {
        return rc;
    }
    rc = ncGraphGetOption(graphHandle,
                          NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS,
                          &outputTensorDesc, &length);
    if (rc != NC_OK) {
        return rc;
    }
    rc = ncFifoCreate("fifoIn0", inFifoType, inFifoHandle);
    if (rc != NC_OK) {
        return rc;
    }
    rc = ncFifoSetOption(*inFifoHandle, NC_RW_FIFO_DATA_TYPE, &inDataType,
                         sizeof(inDataType));
    if (rc != NC_OK) {
        return rc;
    }
    rc = ncFifoAllocate(*inFifoHandle, deviceHandle, &inputTensorDesc,
                        inNumElem);
    if (rc != NC_OK) {
        return rc;
    }
    rc = ncFifoCreate("fifoOut0", outFifoType, outFifoHandle);
    if (rc != NC_OK) {
        ncFifoDestroy(inFifoHandle);
        return rc;
    }
    rc = ncFifoSetOption(*outFifoHandle, NC_RW_FIFO_DATA_TYPE, &outDataType,
                         sizeof(outDataType));
    if (rc != NC_OK) {
        ncFifoDestroy(inFifoHandle);
        ncFifoDestroy(outFifoHandle);
        return rc;
    }
    rc = ncFifoAllocate(*outFifoHandle, deviceHandle, &outputTensorDesc,
                        outNumElem);
    if (rc != NC_OK) {
        ncFifoDestroy(inFifoHandle);
        ncFifoDestroy(outFifoHandle);
        return rc;
    }
    return rc;
}

ncStatus_t ncGlobalSetOption(int option, const void *data,
                             unsigned int dataLength)
{
    if (!data) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL");
        return NC_INVALID_PARAMETERS;
    }
    if (option == NC_RW_LOG_LEVEL && dataLength < sizeof(int)) {
        mvLog(MVLOG_ERROR, "The dataLength is smaller that required %d",
              sizeof(int));
        return NC_INVALID_PARAMETERS;
    }
    switch (option) {
    case NC_RW_LOG_LEVEL:
        {
            mvLog_t log_level = *(mvLog_t *) data;
            if (log_level >= MVLOG_LAST || log_level < 0) {
                mvLog(MVLOG_ERROR, "log_level value is invalid %d\n",
                      log_level);
                return NC_INVALID_PARAMETERS;
            }
            mvLogLevelSet(*(mvLog_t *) data);
            mvLogDefaultLevelSet(*(mvLog_t *) data);    //Allow turning off warnings and errors
            loglevel_initialized = 1;
        }
        break;
    case NC_RO_API_VERSION:
        return NC_UNAUTHORIZED;
        break;
    default:
        mvLog(MVLOG_ERROR, "No such option");
        return NC_INVALID_PARAMETERS;
    }

    return NC_OK;
}

ncStatus_t ncGlobalGetOption(int option, void *data, unsigned int *dataLength)
{
    if (!loglevel_initialized)
        initialize_loglevel();

    if (!dataLength || (*dataLength != 0 && !data)) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL");
        return NC_INVALID_PARAMETERS;
    }
    if (option == NC_RW_LOG_LEVEL && *dataLength < sizeof(int)) {
        mvLog(MVLOG_ERROR, "The dataLength is smaller that required %d",
              sizeof(int));
        *dataLength = sizeof(int);
        return NC_INVALID_DATA_LENGTH;
    }
    switch (option) {
    case NC_RW_LOG_LEVEL:
        *(int *) data = mvLogLevel_ncAPI;
        *dataLength = sizeof(mvLogLevel_ncAPI);
        break;
    case NC_RO_API_VERSION:{
            // We sanitize the situation by trying to reset the devices that have been left open
            if (api_version[0] == 0 && strlen(VERSION_NAME) != 0) {
                int res =
                    sscanf(VERSION_NAME, "%2d.%2d.%2d.%2d", &api_version[0],
                           &api_version[1], &api_version[2], &api_version[3]);
                if (res != 4)
                    mvLog(MVLOG_WARN, "Error parsing api version\n");
                else
                    mvLog(MVLOG_DEBUG, "api version %d.%d.%d.%d\n",
                          api_version[0], api_version[1], api_version[2],
                          api_version[3]);
            }
            unsigned int size = sizeof(api_version);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            memcpy((int *) data, api_version, size);
            *dataLength = size;
            break;
        }
    default:
        mvLog(MVLOG_ERROR, "No such option");
        return NC_INVALID_PARAMETERS;
    }
    return NC_OK;
}


static ncStatus_t getDeviceOptionClass0(struct _devicePrivate_t *d,
                                        ncDeviceOption_t option,
                                        void *data, unsigned int *dataLength)
{
    ncStatus_t rc = NC_OK;

    //*dataLength check
    switch (option) {
    case NC_RO_DEVICE_STATE:
    case NC_RO_DEVICE_ALLOCATED_GRAPH_NUM:
    case NC_RO_DEVICE_ALLOCATED_FIFO_NUM:
    case NC_RO_DEVICE_MEMORY_SIZE:
    case NC_RO_DEVICE_MAX_FIFO_NUM:
    case NC_RO_DEVICE_MAX_GRAPH_NUM:
    case NC_RO_DEVICE_OPTION_CLASS_LIMIT:
    case NC_RO_DEVICE_THERMAL_THROTTLING_LEVEL:
    case NC_RO_DEVICE_CURRENT_MEMORY_USED:
    case NC_RO_DEVICE_MAX_EXECUTORS_NUM:
        {
            unsigned int size = sizeof(int);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
        }
        break;
    default:
        break;
    }
    switch (option) {
    case NC_RO_DEVICE_THERMAL_STATS:
        if (*dataLength < NC_THERMAL_BUFFER_SIZE) {
            mvLog(MVLOG_ERROR,
                  "data length of output buffer (%d) is smaller that required (%d)!\n",
                  *dataLength, NC_THERMAL_BUFFER_SIZE);
            *dataLength = NC_THERMAL_BUFFER_SIZE;
            return NC_INVALID_DATA_LENGTH;
        }
        rc = getThermalStats(d);
        if (rc) {
            return rc;
        }
        memcpy((float *) data, &d->thermal_stats[1], NC_THERMAL_BUFFER_SIZE);
        *dataLength = NC_THERMAL_BUFFER_SIZE;
        break;
    case NC_RO_DEVICE_THERMAL_THROTTLING_LEVEL:
        rc = getThermalStats(d);
        if (rc) {
            return rc;
        }
        d->throttle_happened = d->thermal_stats[0];
        *(int *) data = d->throttle_happened;
        *dataLength = sizeof(int);
        break;
    case NC_RO_DEVICE_STATE:
        *(int *) data = d->state;
        *dataLength = sizeof(int);
        break;
    case NC_RO_DEVICE_ALLOCATED_GRAPH_NUM:
        *(int *) data = deviceGetNumberOfGraphs(d);
        *dataLength = sizeof(int);
        break;
    case NC_RO_DEVICE_ALLOCATED_FIFO_NUM:
        *(int *) data = deviceGetNumberOfFifos(d);
        *dataLength = sizeof(int);
        break;
    case NC_RO_DEVICE_MEMORY_SIZE:
        *(int *) data = d->dev_attr.max_memory;
        *dataLength = sizeof(int);
        break;
    case NC_RO_DEVICE_MAX_FIFO_NUM:
        *(int *) data = d->dev_attr.max_fifos;
        *dataLength = sizeof(int);
        break;
    case NC_RO_DEVICE_MAX_GRAPH_NUM:
        *(int *) data = d->dev_attr.max_graphs;
        *dataLength = sizeof(int);
        break;
    case NC_RO_DEVICE_OPTION_CLASS_LIMIT:
        *(int *) data = d->dev_attr.max_device_opt_class;
        *dataLength = sizeof(int);
        break;
    case NC_RO_DEVICE_NAME:
        if (*dataLength < strlen(d->dev_addr) + 1) {
            mvLog(MVLOG_ERROR,
                  "data length of output buffer (%d) is smaller that required (%d)!\n",
                  *dataLength, d->dev_addr + 1);
            *dataLength = strlen(d->dev_addr) + 1;
            return NC_INVALID_DATA_LENGTH;
        }
        *dataLength = strlen(d->dev_addr) + 1;
        strncpy((char *) data, d->dev_addr, *dataLength);
        break;
    case NC_RO_DEVICE_HW_VERSION:{
            unsigned int size = sizeof(ncDeviceHwVersion_t);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            char *p = strchr(d->dev_addr, '-');
            if (p == NULL) {
                mvLog(MVLOG_WARN, "Error device name is invalid %s\n",
                      d->dev_addr);
                rc = NC_INVALID_PARAMETERS;
                break;
            }
            p++;    //advance to point to the name

            if (strcmp("ma2450", p) == 0) {
                *(int *) data = NC_MA2450;
            } else if (strcmp("ma2480", p) == 0) {
                *(int *) data = NC_MA2480;
            } else {
                rc = NC_INVALID_PARAMETERS;
                mvLog(MVLOG_WARN, "Error device name is invalid %s , %s\n",
                      d->dev_addr);
                break;
            }
            *dataLength = size;
            break;
        }
    case NC_RO_DEVICE_FW_VERSION:{
            unsigned int size = sizeof(d->dev_attr.fw_version);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            memcpy((int *) data, d->dev_attr.fw_version, size);
            *dataLength = size;
            break;
        }
    case NC_RO_DEVICE_CURRENT_MEMORY_USED:{
            uint32_t mem;
            if (deviceGetDeviceMemory(d, &mem)) {
                rc = NC_ERROR;
                break;
            }
            *(int *) data = mem;
            *dataLength = sizeof(int);
            break;
        }
    case NC_RO_DEVICE_MAX_EXECUTORS_NUM:
        *(int *) data = d->dev_attr.max_executors;
        *dataLength = sizeof(int);
        break;
    case NC_RO_DEVICE_MVTENSOR_VERSION:{
            unsigned int size = sizeof(d->dev_attr.mv_tensor_version);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            memcpy((int *) data, d->dev_attr.mv_tensor_version, size);
            *dataLength = size;
            break;
        }
    case NC_RO_DEVICE_DEBUG_INFO:
        return NC_UNSUPPORTED_FEATURE;
    default:
        mvLog(MVLOG_ERROR, "No such option");
        return NC_INVALID_PARAMETERS;
    }
    return rc;
}

ncStatus_t ncDeviceSetOption(struct ncDeviceHandle_t * deviceHandle,
                             int option,
                             const void *data, unsigned int dataLength)
{
    ncStatus_t rc = NC_OK;

    if (!deviceHandle) {
        mvLog(MVLOG_ERROR, "handle is NULL");
        return NC_INVALID_HANDLE;
    }
    if (!data) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL");
        return NC_INVALID_PARAMETERS;
    }

    if (option < DEVICE_CLASS0_BASE ||
        option > (DEVICE_CLASS0_BASE + OPTION_CLASS_SIZE * NC_OPTION_CLASS3)) {
        mvLog(MVLOG_ERROR, "Option %d is invalid", option);
        return NC_INVALID_PARAMETERS;
    }

    ncOptionClass_t opClass = getOptionClass(option, DEVICE_CLASS0_BASE);
    if (opClass < NC_OPTION_CLASS1) {
        mvLog(MVLOG_ERROR, "Class 0 options are read-only");
        return NC_UNAUTHORIZED;
    }
    struct _devicePrivate_t *d = deviceHandle->private_data;
    pthread_mutex_lock(&globalMutex);
    if (findDevice(d)) {
        mvLog(MVLOG_ERROR,
              "This device handle is corrupt or has been destroyed");
        pthread_mutex_unlock(&globalMutex);

        return NC_INVALID_HANDLE;
    }
    pthread_mutex_unlock(&globalMutex);
    if (opClass > d->dev_attr.max_device_opt_class) {
        mvLog(MVLOG_ERROR, "This device FW does not support NC_OPTION_CLASS%d",
              opClass);
        return NC_UNAUTHORIZED;
    }
    //DataLen will be tested in the specific class, since it varies based on option
    //type

    switch (opClass) {
    case NC_OPTION_CLASS0:
        mvLog(MVLOG_ERROR, "Class 0 options are read-only");
        rc = NC_UNAUTHORIZED;   // option class 0 consists of read-only value
        break;
        //no class1 - no write options for device
#ifndef EXCLUDE_HIGHCLASS
    case NC_OPTION_CLASS2:
        rc = setDeviceOptionClass2(d, option, data, dataLength);
        break;
    case NC_OPTION_CLASS3:
        rc = setDeviceOptionClass3(d, option, data, dataLength);
        break;
#endif
    default:
        rc = NC_INVALID_PARAMETERS;
        break;
    }
    return rc;
}

//static options can be read before device is open
static int isDeviceStaticOption(int option)
{
    switch (option) {
    case NC_RO_DEVICE_NAME:
    case NC_RO_DEVICE_STATE:
    case NC_RO_DEVICE_HW_VERSION:
        return 1;
    default:
        return 0;
    }
}

ncStatus_t ncDeviceGetOption(struct ncDeviceHandle_t * deviceHandle,
                             int option, void *data, unsigned int *dataLength)
{
    ncStatus_t rc;

    if (!deviceHandle) {
        mvLog(MVLOG_ERROR, "Device handle is NULL");
        return NC_INVALID_HANDLE;
    }
    if (!deviceHandle || !dataLength || (*dataLength != 0 && !data)) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL");
        return NC_INVALID_PARAMETERS;
    }


    if (option < DEVICE_CLASS0_BASE ||
        option > (DEVICE_CLASS0_BASE + OPTION_CLASS_SIZE * NC_OPTION_CLASS3)) {
        mvLog(MVLOG_ERROR, "Option %d is invalid", option);
        return NC_INVALID_PARAMETERS;
    }

    struct _devicePrivate_t *d = deviceHandle->private_data;

    pthread_mutex_lock(&globalMutex);
    if (!isDeviceStaticOption(option) && d->state != NC_DEVICE_OPENED) {
        mvLog(MVLOG_ERROR, "This device hasn't been openned");
        pthread_mutex_unlock(&globalMutex);
        return NC_UNAUTHORIZED;
    }

    ncOptionClass_t opClass = getOptionClass(option, DEVICE_CLASS0_BASE);
    if (!isDeviceStaticOption(option)) {
        if (findDevice(d)) {
            mvLog(MVLOG_ERROR,
                  "This device handle is corrupt or has been destroyed");
            pthread_mutex_unlock(&globalMutex);
            return NC_INVALID_HANDLE;
        }

        if (d->dev_attr.max_device_opt_class < opClass) {
            mvLog(MVLOG_ERROR,
                  "This device FW does not support NC_OPTION_CLASS%d", opClass);
            pthread_mutex_unlock(&globalMutex);
            return NC_UNAUTHORIZED;
        }
    }
    pthread_mutex_unlock(&globalMutex);

    switch (opClass) {
    case NC_OPTION_CLASS0:
        rc = getDeviceOptionClass0(d, option, data, dataLength);
        break;
        //no class1 - no write options for device
#ifndef EXCLUDE_HIGHCLASS
    case NC_OPTION_CLASS2:
        rc = getDeviceOptionClass2(d, option, data, dataLength);
        break;
    case NC_OPTION_CLASS3:
        rc = getDeviceOptionClass3(d, option, data, dataLength);
        break;
#endif
    default:
        rc = NC_INVALID_PARAMETERS;
        break;
    }
    return rc;
}

static int fifoWriteAccess(struct _fifoPrivate_t *fifoHandle)
{
    if (fifoHandle->type == NC_FIFO_HOST_WO) {
        return 1;
    }
    return 0;
}

static int fifoReadAccess(struct _fifoPrivate_t *fifoHandle)
{
    if (fifoHandle->type == NC_FIFO_HOST_RO) {
        return 1;
    }
    return 0;
}

ncStatus_t ncFifoCreate(const char *name, ncFifoType_t type,
                        struct ncFifoHandle_t ** fifoHandle)
{
    mvLog(MVLOG_INFO, "Init fifo");
    if (!loglevel_initialized)
        initialize_loglevel();

    if (!fifoHandle) {
        mvLog(MVLOG_ERROR, "Fifo handle is NULL");
        return NC_INVALID_HANDLE;
    }

    if (!name) {
        mvLog(MVLOG_ERROR, "name is NULL");
        return NC_INVALID_PARAMETERS;
    }

    if (type != NC_FIFO_HOST_RO && type != NC_FIFO_HOST_WO) {
        mvLog(MVLOG_ERROR, "Fifo typo not supported!");
        return NC_UNSUPPORTED_FEATURE;
    }
    static int fifoIdCounter = 0;
    *fifoHandle = (struct ncFifoHandle_t *) malloc(sizeof(struct ncFifoHandle_t));
    if (!(*fifoHandle)) {
        mvLog(MVLOG_ERROR, "Memory allocation failed");
        return NC_OUT_OF_MEMORY;
    }

    struct _fifoPrivate_t *handle = (struct _fifoPrivate_t *) malloc(sizeof(struct _fifoPrivate_t));
    (*fifoHandle)->private_data = handle;
    if (!handle) {
        mvLog(MVLOG_ERROR, "Memory allocation failed");
        return NC_OUT_OF_MEMORY;
    }

    handle->type = type;
    handle->consumer_cnt = 1;   //default consumers

    handle->state = NC_FIFO_CREATED;
    pthread_mutex_init(&handle->fifo_mutex, NULL);
    handle->consumed_by_graph = 0;
    handle->write_count = 0;
    handle->user_param_in = NULL;
    handle->user_param_out = NULL;
    handle->api_read_element = 0;
    handle->id = fifoIdCounter++;
    handle->num_elements = 0;
    handle->host_tensor_desc_set = 0;
    memset(&handle->host_tensor_desc, 0, sizeof(struct ncTensorDescriptor_t));
    handle->host_tensor_desc.dataType = NC_FIFO_FP32; //default app data type is FP32
    strncpy(handle->name, name, NC_MAX_NAME_SIZE);

    return NC_OK;
}

int pushUserParam(struct _fifoPrivate_t *fH, void *user_param, int isIn)
{
    struct _userParamPrivate_t *new_user_param =
        calloc(1, sizeof(struct _userParamPrivate_t));
    new_user_param->next = NULL;
    if (!new_user_param) {
        mvLog(MVLOG_ERROR, "calloc failed!");
        return NC_OUT_OF_MEMORY;
    }
    new_user_param->data = user_param;
    if (isIn) {
        new_user_param->next = fH->user_param_in;
        fH->user_param_in = new_user_param;
    } else {
        new_user_param->next = fH->user_param_out;
        fH->user_param_out = new_user_param;
    }
    return NC_OK;
}

int popUserParam(struct _fifoPrivate_t *fH, void **user_param, int isIn)
{
    struct _userParamPrivate_t *prev = NULL;
    struct _userParamPrivate_t *curr = NULL;
    if (isIn)
        curr = fH->user_param_in;
    else
        curr = fH->user_param_out;

    if (curr == NULL) {
        if (user_param != NULL)
            *user_param = NULL;
        else
            mvLog(MVLOG_DEBUG, "user_param is null - ignoring \n");
        mvLog(MVLOG_ERROR, "Trying to read user param from an empty queue!");
        return NC_ERROR;
    }

    while (curr->next != NULL) {
        prev = curr;
        curr = curr->next;
    }
    if (user_param != NULL)
        *user_param = curr->data;
    else
        mvLog(MVLOG_DEBUG, "user_param is null - ignoring \n");

    if (prev)
        prev->next = NULL;
    else {
        if (isIn)
            fH->user_param_in = NULL;
        else
            fH->user_param_out = NULL;
    }
    free(curr);
    curr = NULL;
    return NC_OK;
}

void getStrides(ncFifoLayout_t layout, struct ncTensorDescriptor_t* desc,
    ncFifoDataType_t dataType) {
    int baseStride = dataType == NC_FIFO_FP16 ? FP16_DATA_SIZE : sizeof(float);
    switch (layout) {
        case NC_FIFO_HWC:
            desc->cStride = baseStride;
            desc->wStride = desc->cStride * desc->c;
            desc->hStride = desc->wStride * desc->w;
            break;
        case NC_FIFO_CHW:
            desc->wStride = baseStride;
            desc->hStride = desc->wStride * desc->w;
            desc->cStride = desc->hStride * desc->h;
            break;
        case NC_FIFO_HCW:
            desc->wStride = baseStride;
            desc->cStride = desc->wStride * desc->w;
            desc->hStride = desc->cStride * desc->c;
            break;
        case NC_FIFO_CWH:
            desc->hStride = baseStride;
            desc->wStride = desc->hStride * desc->h;
            desc->cStride = desc->wStride * desc->w;
            break;
        case NC_FIFO_WCH:
            desc->hStride = baseStride;
            desc->cStride = desc->hStride * desc->h;
            desc->wStride = desc->cStride * desc->c;
            break;
        case NC_FIFO_WHC:
            desc->cStride = baseStride;
            desc->hStride = desc->cStride * desc->c;
            desc->wStride = desc->hStride * desc->h;
            break;
        default:
            break;
    }
}

static unsigned int getTotalSize(struct ncTensorDescriptor_t* desc) {
    unsigned int maxStride;
    unsigned int maxDim;

    if (desc->wStride == desc->hStride &&
        desc->wStride == desc->cStride) {
        maxDim = MAX(desc->w, desc->h);
        maxDim = MAX(maxDim, desc->c);
        maxStride = desc->wStride;
    } else if (desc->wStride >= desc->hStride &&
               desc->wStride >= desc->cStride) {
        maxStride = desc->wStride;
        maxDim = desc->w;
        if (desc->wStride == desc->hStride)
            maxDim = MAX(desc->w, desc->h);
        else if (desc->wStride == desc->cStride)
            maxDim = MAX(desc->w, desc->c);
    } else if (desc->hStride >= desc->wStride &&
               desc->hStride >= desc->cStride) {
        maxStride = desc->hStride;
        maxDim = desc->h;
        if (desc->hStride == desc->wStride)
            maxDim = MAX(desc->h, desc->w);
        else if (desc->hStride == desc->cStride)
            maxDim = MAX(desc->h, desc->c);
    } else {
        maxStride = desc->cStride;
        maxDim = desc->c;
        if (desc->cStride == desc->wStride)
            maxDim = MAX(desc->c, desc->w);
        else if (desc->cStride == desc->hStride)
            maxDim = MAX(desc->c, desc->h);
    }
    return desc->n * maxStride * maxDim;
}
static unsigned int getElementSize(struct _fifoPrivate_t * handle) {
    return handle->host_tensor_desc.totalSize;
}

ncStatus_t ncFifoAllocate(struct ncFifoHandle_t * fifoHandle,
                          struct ncDeviceHandle_t * device,
                          struct ncTensorDescriptor_t * tensor_desc,
                          unsigned int numElem)
{
    mvLog(MVLOG_INFO, "Creating fifo");
    if (!fifoHandle || !device) {
        mvLog(MVLOG_ERROR, "Fifo or device handle is NULL");
        return NC_INVALID_HANDLE;
    }
    if (!tensor_desc || !numElem) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL");
        return NC_INVALID_PARAMETERS;
    }
    if (tensor_desc->n * tensor_desc->c * tensor_desc->w * tensor_desc->h == 0
        || !tensor_desc->totalSize) {
        mvLog(MVLOG_ERROR,
              "Tensor descriptor is invalid. Total size 0 or other element is zero");
        return NC_INVALID_PARAMETERS;
    }
    struct _fifoPrivate_t *handle = fifoHandle->private_data;
    if (handle->state == NC_FIFO_ALLOCATED) {
        mvLog(MVLOG_ERROR, "Fifo has already been allocated");
        return NC_UNAUTHORIZED;
    }
    if (handle->state != NC_FIFO_CREATED) {
        mvLog(MVLOG_ERROR, "Fifo handle is corrupt or has been destroyed");
        return NC_INVALID_HANDLE;
    }
    struct _devicePrivate_t *d = devices;
    pthread_mutex_lock(&globalMutex);
    while (d) {
        if (d == device->private_data)
            break;
        d = d->next;
    }
    if (!d) {
        pthread_mutex_unlock(&globalMutex);
        mvLog(MVLOG_ERROR, "Device not found!\n");
        return NC_INVALID_PARAMETERS;
    }
    pthread_mutex_unlock(&globalMutex);

    handle->graph_tensor_desc = *tensor_desc;
    if (!handle->host_tensor_desc_set) {
        ncFifoDataType_t saved_data_type = handle->host_tensor_desc.dataType;
        handle->host_tensor_desc = handle->graph_tensor_desc;
        handle->host_tensor_desc.dataType = saved_data_type;
        getStrides(NC_FIFO_HWC, &handle->host_tensor_desc, handle->host_tensor_desc.dataType);
        handle->host_tensor_desc.totalSize = getTotalSize(&handle->host_tensor_desc);
        handle->host_tensor_desc_set = 1;
    } else {
        // it has been set, let's check that shape is good
        // when we add scaling of input, this constraint can be removed
        if (handle->host_tensor_desc.w != handle->graph_tensor_desc.w ||
            handle->host_tensor_desc.h != handle->graph_tensor_desc.h ||
            handle->host_tensor_desc.c != handle->graph_tensor_desc.c ||
            handle->host_tensor_desc.n != handle->graph_tensor_desc.n)
        {
            mvLog(MVLOG_ERROR, "Host tensor descriptor shape doesn't match graph tensor descriptor shape!\n");
            return NC_INVALID_PARAMETERS;
        }
    }
    handle->graphLayout = getLayout(tensor_desc);
    handle->user_param_in = NULL;
    handle->user_param_out = NULL;
    handle->num_elements = numElem;
    handle->consumers_remaining = handle->consumer_cnt; //default consumers
    handle->dev = d;
    handle->next = NULL;

    handle->datasize = getElementSize(handle);

    if (d->fifos)
        handle->next = d->fifos;
    d->fifos = handle;

    graphMonCommand_t cmd;
    cmd.cmdClass = GRAPH_MON_CLASS_BUFFER_CMD;
    cmd.cmd.buffCmd.type = BUFFER_ALLOCATE_CMD;
    struct tensorDescriptor_t privateDesc;
    privateDesc.c = tensor_desc->c;
    privateDesc.n = tensor_desc->n;
    privateDesc.h = tensor_desc->h;
    privateDesc.w = tensor_desc->w;
    privateDesc.totalSize = tensor_desc->totalSize;
    cmd.cmd.buffCmd.desc = privateDesc;
    cmd.cmd.buffCmd.elemCnt = numElem;
    snprintf(cmd.cmd.buffCmd.name, 16, "FIFO%d", handle->id);
    cmd.cmd.buffCmd.id = handle->id;

    uint32_t writeSize;
    if (fifoWriteAccess(handle)) {
        writeSize = tensor_desc->totalSize * numElem;
        cmd.cmd.buffCmd.writeChannel = 1;
    } else {
        cmd.cmd.buffCmd.writeChannel = 0;
        writeSize = 8;  // no write permission on this buffer, so we shouldn't bother allocating buffer on the device
    }
    if (fifoReadAccess(handle)) {
        cmd.cmd.buffCmd.readChannel = 1;
    } else {
        cmd.cmd.buffCmd.readChannel = 0;
    }
    streamId_t streamId = XLinkOpenStream(d->usb_link->linkId,
                                            cmd.cmd.buffCmd.name, writeSize);
    if (streamId == INVALID_STREAM_ID) {
        mvLog(MVLOG_WARN, "can't open stream\n");
        return NC_ERROR;
    }
    handle->streamId = streamId;
    pthread_mutex_lock(&d->graph_streamm);

    if (sendGraphMonitorRequest(d->graph_monitor_stream_id, &cmd)) {
        pthread_mutex_unlock(&d->graph_streamm);
        mvLog(MVLOG_WARN, "can't send command\n");
        return NC_ERROR;
    }
    if (checkGraphMonitorResponse(d->graph_monitor_stream_id)) {
        pthread_mutex_unlock(&d->graph_streamm);
        mvLog(MVLOG_WARN, "myriad NACK\n");
        return NC_ERROR;
    }
    pthread_mutex_unlock(&d->graph_streamm);

    handle->state = NC_FIFO_ALLOCATED;
    return NC_OK;

}

ncStatus_t ncFifoDestroy(struct ncFifoHandle_t ** fifoHandle)
{
    if (!fifoHandle) {
        mvLog(MVLOG_ERROR, "Fifo handle is NULL");
        return NC_INVALID_HANDLE;
    }
    struct ncFifoHandle_t *fh = *fifoHandle;
    if (!fh) {
        mvLog(MVLOG_INFO, "handle is already destroyed");
        return NC_OK;
    }

    struct _fifoPrivate_t *handle = fh->private_data;

    if (handle->state == NC_FIFO_CREATED) {
        free(fh->private_data);
        fh->private_data = NULL;
        free(fh);
        *fifoHandle = NULL;
        return NC_OK;
    }
    if (!findFifo(handle)) {
        mvLog(MVLOG_ERROR,
              "fifo handle seems to be corrupt or has been destroyed");
        return NC_INVALID_HANDLE;
    }
    //clean up fifo
    /*if (fifoReadAccess(handle)) {
        int fillLevel;
        int rc = XLinkGetFillLevel(handle->streamId, 0, &fillLevel);
        if (rc == X_LINK_SUCCESS) {
            while (fillLevel && rc == X_LINK_SUCCESS) {
                rc = XLinkReleaseData(handle->streamId);
                fillLevel--;
            }
        }
    }*/
    //First write to the fifo to stop it's thread
    if (fifoWriteAccess(handle)) {
        int msg = 0xdead;
        if (XLinkWriteData(handle->streamId, (uint8_t *) & msg, sizeof(&msg)) !=
            0) {
            mvLog(MVLOG_ERROR, "Failed to write to fifo before deleting it!");
            return NC_ERROR;
        }
    }

    graphMonCommand_t cmd;
    cmd.cmdClass = GRAPH_MON_CLASS_BUFFER_CMD;
    cmd.cmd.buffCmd.type = BUFFER_DEALLOCATE_CMD;
    cmd.cmd.buffCmd.id = handle->id;

    struct _devicePrivate_t *d = handle->dev;
    pthread_mutex_lock(&d->graph_streamm);
    if (sendGraphMonitorRequest(d->graph_monitor_stream_id, &cmd)) {
        pthread_mutex_unlock(&d->graph_streamm);
        mvLog(MVLOG_WARN, "can't send command\n");
        return NC_ERROR;
    }
    if (checkGraphMonitorResponse(d->graph_monitor_stream_id)) {
        pthread_mutex_unlock(&d->graph_streamm);
        mvLog(MVLOG_WARN, "myriad NACK\n");
        return NC_ERROR;
    }
    pthread_mutex_unlock(&d->graph_streamm);

    pthread_mutex_lock(&d->dev_data_m);
    if (deallocateFifo(handle)) {
        pthread_mutex_unlock(&d->dev_data_m);
        return NC_INVALID_PARAMETERS;
    }
    pthread_mutex_unlock(&d->dev_data_m);

    free(fh->private_data);
    fh->private_data = NULL;
    free(fh);
    *fifoHandle = NULL;
    return NC_OK;

}


ncStatus_t ncFifoWriteElem(struct ncFifoHandle_t * fifoHandle,
                           const void *inputTensor,
                           unsigned int * inputTensorLength,
                           void *userParam)
{
    if (!fifoHandle) {
        mvLog(MVLOG_ERROR, "fifo handle is NULL");
        return NC_INVALID_HANDLE;
    }
    if (inputTensorLength == NULL || *inputTensorLength <= 0) {
        mvLog(MVLOG_ERROR, "inputTensorSize is null or invalid value");
        return NC_INVALID_PARAMETERS;
    }
    struct _fifoPrivate_t *handle = fifoHandle->private_data;
    if (!findFifo(handle)) {
        if (!handle) {
            mvLog(MVLOG_ERROR,
                  "fifo handle seems to be corrupt or has been destroyed");
            return NC_INVALID_HANDLE;
        }
        if (handle->state == NC_FIFO_CREATED) {
            mvLog(MVLOG_ERROR, "FIFO is not yet allocated");
            return NC_NOT_ALLOCATED;
        }
        if (handle->state != NC_FIFO_ALLOCATED) {
            mvLog(MVLOG_ERROR,
                  "FIFO is not yet allocated or have been destroyed.");
            return NC_UNAUTHORIZED;
        }
    }

    if (!inputTensor) {
        mvLog(MVLOG_ERROR, "input tensor is NULL");
        return NC_INVALID_PARAMETERS;
    }
    if (!fifoWriteAccess(handle)) {
        mvLog(MVLOG_ERROR, "No write access to fifo");
        return NC_UNAUTHORIZED;
    }
    if (*inputTensorLength != handle->datasize) {
            mvLog(MVLOG_ERROR,
                  "input tensor length (%d) doesnt match expected value (%d)",
                  *inputTensorLength, handle->datasize);
            *inputTensorLength = handle->datasize;
            return NC_INVALID_DATA_LENGTH;
    }
    struct ncTensorDescriptor_t * inputDesc = &handle->graph_tensor_desc;

    int rc;
    // Convert fp32 to fp16 and/or input layout
    ncFifoLayout_t layout = getLayout(inputDesc);
    ncFifoLayout_t host_layout = getLayout(&handle->host_tensor_desc);
    if (handle->host_tensor_desc.dataType == NC_FIFO_FP32 || layout != host_layout) {
        unsigned char *inputTensorConverted = malloc(inputDesc->totalSize);
        struct ncTensorDescriptor_t hostDesc = handle->host_tensor_desc;
        //getStrides(handle->hostLayout, &srcDesc, handle->datatype);

        if (layout != host_layout) {
            mvLog(MVLOG_DEBUG,
                  "Conversion from host layout %d to graph layout %d, is needed\n",
                  host_layout, layout);
        } else {
            mvLog(MVLOG_DEBUG,
                  "No layout conversion  is needed %d\n",
                  layout);
        }

        convertDataTypeAndLayout((unsigned char*) inputTensor, inputTensorConverted,
                          &hostDesc,
                          inputDesc, handle->host_tensor_desc.dataType, NC_FIFO_FP16);
        rc = XLinkWriteData(handle->streamId, inputTensorConverted,
                            inputDesc->totalSize);
        free(inputTensorConverted);
    } else {
        rc = XLinkWriteData(handle->streamId, inputTensor, *inputTensorLength);
    }
    if (rc != 0)
        return NC_ERROR;

    pthread_mutex_lock(&handle->fifo_mutex);
    rc = pushUserParam(handle, userParam, 1);
    if (rc != NC_OK) {
        pthread_mutex_unlock(&handle->fifo_mutex);
        return rc;
    }
    handle->write_count++;
    pthread_mutex_unlock(&handle->fifo_mutex);

    mvLog(MVLOG_DEBUG, "write count %d num_elements %d userparam %p\n",
          handle->write_count - 1, handle->num_elements, userParam);
    return NC_OK;

}

ncStatus_t ncFifoReadElem(struct ncFifoHandle_t * fifoHandle, void *outputData,
                          unsigned int *outputDataLen, void **userParam)
{
    if (!fifoHandle) {
        mvLog(MVLOG_ERROR, "fifo handle is NULL");
        return NC_INVALID_HANDLE;
    }
    if (!outputDataLen || (*outputDataLen != 0 && !outputData)) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL");
        return NC_INVALID_PARAMETERS;
    }

    struct _fifoPrivate_t *handle = fifoHandle->private_data;
    if (!findFifo(handle)) {
        if (!handle) {
            mvLog(MVLOG_ERROR,
                  "fifo handle seems to be corrupt or has been destroyed");
            return NC_INVALID_HANDLE;
        }
        if (handle->state == NC_FIFO_CREATED) {
            mvLog(MVLOG_ERROR, "FIFO is not yet allocated");
            return NC_NOT_ALLOCATED;
        }
    }

    if (handle->state != NC_FIFO_ALLOCATED) {
        mvLog(MVLOG_ERROR, "FIFO is not yet allocated or have been destroyed.");
        return NC_UNAUTHORIZED;
    }

    if (*outputDataLen < handle->datasize) {
        mvLog(MVLOG_ERROR,
              "This datasize in tensorDesc (%d) is smaller than required (%d)!",
              *outputDataLen, handle->datasize);
        *outputDataLen = handle->datasize;
        return NC_INVALID_DATA_LENGTH;
    }

    if (!fifoReadAccess(handle)) {
        mvLog(MVLOG_ERROR, "FIFO has no read access");
        return NC_UNAUTHORIZED;
    }
    if (handle->api_read_element != 0) {
        mvLog(MVLOG_ERROR, "API already read this element");
        return NC_UNAUTHORIZED;
    }
    streamPacketDesc_t *packet;
    if (!XLinkReadData(handle->streamId, &packet)) {
        // Convert fp16 to fp32 and/or layout
        struct ncTensorDescriptor_t * fifoDesc = &handle->graph_tensor_desc;
        ncFifoLayout_t layout = getLayout(fifoDesc);
        ncFifoLayout_t host_layout = getLayout(&handle->host_tensor_desc);
        //printf("packet->length %d handle->datasize %d \n" , packet->length, handle->datasize);

        if (handle->host_tensor_desc.dataType == NC_FIFO_FP32 ||
            layout != host_layout) {
            struct ncTensorDescriptor_t hostDesc = handle->host_tensor_desc;
            //getStrides(handle->hostLayout, &dstDesc, handle->host_tensor_desc.dataType);
            if (layout != host_layout) {
                mvLog(MVLOG_DEBUG,
                      "Conversion from graph layout %d to host layout %d, is needed\n",
                       layout, host_layout);
            } else {
                mvLog(MVLOG_DEBUG,
                      "No layout conversion  is needed %d\n", layout);
            }

            convertDataTypeAndLayout((unsigned char*) packet->data, outputData,
                           fifoDesc, &hostDesc, NC_FIFO_FP16,
                           handle->host_tensor_desc.dataType );
        } else {
            memcpy(outputData, packet->data, packet->length);
        }
        XLinkReleaseData(handle->streamId);
    }

    //As user should see an API read to be the same as Graph read, we need to wirte the element in 2 queues.
    //if we read it here, we will need to remove the element on the device side
    //to avoid sending a message just for this purpose, we can send it at the next trigger which touches this FIFO.
    pthread_mutex_lock(&handle->fifo_mutex);
    handle->api_read_element = 1;

    handle->consumers_remaining--;
    if (handle->consumers_remaining == 0) {
        handle->api_read_element = 0;
        handle->consumers_remaining = handle->consumer_cnt;
        //no other action required when the element is consumed
    }
    popUserParam(handle, userParam, 0);
    pthread_mutex_unlock(&handle->fifo_mutex);
    *outputDataLen = handle->datasize;
    mvLog(MVLOG_DEBUG, "num_elements %d userparam %p output length %d\n",
          handle->num_elements, userParam, handle->datasize);
    return NC_OK;
}

ncStatus_t ncFifoRemoveElem(struct ncFifoHandle_t * fifoHandle)
{
    return NC_UNSUPPORTED_FEATURE;
}

ncStatus_t ncFifoSetOption(struct ncFifoHandle_t * fifoHandle, int option,
                           const void *data, unsigned int dataLength)
{
    if (!fifoHandle) {
        mvLog(MVLOG_ERROR, "Fifo handle is NULL");
        return NC_INVALID_HANDLE;
    }
    if (!data) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL");
        return NC_INVALID_PARAMETERS;
    }
    if (!fifoHandle->private_data) {
        mvLog(MVLOG_ERROR, "fifo handle is corrupt or has been destroyed");
        return NC_INVALID_HANDLE;
    }
    struct _fifoPrivate_t *f = (struct _fifoPrivate_t *) fifoHandle->private_data;
    if (f->state != NC_FIFO_CREATED && option != NC_RW_FIFO_HOST_TENSOR_DESCRIPTOR) {
        mvLog(MVLOG_ERROR, "cannot set Fifo options after allocation");
        return NC_UNAUTHORIZED;
    }

    switch (option) {
    case NC_RW_FIFO_TYPE:{
            unsigned int size = sizeof(ncFifoType_t);
            if (dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      dataLength, size);
                return NC_INVALID_DATA_LENGTH;
            }
            int tempType = *(ncFifoType_t *) data;
            if (tempType != NC_FIFO_HOST_WO && tempType != NC_FIFO_HOST_RO) {
                 mvLog(MVLOG_ERROR,
                      "Type value set (%d) is invalid!\n",
                      tempType);
                return NC_INVALID_PARAMETERS;
            }
            f->type = tempType;
            break;
        }
    case NC_RW_FIFO_CONSUMER_COUNT:{
            unsigned int size = sizeof(int);
            if (dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      dataLength, size);
                return NC_INVALID_DATA_LENGTH;
            }
            f->consumer_cnt = *(int *) data;
            break;
        }
    case NC_RW_FIFO_DATA_TYPE:{
            unsigned int size = sizeof(ncFifoDataType_t);
            if (dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      dataLength, size);
                return NC_INVALID_DATA_LENGTH;
            }
            int tempDType = *(int *) data;
            if (tempDType != NC_FIFO_FP16 && tempDType != NC_FIFO_FP32) {
                mvLog(MVLOG_ERROR,
                      "dataType value set (%d) is invalid!\n",
                      tempDType);
                return NC_INVALID_PARAMETERS;
            }
            f->host_tensor_desc.dataType = tempDType;
            break;
        }
    case NC_RW_FIFO_HOST_TENSOR_DESCRIPTOR:{
            unsigned int size = sizeof(struct ncTensorDescriptor_t);
            if (dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      dataLength, size);
                return NC_INVALID_DATA_LENGTH;
            }

            int expected_total_size = getTotalSize((struct ncTensorDescriptor_t *) data);
            if (expected_total_size != ((struct ncTensorDescriptor_t *) data)->totalSize) {
                mvLog(MVLOG_ERROR,
                      "totalSize in host tensor descriptor (%d) doesn't match expeected totalSize (%d)!\n",
                      ((struct ncTensorDescriptor_t *) data)->totalSize, expected_total_size);
                return NC_INVALID_PARAMETERS;
            }
            if (f->state == NC_FIFO_ALLOCATED) {
                struct ncTensorDescriptor_t* temp = (struct ncTensorDescriptor_t*) data;
                if (temp->w != f->graph_tensor_desc.w ||
                    temp->h != f->graph_tensor_desc.h ||
                    temp->c != f->graph_tensor_desc.c ||
                    temp->n != f->graph_tensor_desc.n)
                {
                    mvLog(MVLOG_ERROR, "trying to set host tensor decriptor to a shape that doesn't match graph tensor descriptor shape!\n");
                    return NC_INVALID_PARAMETERS;
                }
            }

            f->host_tensor_desc = *(struct ncTensorDescriptor_t *) data;
            f->host_tensor_desc_set = 1;
            f->datasize = getElementSize(f);

            break;
        }
    case NC_RW_FIFO_DONT_BLOCK:
        return NC_UNSUPPORTED_FEATURE;
        break;
    case NC_RO_FIFO_CAPACITY:
    case NC_RO_FIFO_READ_FILL_LEVEL:
    case NC_RO_FIFO_WRITE_FILL_LEVEL:
    case NC_RO_FIFO_GRAPH_TENSOR_DESCRIPTOR:
    case NC_RO_FIFO_STATE:
    case NC_RO_FIFO_ELEMENT_DATA_SIZE:
        return NC_UNAUTHORIZED;
        break;
    default:
        return NC_INVALID_PARAMETERS;
        break;
    }
    return NC_OK;
}

ncStatus_t ncFifoGetOption(struct ncFifoHandle_t * fifoHandle, int option,
                           void *data, unsigned int *dataLength)
{
    if (!dataLength || (*dataLength != 0 && !data)) {
        mvLog(MVLOG_ERROR, "Some of the parameters are NULL");
        return NC_INVALID_PARAMETERS;
    }
    if (!fifoHandle) {
        mvLog(MVLOG_ERROR, "fifo handle is NULL");
        return NC_INVALID_HANDLE;
    }
    if (!fifoHandle->private_data) {
        mvLog(MVLOG_ERROR, "Fifo is corrupt or has been destroyed");
        return NC_INVALID_HANDLE;
    }
    if (fifoHandle->private_data->state == NC_FIFO_CREATED &&
        option != NC_RO_FIFO_STATE && option != NC_RW_FIFO_DATA_TYPE &&
        option != NC_RW_FIFO_DONT_BLOCK && option != NC_RW_FIFO_CONSUMER_COUNT
        && option != NC_RO_FIFO_NAME && option != NC_RW_FIFO_HOST_TENSOR_DESCRIPTOR) {
        mvLog(MVLOG_ERROR,
              "Fifo hasn't been allocated, cannot read those options");
        return NC_NOT_ALLOCATED;
    }
    switch (option) {
    case NC_RW_FIFO_CONSUMER_COUNT:
    case NC_RO_FIFO_CAPACITY:
    case NC_RO_FIFO_READ_FILL_LEVEL:
    case NC_RO_FIFO_WRITE_FILL_LEVEL:
    case NC_RO_FIFO_STATE:
    case NC_RO_FIFO_ELEMENT_DATA_SIZE:
        {
            unsigned int size = sizeof(int);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            break;
        }
    default:
        break;
    }

    switch (option) {
    case NC_RW_FIFO_TYPE:{
            unsigned int size = sizeof(ncFifoType_t);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            *(ncFifoType_t *) data = fifoHandle->private_data->type;
            *dataLength = sizeof(fifoHandle->private_data->type);
            break;
        }
    case NC_RW_FIFO_CONSUMER_COUNT:
        *(int *) data = fifoHandle->private_data->consumer_cnt;
        *dataLength = sizeof(fifoHandle->private_data->consumer_cnt);
        break;
    case NC_RO_FIFO_ELEMENT_DATA_SIZE:
        *(int *) data = getElementSize(fifoHandle->private_data);
        *dataLength = sizeof(fifoHandle->private_data->datasize);
        break;
    case NC_RW_FIFO_DATA_TYPE:
        {
            unsigned int size = sizeof(ncFifoDataType_t);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            *(int *) data = fifoHandle->private_data->host_tensor_desc.dataType;
            *dataLength = sizeof(fifoHandle->private_data->host_tensor_desc.dataType);
            break;
        }
    case NC_RO_FIFO_CAPACITY:
        *(int *) data = fifoHandle->private_data->num_elements;
        *dataLength = sizeof(fifoHandle->private_data->num_elements);
        break;
    case NC_RO_FIFO_GRAPH_TENSOR_DESCRIPTOR:
        {
            unsigned int size = sizeof(struct ncTensorDescriptor_t);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            if (fifoHandle->private_data->state != NC_FIFO_ALLOCATED)
                return NC_UNAUTHORIZED; // before allocation, tensor_desc is NULL
            *(struct ncTensorDescriptor_t *) data =
                fifoHandle->private_data->graph_tensor_desc;
            *dataLength = sizeof(fifoHandle->private_data->graph_tensor_desc);
            break;
        }
    case NC_RW_FIFO_HOST_TENSOR_DESCRIPTOR:
        {
            unsigned int size = sizeof(struct ncTensorDescriptor_t);
            if (*dataLength < size) {
                mvLog(MVLOG_ERROR,
                      "data length of output buffer (%d) is smaller that required (%d)!\n",
                      *dataLength, size);
                *dataLength = size;
                return NC_INVALID_DATA_LENGTH;
            }
            if (fifoHandle->private_data->state != NC_FIFO_ALLOCATED &&
                fifoHandle->private_data->host_tensor_desc_set == 0) {
                mvLog(MVLOG_ERROR,
                      "option NC_RW_FIFO_HOST_TENSOR_DESCRIPTOR cannot be read before it has been set or before Fifo has been allocated");
                return NC_UNAUTHORIZED;
            }
            *(struct ncTensorDescriptor_t *) data =
                fifoHandle->private_data->host_tensor_desc;
            *dataLength = sizeof(fifoHandle->private_data->host_tensor_desc);
            break;
        }
    case NC_RO_FIFO_READ_FILL_LEVEL:
        {
            struct _fifoPrivate_t *fi = fifoHandle->private_data;
            if (!fifoReadAccess(fi))
                return NC_UNAUTHORIZED;

            *dataLength = sizeof(int);
            if (fi->state != NC_FIFO_ALLOCATED) {
                *(int *) data = 0;
                break;
            }
            int fillLevel;
            if (XLinkGetFillLevel(fi->streamId, 0, &fillLevel) == X_LINK_SUCCESS) {
                *(int *) data = (fillLevel / fi->graph_tensor_desc.totalSize);
            } else {
                return NC_UNAUTHORIZED;
            }

            break;
        }
    case NC_RO_FIFO_WRITE_FILL_LEVEL:
        {
            struct _fifoPrivate_t *fi = fifoHandle->private_data;
            if (!fifoWriteAccess(fi))
                return NC_UNAUTHORIZED;

            *dataLength = sizeof(int);
            if (fi->state != NC_FIFO_ALLOCATED) {
                *(int *) data = 0;
                break;
            }
            int fillLevel;
            if (XLinkGetFillLevel(fi->streamId, 1, &fillLevel) == X_LINK_SUCCESS) {
                *(int *) data = (fillLevel / fi->graph_tensor_desc.totalSize);
            } else {
                return NC_ERROR;
            }

            break;
        }
    case NC_RW_FIFO_DONT_BLOCK:
        return NC_UNSUPPORTED_FEATURE;
        break;
    case NC_RO_FIFO_STATE:
        *(int *) data = fifoHandle->private_data->state;
        *dataLength = sizeof(int);
        break;
    case NC_RO_FIFO_NAME:
        if (*dataLength < strlen(fifoHandle->private_data->name) + 1) {
            mvLog(MVLOG_ERROR,
                  "data length of output buffer (%d) is smaller that required (%d)!\n",
                  *dataLength, strlen(fifoHandle->private_data->name) + 1);
            *dataLength = strlen(fifoHandle->private_data->name) + 1;
            return NC_INVALID_DATA_LENGTH;
        }
        *dataLength = strlen(fifoHandle->private_data->name) + 1;
        strncpy((char *) data, fifoHandle->private_data->name, *dataLength);
        break;
    default:
        return NC_INVALID_PARAMETERS;
        break;
    }
    return NC_OK;
}

static ncStatus_t tensorCompatibility(struct ncTensorDescriptor_t *tens1,
                                      struct ncTensorDescriptor_t *tens2)
{
    if (tens1->totalSize != tens2->totalSize ||
        tens1->n != tens2->n || tens1->c != tens2->c ||
        tens1->h != tens2->h || tens1->w != tens2->w)
        return NC_ERROR;
    return NC_OK;
}

ncStatus_t ncGraphQueueInference(struct ncGraphHandle_t * graphHandle,
                                 struct ncFifoHandle_t ** fifoIn,
                                 unsigned int inFifoCount,
                                 struct ncFifoHandle_t ** fifoOut,
                                 unsigned int outFifoCount)
{
    mvLog(MVLOG_INFO, "trigger start\n");
    if (!graphHandle || !fifoIn || !fifoIn[0] || !fifoOut || !fifoOut[0]) {
        mvLog(MVLOG_ERROR, "Graph or fifos handles are NULL");
        return NC_INVALID_HANDLE;
    }
    if (!inFifoCount || !outFifoCount)
        return NC_INVALID_PARAMETERS;
    struct _graphPrivate_t *g = graphHandle->private_data;

    if (!g || g->state != NC_GRAPH_ALLOCATED) {
        mvLog(MVLOG_ERROR, "Graph hasn't been allocated");
        return NC_NOT_ALLOCATED;
    }

    if (g->input_count != inFifoCount || g->output_count != outFifoCount) {
        mvLog(MVLOG_ERROR,
              "number of input or output fifos is not compatible with graph");
        return NC_INVALID_PARAMETERS;
    }

    if (inFifoCount != 1 || outFifoCount != 1) {
        mvLog(MVLOG_ERROR,
              "Currently multiple inputs and outputs are not supported");
        return NC_UNSUPPORTED_FEATURE;
    }
    struct _fifoPrivate_t *fi = fifoIn[0]->private_data;
    struct _fifoPrivate_t *fo = fifoOut[0]->private_data;
    ncStatus_t rc;
    if (fi->state != NC_FIFO_ALLOCATED || fo->state != NC_FIFO_ALLOCATED) {
        mvLog(MVLOG_ERROR, "ffos hasn't been allocated");
        return NC_NOT_ALLOCATED;
    }
    //WO fifos have no graph access
    if (fo->type == NC_FIFO_HOST_WO) {
        //graphs have no access to one of the fifos
        return NC_INVALID_PARAMETERS;
    }
    if (tensorCompatibility(&fi->graph_tensor_desc, &g->input_tensor_desc) != NC_OK ||
        tensorCompatibility(&fo->graph_tensor_desc,
                            &g->output_tensor_desc) != NC_OK) {
        mvLog(MVLOG_WARN,
              "Input/Output tensor shape is not compatible with graph");
        return NC_INVALID_PARAMETERS;
    }

    graphMonCommand_t cmd;
    cmd.cmdClass = GRAPH_MON_CLASS_GRAPH_CMD;
    cmd.cmd.graphCmd.type = GRAPH_TRIGGER_CMD;
    cmd.cmd.graphCmd.id = g->id;
    cmd.cmd.graphCmd.buffId1 = fi->id;
    cmd.cmd.graphCmd.buffId2 = fo->id;

    void *user_param;
    pthread_mutex_lock(&fi->fifo_mutex);
    fi->consumers_remaining--;

    if (fi->consumers_remaining == 0) {
        if (!fi->api_read_element && fifoReadAccess(fi)) {  //the element was entirely consumed by graphs. This means we need to free it up from XLink
            streamPacketDesc_t *packet;
            XLinkReadData(fi->streamId, &packet);
            XLinkReleaseData(fi->streamId);
        }
        fi->consumers_remaining = fi->consumer_cnt;
        fi->api_read_element = 0;
    }
    popUserParam(fi, &user_param, 1);
    if (fi->write_count <= fi->consumed_by_graph) {
        mvLog(MVLOG_WARN,
              "No point on triggering graph. There are no more elements in the input FIFO");
        pthread_mutex_unlock(&fi->fifo_mutex);
        return NC_UNAUTHORIZED;
    }
    fi->consumed_by_graph++;
    pthread_mutex_unlock(&fi->fifo_mutex);

    pthread_mutex_lock(&fo->fifo_mutex);
    rc = pushUserParam(fo, user_param, 0);
    if (rc != NC_OK) {
        pthread_mutex_unlock(&fo->fifo_mutex);
        return rc;
    }
    fo->write_count++;
    pthread_mutex_unlock(&fo->fifo_mutex);

    pthread_mutex_lock(&g->dev->graph_streamm);

    if (sendGraphMonitorRequest(g->dev->graph_monitor_stream_id, &cmd)) {
        pthread_mutex_unlock(&g->dev->graph_streamm);
        mvLog(MVLOG_WARN, "Can't send trigger request");
        return NC_ERROR;
    }
    if (checkGraphMonitorResponse(g->dev->graph_monitor_stream_id)) {
        pthread_mutex_unlock(&g->dev->graph_streamm);
        return NC_ERROR;
    }
    pthread_mutex_unlock(&g->dev->graph_streamm);
    g->started = 1;
    mvLog(MVLOG_INFO, "trigger end\n");
    return NC_OK;
}

ncStatus_t ncGraphQueueInferenceWithFifoElem(struct ncGraphHandle_t *
                                             graphHandle,
                                             struct ncFifoHandle_t * fifoIn,
                                             struct ncFifoHandle_t * fifoOut,
                                             const void *inputTensor,
                                             unsigned int * inputTensorLength,
                                             void *userParam)
{
    ncStatus_t rc = ncFifoWriteElem(fifoIn, inputTensor, inputTensorLength,
                                    userParam);
    if (rc != NC_OK)
        return rc;

    return ncGraphQueueInference(graphHandle, &fifoIn, 1, &fifoOut, 1);
}
