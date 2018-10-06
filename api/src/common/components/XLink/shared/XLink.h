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
/// @brief     Application configuration Leon header
///

#ifndef _XLINK_H
#define _XLINK_H
#include "XLinkPublicDefines.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Initializes XLink and scheduler, on myriad it starts the dispatcher
XLinkError_t XLinkInitialize(XLinkGlobalHandler_t* handler);

// Connects to specific device, starts dispatcher and pings remote
XLinkError_t XLinkConnect(XLinkHandler_t* handler);

// Opens a stream in the remote that can be written to by the local
// Allocates stream_write_size (aligned up to 64 bytes) for that stream
streamId_t XLinkOpenStream(linkId_t id, const char* name, int stream_write_size);

// Close stream for any further data transfer
// Stream will be deallocated when all pending data has been released
XLinkError_t XLinkCloseStream(streamId_t streamId);

// Currently useless
XLinkError_t XLinkGetAvailableStreams(linkId_t id);

XLinkError_t XLinkGetDeviceName(int index, char* name, int nameSize);
XLinkError_t XLinkGetDeviceNameExtended(int index, char* name, int nameSize, int pid);

// Send a package to initiate the writing of data to a remote stream
// Note that the actual size of the written data is ALIGN_UP(size, 64)
XLinkError_t XLinkWriteData(streamId_t streamId, const uint8_t* buffer, int size);

// Currently useless
XLinkError_t XLinkAsyncWriteData();

// Read data from local stream. Will only have something if it was written
// to by the remote
XLinkError_t XLinkReadData(streamId_t streamId, streamPacketDesc_t** packet);

// Release data from stream - This should be called after ReadData
XLinkError_t XLinkReleaseData(streamId_t streamId);

//Read fill level
XLinkError_t XLinkGetFillLevel(streamId_t streamId, int isRemote, int* fillLevel);

// Boot the remote (This is intended as an interface to boot the Myriad
// from PC)
XLinkError_t XLinkBootRemote(const char* deviceName, const char* binaryPath);

// Reset the remote
XLinkError_t XLinkResetRemote(linkId_t id);

//Closes connection only
XLinkError_t XLinkDisconnect(linkId_t id);

// Close all and release all memory
XLinkError_t XLinkResetAll();

// Profiling funcs - keeping them global for now
XLinkError_t XLinkProfStart();
XLinkError_t XLinkProfStop();
XLinkError_t XLinkProfPrint();

#ifdef __cplusplus
}
#endif

#endif
