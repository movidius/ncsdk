# Status Enumeration Class

|Info      | Value         |
|----------|---------------|
|Package   |  mvnc         |
|Module    |  mvncapi      |
|Version   |  1.0          |
|See also  |               |


## Overview
The Status class is an enumeration class that defines status codes returned from calls to the API functions. If the underlying API returns a non-zero status, an exception is usually raised with the corresponding status. The possible status codes are shown below. 


Enumeration Values|Description
------------ | -------------
MVNC_OK |The function call worked as expected.
MVNC_BUSY |The device is busy, retry later.
MVNC_ERROR |An unexpected error was encountered during the function call.
MVNC_OUT_OF_MEMORY |The host is out of memory.
MVNC_DEVICE_NOT_FOUND |There is no device at the given index or name.
MVNC_INVALID_PARAMETERS |At least one of the given parameters is invalid in the context of the function call.
MVNC_TIMEOUT |Timeout in the communication with the device
MVNC_MVCMD_NOT_FOUND |The file named MvNCAPI.mvcmd is installed in the mvnc directory. This message means that the file has been moved or installer failed.
MVNC_NO_DATA |No data to return.
MVNC_GONE |The graph or device has been closed during the operation.
MVNC_UNSUPPORTED_GRAPH_FILE |The graph file is corrupt or may have been created with an incompatible prior version of the NCS toolkit. Try to recompile the graph file with the version of the toolkit that corresponds to the API version.
MVNC_MYRIAD_ERROR |An error has been reported by the Intel® Movidius™ VPU. Use MVNC_DEBUGINFO.

