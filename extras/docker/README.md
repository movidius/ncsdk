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

After the image is built, execute the following command to run the container that we named "ncsdk":

$ docker run --net=host --privileged -v /dev:/dev --name ncsdk -i -t ncsdk /bin/bash

## Additional Information

See the full NCSDK documentation for Docker at https://movidius.github.io/ncsdk/ for more information.