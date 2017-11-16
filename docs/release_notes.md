============================================================
# Movidius Neural Compute SDK Release Notes
# V1.10.01 2017-11-15
============================================================

As of V1.09.00, SDK has been refactored and contains many new features and structural changes. It is recommended you read the documentation to familiarize with the new features and contents. Please see v1.09.00 release notes, using github tag https://github.com/movidius/ncsdk/tree/v1.09.00.06

## SDK Notes: 
### New features:
#### Networks:
1. VGG 16 for both Caffe and Tensorflow
2. Resnet 18 for Caffe
3. Resnet 50 for Caffe
   
## API Notes:
1. No change

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
8. VGG 16
9. Resnet 18
10. Resnet 50

### Tensorflow r1.3
1. inception-v1
2. inception-v2
3. inception-v3
4. inception-v4
5. Inception ResNet v2
6. VGG 16
7. Mobilenet_V1_1.0 variants: 
   - MobileNet_v1_1.0_224
   - MobileNet_v1_1.0_192
   - MobileNet_v1_1.0_160
   - MobileNet_v1_1.0_128
   - MobileNet_v1_0.75_224
   - MobileNet_v1_0.75_192
   - MobileNet_v1_0.75_160
   - MobileNet_v1_0.75_128
   - MobileNet_v1_0.5_224
   - MobileNet_v1_0.5_192
   - MobileNet_v1_0.5_160
   - MobileNet_v1_0.5_128
   - MobileNet_v1_0.25_224
   - MobileNet_v1_0.25_192
   - MobileNet_v1_0.25_160
   - MobileNet_v1_0.25_128

## Firmware Features:
1. Convolutions
  - NxN Convolution with Stride S.
  - The following cases have been extensively tested: 1x1s1,3x3s1,5x5s1,7x7s1, 7x7s2, 7x7s4
  - Group convolution
  - Depth Convolution
  - Dilated convolution
2. Max Pooling Radix NxM with Stride S
3. Average Pooling: Radix NxM with Stride S, Global average pooling
4. Local Response Normalization
5. Relu, Relu-X, Prelu (see erattum #10)
6. Softmax
7. Sigmoid
8. Tanh (see erratum #10)
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
19. Batch Normalization
 

## Bug Fixes:
     Fixed: Installation does not succeed as root user
     Fixed: Support for larger convolutions, enabling VGG 16 support
     Fixed: Issues blocking Resnet 18 and Resnet 50
     
## Errata:
1. Python 2.7 is fully supported for making user applications, but only the helloworld_py example runs as-is in both python 2.7 and 3.5 due to dependencies on modules.
2. SDK tools for tensorflow on Rasbpian Stretch are not supported for this release, due to lack of an integrated tensorflow installer for Rasbpian in the SDK. TF examples are provided with pre-compiled graph files to allow them to run on Rasperry Pi, however the compile, profile, and check functions will not be available on Raspberry Pi, and 'make examples' will generate failures for the tensorflow examples on Raspberry Pi.
3. Depth-wise convolution may not be supported if channel multiplier > 1.
4. If working behind proxy, proper proxy settings must be applied for the installer to succeed. 
5. Although improved, the installer is known to take a long time on Raspberry Pi. Date/time must be correct for SDK installation to succeed on Raspberry Pi.
6. Default system virtual memory swap file size is too small to compile AlexNet on Raspberry Pi.  VGG 16 not verified to compile on Pi.
7. Raspberry Pi users will need to upgrade to Raspbian Stretch for releases after 1.09.
8. Convolution may fail to find a solution for very large inputs.
9. Depth convolution is tested for 3x3 kernels.
10. A TanH layer’s “top” & “bottom” blobs must have different names.  This is different from a ReLU layer, whose “top” & “bottom” should be named the same as its previous layer.
