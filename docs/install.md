# Installation and Configuration
This page provides installation and configuration information needed to use the NCS and the examples provided in this repository. To use the NCS you will need to have the Movidius™ Neural Compute SDK installed on your development computer. The SDK installation provides an option to install the examples in this repostitory.  If you've already installed the SDK on your development computer you may have selected the option to also install these examples.  If you have not already installed the SDK you should follow the instructions in the Example Installation with SDK section in this page, and when prompted select the option to install the examples. 

## Prerequisites
To build and run the examples in this repository you will need to have the following.
- Movidius™ Neural Compute Stick (NCS)
- Movidius™ Neural Compute SDK 
- Development Computer with Supported OS 
  - x86-64 with Ubuntu (64 bit) 16.04 Desktop 
  - Raspberry Pi 3 with Raspian Stretch (starting with SDK 1.09.xx)
    - See [Upgrade Raspian Jessie to Stretch](https://linuxconfig.org/how-to-upgrade-debian-8-jessie-to-debian-9-stretch) 
  - Virtual Machine per the [supported VM configuration](VirtualMachineConfig.md)
- Internet Connection.
- USB Camera (optional)

## Connecting the NCS to a development computer
The NCS connects to the development computer over a USB 2.0 High Speed interface. Plug the NCS directly to a USB port on your development computer or into a powered USB hub that is plugged into your development computer.

![](images/ncs_plugged.jpg)

## Installation SDK and examples
To install the SDK along with the examples in this repository use the following command on your development computer.  This is the typical installation.  If you haven't already installed the SDK on your development computer you should use this command to install.
```
git clone http://github.com/Movidius/ncsdk && cd ncsdk && make install && make examples
```

## Installation of examples without SDK
To install only the examples and not the SDK on your development computer use the following command to clone the repository and then make appropriate examples for your development computer.  If you already have the SDK installed and only need the examples on your machine you should use this command to install. 
```
git clone http://github.com/Movidius/ncsdk && cd ncsdk && make examples
```

## Building Individual Examples
Whether installing with the SDK or without it, both methods above will install and build the examples that are appropriate for your development system including prerequisite software.  Each example comes with its own Makefile that will install only that specific example and any prerequisites that it requires.  To install and build any individual example run the 'make' command from within that example's base directory.  For example to build the GoogLeNet examples type the following command.

```
cd examples/Caffe/GoogLeNet && make

```

The Makefile for each example also has a 'help' target which will display all possible targets.  To see all possible targets for any example use the following command from within the examples top directory.
```
make help

```

## Uninstallation 
To uninstall the SDK type the following command.
```
make uninstall

```


## Installation Manifest
For the list of files that 'make install' will modify on your system (outside of the repository) see the [installation manifest](manifest.md). 
