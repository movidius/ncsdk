/*
* Copyright 2018 Intel Corporation.
* The source code, information and material ("Material") contained herein is
* owned by Intel Corporation or its suppliers or licensors, and title to such
* Material remains with Intel Corporation or its suppliers or licensors.
* The Material contains proprietary information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright laws and treaty
* provisions.
* No part of the Material may be used, copied, reproduced, modified, published,
* uploaded, posted, transmitted, distributed or disclosed in any way without
* Intel's prior express written permission. No license under any patent,
* copyright or other intellectual property rights in the Material is granted to
* or conferred upon you, either expressly, by implication, inducement, estoppel
* or otherwise.
* Any license under such intellectual property rights must be express and
* approved by Intel in writing.
*/

///
/// @brief     Application configuration Leon header
///

#include "UsbLinkPlatform.h"

#include <stdio.h>
#include <stdlib.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/timeb.h>

#include <errno.h>

#if (defined(_WIN32) || defined(_WIN64) )
#include "usb_winusb.h"
#include "gettime.h"
#include "win_pthread.h"
extern void initialize_usb_boot();
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <libusb.h>
#include <pthread.h>
#endif
#include "usb_boot.h"

#ifdef USE_LINK_JTAG
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif  /*USE_LINK_JTAG*/

#define USB_LINK_SOCKET_PORT 5678

#define USBLINK_ERROR_PRINT
#ifdef USBLINK_ERROR_PRINT
#define USBLINK_ERROR(...) printf(__VA_ARGS__)
#else
#define USBLINK_ERROR(...) (void)0
#endif  /*USBLINK_ERROR_PRINT*/

#ifdef USBLINKDEBUG
#define USBLINK_PRINT(...) printf(__VA_ARGS__)
#else
#define USBLINK_PRINT(...) (void)0
#endif  /*USBLINKDEBUG*/


#ifndef USE_USB_VSC
int usbFdWrite = -1;
int usbFdRead = -1;
#endif  /*USE_USB_VSC*/

static int statuswaittimeout = 5;

#include <assert.h>

pthread_t readerThreadId;

#define MAX_EVENTS 64

uint32_t numPoolsAllocated = 0;

#define USB_ENDPOINT_IN 0x81
#define USB_ENDPOINT_OUT 0x01

//libusb_device_handle* f;

static double seconds()
{
    static double s;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    if(!s)
        s = ts.tv_sec + ts.tv_nsec * 1e-9;
    return ts.tv_sec + ts.tv_nsec * 1e-9 - s;
}

static int usb_write(libusb_device_handle *f, const void *data, size_t size, unsigned int timeout)
{
    while(size > 0)
    {
        int bt,ss = size;
        if(ss > 1024*1024*10)
            ss = 1024*1024*10;
#if (defined(_WIN32) || defined(_WIN64) )
        int rc = usb_bulk_write(f, USB_ENDPOINT_OUT, (unsigned char *)data, ss, &bt, timeout);
#else
        int rc = libusb_bulk_transfer(f, USB_ENDPOINT_OUT, (unsigned char *)data, ss, &bt, timeout);
#endif
        if(rc)
            return rc;
        data = (char *)data + bt;
        size -= bt;
    }
    return 0;
}

static int usb_read(libusb_device_handle *f, void *data, size_t size, unsigned int timeout)
{
    while(size > 0)
    {
        int bt,ss = size;
        if(ss > 1024*1024*10)
            ss = 1024*1024*10;
#if (defined(_WIN32) || defined(_WIN64))
        int rc = usb_bulk_read(f, USB_ENDPOINT_IN, (unsigned char *)data, ss, &bt, timeout);
#else
        int rc = libusb_bulk_transfer(f, USB_ENDPOINT_IN,(unsigned char *)data, ss, &bt, timeout);
#endif
        if(rc)
            return rc;
        data = ((char *)data) + bt;
        size -= bt;
    }
    return 0;
}

