# Copyright 2018 Intel Corporation.
# The source code, information and material ("Material") contained herein is
# owned by Intel Corporation or its suppliers or licensors, and title to such
# Material remains with Intel Corporation or its suppliers or licensors.
# The Material contains proprietary information of Intel or its suppliers and
# licensors. The Material is protected by worldwide copyright laws and treaty
# provisions.
# No part of the Material may be used, copied, reproduced, modified, published,
# uploaded, posted, transmitted, distributed or disclosed in any way without
# Intel's prior express written permission. No license under any patent,
# copyright or other intellectual property rights in the Material is granted to
# or conferred upon you, either expressly, by implication, inducement, estoppel
# or otherwise.
# Any license under such intellectual property rights must be express and
# approved by Intel in writing.
import os
import sys
import numpy
import warnings
import platform
import numbers
from enum import Enum
from ctypes import *

# The toolkit wants its local version
try:
    if 'Windows' in platform.architecture()[1]:
        f = CDLL("./mvnc.dll")
    else:
        f = CDLL("./libmvnc.so")
except:
    if 'Windows' in platform.architecture()[1]:
        path = os.path.abspath(__file__)
        dir_path = os.path.dirname(path)
        f = CDLL(os.path.join(dir_path, "mvnc.dll"))
    else:
        f = CDLL("libmvnc.so")

warnings.simplefilter('default', DeprecationWarning)

"""Define statements"""
MAX_NAME_SIZE = 28
THERMAL_BUFFER_SIZE = 100
DEBUG_BUFFER_SIZE = 120
VERSION_MAX_SIZE = 4

OPTION_CLASS_SIZE = 100
GRAPH_CLASS0_BASE = 1000
DEVICE_CLASS0_BASE = 2000
SUPPORT_HIGHCLASS = False


class EnumDeprecationHelper(object):
    def __init__(self, new_target, deprecated_values):
        self.new_target = new_target
        self.deprecated_values = deprecated_values

    def __call__(self, *args, **kwargs):
        return self.new_target(*args, **kwargs)

    def __getattr__(self, attr):
        if (attr in self.deprecated_values):
            warnings.warn('\033[93m' + "\"" + attr +
                          "\" is deprecated. Please use \"" +
                          self.deprecated_values[attr] + "\"!" + '\033[0m',
                          DeprecationWarning, stacklevel=2)
            return getattr(self.new_target, self.deprecated_values[attr])
        return getattr(self.new_target, attr)


"""Enum declarations"""


class Status(Enum):
    OK = 0
    BUSY = -1                               # Device is busy, retry later
    ERROR = -2                              # Error communicating with the device
    OUT_OF_MEMORY = -3                      # Out of memory
    DEVICE_NOT_FOUND = -4                   # No device at the given index or name
    INVALID_PARAMETERS = -5                 # At least one of the given parameters is wrong
    TIMEOUT = -6                            # Timeout in the communication with the device
    MVCMD_NOT_FOUND = -7                    # The file to boot Myriad was not found
    NOT_ALLOCATED = -8                      # The graph or device has been closed during the operation
    UNAUTHORIZED = -9
    UNSUPPORTED_GRAPH_FILE = -10            # The graph file version is not supported
    UNSUPPORTED_CONFIGURATION_FILE = -11    # The configuration file version is not supported
    UNSUPPORTED_FEATURE = -12               # Not supported by this FW version
    MYRIAD_ERROR = -13                      # An error has been reported by the device, use NC_DEVICE_DEBUG_INFO or NC_GRAPH_DEBUG_INFO
    INVALID_DATA_LENGTH = -14               # invalid data length has been passed when reading an array/buffer option
    INVALID_HANDLE = -15                    # handle to object that is invalid

class LogLevel(Enum):
    DEBUG = 0        # debug and above (full verbosity)
    INFO = 1         # info and above
    WARN = 2         # warnings and above
    ERROR = 3        # errors and above
    FATAL = 4        # fatal only

