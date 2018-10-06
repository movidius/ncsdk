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