libusb_device_handle *usblink_open(const char *path)
{
    usbBootError_t rc = USB_BOOT_DEVICE_NOT_FOUND;
    libusb_device_handle *h = NULL;
    libusb_device *dev = NULL;
    double waittm = seconds() + statuswaittimeout;
    while(seconds() < waittm){
        int size = path != NULL ? strlen(path) : 0;
        rc = usb_find_device(0, (char *)path, size, (void **)&dev, DEFAULT_OPENVID, DEFAULT_OPENPID);
        if(rc == USB_BOOT_SUCCESS)
            break;
        usleep(1000);
    }
    if (rc == USB_BOOT_TIMEOUT || rc == USB_BOOT_DEVICE_NOT_FOUND) // Timeout
        return 0;
#if (defined(_WIN32) || defined(_WIN64) )
	h = usb_open_device(dev, NULL, 0, NULL);
	int libusb_rc = ((h != NULL) ? (0) : (-1));
	if (libusb_rc < 0)
	{
		usb_close_device(h);
		usb_free_device(dev);
		return 0;
	}
	//usb_close_device(h);
	//usb_free_device(dev);
#else
    int libusb_rc = libusb_open(dev, &h);
    if (libusb_rc < 0)
    {
        libusb_unref_device(dev);
        return 0;
    }
    libusb_unref_device(dev);
    libusb_rc = libusb_claim_interface(h, 0);
    if(libusb_rc < 0)
    {
        libusb_close(h);
        return 0;
    }
#endif
    return h;
}

void usblink_close(libusb_device_handle *f)
{
#if (defined(_WIN32) || defined(_WIN64))
    usb_close_device(f);
#else
    libusb_release_interface(f, 0);
    libusb_close(f);
#endif
}

int USBLinkWrite(void* fd, void* data, int size, unsigned int timeout)
{
    int rc = 0;
#ifndef USE_USB_VSC
    int byteCount = 0;
#ifdef USE_LINK_JTAG
    while (byteCount < size){
        byteCount += write(usbFdWrite, &((char*)data)[byteCount], size - byteCount);
        printf("write %d %d\n", byteCount, size);
    }
#else
    if(usbFdWrite < 0)
    {
        return -1;
    }
    while(byteCount < size)
    {
       int toWrite = (PACKET_LENGTH && (size - byteCount > PACKET_LENGTH)) \
                        ? PACKET_LENGTH:size - byteCount;
       int wc = write(usbFdWrite, ((char*)data) + byteCount, toWrite);
//       printf("wwrite %x %d %x %d\n", wc, handler->commFd, ((char*)data) + byteCount, toWrite);

       if ( wc != toWrite)
       {
           return -2;
       }

       byteCount += toWrite;
       unsigned char acknowledge;
       int rc;
       rc = read(usbFdWrite, &acknowledge, sizeof(acknowledge));
       if ( rc < 0)
       {
           return -2;
       }
       if (acknowledge == 0xEF)
       {
//         printf("read %x\n", acknowledge);
       }
       else
       {
//         printf("read err %x %d\n", acknowledge, rc);
           return -2;
       }
    }
#endif  /*USE_LINK_JTAG*/
#else
    rc = usb_write((libusb_device_handle *) fd, data, size, timeout);
#endif  /*USE_USB_VSC*/
    return rc;
}

 int USBLinkRead(void* fd, void* data, int size, unsigned int timeout)
{
    //printf("%s() fd %p size %d\n", __func__, fd, size);
    int rc = 0;
#ifndef USE_USB_VSC
    int nread =  0;
#ifdef USE_LINK_JTAG
    while (nread < size){
        nread += read(usbFdWrite, &((char*)data)[nread], size - nread);
        printf("read %d %d\n", nread, size);
    }
#else
    if(usbFdRead < 0)
    {
        return -1;
    }

    while(nread < size)
    {
        int toRead = (PACKET_LENGTH && (size - nread > PACKET_LENGTH)) \
                        ? PACKET_LENGTH : size - nread;

        while(toRead > 0)
        {
            rc = read(usbFdRead, &((char*)data)[nread], toRead);
//          printf("read %x %d\n", *(int*)data, nread);
            if ( rc < 0)
            {
                return -2;
            }
            toRead -=rc;
            nread += rc;
        }
        unsigned char acknowledge = 0xEF;
        int wc = write(usbFdRead, &acknowledge, sizeof(acknowledge));
        if (wc == sizeof(acknowledge))
        {
//          printf("write %x %d\n", acknowledge, wc);
        }
        else
        {
            return -2;
        }
    }
#endif  /*USE_LINK_JTAG*/
#else
    rc = usb_read((libusb_device_handle *) fd, data, size, timeout);
#endif  /*USE_USB_VSC*/
    return rc;
}