class GlobalOption(Enum):
    RW_LOG_LEVEL = 0       # Log level, int, default is WARN
    RO_API_VERSION = 1     # returns api version. array of unsigned int of size 4
                           # major.minor.hotfix.rc


class GraphOption(Enum):
    RO_GRAPH_STATE = 1000                  # returns graph state; CREATED, ALLOCATED, WAITING_FOR_BUFFERS, RUNNING, DESTROYED
    RO_TIME_TAKEN = 1001                   # Return time taken for last inference (float *)
    RO_INPUT_COUNT = 1002                  # Returns number of inputs, size of array returned by NC_RO_INPUT_TENSOR_DESCRIPTORS, int
    RO_OUTPUT_COUNT = 1003                 # Returns number of outputs, size of array returned by NC_RO_OUTPUT_TENSOR_DESCRIPTORS, int
    RO_INPUT_TENSOR_DESCRIPTORS = 1004     # Return a tensorDescriptor pointer array which describes the graph inputs in order.
                                           # Can be used for Buffer creation. The length of the array can be retrieved using the INPUT_COUNT option
    RO_OUTPUT_TENSOR_DESCRIPTORS = 1005    # Return a tensorDescriptor pointer array which describes the graph outputs in order.
                                           # Can be used for Buffer creation. The length of the array can be retrieved using the OUTPUT_COUNT option
    RO_DEBUG_INFO = 1006                   # Return debug info, string
    RO_GRAPH_NAME = 1007                   # Returns/sets name of the graph, string
    RO_CLASS_LIMIT = 1008                  # return the highest option class supported
    RO_GRAPH_VERSION = 1009                # returns graph version, string
    RO_TIME_TAKEN_ARRAY_SIZE = 1011        # Return size of array for time taken option, int
    RW_EXECUTORS_NUM = 1110


class DeviceState(Enum):
    CREATED = 0
    OPENED = 1
    CLOSED = 2


class GraphState(Enum):
    CREATED = 0
    ALLOCATED = 1
    WAITING_FOR_BUFFERS = 2
    RUNNING = 3


class FifoState(Enum):
    CREATED = 0
    ALLOCATED = 1


class DeviceOption(Enum):
    RO_THERMAL_STATS = 2000                # Return temperatures, float *, not for general use
    RO_THERMAL_THROTTLING_LEVEL = 2001     # 1=TEMP_LIM_LOWER reached, 2=TEMP_LIM_HIGHER reached
    RO_DEVICE_STATE = 2002                 # CREATED, OPENED, CLOSED, DESTROYED
    RO_CURRENT_MEMORY_USED = 2003          # Returns current device memory usage
    RO_MEMORY_SIZE = 2004                  # Returns device memory size
    RO_MAX_FIFO_NUM = 2005                 # return the maximum number of fifos supported
    RO_ALLOCATED_FIFO_NUM = 2006           # return the number of currently allocated fifos
    RO_MAX_GRAPH_NUM = 2007                # return the maximum number of graphs supported
    RO_ALLOCATED_GRAPH_NUM = 2008          # return the number of currently allocated graphs
    RO_CLASS_LIMIT = 2009                  # return the highest option class supported
    RO_FW_VERSION = 2010                   # return device firmware version, array of unsigned int, size 4
                                           # major.minor.hw.build number
    RO_DEBUG_INFO = 2011                   # Return debug info, string, not supported yet
    RO_MVTENSOR_VERSION = 2012             # returns mv tensor version, array of unsigned int, size 2
                                           # major.minor
    RO_DEVICE_NAME = 2013                  # returns device name, string
    RO_MAX_EXECUTORS_NUM = 2014            # Maximum number of executers per graph
    RO_HW_VERSION = 2015                   # returns HW Version, enum

class DeviceHwVersion(Enum):
    MA2450 = 0
    MA2480 = 1


class FifoType(Enum):
    HOST_RO = 0             # fifo can be read through the API but can not be written (graphs can read and write data)
    HOST_WO = 1             # fifo can be written through the API but can not be read (graphs can read but can not write)


class FifoDataType(Enum):
    FP16 = 0
    FP32 = 1

