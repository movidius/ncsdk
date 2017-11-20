# Virtual Machine Configurations

The following configuration has been tested with the 1.09 SDK release.

## General Requirements
- Virtualbox 5.1.28 (later releases should be fine, but have not been tested)
- Guest Extensions installed
- You will need to select USB 3.0 and create two filters: 
  - USB2 filter with vendor ID 03e7
  - USB3 filter with vendor ID 040e
- Host OS (these have been tested, other may work):
  - OS X Yosemite 10.10.5
  - Windows 10 Enterprise
  - Ubuntu 16.04 Desktop
- Guest OS: 
  - Ubuntu 16.04 Desktop

## Installation Order
- Install Ubuntu 16.04 on VM
- Install updates (apt-get update, apt-get upgrade)
- Install guest extensions (virtualbox menu devices / Insert guest additions CD image)
- Setup USB filters
- Install NCSDK with 'make install' ([Installation Instructions](install.md))
- Insert Intel® Movidius™ NCS device to USB port
- Install examples with 'make examples'; if it doesn’t work, re-insert key and try again

## Notes
- During operation applications, will need 2s delay between close and re-opening NCS device
- VM RAM needs to be 2 GB, or caffe compile will likely fail
