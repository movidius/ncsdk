/*
*
* Copyright (c) 2017-2018 Intel Corporation. All Rights Reserved
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "XLinkPlatform.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <assert.h>
#include <errno.h>

#if (defined(_WIN32) || defined(_WIN64) )
#include "usb_winusb.h"
#include "gettime.h"
extern void initialize_usb_boot();
#include "win_pthread.h"
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <libusb.h>
#include <pthread.h>
#endif

#include "pcie_host.h"
#include "usb_boot.h"

#define MAX_EVENTS 64
#define USB_ENDPOINT_IN 0x81
#define USB_ENDPOINT_OUT 0x01

//CDC io device descriptors
int usbFdWrite = -1;
int usbFdRead = -1;
//PCIe io device descriptors
int pcieFd = -1;

//Communication protocol used
int gl_protocol = -1;

static int statuswaittimeout = 5;
pthread_t readerThreadId;

#if (defined(_WIN32) || defined(_WIN64) )
#define __attribute__(x)
#endif

#if defined(USE_PCIE)
size_t gl_packetLength = PCIE_MAX_BUFFER_SIZE;
#else
size_t gl_packetLength = PACKET_LENGTH;
#endif

/*#################################################################################
################################# AUXILIARY FUNCTIONS #############################
##################################################################################*/
static double seconds()
{
    static double s;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    if(!s)
        s = ts.tv_sec + ts.tv_nsec * 1e-9;
    return ts.tv_sec + ts.tv_nsec * 1e-9 - s;
}

/*#################################################################################
################################## USB CDC FUNCTIONS ##############################
##################################################################################*/
static xLinkPlatformErrorCode_t cdc_convert_error_code(int rc) {
    switch(rc) {
        case 0:
            return X_LINK_PLATFORM_SUCCESS;
        case -1:
            return X_LINK_PLATFORM_DEVICE_NOT_FOUND;
        //No timeout case for cdc
        default:
            return X_LINK_PLATFORM_ERROR;
    }
}
static int cdc_usb_write(void *f __attribute__((unused)),
                        void* data, size_t size,
                        unsigned int timeout __attribute__((unused)))
{
#if (!defined(_WIN32) && !defined(_WIN64))
    size_t byteCount = 0;
    if(usbFdWrite < 0)
    {
        return cdc_convert_error_code(-1);
    }
    while(byteCount < size) {
       int toWrite = (gl_packetLength && ((size - byteCount) > gl_packetLength)) \
                        ? gl_packetLength : size - byteCount;
       int wc = write(usbFdWrite, ((char*)data) + byteCount, toWrite);
       if (wc != toWrite) {
           return cdc_convert_error_code(-2);
       }
       byteCount += toWrite;

       //Acknowledge /*
       unsigned char acknowledge;
       int rc = read(usbFdWrite, &acknowledge, sizeof(acknowledge));

       if ( rc < 0) {
           return cdc_convert_error_code(-2);
       }
       if (acknowledge == 0xEF) {
           //printf("read ack %x\n", acknowledge);
       }
       else {
           printf("read ack err %x %d\n", acknowledge, rc);
           return cdc_convert_error_code(-2);
       } //*/
    }
#endif
    return 0;
}

static int cdc_usb_read(void *f __attribute__((unused)),
                        void* data, size_t size,
                        unsigned int timeout __attribute__((unused)))
{
#if (!defined(_WIN32) && !defined(_WIN64))
    if(usbFdRead < 0) {
        return cdc_convert_error_code(-1);
    }
    size_t byteCount =  0;
    int toRead = 0;

    while(byteCount < size) {
        toRead = (gl_packetLength && ((size - byteCount) > gl_packetLength)) \
                        ? gl_packetLength : size - byteCount;
        while(toRead > 0)
        {
            int rc = read(usbFdRead, &((char*)data)[byteCount], toRead);
            if (rc < 0) {
                return cdc_convert_error_code(-2);
            }
            toRead -= rc;
            byteCount += rc;
        }
        //Acknowledge /*
        unsigned char acknowledge = 0xEF;
        int wc = write(usbFdRead, &acknowledge, sizeof(acknowledge));
        if (wc == sizeof(acknowledge)) {
          //printf("write %x %d\n", acknowledge, wc);
        }
        else {
           return cdc_convert_error_code(-2);
        }   //*/
    }
#endif
    return 0;
}

