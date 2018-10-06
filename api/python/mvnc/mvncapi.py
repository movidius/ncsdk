# Copyright (c) 2017-2018 Intel Corporation. All Rights Reserved
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import numpy
import warnings
from enum import Enum
from ctypes import *

# The toolkit wants its local version
try:
    f = CDLL("./libmvnc.so")
except:
    f = CDLL("libmvnc.so")

warnings.simplefilter('default', DeprecationWarning)


class EnumDeprecationHelper(object):
    def __init__(self, new_target, deprecated_values):
        self.new_target = new_target
        self.deprecated_values = deprecated_values

    def __call__(self, *args, **kwargs):
        return self.new_target(*args, **kwargs)

    def __getattr__(self, attr):
        if (attr in self.deprecated_values):
                warnings.warn('\033[93m' + "\"" + attr + "\" is deprecated. Please use \"" +
                              self.deprecated_values[attr] + "\"!" + '\033[0m',
                              DeprecationWarning, stacklevel=2)
                return getattr(self.new_target, self.deprecated_values[attr])
        return getattr(self.new_target, attr)


class mvncStatus(Enum):
    OK = 0
    BUSY = -1
    ERROR = -2
    OUT_OF_MEMORY = -3
    DEVICE_NOT_FOUND = -4
    INVALID_PARAMETERS = -5
    TIMEOUT = -6
    MVCMD_NOT_FOUND = -7
    NO_DATA = -8
    GONE = -9
    UNSUPPORTED_GRAPH_FILE = -10
    MYRIAD_ERROR = -11

Status = EnumDeprecationHelper(mvncStatus, {"MVCMDNOTFOUND": "MVCMD_NOT_FOUND",
                                            "NODATA": "NO_DATA",
                                            "UNSUPPORTEDGRAPHFILE": "UNSUPPORTED_GRAPH_FILE",
                                            "MYRIADERROR": "MYRIAD_ERROR"})


class mvncGlobalOption(Enum):
    LOG_LEVEL = 0

GlobalOption = EnumDeprecationHelper(mvncGlobalOption, {"LOGLEVEL": "LOG_LEVEL"})


class mvncDeviceOption(Enum):
    TEMP_LIM_LOWER = 1
    TEMP_LIM_HIGHER = 2
    BACKOFF_TIME_NORMAL = 3
    BACKOFF_TIME_HIGH = 4
    BACKOFF_TIME_CRITICAL = 5
    TEMPERATURE_DEBUG = 6
    THERMAL_STATS = 1000
    OPTIMISATION_LIST = 1001
    THERMAL_THROTTLING_LEVEL = 1002

DeviceOption = EnumDeprecationHelper(mvncDeviceOption, {"THERMALSTATS": "THERMAL_STATS",
                                                        "OPTIMISATIONLIST": "OPTIMISATION_LIST"})


class mvncGraphOption(Enum):
    ITERATIONS = 0
    NETWORK_THROTTLE = 1
    DONT_BLOCK = 2
    TIME_TAKEN = 1000
    DEBUG_INFO = 1001

GraphOption = EnumDeprecationHelper(mvncGraphOption, {"DONTBLOCK": "DONT_BLOCK",
                                                      "TIMETAKEN": "TIME_TAKEN",
                                                      "DEBUGINFO": "DEBUG_INFO"})


def EnumerateDevices():
    name = create_string_buffer(28)
    i = 0
    devices = []
    while True:
        if f.mvncGetDeviceName(i, name, 28) != 0:
            break
        devices.append(name.value.decode("utf-8"))
        i = i + 1
    return devices


def SetGlobalOption(opt, data):
    data = c_int(data)
    status = f.mvncSetGlobalOption(opt.value, pointer(data), sizeof(data))
    if status != Status.OK.value:
        raise Exception(Status(status))


def GetGlobalOption(opt):
    if opt == GlobalOption.LOG_LEVEL:
        optsize = c_uint()
        optvalue = c_uint()
        status = f.mvncGetGlobalOption(opt.value, byref(optvalue), byref(optsize))
        if status != Status.OK.value:
            raise Exception(Status(status))
        return optvalue.value
    optsize = c_uint()
    optdata = POINTER(c_byte)()
    status = f.mvncGetDeviceOption(0, opt.value, byref(optdata), byref(optsize))
    if status != Status.OK.value:
        raise Exception(Status(status))
    v = create_string_buffer(optsize.value)
    memmove(v, optdata, optsize.value)
    return v.raw


