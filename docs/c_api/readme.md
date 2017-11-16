# Intel® Movidius™ Neural Compute SDK C API

The SDK comes with a C language API that enables developers to create applications in C or C++ that utilize hardware-accelerated Deep Neural Networks via the Intel® Movidius™ Neural Compute Stick (Intel® Movidius™ NCS). The C API is provided as a header file (mvnc.h) and an associated library file (libmvnc.so), both of which are placed on the development computer when the SDK is installed. Details of the C API are provided below and within the documentation avilable via the link in each line. 

## Enumerations
- [mvncStatus](mvncStatus.md): Contains possible return values for API functions.
- [mvncDeviceOptions](mvncDeviceOptions.md): Contains all possible options to get/set for an Intel Movidius NCS device, and their data types.
- [mvncGraphOptions](mvncGraphOptions.md): Contains all possible options to get/set for a graph, and their data types.
- [mvncGlobalOptions](mvncGlobalOptions.md): Contains all possible global options to get/set, and their data types.

## Functions
- [mvncGetDeviceName](mvncGetDeviceName.md): Retrieves the name of an Intel Movidius NCS device that can be opened.
- [mvncOpenDevice](mvncOpenDevice.md): Opens an Intel Movidius NCS device for use by the application. 
- [mvncAllocateGraph](mvncAllocateGraph.md): Allocates a graph for a specific Intel Movidius NCS device in preparation for computing inferences.
- [mvncDeallocateGraph](mvncDeallocateGraph.md): Deallocates and frees resouces associated with a graph.
- [mvncLoadTensor](mvncLoadTensor.md): Initiates an inference by providing input to the neural network.
- [mvncGetResult](mvncGetResult.md): Retrieves the result of an inference that was previously initiated.
- [mvncSetGraphOption](mvncSetGraphOption.md): Sets an option for a graph.
- [mvncGetGraphOption](mvncGetGraphOption.md): Retrieves the current value of an option for a graph.
- [mvncSetDeviceOption](mvncSetDeviceOption.md): Sets an option for an Intel Movidius NCS device.
- [mvncGetDeviceOption](mvncGetDeviceOption.md): Retrieves the current value of an option for an Intel Movidius NCS device.
- [mvncSetGlobalOption](mvncSetGlobalOption.md): Sets a global option for an application.
- [mvncGetGlobalOption](mvncGetGlobalOption.md): Retrieves the current value of a global option for an application.
- [mvncCloseDevice](mvncCloseDevice.md): Closes a previously opened Intel Movidius NCS device.