static int cdc_usb_open(const char* devPathRead,
                        const char* devPathWrite,
                        void **fd __attribute__((unused)))
{
#if (!defined(_WIN32) && !defined(_WIN64))
    usbFdRead= open(devPathRead, O_RDWR);
    if(usbFdRead < 0)
    {
        return -1;
    }
    // set tty to raw mode
    struct termios  tty;
    speed_t     spd;
    int rc;
    rc = tcgetattr(usbFdRead, &tty);
    if (rc < 0) {
        usbFdRead = -1;
        return -2;
    }

    spd = B115200;
    cfsetospeed(&tty, (speed_t)spd);
    cfsetispeed(&tty, (speed_t)spd);

    cfmakeraw(&tty);

    rc = tcsetattr(usbFdRead, TCSANOW, &tty);
    if (rc < 0) {
        usbFdRead = -1;
        return -2;
    }

    usbFdWrite= open(devPathWrite, O_RDWR);
    if(usbFdWrite < 0)
    {
        usbFdWrite = -1;
        return -2;
    }
    // set tty to raw mode
    rc = tcgetattr(usbFdWrite, &tty);
    if (rc < 0) {
        usbFdWrite = -1;
        return -2;
    }

    spd = B115200;
    cfsetospeed(&tty, (speed_t)spd);
    cfsetispeed(&tty, (speed_t)spd);

    cfmakeraw(&tty);

    rc = tcsetattr(usbFdWrite, TCSANOW, &tty);
    if (rc < 0) {
        usbFdWrite = -1;
        return -2;
    }
#endif
    return 0;
}

static int cdc_usb_close(void *f __attribute__((unused)))
{
#if (!defined(_WIN32) && !defined(_WIN64))
    if (usbFdRead != -1){
        close(usbFdRead);
        usbFdRead = -1;
    }
    if (usbFdWrite != -1){
        close(usbFdWrite);
        usbFdWrite = -1;
    }
#endif
    return 0;
}

/*#################################################################################
################################## USB VSC FUNCTIONS ##############################
##################################################################################*/
static xLinkPlatformErrorCode_t vsc_convert_error_code(int rc) {
    switch(rc) {
        case 0:
            return X_LINK_PLATFORM_SUCCESS;
        case LIBUSB_ERROR_TIMEOUT:
            return X_LINK_PLATFORM_TIMEOUT;
        case LIBUSB_ERROR_NO_DEVICE:
            return X_LINK_PLATFORM_DEVICE_NOT_FOUND;
        default:
            return X_LINK_PLATFORM_ERROR;
    }
}
static int vsc_usb_write(void *f, void *data, size_t size, unsigned int timeout)
{
    while(size > 0)
    {
        int bt,ss = size;
        if(ss > 1024*1024*10)
            ss = 1024*1024*10;
 #if (defined(_WIN32) || defined(_WIN64) )
        int rc = usb_bulk_write(f, USB_ENDPOINT_OUT, (unsigned char *)data, ss, &bt, timeout);
 #else
        int rc = libusb_bulk_transfer((libusb_device_handle*)f,
                                    USB_ENDPOINT_OUT,
                                    (unsigned char *)data,
                                    ss,
                                    &bt,
                                    timeout);
 #endif
        if(rc)
            return vsc_convert_error_code(rc);
        data = (char *)data + bt;
        size -= bt;
    }
    return 0;
}

static int vsc_usb_read(void *f, void *data, size_t size, unsigned int timeout)
{
    while(size > 0)
    {
        int bt, ss = size;
        if(ss > 1024*1024*10)
            ss = 1024*1024*10;
#if (defined(_WIN32) || defined(_WIN64) )
        int rc = usb_bulk_read(f, USB_ENDPOINT_IN, (unsigned char *)data, ss, &bt, timeout);
#else
        int rc = libusb_bulk_transfer((libusb_device_handle*)f,
                                    USB_ENDPOINT_IN,
                                    (unsigned char *)data,
                                    ss,
                                    &bt,
                                    timeout);
#endif
        if(rc)
            return vsc_convert_error_code(rc);
        data = ((char *)data) + bt;
        size -= bt;
    }
    return 0;
}