class Device:
    def __init__(self, name):
        self.handle = c_void_p()
        self.name = name

    def OpenDevice(self):
        status = f.mvncOpenDevice(bytes(bytearray(self.name, "utf-8")), byref(self.handle))
        if status != Status.OK.value:
            raise Exception(Status(status))

    def CloseDevice(self):
        status = f.mvncCloseDevice(self.handle)
        self.handle = c_void_p()
        if status != Status.OK.value:
            raise Exception(Status(status))

    def SetDeviceOption(self, opt, data):
        if opt == DeviceOption.TEMP_LIM_HIGHER or opt == DeviceOption.TEMP_LIM_LOWER:
            data = c_float(data)
        else:
            data = c_int(data)
        status = f.mvncSetDeviceOption(self.handle, opt.value, pointer(data), sizeof(data))
        if status != Status.OK.value:
            raise Exception(Status(status))

    def GetDeviceOption(self, opt):
        if opt == DeviceOption.TEMP_LIM_HIGHER or opt == DeviceOption.TEMP_LIM_LOWER:
            optdata = c_float()
        elif (opt == DeviceOption.BACKOFF_TIME_NORMAL or opt == DeviceOption.BACKOFF_TIME_HIGH or
              opt == DeviceOption.BACKOFF_TIME_CRITICAL or opt == DeviceOption.TEMPERATURE_DEBUG or
              opt == DeviceOption.THERMAL_THROTTLING_LEVEL):
            optdata = c_int()
        else:
            optdata = POINTER(c_byte)()
        optsize = c_uint()
        status = f.mvncGetDeviceOption(self.handle, opt.value, byref(optdata), byref(optsize))
        if status != Status.OK.value:
            raise Exception(Status(status))
        if opt == DeviceOption.TEMP_LIM_HIGHER or opt == DeviceOption.TEMP_LIM_LOWER:
            return optdata.value
        elif (opt == DeviceOption.BACKOFF_TIME_NORMAL or opt == DeviceOption.BACKOFF_TIME_HIGH or
              opt == DeviceOption.BACKOFF_TIME_CRITICAL or opt == DeviceOption.TEMPERATURE_DEBUG or
              opt == DeviceOption.THERMAL_THROTTLING_LEVEL):
            return optdata.value
        v = create_string_buffer(optsize.value)
        memmove(v, optdata, optsize.value)
        if opt == DeviceOption.OPTIMISATION_LIST:
            l = []
            for i in range(40):
                if v.raw[i * 50] != 0:
                    ss = v.raw[i * 50:]
                    end = ss.find(b'\x00')
                    val = ss[0:end].decode()
                    if val:
                        l.append(val)
            return l
        if opt == DeviceOption.THERMAL_STATS:
            return numpy.frombuffer(v.raw, dtype=numpy.float32)
        return int.from_bytes(v.raw, byteorder='little')

    def AllocateGraph(self, graphfile):
        hgraph = c_void_p()
        status = f.mvncAllocateGraph(self.handle, byref(hgraph), graphfile, len(graphfile))
        if status != Status.OK.value:
            raise Exception(Status(status))
        return Graph(hgraph)


class Graph:
    def __init__(self, handle):
        self.handle = handle
        self.userobjs = {}

    def SetGraphOption(self, opt, data):
        data = c_int(data)
        status = f.mvncSetGraphOption(self.handle, opt.value, pointer(data), sizeof(data))
        if status != Status.OK.value:
            raise Exception(Status(status))

    def GetGraphOption(self, opt):
        if opt == GraphOption.ITERATIONS or opt == GraphOption.NETWORK_THROTTLE or opt == GraphOption.DONT_BLOCK:
            optdata = c_int()
        else:
            optdata = POINTER(c_byte)()
        optsize = c_uint()
        status = f.mvncGetGraphOption(self.handle, opt.value, byref(optdata), byref(optsize))
        if status != Status.OK.value:
            raise Exception(Status(status))
        if opt == GraphOption.ITERATIONS or opt == GraphOption.NETWORK_THROTTLE or opt == GraphOption.DONT_BLOCK:
            return optdata.value
        v = create_string_buffer(optsize.value)
        memmove(v, optdata, optsize.value)
        if opt == GraphOption.TIME_TAKEN:
            return numpy.frombuffer(v.raw, dtype=numpy.float32)
        if opt == GraphOption.DEBUG_INFO:
            return v.raw[0:v.raw.find(0)].decode()
        return int.from_bytes(v.raw, byteorder='little')

    def DeallocateGraph(self):
        status = f.mvncDeallocateGraph(self.handle)
        self.handle = 0
        if status != Status.OK.value:
            raise Exception(Status(status))

    def LoadTensor(self, tensor, userobj):
        tensor = tensor.tostring()
        userobj = py_object(userobj)
        key = c_long(addressof(userobj))
        self.userobjs[key.value] = userobj
        status = f.mvncLoadTensor(self.handle, tensor, len(tensor), key)
        if status == Status.BUSY.value:
            return False
        if status != Status.OK.value:
            del self.userobjs[key.value]
            raise Exception(Status(status))
        return True

    def GetResult(self):
        tensor = c_void_p()
        tensorlen = c_uint()
        userobj = c_long()
        status = f.mvncGetResult(self.handle, byref(tensor), byref(tensorlen), byref(userobj))
        if status == Status.NO_DATA.value:
            return None, None
        if status != Status.OK.value:
            raise Exception(Status(status))
        v = create_string_buffer(tensorlen.value)
        memmove(v, tensor, tensorlen.value)
        tensor = numpy.frombuffer(v.raw, dtype=numpy.float16)
        retuserobj = self.userobjs[userobj.value]
        del self.userobjs[userobj.value]
        return tensor, retuserobj.value