class TensorDescriptor(Structure):
    _fields_ = [('n', c_uint),  # batch size, currently only 1 is supported
                ('c', c_uint),  # number of channels
                ('w', c_uint),  # width
                ('h', c_uint),  # height
                ('totalSize', c_uint),  # total size of the data in tensor = largest stride* dim size
                ('cStride', c_uint),    # stride in the channels' dimension
                ('wStride', c_uint),    # stride in the horizontal dimension
                ('hStride', c_uint),    # stride in the vertical dimension
                ('dataType', c_uint)]   # data type of the tensor, FP32 or FP16


class FifoOption(Enum):
    RW_TYPE = 0                # configure the fifo type to one type from ncFifoType_t
    RW_CONSUMER_COUNT = 1      # The number of consumers of elements (the number of times data must be read by a graph or host before the element is removed. Defaults to 1. Host can read only once always.
    RW_DATA_TYPE = 2           # 0 for fp16, 1 for fp32. If configured to fp32, the API will convert the data to the internal fp16 format automatically
    RW_DONT_BLOCK = 3          # WriteTensor will return NC_OUT_OF_MEMORY instead of blocking, GetResult will return NO_DATA, not supported yet
    RO_CAPACITY = 4            # return number of maximum elements in the fifo
    RO_READ_FILL_LEVEL = 5     # return number of tensors in the buffer
    RO_WRITE_FILL_LEVEL = 6    # return number of tensors in the buffer
    RO_GRAPH_TENSOR_DESCRIPTOR = 7   # return the tensor descriptor of the FIFO
    RO_TENSOR_DESCRIPTOR = RO_GRAPH_TENSOR_DESCRIPTOR # deprecated
    RO_STATE = 8               # return the fifo state; CREATED, ALLOCATED, DESTROYED
    RO_NAME = 9                # returns fifo name
    RO_ELEMENT_DATA_SIZE = 10  # element data size in bytes, int
    RW_HOST_TENSOR_DESCRIPTOR = 11  # App's tensor descriptor, defaults to none strided channel minor


def enumerate_devices():
    """
    :return array: list of Device handles
    """
    i = 0
    devices = []
    while True:
        handle = c_void_p()
        if f.ncDeviceCreate(i, byref(handle)) != Status.OK.value:
            break
        devices.append(handle)
        i = i + 1
    return devices


def global_set_option(option, value):
    """
    :param option: a GlobalOption enumeration
    :param value: value for the option
    """
    if option == GlobalOption.RW_LOG_LEVEL and (isinstance(value, int) == False):
        data = c_int(value.value)
    else:
        data = c_int(value)
    status = f.ncGlobalSetOption(option.value, pointer(data), sizeof(data))
    if status != Status.OK.value:
        raise Exception(Status(status))


def global_get_option(option):
    """
    :param option: a GlobalOption enumeration
    :return option: value for the option
    """
    # Determine option data type
    if option == GlobalOption.RW_LOG_LEVEL:
        optdata = c_uint()

        def get_optval(raw_optdata): return raw_optdata.value
    elif option == GlobalOption.RO_API_VERSION:
        optdata = create_string_buffer(VERSION_MAX_SIZE * sizeof(c_uint))

        def get_optval(raw_optdata): return numpy.frombuffer(raw_optdata, c_uint)
    else:
        raise Exception(Status.INVALID_PARAMETERS)

    # Read global option
    optsize = c_uint(sizeof(optdata))
    status = f.ncGlobalGetOption(option.value, byref(optdata), byref(optsize))
    if status == Status.INVALID_DATA_LENGTH.value:
        # Create a buffer of the correct size and try again
        optdata = create_string_buffer(optsize.value)
        status = f.ncGlobalGetOption(
            option.value, byref(optdata), byref(optsize))
    if status != Status.OK.value:
        raise Exception(Status(status))

    # Get appropriately formatted option value and return
    return get_optval(optdata)


def getOptionClass(option, base):
    return int((option.value - base)/OPTION_CLASS_SIZE)


