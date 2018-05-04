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
/// @brief     MVNC host-device communication structures
///


// Includes
// ----------------------------------------------------------------------------

#ifndef _NC_PRIVATE_TYPES_H_
#define _NC_PRIVATE_TYPES_H_

#include <pthread.h>
#include <mvnc.h>
#include "ncCommPrivate.h"
#include "XLinkPublicDefines.h"
#define NC_MAX_NAME_SIZE        28

typedef enum {
    NC_OPTION_CLASS0 = 0,
    NC_OPTION_CLASS1 = 1,
    NC_OPTION_CLASS2 = 2,
    NC_OPTION_CLASS3 = 3,
} ncOptionClass_t;

struct _devicePrivate_t {
    int throttle_happened;
    float *thermal_stats;
    char *dev_addr;             // Device USB address as returned by usb_
    char *dev_addr2;            // USB address post FW flash (might for some devices)
    char *dev_file;             // Device filename in /dev directory
    char *optimisation_list;
    XLinkHandler_t *usb_link;
    struct _devicePrivate_t *next;  // Next device in chain
    struct _graphPrivate_t *graphs; // List of associated graphs
    struct _fifoPrivate_t *fifos;   // List of associated fifos
    streamId_t device_mon_stream_id;
    streamId_t graph_monitor_stream_id;
    pthread_mutex_t dev_data_m;
    pthread_mutex_t dev_stream_m;
    pthread_mutex_t graph_streamm;
    deviceCapabilities_t dev_attr;
    ncDeviceState_t state;
} *devices;
struct _userParamPrivate_t {
    void *data;
    struct _userParamPrivate_t *next;
};
struct _graphPrivate_t {
    uint32_t id;
    uint32_t blob_version[2];
    int started;
    int batch_size;
    int executors_number;
    int input_count;
    int output_count;
    struct ncTensorDescriptor_t input_tensor_desc;
    struct ncTensorDescriptor_t output_tensor_desc;
    unsigned nstages;
    struct _devicePrivate_t *dev;
    struct _graphPrivate_t *next;
    char name[NC_MAX_NAME_SIZE];
    streamId_t graph_stream_id;
    ncGraphState_t state;
};

struct _fifoPrivate_t {
    ncFifoType_t type;
    int consumer_cnt;
    uint32_t id;
    streamId_t streamId;
    struct ncTensorDescriptor_t tensor_desc;
    struct _devicePrivate_t *dev;
    struct _fifoPrivate_t *next;
    char name[NC_MAX_NAME_SIZE];
    struct _userParamPrivate_t *user_param_in;  //used for write fifo
    struct _userParamPrivate_t *user_param_out; //used for read fifo
    int write_count;
    int consumed_by_graph;
    int num_elements;
    int api_read_element;
    int api_read_adjust;
    int datatype;
    int consumers_remaining;
    int datasize;
    pthread_mutex_t fifo_mutex;
    ncFifoState_t state;
};
#endif