static int vsc_usb_open(const char* devPathRead __attribute__((unused)),
                        const char* devPathWrite,
                        void** fd)
{
    usbBootError_t rc = USB_BOOT_DEVICE_NOT_FOUND;
    libusb_device_handle *h = NULL;
    libusb_device *dev = NULL;
    double waittm = seconds() + statuswaittimeout;
    while(seconds() < waittm){
        int size = devPathWrite != NULL ? strlen(devPathWrite) : 0;

        rc = usb_find_device(0, (char *)devPathWrite, size, (void **)&dev, \
                                DEFAULT_OPENVID, DEFAULT_OPENPID);
        if(rc == USB_BOOT_SUCCESS)
            break;
        usleep(1000);
    }
    if (rc == USB_BOOT_TIMEOUT || rc == USB_BOOT_DEVICE_NOT_FOUND) // Timeout
        return -1;

 #if (defined(_WIN32) || defined(_WIN64) )
        h = usb_open_device(dev, NULL, 0, NULL);
        int libusb_rc = ((h != NULL) ? (0) : (-1));
        if (libusb_rc < 0) {
            usb_close_device(h);
            usb_free_device(dev);
            return -1;
        }
       //usb_close_device(h);
       //usb_free_device(dev);
 #else
    int libusb_rc = libusb_open(dev, &h);
    if (libusb_rc < 0)
    {
        libusb_unref_device(dev);
        return -1;
    }
    libusb_unref_device(dev);
    libusb_rc = libusb_claim_interface(h, 0);
    if(libusb_rc < 0)
    {
        libusb_close(h);
        return -1;
    }
 #endif
    *fd = h;
    if (*fd == 0)
    {
        // could fail due to port name change
        //printf("fail\n");
        return -1;
    }
    //dev = new fastmemDevice("usb");
    //int usbInit = dev->fastmemInit();
    if(*fd)
        return 0;

    return -1;
}

static int vsc_usb_close(void *f)
{
 #if (defined(_WIN32) || defined(_WIN64))
    usb_close_device(f);
 #else
    libusb_release_interface((libusb_device_handle*)f, 0);
    libusb_close((libusb_device_handle*)f);
 #endif
    return 0;
}

static int write_pending = 0;
static int read_pending = 0;

/*#################################################################################
################################### PCIe FUNCTIONS ################################
##################################################################################*/
#define UNUSED __attribute__((unused))

static int pcie_host_write(UNUSED void *f, void *data, size_t size,
                           UNUSED unsigned int timeout)
{
    int left = size;
    while(left > 0)
    {
        write_pending = 1;

        int bt = pcie_write(pcieFd, data, left);

        if (bt == -EAGAIN)
        {
            // Let read commands be submitted
            if (read_pending > 0)
                usleep(1000);
            continue;
        }

        write_pending = 0;

        if(bt < 0)
            return bt;
        data = ((char *)data) + bt;
        left -= bt;
    }

    return size;
}

static int pcie_host_read(UNUSED void *f, void *data, size_t size,
                          UNUSED unsigned int timeout)
{
    int left = size;
    while(left > 0)
    {
        read_pending = 1;

        int bt = pcie_read(pcieFd, data, left);

        if (bt == -EAGAIN) {
            // Let write commands be submitted
            if (write_pending > 0)
                usleep(1000);
            continue;
        }

        read_pending = 0;

        if(bt < 0)
            return bt;
        data = ((char *)data) + bt;
        left -= bt;
    }

    return size;
}

static int pcie_host_open(UNUSED const char* devPathRead,
                          const char* devPathWrite,
                          UNUSED void** fd)
{
    return pcie_init(devPathWrite, &pcieFd);
}

static int pcie_host_close(UNUSED void *f)
{
    pcie_close(pcieFd);
    return 0;
}

#undef UNUSED
/*############################### FUNCTION ARRAYS #################################*/
/*These arrays hold the write/read/open/close operation functions
specific for each communication protocol.
Add more functions if adding another protocol*/
int (*write_fcts[protocols])(void*, void*, size_t, unsigned int) = \
                            {vsc_usb_write, cdc_usb_write, pcie_host_write};
int (*read_fcts[protocols])(void*, void*, size_t, unsigned int) = \
                            {vsc_usb_read, cdc_usb_read, pcie_host_read};
int (*open_fcts[protocols])(const char*, const char*, void**) = \
                            {vsc_usb_open, cdc_usb_open, pcie_host_open};
int (*close_fcts[protocols])(void*) = \
                            {vsc_usb_close, cdc_usb_close, pcie_host_close};

