///
/// @file      pcie_host.h
/// @copyright All code copyright Intel Corporation 2018, all rights reserved.
///            For License Warranty see: common/license.txt
///
#ifndef PCIE_HOST_H
#define PCIE_HOST_H

#include "XLinkPlatform.h"

int pcie_init(const char *slot, int *fd);
int pcie_write(int fd, void * buf, size_t bufSize);
int pcie_read(int fd, void *buf, size_t bufSize);
int pcie_close(int fd);
xLinkPlatformErrorCode_t pcie_find_device_port(int index, char* port_name, int size);

#endif
