/*
* Copyright 2017 Intel Corporation.
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