/*#################################################################################
###################################################################################
###################################### EXTERNAL ###################################
###################################################################################
##################################################################################*/
int XLinkPlatformConnect(const char* devPathRead, const char* devPathWrite, void** fd)
{
    return open_fcts[gl_protocol](devPathRead, devPathWrite, fd);
}

int XLinkWrite(void* fd, void* data, int size, unsigned int timeout)
{
    return write_fcts[gl_protocol](fd, data, size, timeout);
}

 int XLinkRead(void* fd, void* data, int size, unsigned int timeout)
{
    return read_fcts[gl_protocol](fd, data, size, timeout);
}

int XLinkPlatformResetRemote(void* fd)
{
    return close_fcts[gl_protocol](fd);
}

int XLinkPlatformInit(protocol_t protocol, int loglevel)
{
    gl_protocol = protocol;
    usb_loglevel = loglevel;
#if (defined(_WIN32) || defined(_WIN64))
    initialize_usb_boot();
#endif
    return 0;
}

static int getDeviceName(int index, char* name, int nameSize , int pid)
{
    switch(gl_protocol){

        case Pcie:
        {
            return pcie_find_device_port(index, name, nameSize);
            break;
        }
        case Ipc:
            break;
        case UsbCDC:
        case UsbVSC:
            /*should have common device(temporary moved to 'default')*/
        default:
            switch(usb_find_device(index, name, nameSize, 0, 0, pid))
            {
                case USB_BOOT_SUCCESS:
                    return X_LINK_PLATFORM_SUCCESS;
                case USB_BOOT_DEVICE_NOT_FOUND:
                    return X_LINK_PLATFORM_DEVICE_NOT_FOUND;
                case USB_BOOT_TIMEOUT:
                    return X_LINK_PLATFORM_TIMEOUT;
                default:
                    return X_LINK_PLATFORM_ERROR;
            }
            break;
    }
    return X_LINK_PLATFORM_SUCCESS;
}

int XLinkPlatformGetDeviceName(int index, char* name, int nameSize)
{
    return getDeviceName(index, name, nameSize, 0);
}

int XLinkPlatformGetDeviceNameExtended(int index, char* name, int nameSize, int pid)
{
    return getDeviceName(index, name, nameSize, pid);
}

int XLinkPlatformBootRemote(const char* deviceName, const char* binaryPath)
{
#ifndef XLINK_NO_BOOT
    unsigned filesize;
    FILE *fp;
    char *tx_buf;
    char subaddr[28+2];
    int rc;

    /* Don't try to boot FW if PCIe */
#ifdef USE_PCIE
        return 0;
#endif
    if (usbFdRead != -1){
        close(usbFdRead);
        usbFdRead = -1;
    }
    if (usbFdWrite != -1){
        close(usbFdWrite);
        usbFdWrite = -1;
    }
    // Load the executable
    fp = fopen(binaryPath, "rb");
    if(fp == NULL)
    {
        if(usb_loglevel)
            perror(binaryPath);
        return -7;
    }
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);
    if(!(tx_buf = (char*)malloc(filesize)))
    {
        if(usb_loglevel)
            perror("buffer");
        fclose(fp);
        return -3;
    }
    if(fread(tx_buf, 1, filesize, fp) != filesize)
    {
        if(usb_loglevel)
            perror(binaryPath);
        fclose(fp);
        free(tx_buf);
        return -7;
    }
    fclose(fp);

    // This will be the string to search for in /sys/dev/char links
    int chars_to_write = snprintf(subaddr, 28, "-%s:", deviceName);
    if(chars_to_write >= 28) {
        printf("Path to your boot util is too long for the char array here!\n");
    }
    // Boot it
    rc = usb_boot(deviceName, tx_buf, filesize);
    free(tx_buf);
    if(rc)
    {
        return rc;
    }
    if(usb_loglevel > 1)
        fprintf(stderr, "Boot successful, device address %s\n", deviceName);
#endif
    return 0;
}

void deallocateData(void* ptr,uint32_t size, uint32_t alignment)
{
    if (!ptr)
        return;
#if (defined(_WIN32) || defined(_WIN64) )
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

void* allocateData(uint32_t size, uint32_t alignment)
{
    void* ret = NULL;
#if (defined(_WIN32) || defined(_WIN64) )
    ret = _aligned_malloc(size, alignment);
#else
    posix_memalign(&ret, alignment, size);
#endif
    return ret;
}

