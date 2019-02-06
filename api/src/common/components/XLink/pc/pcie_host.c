///
/// @file      pcie_host.h
/// @copyright All code copyright Intel Corporation 2018, all rights reserved.
///            For License Warranty see: common/license.txt
///
#include "XLinkPlatform.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>

#define PCIE_DEVICE_ID 0x6200
#define PCIE_VENDOR_ID 0x8086
#define MAX_PATH_SIZE 256

int pcie_write(int fd, void *buf, size_t bufSize)
{
    int ret = write(fd, buf, bufSize);

    if (ret < 0)
    {
        return -errno;
    }

    return ret;
}

int pcie_read(int fd, void *buf, size_t bufSize)
{
    int ret = read(fd, buf, bufSize);

    if (ret < 0)
    {
        return -errno;
    }

    return ret;
}

int pcie_init(const char *slot, int *fd)
{
    *fd = open(slot, O_RDWR);

    if (*fd == -1)
        return -1;

    return 0;
}

int pcie_close(int fd)
{
    close(fd);
    return 0;
}

xLinkPlatformErrorCode_t pcie_find_device_port(int index, char* port_name, int size) {
    xLinkPlatformErrorCode_t rc = X_LINK_PLATFORM_DEVICE_NOT_FOUND;
    struct dirent *entry;
    DIR *dp;

    if (port_name == NULL)
        return X_LINK_PLATFORM_ERROR;

    dp = opendir("/sys/class/mxlk/");
    if (dp == NULL)
    {
        perror("opendir");
        return X_LINK_PLATFORM_DRIVER_NOT_LOADED;
    }

    // All entries in this (virtual) directory are generated when the driver
    // is loaded, and correspond 1:1 to entries in /dev/

    int device_cnt = 0;
    while((entry = readdir(dp))) {
        //printf("Entry: %s %d\n", entry->d_name, strncmp(entry->d_name, "mxlink", 6));
        // Compare the beginning of the name to make sure it is a device name
        if (strncmp(entry->d_name, "mxlk", 4) == 0)
        {
            if (device_cnt == index)
            {
                snprintf(port_name, size, "/dev/%s", entry->d_name);
                //printf("Found port %s for index %d\n", port_name, index);
                rc = X_LINK_PLATFORM_SUCCESS;
                break;
            }

            device_cnt++;
        }
    }
    closedir(dp);

    return rc;
}
