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

#ifndef __MVNC_DEPRECATED_H_INCLUDED__
#define __MVNC_DEPRECATED_H_INCLUDED__

#ifdef __cplusplus
extern "C"
{
#endif

typedef mvncGraphOptions GraphOptions __attribute__ \
			((deprecated("GraphOptions is deprecated. Please use mvncGraphOptions")));
typedef mvncDeviceOptions DeviceOptions __attribute__ \
			((deprecated("DeviceOptions is deprecated. Please use mvncDeviceOptions")));

// Deprecated Define
#define MVNC_MAXNAMESIZE  _Pragma("GCC warning \"'MVNC_MAXNAMESIZE' is deprecated. Please use 'MVNC_MAX_NAME_SIZE'\"") MVNC_MAX_NAME_SIZE

// Deprecated Global Options
#define MVNC_LOGLEVEL	_Pragma("GCC warning \"'MVNC_LOGLEVEL' is deprecated. Please use 'MVNC_LOG_LEVEL'\"") MVNC_LOG_LEVEL

// Deprecated status values
#define MVNC_MVCMDNOTFOUND          _Pragma("GCC warning \"'MVNC_MVCMDNOTFOUND' is deprecated. Please use 'MVNC_MVCMD_NOT_FOUND'\"") MVNC_MVCMD_NOT_FOUND
#define MVNC_NODATA                 _Pragma("GCC warning \"'MVNC_NO_DATA' is deprecated. Please use 'MVNC_NO_DATA'\"") MVNC_NO_DATA
#define MVNC_UNSUPPORTEDGRAPHFILE   _Pragma("GCC warning \"'MVNC_UNSUPPORTEDGRAPHFILE' is deprecated. Please use 'MVNC_UNSUPPORTED_GRAPH_FILE'\"") MVNC_UNSUPPORTED_GRAPH_FILE
#define MVNC_MYRIADERROR            _Pragma("GCC warning \"'MVNC_MYRIADERROR' is deprecated. Please use 'MVNC_MYRIAD_ERROR'\"") MVNC_MYRIAD_ERROR

// Deprecated Graph Options values
#define MVNC_DONTBLOCK  _Pragma("GCC warning \"'MVNC_DONTBLOCK' is deprecated. Please use 'MVNC_DONT_BLOCK'\"") MVNC_DONT_BLOCK
#define MVNC_TIMETAKEN  _Pragma("GCC warning \"'MVNC_TIMETAKEN' is deprecated. Please use 'MVNC_TIME_TAKEN'\"") MVNC_TIME_TAKEN
#define MVNC_DEBUGINFO  _Pragma("GCC warning \"'MVNC_DEBUGINFO' is deprecated. Please use 'MVNC_DEBUG_INFO'\"") MVNC_DEBUG_INFO

// Deprecated Device Options Values
#define MVNC_THERMALSTATS      _Pragma("GCC warning \"'MVNC_THERMALSTATS' is deprecated. Please use 'MVNC_THERMAL_STATS'\"") MVNC_THERMAL_STATS
#define MVNC_OPTIMISATIONLIST  _Pragma("GCC warning \"'MVNC_OPTIMISATIONLIST' is deprecated. Please use 'MVNC_OPTIMISATION_LIST'\"") MVNC_OPTIMISATION_LIST

#ifdef __cplusplus
}
#endif

#endif
