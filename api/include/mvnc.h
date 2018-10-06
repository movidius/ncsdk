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

#ifndef __MVNC_H_INCLUDED__
#define __MVNC_H_INCLUDED__

#ifdef __cplusplus
extern "C"
{
#endif

#define MVNC_MAX_NAME_SIZE 28

typedef enum {
	MVNC_OK = 0,
	MVNC_BUSY = -1,                     // Device is busy, retry later
	MVNC_ERROR = -2,                    // Error communicating with the device
	MVNC_OUT_OF_MEMORY = -3,            // Out of memory
	MVNC_DEVICE_NOT_FOUND = -4,         // No device at the given index or name
	MVNC_INVALID_PARAMETERS = -5,       // At least one of the given parameters is wrong
	MVNC_TIMEOUT = -6,                  // Timeout in the communication with the device
	MVNC_MVCMD_NOT_FOUND = -7,         // The file to boot Myriad was not found
	MVNC_NO_DATA = -8,                  // No data to return, call LoadTensor first
	MVNC_GONE = -9,                     // The graph or device has been closed during the operation
	MVNC_UNSUPPORTED_GRAPH_FILE = -10,  // The graph file version is not supported
	MVNC_MYRIAD_ERROR = -11,            // An error has been reported by the device, use MVNC_DEBUG_INFO
} mvncStatus;

typedef enum {
	MVNC_LOG_LEVEL = 0, // Log level, int, 0 = nothing, 1 = errors, 2 = verbose
} mvncGlobalOptions;

typedef enum {
	MVNC_ITERATIONS = 0,        // Number of iterations per inference, int, normally 1, not for general use
	MVNC_NETWORK_THROTTLE = 1,  // Measure temperature once per inference instead of once per layer, int, not for general use
	MVNC_DONT_BLOCK = 2,        // LoadTensor will return BUSY instead of blocking, GetResult will return NO_DATA, int
	MVNC_TIME_TAKEN = 1000,	    // Return time taken for inference (float *)
	MVNC_DEBUG_INFO = 1001,     // Return debug info, string
} mvncGraphOptions;

typedef enum {
	MVNC_TEMP_LIM_LOWER = 1,                // Temperature for short sleep, float, not for general use
	MVNC_TEMP_LIM_HIGHER = 2,               // Temperature for long sleep, float, not for general use
	MVNC_BACKOFF_TIME_NORMAL = 3,           // Normal sleep in ms, int, not for general use
	MVNC_BACKOFF_TIME_HIGH = 4,             // Short sleep in ms, int, not for general use
	MVNC_BACKOFF_TIME_CRITICAL = 5,         // Long sleep in ms, int, not for general use
	MVNC_TEMPERATURE_DEBUG = 6,             // Stop on critical temperature, int, not for general use
	MVNC_THERMAL_STATS = 1000,              // Return temperatures, float *, not for general use
	MVNC_OPTIMISATION_LIST = 1001,          // Return optimisations list, char *, not for general use
	MVNC_THERMAL_THROTTLING_LEVEL = 1002,	// 1=TEMP_LIM_LOWER reached, 2=TEMP_LIM_HIGHER reached
} mvncDeviceOptions;

mvncStatus mvncGetDeviceName(int index, char *name, unsigned int nameSize);
mvncStatus mvncOpenDevice(const char *name, void **deviceHandle);
mvncStatus mvncCloseDevice(void *deviceHandle);
mvncStatus mvncAllocateGraph(void *deviceHandle, void **graphHandle, const void *graphFile, unsigned int graphFileLength);
mvncStatus mvncDeallocateGraph(void *graphHandle);
mvncStatus mvncSetGlobalOption(int option, const void *data, unsigned int dataLength);
mvncStatus mvncGetGlobalOption(int option, void *data, unsigned int *dataLength);
mvncStatus mvncSetGraphOption(void *graphHandle, int option, const void *data, unsigned int dataLength);
mvncStatus mvncGetGraphOption(void *graphHandle, int option, void *data, unsigned int *dataLength);
mvncStatus mvncSetDeviceOption(void *deviceHandle, int option, const void *data, unsigned int dataLength);
mvncStatus mvncGetDeviceOption(void *deviceHandle, int option, void *data, unsigned int *dataLength);
mvncStatus mvncLoadTensor(void *graphHandle, const void *inputTensor, unsigned int inputTensorLength, void *userParam);
mvncStatus mvncGetResult(void *graphHandle, void **outputData, unsigned int *outputDataLength, void **userParam);

#include "mvnc_deprecated.h"
#ifdef __cplusplus
}
#endif

#endif
