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

// USB utility for use with Myriad2v2 ROM
// Very heavily modified from Sabre version of usb_boot
// Copyright(C) 2015 Movidius Ltd.


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#if (defined(_WIN32) || defined(_WIN64) )
#include "usb_winusb.h"
#include "gettime.h"
#else
#include <unistd.h>
#include <getopt.h>
#include <libusb.h>
#endif
#include "usb_boot.h"



#define DEFAULT_VID                 0x03E7

#define DEFAULT_WRITE_TIMEOUT       2000
#define DEFAULT_CONNECT_TIMEOUT     20          // in 100ms units
#define DEFAULT_CHUNKSZ             1024*1024

#define OPEN_DEV_ERROR_MESSAGE_LENGTH 128

static unsigned int bulk_chunklen = DEFAULT_CHUNKSZ;
static int write_timeout = DEFAULT_WRITE_TIMEOUT;
static int connect_timeout = DEFAULT_CONNECT_TIMEOUT;
static int initialized;

typedef struct {
    int pid;
    char name[10];
} deviceBootInfo_t;

static deviceBootInfo_t supportedDevices[] = {
    {
        .pid = 0x2150,
        .name = "ma2450"
    },
    {
        .pid = 0x2485,
        .name = "ma2480"
    },
    {
        //To support the case where the port name change, or it's already booted
        .pid = DEFAULT_OPENPID,
        .name = ""
    }
};
// for now we'll only use the loglevel for usb boot. can bring it into
// the rest of usblink later
// use same levels as mvnc_loglevel for now
int usb_loglevel;
#if (defined(_WIN32) || defined(_WIN64) )
void initialize_usb_boot()
{

    // We sanitize the situation by trying to reset the devices that have been left open
    initialized = 1;
}
#else
void __attribute__((constructor)) usb_library_load()
{
    initialized = !libusb_init(NULL);
    //libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
}

void __attribute__((destructor)) usb_library_unload()
{
    if(initialized)
        libusb_exit(NULL);
}
#endif

typedef struct timespec highres_time_t;

static inline void highres_gettime(highres_time_t *ptr) {
    clock_gettime(CLOCK_REALTIME, ptr);
}

static inline double highres_elapsed_ms(highres_time_t *start, highres_time_t *end) {
    struct timespec temp;
    if((end->tv_nsec - start->tv_nsec) < 0) {
        temp.tv_sec = end->tv_sec - start->tv_sec - 1;
        temp.tv_nsec = 1000000000 + end->tv_nsec-start->tv_nsec;
    } else {
        temp.tv_sec = end->tv_sec - start->tv_sec;
        temp.tv_nsec = end->tv_nsec - start->tv_nsec;
    }
    return (double)(temp.tv_sec * 1000) + (((double)temp.tv_nsec) * 0.000001);
}

static const char *get_pid_name(int pid)
{
    int n = sizeof(supportedDevices)/sizeof(supportedDevices[0]);
    int i;

    for (i = 0; i < n; i++)
    {
        if (supportedDevices[i].pid == pid)
            return supportedDevices[i].name;
    }
	  if(usb_loglevel)
		    fprintf(stderr, "%s(): Error pid:=%i not supported\n", __func__, pid);
    return ""; //shouldn't happen
}

const char * usb_get_pid_name(int pid)
{
    return get_pid_name(pid);
}

static int get_pid_by_name(const char* name)
{
    char* p = strchr(name, '-');
    if (p == NULL) {
        fprintf(stderr, "%s(): Error name not supported\n", __func__);
        return -1;
    }
    p++; //advance to point to the name
    int i;
    int n = sizeof(supportedDevices)/sizeof(supportedDevices[0]);

    for (i = 0; i < n; i++)
    {
        if (strcmp(supportedDevices[i].name, p) == 0)
            return supportedDevices[i].pid;
    }
    return -1;
}

static int is_pid_supported(int pid)
{
    int n = sizeof(supportedDevices)/sizeof(supportedDevices[0]);
    int i;
    for (i = 0; i < n; i++) {
        if (supportedDevices[i].pid == pid)
            return 1;
    }
    return 0;
}

