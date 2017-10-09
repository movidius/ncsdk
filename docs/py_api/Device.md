# Device class

The Device class represents the NCS device.  Typically one instance of this class is created for each physical NCS device that is plugged into the system so multiple instances may exist if you have multiple devices attached ot your system.

# Usage
To use the Device class you must create and initialize it by name.  The valid names to use can be determined by calling the mvncapi module function EnumerateDevices().  Once you have successfully created an instance of this class the typical usage is to call OpenDevice(), AllocateGraph(), use the graph, CloseDevice(). 

# Device methods
## [\_\_init\_\_](Device.__init__.md)
## [OpenDevice](Device.OpenDevice.md)
## [CloseDevice](Device.CloseDevice.md)
## [SetDeviceOption](Device.SetDeviceOption.md)
## [GetDeviceOption](Device.GetDeviceOption.md)
## [AllocateGraph](Device.AllocateGraph.md)