class Fifo:
    def __init__(self, name, fifo_type, skip_init=False):
        self.handle = c_void_p()
        self.device = None
        pName = name.encode('ascii')

        if skip_init == False:
            status = f.ncFifoCreate(pName, fifo_type.value, byref(self.handle))
            if status != Status.OK.value:
                raise Exception(Status(status))

    def allocate(self, device, tensor_desc, n_elem):
        """
        allocate a buffer. A buffer is a FIFO container, which based on type allows some of the following accesses:
        * read from HOST
        * read from device (graph)
        * write from HOST
        * write from device
        """
        self.device = device
        n_elem = c_uint(n_elem)
        status = f.ncFifoAllocate(
            self.handle, device.handle, byref(tensor_desc), n_elem)
        if status != Status.OK.value:
            raise Exception(Status(status))

    def destroy(self):
        """
        destroy a buffer.
        """
        status = f.ncFifoDestroy(byref(self.handle))
        self.handle = 0
        if status != Status.OK.value:
            raise Exception(Status(status))

    def set_option(self, option, value):
        """
        Modify fifo options.
        """
        if (option == FifoOption.RW_DATA_TYPE or option == FifoOption.RW_TYPE):
            data = c_int(value.value)
        elif (option != FifoOption.RW_HOST_TENSOR_DESCRIPTOR):
            data = c_int(value)

        if (option != FifoOption.RW_HOST_TENSOR_DESCRIPTOR):
            status = f.ncFifoSetOption(
                self.handle, option.value, pointer(data), sizeof(data))
        else:
            status = f.ncFifoSetOption(
                self.handle, option.value, byref(value), sizeof(value))

        if status != Status.OK.value:
            raise Exception(Status(status))

    def get_option(self, option):
        """
        Read fifo options.
        """
        # Determine option data type
        if (option == FifoOption.RW_TYPE or
                option == FifoOption.RW_CONSUMER_COUNT or
                option == FifoOption.RW_DATA_TYPE or
                option == FifoOption.RW_DONT_BLOCK or
                option == FifoOption.RO_CAPACITY or
                option == FifoOption.RO_ELEMENT_DATA_SIZE or
                option == FifoOption.RO_READ_FILL_LEVEL or
                option == FifoOption.RO_WRITE_FILL_LEVEL or
                option == FifoOption.RO_STATE):
            # int
            optdata = c_int()

            def get_optval(raw_optdata): return raw_optdata.value
        elif (option == FifoOption.RO_GRAPH_TENSOR_DESCRIPTOR or
            option == FifoOption.RO_TENSOR_DESCRIPTOR or
            option == FifoOption.RW_HOST_TENSOR_DESCRIPTOR):
            # TensorDescriptor struct
            optdata = TensorDescriptor()

            def get_optval(raw_optdata): return raw_optdata
        elif (option == FifoOption.RO_NAME):
            # string
            optdata = create_string_buffer(MAX_NAME_SIZE)

            def get_optval(raw_optdata): return raw_optdata.value.decode()
        else:
            raise Exception(Status.INVALID_PARAMETERS)

        # Read device option
        optsize = c_uint(sizeof(optdata))
        status = f.ncFifoGetOption(
            self.handle, option.value, byref(optdata), byref(optsize))
        if status == Status.INVALID_DATA_LENGTH.value:
            # Create a buffer of the correct size and try again
            optdata = create_string_buffer(optsize.value)
            status = f.ncFifoGetOption(
                self.handle, option.value, byref(optdata), byref(optsize))
        if status != Status.OK.value:
            raise Exception(Status(status))

        # Get appropriately formatted option value and return
        return get_optval(optdata)

    def write_elem(self, input_tensor, user_obj):
        """
        Send an input to a selected fifo. The fifo type must allow HOST write access to the fifo.
        """
        tensor = input_tensor.tostring()
        user_obj = py_object(user_obj)
        key = c_long(addressof(user_obj))
        self.device.userobjs[key.value] = user_obj
        input_len = c_uint(len(tensor))
        status = f.ncFifoWriteElem(self.handle, tensor, byref(input_len), key)
        if status != Status.OK.value:
            raise Exception(Status(status))

    def read_elem(self):
        """
        Receive result from a fifo. The fifo type must allow HOST read access to the fifo.
        Blocks if necessary. The output is fp32 or fp16 data, depending on the fifo configuration.
        This operation will pop (remove and return) the data in case of CONSUMER_COUNT=1.
        In case the count is bigger than one, the element will be removed only when all consumers consumed the data.
        Even if the count is bigger than one, the API can read only once.
        """

        # first read element size to allocate buffer for output
        elementsize = c_int()
        optsize = c_uint(sizeof(elementsize))
        status = f.ncFifoGetOption(
            self.handle, FifoOption.RO_ELEMENT_DATA_SIZE.value, byref(elementsize), byref(optsize))
        if status != Status.OK.value:
            raise Exception(Status(status))

        # Get datatype
        optdata = c_int()
        optsize = c_uint(sizeof(optdata))
        status = f.ncFifoGetOption(
            self.handle, FifoOption.RW_DATA_TYPE.value, byref(optdata), byref(optsize))
        if status != Status.OK.value:
            raise Exception(Status(status))
        datatype = optdata.value
        tensor = create_string_buffer(elementsize.value)
        user_obj = c_long()
        status = f.ncFifoReadElem(self.handle, byref(
            tensor), byref(elementsize), byref(user_obj))
        # if status == Status.NO_DATA.value:
        #    return None, None
        if status != Status.OK.value:
            raise Exception(Status(status))

        # Convert to numpy array
        if (datatype == FifoDataType.FP32.value):
            tensor = numpy.fromstring(tensor.raw, dtype=numpy.float32)
        else:
            tensor = numpy.fromstring(tensor.raw, dtype=numpy.float16)

        retuserobj = self.device.userobjs[user_obj.value]
        del self.device.userobjs[user_obj.value]
        return tensor, retuserobj.value

    def remove_elem(self):  # not supported yet
        """
        Remove oldest element from a buffer.
        """
        status = f.ncFifoRemoveElem(byref(self.handle))
        if status != Status.OK.value:
            raise Exception(Status(status))


