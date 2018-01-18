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

// Common logging macros
#define PRINT_DEBUG(...)  {if (mvnc_loglevel > 1) fprintf(__VA_ARGS__);}
#define PRINT_DEBUG_F(...)  {if (mvnc_loglevel > 1) \
                                { fprintf(__VA_ARGS__); fflush(stderr); } }\

#define PRINT_INFO(...)   {if (mvnc_loglevel > 0) fprintf(__VA_ARGS__);}
#define PRINT_INFO_F(...)  {if (mvnc_loglevel > 0) \
                                { fprintf(__VA_ARGS__); fflush(stderr); } }\

#define PRINT(...) fprintf(stderr,__VA_ARGS__)

// Common defines
#define DEFAULT_VID				0x03E7
#define DEFAULT_PID				0x2150	// Myriad2v2 ROM
#define DEFAULT_OPEN_VID			DEFAULT_VID
#define DEFAULT_OPEN_PID			0xf63b	// Once opened in VSC mode, VID/PID change