int UsbLinkPlatformGetDeviceName(int index, char* name, int nameSize)
{
    usbBootError_t rc = usb_find_device(index, name, nameSize, 0, 0, 0);
    switch(rc) {
        case USB_BOOT_SUCCESS:
            return USB_LINK_PLATFORM_SUCCESS;
        case USB_BOOT_DEVICE_NOT_FOUND:
            return USB_LINK_PLATFORM_DEVICE_NOT_FOUND;
        case USB_BOOT_TIMEOUT:
            return USB_LINK_PLATFORM_TIMEOUT;
        default:
            return USB_LINK_PLATFORM_ERROR;
    }
}
//#define XLINK_NO_BOOT
int UsbLinkPlatformBootRemote(const char* deviceName, const char* binaryPath)
{
#ifndef XLINK_NO_BOOT

    unsigned filesize;
    FILE *fp;
    char *tx_buf;
    char subaddr[28+2];
    int rc;

#ifndef USE_USB_VSC
    if (usbFdRead != -1){
        close(usbFdRead);
        usbFdRead = -1;
    }
    if (usbFdWrite != -1){
        close(usbFdWrite);
        usbFdWrite = -1;
    }
#endif  /*USE_USB_VSC*/

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

int USBLinkPlatformResetRemote(void* fd)
{
#ifndef USE_USB_VSC
#ifdef USE_LINK_JTAG
    /*Nothing*/
#else
    if (usbFdRead != -1){
        close(usbFdRead);
        usbFdRead = -1;
    }
    if (usbFdWrite != -1){
        close(usbFdWrite);
        usbFdWrite = -1;
    }
#endif  /*USE_LINK_JTAG*/
#else
    usblink_close((libusb_device_handle *) fd);
#endif  /*USE_USB_VSC*/
    return -1;
}
int UsbLinkPlatformConnect(const char* devPathRead, const char* devPathWrite, void** fd)
{
    #ifndef USE_USB_VSC
#ifdef USE_LINK_JTAG
    struct sockaddr_in serv_addr;
    usbFdWrite = socket(AF_INET, SOCK_STREAM, 0);
    usbFdRead = socket(AF_INET, SOCK_STREAM, 0);
    assert(usbFdWrite >=0);
    assert(usbFdRead >=0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(USB_LINK_SOCKET_PORT);

    if (connect(usbFdWrite, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting");
        exit(1);
    }
    printf("this is working\n");
    return 0;

#else
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
    return 0;
#endif  /*USE_LINK_JTAG*/
#else
    *fd = usblink_open(devPathWrite);
    if (*fd == 0)
    {
       /* could fail due to port name change */
       //printf("fail\n");
       return -1;
    }
//    dev = new fastmemDevice("usb");
//    int usbInit = dev->fastmemInit();
    if(*fd)
        return 0;
    else
        return -1;
#endif  /*USE_USB_VSC*/
}
int UsbLinkPlatformInit(int loglevel)
{
    usb_loglevel = loglevel;
#if (defined(_WIN32) || defined(_WIN64))
    initialize_usb_boot();
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