class Device:
    def __init__(self, handle):
        """
        """
        self.handle = handle
        self.userobjs = {}

    def open(self):
        """
        Open function boots and initializes the device.
        """
        status = f.ncDeviceOpen(self.handle)
        if status != Status.OK.value:
            raise Exception(Status(status))

    def close(self):
        """
        Close function will reset the device and stop the communication.
        """
        status = f.ncDeviceClose(self.handle)
        if status != Status.OK.value:
            raise Exception(Status(status))

    def destroy(self):
        """
        Destroy device handler, and clears allocated memory
        """
        status = f.ncDeviceDestroy(byref(self.handle))
        self.handle = c_void_p()
        if status != Status.OK.value:
            raise Exception(Status(status))

    def set_option(self, option, value):
        """
        Set an optional feature of the device.
        :param option: a DeviceOption enumeration
        :param value: value for the option
        """
        # Handle high class
        opClass = getOptionClass(option, DEVICE_CLASS0_BASE)
        if (opClass > 1 and SUPPORT_HIGHCLASS is True):
            data = self.set_option_hc(f, Status, option, value)
        else:
            # Determine option data type
            if (option == DeviceOption.RO_THERMAL_THROTTLING_LEVEL or option == DeviceOption.RO_DEVICE_STATE or
                option == DeviceOption.RO_CURRENT_MEMORY_USED or option == DeviceOption.RO_MEMORY_SIZE or
                option == DeviceOption.RO_MAX_FIFO_NUM or option == DeviceOption.RO_ALLOCATED_FIFO_NUM or
                option == DeviceOption.RO_MAX_GRAPH_NUM or option == DeviceOption.RO_ALLOCATED_GRAPH_NUM or
                    option == DeviceOption.RO_CLASS_LIMIT or option == DeviceOption.RO_HW_VERSION):
                data = c_int(value)
            else:
                raise Exception(Status.INVALID_PARAMETERS)

        # Write device option
        status = f.ncDeviceSetOption(
            self.handle, option.value, pointer(data), sizeof(data))
        if status != Status.OK.value:
            raise Exception(Status(status))

    def get_option(self, option):
        """
        Get optional information from the device.
        :param option: a DeviceOption enumeration
        :return option: value for the option
        """
        # Handle high class
        opClass = getOptionClass(option, DEVICE_CLASS0_BASE)
        if (opClass > 1 and SUPPORT_HIGHCLASS is True):
            ret = self.get_option_hc(f, Status, option)
            return ret

        # Determine option data type
        if (option == DeviceOption.RO_THERMAL_THROTTLING_LEVEL or
                option == DeviceOption.RO_DEVICE_STATE or
                option == DeviceOption.RO_CURRENT_MEMORY_USED or
                option == DeviceOption.RO_MEMORY_SIZE or
                option == DeviceOption.RO_MAX_FIFO_NUM or
                option == DeviceOption.RO_ALLOCATED_FIFO_NUM or
                option == DeviceOption.RO_MAX_GRAPH_NUM or
                option == DeviceOption.RO_ALLOCATED_GRAPH_NUM or
                option == DeviceOption.RO_CLASS_LIMIT or
                option == DeviceOption.RO_MAX_EXECUTORS_NUM or
                option == DeviceOption.RO_HW_VERSION):
            # int
            optdata = c_int()

            def get_optval(raw_optdata): return raw_optdata.value
        elif (option == DeviceOption.RO_THERMAL_STATS):
            # float array
            optdata = create_string_buffer(THERMAL_BUFFER_SIZE)

            def get_optval(raw_optdata):
                return numpy.frombuffer(raw_optdata, c_float)
        elif (option == DeviceOption.RO_FW_VERSION or
              option == DeviceOption.RO_MVTENSOR_VERSION):
            # unsigned int array
            optdata = create_string_buffer(VERSION_MAX_SIZE * sizeof(c_uint))

            def get_optval(raw_optdata):
                return numpy.frombuffer(raw_optdata, c_uint)
        elif option == DeviceOption.RO_DEBUG_INFO:
            # string
            optdata = create_string_buffer(DEBUG_BUFFER_SIZE)

            def get_optval(raw_optdata): return raw_optdata.value.decode()
        elif option == DeviceOption.RO_DEVICE_NAME:
            # string
            optdata = create_string_buffer(MAX_NAME_SIZE)

            def get_optval(raw_optdata): return raw_optdata.value.decode()
        else:
            raise Exception(Status.INVALID_PARAMETERS)

        # Read device option
        optsize = c_uint(sizeof(optdata))
        status = f.ncDeviceGetOption(
            self.handle, option.value, byref(optdata), byref(optsize))
        if status == Status.INVALID_DATA_LENGTH.value:
            # Create a buffer of the correct size and try again
            optdata = create_string_buffer(optsize.value)
            status = f.ncDeviceGetOption(
                self.handle, option.value, byref(optdata), byref(optsize))
        if status != Status.OK.value:
            raise Exception(Status(status))

        # Get appropriately formatted option value and return
        return get_optval(optdata)


