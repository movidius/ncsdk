# Virtual Machine Configurations

The following configuration has been tested with the 1.09 SDK release

## General Requirements
- Virtualbox 5.1.28 (later releases should be fine but not tested)
- Guest Extensions installed
- You will need to select usb3.0 and create two filters: 
  - USB2 filter with vendor ID 03e7 and product ID 2150
  - USB3 filter with vendor ID 040e and product ID f63b
- Host OS (these have been tested, other may work):
  - OSX Yosemite 10.10.5
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
- Insert NCS device to USB port
- Install examples with 'make examples' if doesn’t work re-insert key and try again

## Notes
- During operation applications will need 2s delay between close and re-openign NCS device
- VM RAM needs to be 2GB or caffe compile will likely fail
