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

#ifndef _USBLINKCOMMONDEFINES_H
#define _USBLINKCOMMONDEFINES_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#define MAX_NAME_LENGTH 52
// Packet length will define the maximum message length between pc and myriad. All bigger messages than this number will be split in multiple messages
#define PACKET_LENGTH (64*1024)

typedef struct bufferEntryDesc_t {
	char name[MAX_NAME_LENGTH];
	uint8_t *data;
	uint32_t length;
} bufferEntryDesc_t;

typedef enum {
	USB_LINK_GET_MYRIAD_STATUS = 0,
	USB_LINK_RESET_REQUEST,
	USB_LINK_HOST_SET_DATA,
	USB_LINK_HOST_GET_DATA
} hostcommands_t;

typedef enum {
	MYRIAD_NOT_INIT = 0,
	MYRIAD_INITIALIZED = 0x11,
	MYRIAD_WAITING = 0x22,
	MYRIAD_RUNNING = 0x33,
	MYRIAD_FINISHED = 0x44,
	MYRIAD_PENDING = 0x55,
} myriadStatus_t;

typedef struct usbHeader_t {
	uint8_t cmd;
	uint8_t hostready;
	uint16_t reserved;
	uint32_t dataLength;
	uint32_t offset;
	char name[MAX_NAME_LENGTH];
} usbHeader_t;

#ifdef __cplusplus
}
#endif
#endif
/* end of include file */
