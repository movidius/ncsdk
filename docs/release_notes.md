============================================================
# Movidius Neural Compute SDK Release Notes
# V1.09.00 2017-10-10
============================================================

## SDK Notes: 
SDK has been refactored and contains many new features and structural changes. It is recommended you read the documentation to familiarize with the new features and contents.  A partial list of new features:

1. New, unified, faster installer and uninstaller.
2. Now supports complete SDK installation on Raspberry Pi.
3. System installation of tools and API libraries.
4. API support for Python 2.7.
5. Source code included for API, for porting to other architectures or Linux distributions.
6. Tools support for Raspberry Pi.
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
4. Inception ResNet v2
5. Mobilenet_V1_1.0_224 (preview -- see erratum #3.)

## Firmware Features:
1. Convolutions
  - NxN Convolution with Stride S.
  - The following cases have been extensively tested: 1x1s1,3x3s1,5x5s1,7x7s1, 7x7s2, 7x7s4
  - Group convolution
  - Depth Convolution (limited support -- see erratum #10.)
2. Max Pooling Radix NxM with Stride S
3. Average Pooling Radix NxM with Stride S
4. Local Response Normalization
5. Relu, Relu-X, Prelu
6. Softmax
7. Sigmoid
8. Tanh
9. Deconvolution
10. Slice
11. Scale
12. ElmWise unit : supported operations - sum, prod, max 
13. Fully Connected Layers (limited support -- see erratum #8)
14. Reshape
15. Flatten
16. Power
17. Crop
18. ELU

## Bug Fixes:
1. USB protocol bug fixes, for expanded compatibility with hubs and hosts. In particular, fix for devices with maxpacket of 64.
2. Fixed -- when a graph execution fails, the result for a previous execution is erroneously returned.
     
## Errata:
1. Python 2.7 is fully supported for making user applications, but only the helloworld_py example runs as-is in both python 2.7 and 3.5 due to dependencies on modules.
2. SDK tools for tensorflow on Rasbpian Stretch are not supported for this release, due to lack of an integrated tensorflow installer for Rasbpian in the SDK. TF examples are provided with pre-compiled graph files to allow them to run on Rasperry Pi, however the compile, profile, and check functions will not be available on Raspberry Pi, and 'make examples' will generate failures for the tensorflow examples on Raspberry Pi.
3. Depth-wise convolution is not optimized, leading to low performance of Mobilenet, and does not support channel multiplier >1.
4. If working behind proxy, proper proxy settings must be applied for the installer to succeed. 
5. Although improved, the installer is known to take a long time on Raspberry Pi. Date/time must be correct for SDK installation to succeed on Raspberry Pi.
6. Default system virtual memory swap file size is too small to compile AlexNet on Raspberry Pi.
7. Raspberry Pi users will need to upgrade to Raspbian Stretch for this release.
8. Fully Connected Layers may produce erroneous results if input size is not a multiple of 8.
9. Convolution may fail to find a solution for very large inputs.
10. Depth convolution is tested for 3x3 kernels.
11. TensorFlow-like padding not correctly supported in some convolution cases, such as when stride=2 and even input size for 3x3 convolution.