class Graph:
    def __init__(self, name):
        self.handle = c_void_p()
        self.device = None
        pName = name.encode('ascii')

        """
        Initalize a new graph.
        This function will not send anything to the device,
        all parameters can be configured after calling this function.
        Takes a name as input.  Returns a handle to the graph.
        """
        status = f.ncGraphCreate(pName, byref(self.handle))
        if status != Status.OK.value:
            raise Exception(Status(status))

    def allocate(self, device, graph_buffer):
        """
        Allocate a graph on the device. When calling this function, the graph is sent to the device,
        no STATIC parameters of the graph can be configured after this function call.
        """
        self.device = device
        status = f.ncGraphAllocate(device.handle, self.handle, graph_buffer, len(graph_buffer))
        if status != Status.OK.value:
            raise Exception(Status(status))

    def allocate_with_fifos(self, device, graph_buffer,
                                  input_fifo_type=FifoType.HOST_WO, input_fifo_num_elem=2,
                                  input_fifo_data_type=FifoDataType.FP32,
                                  output_fifo_type=FifoType.HOST_RO, output_fifo_num_elem=2,
                                  output_fifo_data_type=FifoDataType.FP32):
        """
        Convenience function to initialize & allocate graph with Input & Output fifos
        """
        input_fifo = Fifo("", None, skip_init=True)
        output_fifo = Fifo("", None, skip_init=True)
        input_fifo.device = device
        output_fifo.device = device
        in_num_elem = c_uint(input_fifo_num_elem)
        out_num_elem = c_uint(output_fifo_num_elem)
        self.device = device

        status = f.ncGraphAllocateWithFifosEx(device.handle, self.handle,
                                              graph_buffer, len(graph_buffer),
                                              byref(input_fifo.handle),
                                              input_fifo_type.value, in_num_elem,
                                              input_fifo_data_type.value,
                                              byref(output_fifo.handle),
                                              output_fifo_type.value, out_num_elem,
                                              output_fifo_data_type.value)
        if status != Status.OK.value:
            raise Exception(Status(status))

        return input_fifo, output_fifo

    def set_option(self, option, value):
        """
        Set an optional feature of the graph.
        :param option: a GraphOption enumeration
        :param value: value for the option
        """
        # Write device option
        data = c_int(value)
        status = f.ncGraphSetOption(
            self.handle, option.value, pointer(data), sizeof(data))
        if status != Status.OK.value:
            raise Exception(Status(status))

    def get_option(self, option):
        """
        Get optional information from the graph.
        :param option: a GraphOption enumeration
        :return option: value for the option
        """
        # Handle high class
        opClass = getOptionClass(option, GRAPH_CLASS0_BASE)
        if (opClass > 1 and SUPPORT_HIGHCLASS is True):
            ret = self.get_option_hc(f, Status, option)
            return ret

        # Determine option data type
        if (option == GraphOption.RO_GRAPH_STATE or
                option == GraphOption.RO_INPUT_COUNT or
                option == GraphOption.RO_OUTPUT_COUNT or
                option == GraphOption.RO_CLASS_LIMIT or
                option == GraphOption.RO_TIME_TAKEN_ARRAY_SIZE or
                option == GraphOption.RW_EXECUTORS_NUM):
            # int
            optdata = c_int()

            def get_optval(raw_optdata): return raw_optdata.value
        elif (option == GraphOption.RO_TIME_TAKEN):
            # float array
            arraysize = c_int()
            optsize = c_uint(sizeof(arraysize))
            status = f.ncGraphGetOption(
                self.handle, GraphOption.RO_TIME_TAKEN_ARRAY_SIZE.value, byref(arraysize), byref(optsize))
            if status != Status.OK.value:
                raise Exception(Status(status))
            optdata = create_string_buffer(arraysize.value)

            def get_optval(raw_optdata):
                return numpy.frombuffer(raw_optdata, c_float)
        elif (option == GraphOption.RO_GRAPH_VERSION):
            # unsigned int array
            optdata = create_string_buffer(VERSION_MAX_SIZE * sizeof(c_uint))

            def get_optval(raw_optdata):
                return numpy.frombuffer(raw_optdata, c_uint)
        elif option == GraphOption.RO_DEBUG_INFO:
            # string
            optdata = create_string_buffer(DEBUG_BUFFER_SIZE)

            def get_optval(raw_optdata): return raw_optdata.value.decode()
        elif option == GraphOption.RO_GRAPH_NAME:
            # string
            optdata = create_string_buffer(MAX_NAME_SIZE)

            def get_optval(raw_optdata): return raw_optdata.value.decode()
        elif option == GraphOption.RO_INPUT_TENSOR_DESCRIPTORS:
            # list of TensorDescriptor structs

            # Get the number of input TensorDescriptors
            desc_count = c_int()
            optsize = c_uint(sizeof(desc_count))
            status = f.ncGraphGetOption(
                self.handle, GraphOption.RO_INPUT_COUNT.value, byref(desc_count), byref(optsize))
            if status != Status.OK.value:
                raise Exception(Status(status))

            # Create an appropriately sized buffer for getting the TensorDescriptor data
            optdata = create_string_buffer(
                desc_count.value * sizeof(TensorDescriptor))

            def get_optval(raw_optdata):
                # Split the ctypes char array into a list of TensorDescriptors and return the list
                desc_list = []
                struct_size = sizeof(TensorDescriptor)
                for i in range(0, desc_count.value * struct_size, struct_size):
                    desc_bytes = raw_optdata[i:i+struct_size]
                    desc_list.append(TensorDescriptor.from_buffer_copy(desc_bytes))
                return desc_list
        elif option == GraphOption.RO_OUTPUT_TENSOR_DESCRIPTORS:
            # list of TensorDescriptor structs

            # Get the number of output TensorDescriptors
            desc_count = c_int()
            optsize = c_uint(sizeof(desc_count))
            status = f.ncGraphGetOption(
                self.handle, GraphOption.RO_OUTPUT_COUNT.value, byref(desc_count), byref(optsize))
            if status != Status.OK.value:
                raise Exception(Status(status))

            # Create an appropriately sized buffer for getting the TensorDescriptor data
            optdata = create_string_buffer(
                desc_count.value * sizeof(TensorDescriptor))

            def get_optval(raw_optdata):
                # Split the ctypes char array into a list of TensorDescriptors and return the list
                desc_list = []
                struct_size = sizeof(TensorDescriptor)
                for i in range(0, desc_count.value * struct_size, struct_size):
                    desc_bytes = raw_optdata[i:i+struct_size]
                    desc_list.append(TensorDescriptor.from_buffer_copy(desc_bytes))
                return desc_list
        else:
            raise Exception(Status.INVALID_PARAMETERS)

        # Read device option
        optsize = c_uint(sizeof(optdata))
        status = f.ncGraphGetOption(
            self.handle, option.value, byref(optdata), byref(optsize))
        if status == Status.INVALID_DATA_LENGTH.value:
            # Create a buffer of the correct size and try again
            optdata = create_string_buffer(optsize.value)
            status = f.ncGraphGetOption(
                self.handle, option.value, byref(optdata), byref(optsize))
        if status != Status.OK.value:
            raise Exception(Status(status))

        # Get appropriately formatted option value and return
        return get_optval(optdata)

    def queue_inference(self, input_fifo, output_fifo):
        """
        Trigger graph execution with specified inputs and outputs.
        """
        status = f.ncGraphQueueInference(self.handle, byref(input_fifo.handle),
                                         1, byref(output_fifo.handle), 1)
        if status != Status.OK.value:
            raise Exception(Status(status))

    def queue_inference_with_fifo_elem(self, input_fifo, output_fifo, input_tensor, user_obj):
        """
        Trigger graph execution with specified inputs and outputs.
        """
        tensor = input_tensor.tostring()
        user_obj = py_object(user_obj)
        key = c_long(addressof(user_obj))
        input_fifo.device.userobjs[key.value] = user_obj
        input_len = c_uint(len(tensor))
        status = f.ncGraphQueueInferenceWithFifoElem(self.handle, input_fifo.handle,
                                                     output_fifo.handle, tensor,
                                                     byref(input_len), key)
        if status != Status.OK.value:
            raise Exception(Status(status))

    def destroy(self):
        """
        destroy a graph on the device.
        """
        status = f.ncGraphDestroy(byref(self.handle))
        self.handle = 0
        if status != Status.OK.value:
            raise Exception(Status(status))


# Add highclass methods
try:
    from mvnc.highclass import *
    Device.set_option_hc = device_set_option_hc
    Device.get_option_hc = device_get_option_hc
    Graph.get_option_hc = graph_get_option_hc
    SUPPORT_HIGHCLASS = True
except:
    pass
