# ncs_boot_devices: Loads and boots devices with runtime FW, without resetting them

Sample can be used as a utility to load runtime FW on devices and boot them without resetting them.
## Prerequisites

This code example requires that the following components are available:
1. Movidius Neural Compute Stick
2. Movidius Neural Compute SDK
3. MVNC_API_PATH environemnt variable is set to the source of the API


## Building the Example
To run the example code do the following :
1. Open a terminal and change directory to the ncs_boot_devices base directory
2. Type the following command in the terminal: "make ncs_boot_devices" or "make ncs_boot_devices MVNC_API_PATH=<path_to_api_src>" where <path_to_api_src> is your local copy of the api sources


## Running the Example
To run the example code do the following :
1. Open a terminal and change directory to the ncs_boot_devices base directory
2. Make sure MVNC_API_PATH env variable is set to point to the API src directory
3. Type the following command in the terminal: make run

When the application runs normally and is able to connect to the NCS device the output will be similar to this:

~~~
Hello NCS! Device 0 opened normally.
No more NCS devices found.
Successfully Booted 1 devices
~~~



