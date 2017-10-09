# Introduction
A Makefile is provided to compile profile and check all the caffe and tensorflow networks on the Neural Compute Stick using the Neural Compute SDK.  The Makefile also builds some example applications that demonstrate the usage of the compiled networks.  Each network includes python and/or C++ example code right along with the network and the apps directory contains some other example programs as well.

## make all
Executing 'make all' builds all the following networks and applications

* Caffe
  * GoogLeNet
  * AlexNet
  * SqueezeNet

* TensorFlow
  * Inception V1
  * Inception V3
  
* Applications
  * hello_ncs_py
  * hello_ncs_cpp
  * multistick_cpp

## make compile
   Compiles all the networks
## make profile
   Runs profile on all the networks
## make check
   Runs 'make check' on all the networks
## make run
   Runs the python example in all the networks