#if (!defined(_WIN32) && !defined(_WIN64) )
static const char *gen_addr(libusb_device *dev, int pid)
{
    static char buff[4 * 7 + 7];    // '255-' x 7 (also gives us nul-terminator for last entry)
                                    // 7 => to add "-maXXXX"
    uint8_t pnums[7];
    int pnum_cnt, i;
    char *p;

    pnum_cnt = libusb_get_port_numbers(dev, pnums, 7);
    if (pnum_cnt == LIBUSB_ERROR_OVERFLOW) {
        // shouldn't happen!
        strcpy(buff, "<error>");
        return buff;
    }
    p = buff;
    for (i = 0; i < pnum_cnt - 1; i++)
        p += sprintf(p, "%u.", pnums[i]);
    p += sprintf(p, "%u", pnums[i]);

    sprintf(p, "-%s", get_pid_name(pid));
    return buff;
}
#endif
// if device is NULL, return device address for device at index idx
// if device is not NULL, search by name and return device struct
usbBootError_t usb_find_device(unsigned idx, char *addr, unsigned addrsize, void **device, int vid, int pid)
{
    int res;
#if (defined(_WIN32) || defined(_WIN64) )
    //static libusb_device **devs;
    // 2 => vid
    // 2 => pid
    // '255-' x 7 (also gives us nul-terminator for last entry)
    // 7 => to add "-maXXXX"
    uint8_t devs[15][2 + 2 + 4 * 7 + 7] = { 0 };//to store ven_id,dev_id;
#else
    static libusb_device **devs;
    libusb_device *dev;
    struct libusb_device_descriptor desc;
#endif
    int count = 0;
    size_t i;

    if(!initialized)
    {
        if(usb_loglevel)
            fprintf(stderr, "Library has not been initialized when loaded\n");
        return USB_BOOT_ERROR;
    }
#if (defined(_WIN32) || defined(_WIN64))
    if (!devs || idx == 0)
    {
        if (((res = usb_list_devices(vid, pid, &devs)) < 0))
        {
            if (usb_loglevel)
                fprintf(stderr, "Unable to get USB device list: %s\n", libusb_strerror(res));
            return USB_BOOT_ERROR;
        }
    }
#else
    if(!devs || idx == 0)
    {
        if(devs)
        {
            libusb_free_device_list(devs, 1);
            devs = 0;
        }
        if((res = libusb_get_device_list(NULL, &devs)) < 0)
        {
            if(usb_loglevel)
                fprintf(stderr, "Unable to get USB device list: %s\n", libusb_strerror(res));
            return USB_BOOT_ERROR;
        }
    }
#endif
    i = 0;
#if (defined(_WIN32) || defined(_WIN64))
    while (res-- > 0)
    {
        if (((int)(devs[res][0] << 8 | devs[res][1]) == vid && (devs[res][2] << 8 | devs[res][3]) == pid) ||
            (pid == 0 && vid == 0 &&
            (((int)(devs[res][0] << 8 | devs[res][1]) == DEFAULT_VID && is_pid_supported((int)(devs[res][2] << 8 | devs[res][3])) == 1) ||
            ((int)(devs[res][0] << 8 | devs[res][1]) == DEFAULT_OPENVID && (int)(devs[res][2] << 8 | devs[res][3]) == DEFAULT_OPENPID))))
        {
            if (device)
            {
                const char *caddr = &devs[res][4];
                if (!strcmp(caddr, addr))
                {
                    if (usb_loglevel > 1)
                        fprintf(stderr, "Found Address: %s - VID/PID %04x:%04x\n", addr, (int)(devs[res][0] << 8 | devs[res][1]), (int)(devs[res][2] << 8 | devs[res][3]));
                    *device = enumerate_usb_device(vid, pid, addr, 0);
                    return USB_BOOT_SUCCESS;
                }
            }
            else if (idx == count)
            {
                const char *caddr = &devs[res][4];
                if (usb_loglevel > 1)
                    fprintf(stderr, "Device %d Address: %s - VID/PID %04x:%04x\n", idx, caddr, (int)(devs[res][0] << 8 | devs[res][1]), (int)(devs[res][2] << 8 | devs[res][3]));
                strncpy(addr, caddr, addrsize);
                return USB_BOOT_SUCCESS;
            }
            count++;
        }
    }
#else
    while ((dev = devs[i++]) != NULL)
    {
        if((res = libusb_get_device_descriptor(dev, &desc)) < 0)
        {
            if(usb_loglevel)
                fprintf(stderr, "Unable to get USB device descriptor: %s\n", libusb_strerror(res));
            continue;
        }

        if((desc.idVendor == vid && desc.idProduct == pid) ||
            (pid == 0 && vid == 0 &&
                ( (desc.idVendor == DEFAULT_VID && is_pid_supported(desc.idProduct) == 1) ||
                  (desc.idVendor == DEFAULT_OPENVID && desc.idProduct == DEFAULT_OPENPID)) ))
        {
            if(device)
            {
                const char *caddr = gen_addr(dev, get_pid_by_name(addr));
                if(!strcmp(caddr, addr))
                {
                    if(usb_loglevel > 1)
                        fprintf(stderr, "Found Address: %s - VID/PID %04x:%04x\n", addr, desc.idVendor, desc.idProduct);
                    libusb_ref_device(dev);
                    libusb_free_device_list(devs, 1);
                    *device = dev;
                    devs = 0;
                    return USB_BOOT_SUCCESS;
                }
            } else if(idx == count)
            {
                const char *caddr = gen_addr(dev, desc.idProduct);
                if (usb_loglevel > 1)
                    fprintf(stderr, "Device %d Address: %s - VID/PID %04x:%04x\n", idx, caddr, desc.idVendor, desc.idProduct);
                strncpy(addr, caddr, addrsize);
                return USB_BOOT_SUCCESS;
            }
            count++;
        }
    }
    libusb_free_device_list(devs, 1);
    devs = 0;
#endif
    return USB_BOOT_DEVICE_NOT_FOUND;
}
#if (!defined(_WIN32) && !defined(_WIN64) )
static libusb_device_handle *usb_open_device(libusb_device *dev, uint8_t *endpoint, char *err_string_buff, int err_max_len)
{
    struct libusb_config_descriptor *cdesc;
    const struct libusb_interface_descriptor *ifdesc;
    libusb_device_handle *h = NULL;
    int res, i;

    if((res = libusb_open(dev, &h)) < 0)
    {
        snprintf(err_string_buff, err_max_len, "cannot open device: %s\n", libusb_strerror(res));
        return 0;
    }
    if((res = libusb_set_configuration(h, 1)) < 0)
    {
        snprintf(err_string_buff, err_max_len, "setting config 1 failed: %s\n", libusb_strerror(res));
        libusb_close(h);
        return 0;
    }
    if((res = libusb_claim_interface(h, 0)) < 0)
    {
        snprintf(err_string_buff, err_max_len, "claiming interface 0 failed: %s\n", libusb_strerror(res));
        libusb_close(h);
        return 0;
    }
    if((res = libusb_get_config_descriptor(dev, 0, &cdesc)) < 0)
    {
        snprintf(err_string_buff, err_max_len, "Unable to get USB config descriptor: %s\n", libusb_strerror(res));
        libusb_close(h);
        return 0;
    }
    ifdesc = cdesc->interface->altsetting;
    for(i=0; i<ifdesc->bNumEndpoints; i++)
    {
        if(usb_loglevel > 1)
            fprintf(stderr, "Found EP 0x%02x : max packet size is %u bytes\n",
                ifdesc->endpoint[i].bEndpointAddress, ifdesc->endpoint[i].wMaxPacketSize);
        if((ifdesc->endpoint[i].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_BULK)
            continue;
        if( !(ifdesc->endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) )
        {
            *endpoint = ifdesc->endpoint[i].bEndpointAddress;
            bulk_chunklen = ifdesc->endpoint[i].wMaxPacketSize;
            libusb_free_config_descriptor(cdesc);
            return h;
        }
    }
    libusb_free_config_descriptor(cdesc);
    strcpy(err_string_buff, "Unable to find BULK OUT endpoint\n");
    libusb_close(h);
    return 0;
}
#endif
// timeout: -1 = no (infinite) timeout, 0 = must happen immediately
static int wait_findopen(const char *device_address, int timeout, libusb_device **dev, libusb_device_handle **devh, uint8_t *endpoint)
{
    int i, rc;
    char last_open_dev_err[OPEN_DEV_ERROR_MESSAGE_LENGTH];

    usleep(100000);
    if(usb_loglevel > 1)
    {
        // I know about switch(), but for some reason -1 is picked up correctly
        if(timeout == -1)
            fprintf(stderr, "Starting wait for connect, no timeout\n");
        else if(timeout == 0)
            fprintf(stderr, "Trying to connect\n");
        else fprintf(stderr, "Starting wait for connect with %ums timeout\n", timeout * 100);
    }
    last_open_dev_err[0] = 0;
    i = 0;
    for(;;)
    {
        int addr_size = device_address != NULL ? strlen(device_address) : 0;
        rc = usb_find_device(0, (char *)device_address, addr_size, (void **)dev,
                                DEFAULT_VID, get_pid_by_name(device_address));
        if(rc < 0)
            return USB_BOOT_ERROR;
        if(!rc)
        {
            if( (*devh = usb_open_device(*dev, endpoint, last_open_dev_err, OPEN_DEV_ERROR_MESSAGE_LENGTH)) )
            {
                if(usb_loglevel > 1)
                    fprintf(stderr, "Found and opened device\n");
                return 0;
            }
#if (!defined(_WIN32) && !defined(_WIN64) )
            libusb_unref_device(*dev);
#endif
        }
        if(timeout != -1 && i == timeout)
        {
            if(usb_loglevel)
            {
                if(last_open_dev_err[0])
                    fprintf(stderr, "%s", last_open_dev_err);
                fprintf(stderr, "error: device not found!\n");
            }
            return rc ? USB_BOOT_DEVICE_NOT_FOUND : USB_BOOT_TIMEOUT;
        }
        i++;
        usleep(100000);
    }
    return 0;
}

static int send_file(libusb_device_handle *h, uint8_t endpoint, const uint8_t *tx_buf, unsigned filesize)
{
    int rc;
    int twb, wbr, wb;
    const uint8_t *p;
    double elapsedTime;
    highres_time_t t1, t2;

    elapsedTime = 0;
    twb = 0;
    p = tx_buf;
    if(usb_loglevel > 1)
        fprintf(stderr, "Performing bulk write of %u bytes...\n", filesize);
    while(twb < filesize)
    {
        highres_gettime(&t1);
        wb = filesize - twb;
        if(wb > bulk_chunklen)
            wb = bulk_chunklen;
        wbr = 0;
#if (defined(_WIN32) || defined(_WIN64) )
        rc = usb_bulk_write(h, endpoint, (void *)p, wb, &wbr, write_timeout);
#else
        rc = libusb_bulk_transfer(h, endpoint, (void *)p, wb, &wbr, write_timeout);
#endif
        if(rc || (wb != wbr))
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
                break;
            if(usb_loglevel)
                fprintf(stderr, "bulk write: %s (%d bytes written, %d bytes to write)\n", libusb_strerror(rc), wbr, wb);
            if(rc == LIBUSB_ERROR_TIMEOUT)
                return USB_BOOT_TIMEOUT;
            else return USB_BOOT_ERROR;
        }
        highres_gettime(&t2);
        elapsedTime += highres_elapsed_ms(&t1, &t2);
        twb += wbr;
        p += wbr;
    }
    if(usb_loglevel > 1)
    {
        double MBpS = ((double)filesize / 1048576.) / (elapsedTime * 0.001);
        fprintf(stderr, "Successfully sent %u bytes of data in %lf ms (%lf MB/s)\n", filesize, elapsedTime, MBpS);
    }
    return 0;
}

int usb_boot(const char *addr, const void *mvcmd, unsigned size)
{
    int rc = 0;
#if (defined(_WIN32) || defined(_WIN64) )
    void *dev = NULL;
    struct _usb_han *h;
#else
    libusb_device *dev;
    libusb_device_handle *h;
#endif
    uint8_t endpoint;

    rc = wait_findopen(addr, connect_timeout, &dev, &h, &endpoint);
    if(rc)
        return rc;
    rc = send_file(h, endpoint, mvcmd, size);
#if (defined(_WIN32) || defined(_WIN64) )
    usb_close_device(h);
    usb_free_device(dev);
    //usb_shutdown();
#else
    libusb_release_interface(h, 0);
    libusb_close(h);
    libusb_unref_device(dev);
#endif
    return rc;
}
