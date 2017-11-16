# mvncStatus enumeration

Info         | Value/s
------------ | -------------
Header       |mvnc.h
Version      |1.0
See also     | 

## Overview
A value from this enumeration is returned from most of the C API functions.  The table below provides details on the meaning of each of the values in the enumeration.

constant | description
-------- | -----------
MVNC_OK | The function call worked as expected.
MVNC_BUSY | The device is busy, retry later.
MVNC_ERROR | An unexpected error was encountered during the function call.
MVNC_OUT_OF_MEMORY | The host is out of memory.
MVNC_DEVICE_NOT_FOUND | There is no device at the given index or name.
MVNC_INVALID_PARAMETERS | At least one of the given parameters is invalid in the context of the function call.
MVNC_TIMEOUT | Timeout in the communication with the device.
MVNC_MVCMD_NOT_FOUND | The file named MvNCAPI.mvcmd should be installed in the mvnc directory. This message may mean that the installation failed.
MVNC_NO_DATA | No data to return.
MVNC_GONE | The graph or device has been closed during the operation.
MVNC_UNSUPPORTED_GRAPH_FILE | The graph file may have been created with an incompatible prior version of the Toolkit. Try to recompile the graph file with the version of the Toolkit that corresponds to the API version.
MVNC_MYRIAD_ERROR | An error has been reported by Intel® Movidius™ VPU. Use MVNC_DEBUGINFO to get more information.
