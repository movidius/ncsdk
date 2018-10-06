# Introduction

This Dockerfile can be used to easily build a Docker image that has the Intel® Movidius™ Neural Compute SDK installed on Ubuntu 16.04.

## Build the image

Use the following command to build the image:

$ docker build -t ncsdk -f ./extras/docker/Dockerfile https://github.com/movidius/ncsdk.git#ncsdk2

If you've already cloned the NCSDK, you can execute the following command from within the ncsdk directory instead:

$ docker build -t ncsdk -f ./extras/docker/Dockerfile .

This creates an image named "ncsdk".

*Note: If you are running Docker behind a proxy you must configure proxy settings for Docker before you can build the image. See the NCSDK Docker documentation for more information.*

## Run the container

1. To boot devices with runtime FW compile and run the app ./ncs_boot_devices/
2. run the command ./docker_cmd.sh 

## Additional Information

See the full NCSDK documentation for Docker at https://movidius.github.io/ncsdk/ for more information.
