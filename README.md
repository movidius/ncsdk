# Intel® Movidius™ Neural Compute SDK
This Intel® Movidius™ Neural Compute software developer kit (NCSDK) is provided for users of the [Intel® Movidius™ Neural Compute Stick](https://developer.movidius.com/) (Intel® Movidius™ NCS). It includes software tools, an API, and examples, so developers can create software that takes advantage of the accelerated neural network capability provided by the Intel Movidius NCS hardware.

# Neural Compute SDK2
## [NCSDK v2](https://github.com/movidius/ncsdk/releases) releases are now available. 

-------
All documentation in the docs directory is now in HTML format which is **best viewed from the documentation site: https://movidius.github.io/ncsdk/** 
-------
-------
With this release the existing NCAPI v1 has been rearchitected into NCAPI v2 which will pave the way for future enhancements and capabilities, as well add some now!  While users are transitioning to this new NCAPI v2 the legacy NCSDK v1.x release will stay on the master branch and NCSDK2 will be on the [ncsdk2](https://github.com/movidius/ncsdk/tree/ncsdk2) branch.  At some point in the not too distant future, NCSDK2 will move to the master.

To help you get ready for NCSDK2 you can take a look at some of the [changes in NCAPI v2](https://movidius.github.io/ncsdk/ncapi/readme.html) as well as the [NCSDK2 Release Notes](https://movidius.github.io/ncsdk/release_notes.html).

To install NCSDK 2.x you can use the following command to clone the ncsdk2 branch
```bash 
git clone -b ncsdk2 https://github.com/movidius/ncsdk.git
```
Or if you would rather install the legacy NCSDK 1.x you can use the following command to clone as has always been the case
```bash 
git clone https://github.com/movidius/ncsdk.git
```

# Installation
The provided Makefile helps with installation. Clone this repository and then run the following command to install the NCSDK:

```
make install
```

# Examples
The Neural Compute SDK also includes examples. After cloning and running 'make install,' run the following command to install the examples:
```
make examples
```

## NCAPPZOO Examples
For additional examples, please see the Neural Compute App Zoo available at [http://www.github.com/movidius/ncappzoo](http://www.github.com/movidius/ncappzoo). The ncappzoo is a valuable resource for NCS users and includes community developed applications and neural networks for the NCS.

# Documentation
The complete Intel Movidius Neural Compute SDK documentation can be viewed at [https://movidius.github.io/ncsdk/](https://movidius.github.io/ncsdk/)

# Getting Started Video
For installation and general instructions to get started with the NCSDK, take a look at this [video](https://www.youtube.com/watch?v=fESFVNcQVVA)

# Troubleshooting and Tech Support
Be sure to check the [NCS Troubleshooting Guide](https://ncsforum.movidius.com/discussion/370/intel-ncs-troubleshooting-help-and-guidelines#latest) if you run into any issues with the NCS or NCSDK.

Also for general tech support issues the [NCS User Forum](https://developer.movidius.com/forums) is recommended and contains community discussions on many issues and resolutions.
