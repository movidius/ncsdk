FROM ubuntu:16.04

# Install necessary packages for the installer
RUN apt-get update && apt-get install -y \
    build-essential \
    sed \
    sudo \
    tar \
    udev \
    wget \
    && apt-get clean

# Copy over the NCSDK
COPY ./ ncsdk/

# Set the current working directory to the cloned ncsdk directory
WORKDIR "/ncsdk"

# Run the installer
RUN make install
