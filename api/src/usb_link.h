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

#include "USBLinkDefines.h"

int usblink_sendcommand(void *f, hostcommands_t command);
int usblink_resetmyriad(void *f);
int usblink_getmyriadstatus(void *f, myriadStatus_t *myriadState);
void *usblink_open(const char *path);
void usblink_close(void *f);
int usblink_setdata(void *f, const char *name, const void *data, unsigned int length, int hostready);
int usblink_getdata(void *f, const char *name, void *data, unsigned int length, unsigned int offset, int hostready);
void usblink_resetall();
