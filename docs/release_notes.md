============================================================
# Movidius Neural Compute SDK Release Notes
# V1.09.00 2017-10-09
============================================================

## SDK Notes: 
SDK has been refactored and contains many new features and structural changes. It is recommended you read the documentation to familiarize with the new features and contents.  A partial list of new features:

1. New, unified, faster installer and uninstaller.
2. Now supports complete SDK installation on Raspberry Pi
3. System installation of tools and API libraries.
4. API support for Python 2.7.
5. Source code included for API, for porting to other architectures or Linux distributions
6. Tools support for Pi.
7. Tensorflow R1.3 support for tools (only on Ubuntu 16.04 LTS currently).
8. More network support, see documentation for details!
9. Support for SDK on Ubuntu 16.04 LTS as guest OS, and Win10, OSX, and Ubuntu 16.04 as host OS. See docs/VirtualMachineConfig.

## API Notes:
1. API supported on both python 2.7 and python 3.5.
2. Some APIs deprecated, will emit the "deprecated" warning if used. Users expected to move to using new APIs for these functions.

## Network Notes:
Support for the following networks has been tested.

### Caffe
1. GoogleNet V1 
2. SqueezeNet V1.1 
3. LeNet 
4. CaffeNet 
5. VGG (Sousmith VGG_A)
6. Alexnet
7. TinyYolo v1


### Tensorflow r1.3
1. inception-v1
2. inception-v3
3. inception-v4
4. Inception ResNet v1 (FaceNet) 
5. Inception ResNet v2  
6. Mobilenet_V1_1.0_224

## Bug Fixes:
1. USB protocol bug fixes, for expanded compatibility with hubs and hosts.  Fix for devices with maxpacket of 64.
2. When a graph execution fails, the result for a previous execution is erroneously returned.
     
## Errata:
1. Python 2.7 is fully supported for making your own applications, but only the helloworld_py example runs as-is in both python 2.7 and 3.5 due to dependencies on modules
2. SDK tools for tensorflow on Rasbpian Stretch are not supported for this release, due to lack of an integrated tensorflow installer for Rasbpian in the SDK. TF examples are provided with pre-compiled graph files to allow them to run on Pi, however the compile, profile, and check functions will not be available on Pi, and 'make examples' will generate failures for the tensorflow examples on Rasbpian.
3. mobilenet is not optimized. Support for this network should be considered a preview.
4. Restriction on fully connected layers -- input dimension must be a multiple of 16.
5. If working behind proxy, proper proxy settings must be applied for the installer to succeed. 
6. Although improved, the Installer is known to take a long time on Raspberry Pi. Date/time must be correct for SDK installation to succeed on Raspbian.
7. Default system virtual memory swap file size is too small to compile AlexNet on Raspberry Pi.
8. Raspbian users will need to upgrade to Raspbian Stretch for this release.
