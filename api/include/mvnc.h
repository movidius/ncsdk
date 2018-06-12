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

#ifndef __NC_H_INCLUDED__
#define __NC_H_INCLUDED__

#ifdef __cplusplus
extern "C"
{
#endif
#define NC_MAX_NAME_SIZE 28
#define NC_THERMAL_BUFFER_SIZE 100
#define NC_DEBUG_BUFFER_SIZE   120
#define NC_VERSION_MAX_SIZE 4

#if (defined (WINNT) || defined(_WIN32) || defined(_WIN64) )
#include <windows.h>	// for Sleep()
#define dllexport __declspec( dllexport )
#else
#define dllexport
#endif //WINNT

typedef enum {
    NC_OK = 0,
    NC_BUSY = -1,                     // Device is busy, retry later
    NC_ERROR = -2,                    // Error communicating with the device
    NC_OUT_OF_MEMORY = -3,            // Out of memory
    NC_DEVICE_NOT_FOUND = -4,         // No device at the given index or name
    NC_INVALID_PARAMETERS = -5,       // At least one of the given parameters is wrong
    NC_TIMEOUT = -6,                  // Timeout in the communication with the device
    NC_MVCMD_NOT_FOUND = -7,          // The file to boot Myriad was not found
    NC_NOT_ALLOCATED = -8,            // The graph or device has been closed during the operation
    NC_UNAUTHORIZED = -9,             // Unauthorized operation
    NC_UNSUPPORTED_GRAPH_FILE = -10,  // The graph file version is not supported
    NC_UNSUPPORTED_CONFIGURATION_FILE = -11, // The configuration file version is not supported
    NC_UNSUPPORTED_FEATURE = -12,     // Not supported by this FW version
    NC_MYRIAD_ERROR = -13,            // An error has been reported by the device
                                      // use  NC_DEVICE_DEBUG_INFO or NC_GRAPH_DEBUG_INFO
    NC_INVALID_DATA_LENGTH = -14,      // invalid data length has been passed when get/set option
    NC_INVALID_HANDLE = -15           // handle to object that is invalid
} ncStatus_t;

typedef enum {
    NC_LOG_DEBUG = 0,   // debug and above (full verbosity)
    NC_LOG_INFO,        // info and above
    NC_LOG_WARN,        // warning and above
    NC_LOG_ERROR,       // errors and above
    NC_LOG_FATAL,       // fatal only
} ncLogLevel_t;

typedef enum {
    NC_RW_LOG_LEVEL = 0,    // Log level, int, default NC_LOG_WARN
    NC_RO_API_VERSION = 1,  // retruns API Version. array of unsigned int of size 4
                            //major.minor.hotfix.rc
} ncGlobalOption_t;

typedef enum {
    NC_RO_GRAPH_STATE = 1000,           // Returns graph state: CREATED, ALLOCATED, WAITING_FOR_BUFFERS, RUNNING, DESTROYED
    NC_RO_GRAPH_TIME_TAKEN = 1001,      // Return time taken for last inference (float *)
    NC_RO_GRAPH_INPUT_COUNT = 1002,     // Returns number of inputs, size of array returned
                                        // by NC_RO_INPUT_TENSOR_DESCRIPTORS, int
    NC_RO_GRAPH_OUTPUT_COUNT = 1003,    // Returns number of outputs, size of array returned
                                        // by NC_RO_OUTPUT_TENSOR_DESCRIPTORS,int
    NC_RO_GRAPH_INPUT_TENSOR_DESCRIPTORS = 1004,  // Return a tensorDescriptor pointer array
                                            // which describes the graph inputs in order.
                                            // Can be used for fifo creation.
                                            // The length of the array can be retrieved
                                            // using the NC_RO_INPUT_COUNT option

    NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS = 1005, // Return a tensorDescriptor pointer
                                            // array which describes the graph
                                            // outputs in order. Can be used for
                                            // fifo creation. The length of the
                                            // array can be retrieved using the
                                            // NC_RO_OUTPUT_COUNT option

    NC_RO_GRAPH_DEBUG_INFO = 1006,          // Return debug info, string
    NC_RO_GRAPH_NAME = 1007,                // Returns name of the graph, string
    NC_RO_GRAPH_OPTION_CLASS_LIMIT = 1008,  // return the highest option class supported
    NC_RO_GRAPH_VERSION = 1009,             // returns graph version, string
    NC_RO_GRAPH_TIME_TAKEN_ARRAY_SIZE = 1011, // Return size of array for time taken option, int
    NC_RW_GRAPH_EXECUTORS_NUM = 1110,
} ncGraphOption_t;

typedef enum {
    NC_DEVICE_CREATED = 0,
    NC_DEVICE_OPENED = 1,
    NC_DEVICE_CLOSED = 2,
} ncDeviceState_t;

typedef enum {
    NC_GRAPH_CREATED = 0,
    NC_GRAPH_ALLOCATED = 1,
    NC_GRAPH_WAITING_FOR_BUFFERS = 2,
    NC_GRAPH_RUNNING = 3,
} ncGraphState_t;

typedef enum {
    NC_FIFO_CREATED = 0,
    NC_FIFO_ALLOCATED = 1,
} ncFifoState_t;

typedef enum {
    NC_MA2450 = 0,
    NC_MA2480 = 1,
} ncDeviceHwVersion_t;

typedef enum {
    NC_RO_DEVICE_THERMAL_STATS = 2000,          // Return temperatures, float *, not for general use
    NC_RO_DEVICE_THERMAL_THROTTLING_LEVEL = 2001,   // 1=TEMP_LIM_LOWER reached, 2=TEMP_LIM_HIGHER reached
    NC_RO_DEVICE_STATE = 2002,                  // Returns device state: CREATED, OPENED, CLOSED, DESTROYED
    NC_RO_DEVICE_CURRENT_MEMORY_USED = 2003,    // Returns current device memory usage
    NC_RO_DEVICE_MEMORY_SIZE = 2004,            // Returns device memory size
    NC_RO_DEVICE_MAX_FIFO_NUM = 2005,           // return the maximum number of fifos supported
    NC_RO_DEVICE_ALLOCATED_FIFO_NUM = 2006,     // return the number of currently allocated fifos
    NC_RO_DEVICE_MAX_GRAPH_NUM = 2007,          // return the maximum number of graphs supported
    NC_RO_DEVICE_ALLOCATED_GRAPH_NUM = 2008,    //  return the number of currently allocated graphs
    NC_RO_DEVICE_OPTION_CLASS_LIMIT = 2009,     //  return the highest option class supported
    NC_RO_DEVICE_FW_VERSION = 2010,             // return device firmware version, array of unsigned int of size 4
                                                //major.minor.hwtype.buildnumber
    NC_RO_DEVICE_DEBUG_INFO = 2011,             // Return debug info, string, not supported yet
    NC_RO_DEVICE_MVTENSOR_VERSION = 2012,       // returns mv tensor version, array of unsigned int of size 2
                                                //major.minor
    NC_RO_DEVICE_NAME = 2013,                   // returns device name as generated internally
    NC_RO_DEVICE_MAX_EXECUTORS_NUM = 2014,      //Maximum number of executers per graph
    NC_RO_DEVICE_HW_VERSION = 2015,             //returns HW Version, enum
} ncDeviceOption_t;

typedef struct _devicePrivate_t devicePrivate_t;
typedef struct _graphPrivate_t graphPrivate_t;
typedef struct _fifoPrivate_t fifoPrivate_t;
typedef struct _ncTensorDescriptorPrivate_t ncTensorDescriptorPrivate_t;

struct ncFifoHandle_t {
    // keep place for public data here
    fifoPrivate_t* private_data;
};

struct ncGraphHandle_t {
    // keep place for public data here
    graphPrivate_t* private_data;
};

struct ncDeviceHandle_t {
    // keep place for public data here
    devicePrivate_t* private_data;
};

typedef enum {
    NC_FIFO_HOST_RO = 0, // fifo can be read through the API but can not be
                         // written ( graphs can read and write data )
    NC_FIFO_HOST_WO = 1, // fifo can be written through the API but can not be
                         // read (graphs can read but can not write)
} ncFifoType_t;

typedef enum {
    NC_FIFO_FP16 = 0,
    NC_FIFO_FP32 = 1,
} ncFifoDataType_t;


struct ncTensorDescriptor_t {
    unsigned int n;         // batch size, currently only 1 is supported
    unsigned int c;         // number of channels
    unsigned int w;         // width
    unsigned int h;         // height
    unsigned int totalSize; // Total size of the data in tensor = largest stride* dim size
    unsigned int cStride;   // Stride in the channels' dimension
    unsigned int wStride;   // Stride in the horizontal dimension
    unsigned int hStride;   // Stride in the vertical dimension
    ncFifoDataType_t dataType;  // data type of the tensor, FP32 or FP16
};

typedef enum {
    NC_RW_FIFO_TYPE = 0,            // configure the fifo type to one type from ncFifoType_t
    NC_RW_FIFO_CONSUMER_COUNT = 1,  // The number of consumers of elements
                                    // (the number of times data must be read by
                                    // a graph or host before the element is removed.
                                    // Defaults to 1. Host can read only once always.
    NC_RW_FIFO_DATA_TYPE = 2,       // 0 for fp16, 1 for fp32. If configured to fp32,
                                    // the API will convert the data to the internal
                                    // fp16 format automatically
    NC_RW_FIFO_DONT_BLOCK = 3,      // WriteTensor will return NC_OUT_OF_MEMORY instead
                                    // of blocking, GetResult will return NO_DATA, not supported yet
    NC_RO_FIFO_CAPACITY = 4,        // return number of maximum elements in the buffer
    NC_RO_FIFO_READ_FILL_LEVEL = 5,     // return number of tensors in the read buffer
    NC_RO_FIFO_WRITE_FILL_LEVEL = 6,    // return number of tensors in a write buffer
    NC_RO_FIFO_GRAPH_TENSOR_DESCRIPTOR = 7,   // return the tensor descriptor of the FIFO
    NC_RO_FIFO_TENSOR_DESCRIPTOR = NC_RO_FIFO_GRAPH_TENSOR_DESCRIPTOR,   // deprecated
    NC_RO_FIFO_STATE = 8,               // return the fifo state, returns CREATED, ALLOCATED,DESTROYED
    NC_RO_FIFO_NAME = 9,                // return fifo name
    NC_RO_FIFO_ELEMENT_DATA_SIZE = 10,  //element data size in bytes, int
    NC_RW_FIFO_HOST_TENSOR_DESCRIPTOR = 11,  // App's tensor descriptor, defaults to none strided channel minor
} ncFifoOption_t;

// Global
dllexport ncStatus_t ncGlobalSetOption(int option, const void *data,
	unsigned int dataLength);
dllexport ncStatus_t ncGlobalGetOption(int option, void *data,
                        unsigned int *dataLength);

// Device
dllexport ncStatus_t ncDeviceSetOption(struct ncDeviceHandle_t *deviceHandle,
                        int option, const void *data,
                        unsigned int dataLength);
dllexport ncStatus_t ncDeviceGetOption(struct ncDeviceHandle_t *deviceHandle,
                        int option, void *data, unsigned int *dataLength);
dllexport ncStatus_t ncDeviceCreate(int index,struct ncDeviceHandle_t **deviceHandle);
dllexport ncStatus_t ncDeviceOpen(struct ncDeviceHandle_t *deviceHandle);
dllexport ncStatus_t ncDeviceClose(struct ncDeviceHandle_t *deviceHandle);
dllexport ncStatus_t ncDeviceDestroy(struct ncDeviceHandle_t **deviceHandle);

// Graph
dllexport ncStatus_t ncGraphCreate(const char* name, struct ncGraphHandle_t **graphHandle);
dllexport ncStatus_t ncGraphAllocate(struct ncDeviceHandle_t *deviceHandle,
                        struct ncGraphHandle_t *graphHandle,
                        const void *graphBuffer, unsigned int graphBufferLength);
dllexport ncStatus_t ncGraphDestroy(struct ncGraphHandle_t **graphHandle);
dllexport ncStatus_t ncGraphSetOption(struct ncGraphHandle_t *graphHandle,
                        int option, const void *data, unsigned int dataLength);
dllexport ncStatus_t ncGraphGetOption(struct ncGraphHandle_t *graphHandle,
                        int option, void *data,
                        unsigned int *dataLength);
dllexport ncStatus_t ncGraphQueueInference(struct ncGraphHandle_t *graphHandle,
                        struct ncFifoHandle_t** fifoIn, unsigned int inFifoCount,
                        struct ncFifoHandle_t** fifoOut, unsigned int outFifoCount);

//Helper functions
dllexport ncStatus_t ncGraphQueueInferenceWithFifoElem(struct ncGraphHandle_t *graphHandle,
                        struct ncFifoHandle_t* fifoIn,
                        struct ncFifoHandle_t* fifoOut, const void *inputTensor,
                        unsigned int * inputTensorLength, void *userParam);
dllexport ncStatus_t ncGraphAllocateWithFifos(struct ncDeviceHandle_t* deviceHandle,
                        struct ncGraphHandle_t* graphHandle,
                        const void *graphBuffer, unsigned int graphBufferLength,
                        struct ncFifoHandle_t ** inFifoHandle,
                        struct ncFifoHandle_t ** outFifoHandle);

dllexport ncStatus_t ncGraphAllocateWithFifosEx(struct ncDeviceHandle_t* deviceHandle,
                        struct ncGraphHandle_t* graphHandle,
                        const void *graphBuffer, unsigned int graphBufferLength,
                        struct ncFifoHandle_t ** inFifoHandle, ncFifoType_t inFifoType,
                        int inNumElem, ncFifoDataType_t inDataType,
                        struct ncFifoHandle_t ** outFifoHandle,  ncFifoType_t outFifoType,
                        int outNumElem, ncFifoDataType_t outDataType);
// Fifo
dllexport ncStatus_t ncFifoCreate(const char* name, ncFifoType_t type,
                        struct ncFifoHandle_t** fifoHandle);
dllexport ncStatus_t ncFifoAllocate(struct ncFifoHandle_t* fifoHandle,
                        struct ncDeviceHandle_t* device,
                        struct ncTensorDescriptor_t* tensorDesc,
                        unsigned int numElem);
dllexport ncStatus_t ncFifoSetOption(struct ncFifoHandle_t* fifoHandle, int option,
                        const void *data, unsigned int dataLength);
dllexport ncStatus_t ncFifoGetOption(struct ncFifoHandle_t* fifoHandle, int option,
                        void *data, unsigned int *dataLength);


dllexport ncStatus_t ncFifoDestroy(struct ncFifoHandle_t** fifoHandle);
dllexport ncStatus_t ncFifoWriteElem(struct ncFifoHandle_t* fifoHandle, const void *inputTensor,
                        unsigned int * inputTensorLength, void *userParam);
dllexport ncStatus_t ncFifoReadElem(struct ncFifoHandle_t* fifoHandle, void *outputData,
                        unsigned int* outputDataLen, void **userParam);
dllexport ncStatus_t ncFifoRemoveElem(struct ncFifoHandle_t* fifoHandle); //not supported yet
#ifdef __cplusplus
}
#endif

#endif
