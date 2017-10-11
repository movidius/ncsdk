/*
* Copyright 2017 Intel Corporation.
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
// Author: David Steinberg <david.steinberg@movidius.com>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <libusb.h>
#include "usb_boot.h"
#include "mvnc.h"
#include "common.h"

#define DEFAULT_WRITE_TIMEOUT		2000
#define DEFAULT_CONNECT_TIMEOUT		20	// in 100ms units
#define DEFAULT_CHUNK_SZ			1024 * 1024

static unsigned int bulk_chunk_len = DEFAULT_CHUNK_SZ;
static int write_timeout = DEFAULT_WRITE_TIMEOUT;
static int connect_timeout = DEFAULT_CONNECT_TIMEOUT;
static int initialized;

void __attribute__ ((constructor)) usb_library_load()
{
	initialized = !libusb_init(NULL);
}

void __attribute__ ((destructor)) usb_library_unload()
{
	if (initialized)
		libusb_exit(NULL);
}

typedef struct timespec highres_time_t;

static inline void highres_gettime(highres_time_t *ptr)
{
	clock_gettime(CLOCK_REALTIME, ptr);
}

static inline double highres_elapsed_ms(highres_time_t *start, highres_time_t *end)
{
	struct timespec temp;
	if ((end->tv_nsec - start->tv_nsec) < 0) {
		temp.tv_sec = end->tv_sec - start->tv_sec - 1;
		temp.tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
	} else {
		temp.tv_sec = end->tv_sec - start->tv_sec;
		temp.tv_nsec = end->tv_nsec - start->tv_nsec;
	}
	return (double)(temp.tv_sec * 1000) + (((double)temp.tv_nsec) * 0.000001);
}

static const char *gen_addr(libusb_device *dev)
{
	static char buff[4 * 7] = "";	// '255-' x 7 (also gives us nul-terminator for last entry)
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
		p += snprintf(p, sizeof(buff) - strlen(buff), "%u.", pnums[i]);
	snprintf(p, sizeof(buff) - strlen(buff), "%u", pnums[i]);
	return buff;
}

// if device is NULL, return device address for device at index idx
// if device is not NULL, search by name and return device struct
int usb_find_device(unsigned idx, char *addr, unsigned addr_size, void **device,
		    int vid, int pid)
{
	static libusb_device **devs;
	libusb_device *dev;
	struct libusb_device_descriptor desc;
	int count = 0;
	size_t i;
	int res;

	if (!initialized) {
		PRINT_INFO(stderr,
			   "Library has not been initialized when loaded\n");
		return MVNC_ERROR;
	}
	if (!devs || idx == 0) {
		if (devs) {
			libusb_free_device_list(devs, 1);
			devs = 0;
		}
		if ((res = libusb_get_device_list(NULL, &devs)) < 0) {
			PRINT_INFO(stderr,
				   "Unable to get USB device list: %s\n",
				   libusb_strerror(res));
			return MVNC_ERROR;
		}
	}

	i = 0;
	while ((dev = devs[i++]) != NULL) {
		if ((res = libusb_get_device_descriptor(dev, &desc)) < 0) {
			PRINT_INFO(stderr,
				   "Unable to get USB device descriptor: %s\n",
				   libusb_strerror(res));
			continue;
		}
		if ((desc.idVendor == vid && desc.idProduct == pid) ||
		    (pid == 0 && vid == 0 && ((desc.idVendor == DEFAULT_VID
					       && desc.idProduct == DEFAULT_PID)
					      || (desc.idVendor ==
						  DEFAULT_OPEN_VID &&
						  desc.idProduct ==
						  DEFAULT_OPEN_PID)))) {
			if (device) {
				const char *caddr = gen_addr(dev);
				if (!strcmp(caddr, addr)) {
					PRINT_DEBUG(stderr,
						    "Found Address: %s - VID/PID %04x:%04x\n",
						    addr, desc.idVendor, desc.idProduct);
					libusb_ref_device(dev);
					libusb_free_device_list(devs, 1);
					*device = dev;
					devs = 0;
					return 0;
				}
			} else if (idx == count) {
				const char *caddr = gen_addr(dev);
				PRINT_DEBUG(stderr,
					    "Device %d Address: %s - VID/PID %04x:%04x\n",
					    idx, caddr, desc.idVendor, desc.idProduct);
				strncpy(addr, caddr, addr_size);
				return 0;
			}
			count++;
		}
	}
	libusb_free_device_list(devs, 1);
	devs = 0;
	return MVNC_DEVICE_NOT_FOUND;
}

static libusb_device_handle *usb_open_device(libusb_device *dev, uint8_t *endpoint,
					     char *err_string_buff, unsigned buff_size)
{
	struct libusb_config_descriptor *cdesc;
	const struct libusb_interface_descriptor *ifdesc;
	libusb_device_handle *h = NULL;
	int res, i;

	if ((res = libusb_open(dev, &h)) < 0) {
		snprintf(err_string_buff, buff_size, "cannot open device: %s\n",
			 libusb_strerror(res));
		return 0;
	}
	if ((res = libusb_set_configuration(h, 1)) < 0) {
		snprintf(err_string_buff, buff_size,
			 "setting config 1 failed: %s\n", libusb_strerror(res));
		libusb_close(h);
		return 0;
	}
	if ((res = libusb_claim_interface(h, 0)) < 0) {
		snprintf(err_string_buff, buff_size,
			 "claiming interface 0 failed: %s\n",
			 libusb_strerror(res));
		libusb_close(h);
		return 0;
	}
	if ((res = libusb_get_config_descriptor(dev, 0, &cdesc)) < 0) {
		snprintf(err_string_buff, buff_size,
			 "Unable to get USB config descriptor: %s\n",
			 libusb_strerror(res));
		libusb_close(h);
		return 0;
	}

	ifdesc = cdesc->interface->altsetting;
	for (i = 0; i < ifdesc->bNumEndpoints; i++) {
		PRINT_DEBUG(stderr,
			    "Found EP 0x%02x : max packet size is %u bytes\n",
			    ifdesc->endpoint[i].bEndpointAddress,
			    ifdesc->endpoint[i].wMaxPacketSize);
		if ((ifdesc->endpoint[i].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) !=
		    LIBUSB_TRANSFER_TYPE_BULK)
			continue;
		if (!
		    (ifdesc->endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)) {
			*endpoint = ifdesc->endpoint[i].bEndpointAddress;
			bulk_chunk_len = ifdesc->endpoint[i].wMaxPacketSize;
			libusb_free_config_descriptor(cdesc);
			return h;
		}
	}
	libusb_free_config_descriptor(cdesc);
	strcpy(err_string_buff, "Unable to find BULK OUT endpoint\n");
	libusb_close(h);
	return 0;
}

// timeout: -1 = no (infinite) timeout, 0 = must happen immediately
static int wait_findopen(const char *device_address, int timeout,
			 libusb_device ** dev, libusb_device_handle ** devh,
			 uint8_t * endpoint)
{
	int i, rc;
	char last_open_dev_err[128];

	usleep(100000);
	if (mvnc_loglevel > 1) {
		// I know about switch(), but for some reason -1 is picked up correctly
		if (timeout == -1)
			PRINT("Starting wait for connect, no timeout\n");
		else if (timeout == 0)
			PRINT("Trying to connect\n");
		else
			PRINT("Starting wait for connect with %ums timeout\n", timeout * 100);
	}

	last_open_dev_err[0] = 0;
	i = 0;
	for (;;) {
		rc = usb_find_device(0, (char *) device_address, 0,
				     (void **) dev, DEFAULT_VID, DEFAULT_PID);
		if (rc < 0)
			return MVNC_ERROR;
		if (!rc) {
			if ((*devh = usb_open_device(*dev, endpoint, last_open_dev_err, 128))) {
				PRINT_DEBUG(stderr, "Found and opened device\n");
				return 0;
			}
			libusb_unref_device(*dev);
		}

		if (timeout != -1 && i == timeout) {
			PRINT_INFO(stderr, "%serror: device not found!\n",
				   last_open_dev_err[0] ? last_open_dev_err : "");
			return rc ? MVNC_DEVICE_NOT_FOUND : MVNC_TIMEOUT;
		}
		i++;
		usleep(100000);
	}
}

static int send_file(libusb_device_handle * h, uint8_t endpoint,
		     const uint8_t * tx_buf, unsigned file_size)
{
	const uint8_t *p;
	int rc;
	int wb, twb, wbr;
	double elapsed_time;
	highres_time_t t1, t2;

	elapsed_time = 0;
	twb = 0;
	p = tx_buf;
	PRINT_DEBUG(stderr, "Performing bulk write of %u bytes...\n",
		    file_size);

	while (twb < file_size) {
		highres_gettime(&t1);
		wb = file_size - twb;
		if (wb > bulk_chunk_len)
			wb = bulk_chunk_len;
		wbr = 0;
		rc = libusb_bulk_transfer(h, endpoint, (void *) p, wb, &wbr,
					  write_timeout);

		if (rc || (wb != wbr)) {
			if (rc == LIBUSB_ERROR_NO_DEVICE)
				break;

			PRINT_INFO(stderr,
				   "bulk write: %s (%d bytes written, %d bytes to write)\n",
				   libusb_strerror(rc), wbr, wb);
			if (rc == LIBUSB_ERROR_TIMEOUT)
				return MVNC_TIMEOUT;
			else
				return MVNC_ERROR;
		}
		highres_gettime(&t2);
		elapsed_time += highres_elapsed_ms(&t1, &t2);
		twb += wbr;
		p += wbr;
	}
	PRINT_DEBUG(stderr,
		    "Successfully sent %u bytes of data in %lf ms (%lf MB/s)\n",
		    file_size, elapsed_time,
		    ((double) file_size / 1048576.) / (elapsed_time * 0.001));
	return 0;
}

int usb_boot(const char *addr, const void *mvcmd, unsigned size)
{
	int rc;
	libusb_device *dev;
	libusb_device_handle *h;
	uint8_t endpoint;

	rc = wait_findopen(addr, connect_timeout, &dev, &h, &endpoint);
	if (rc)
		return rc;
	rc = send_file(h, endpoint, mvcmd, size);
	libusb_release_interface(h, 0);
	libusb_close(h);
	libusb_unref_device(dev);
	return rc;
}
