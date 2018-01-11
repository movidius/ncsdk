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

#define _GNU_SOURCE
#include <dlfcn.h>		// For dladdr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>
#include "mvnc.h"
#include "usb_link.h"
#include "usb_boot.h"
#include "common.h"

// Graph file structure
#define HEADER_LENGTH	264
#define STAGE_LENGTH 	227
#define VERSION_OFFSET 	36
#define GRAPH_VERSION 	2
#define N_STAGES_OFFSET 240
#define FIRST_SHAVE_OFFSET 248
#define N_OUTPUTS_OFFSET (HEADER_LENGTH + 136)
#define X_OUT_STRIDE_OFFSET (HEADER_LENGTH + 172)

#define THERMAL_BUFFER_SIZE 100
#define DEBUG_BUFFER_SIZE 	120

#define MAX_OPTIMISATIONS 		40
#define OPTIMISATION_NAME_LEN 	50
#define OPTIMISATION_LIST_BUFFER_SIZE (MAX_OPTIMISATIONS * OPTIMISATION_NAME_LEN)

#define MAX_PATH_LENGTH 		255
#define STATUS_WAIT_TIMEOUT     15

static int initialized = 0;
static pthread_mutex_t mm = PTHREAD_MUTEX_INITIALIZER;

int mvnc_loglevel = 0;

/////////////////////////// Structs /////////////////////////////
struct Graph;

struct Device {
	int backoff_time_normal, backoff_time_high, backoff_time_critical;
	int temperature_debug, throttle_happened;
	float temp_lim_upper, temp_lim_lower;
	float *thermal_stats;
	char *dev_addr;		// Device USB address as returned by usb_
	char *dev_file;		// Device filename in /dev directory
	char *optimisation_list;
	void *usb_link;
	struct Device *next;	// Next device in chain
	struct Graph *graphs;	// List of associated graphs
	pthread_mutex_t mm;
} *devices;

struct Graph {
	int started;
	int have_data;
	int dont_block;
	int input_idx;
	int output_idx;
	int failed;
	int iterations;
	int network_throttle;
	unsigned noutputs;
	unsigned nstages;
	struct Device *dev;
	struct Graph *next;
	char *aux_buffer;
	char *debug_buffer;
	float *time_taken;
	void *user_param[2];
	void *output_data;
};

static double time_in_seconds()
{
	static double s;
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	if (!s)
		s = ts.tv_sec + ts.tv_nsec * 1e-9;
	return ts.tv_sec + ts.tv_nsec * 1e-9 - s;
}

static void initialize()
{
	// We sanitize the situation by trying to reset the devices that have been left open
	initialized = 1;
	usblink_resetall();
}

mvncStatus mvncGetDeviceName(int index, char *name, unsigned int nameSize)
{
	if (index < 0 || !name || nameSize < MVNC_MAX_NAME_SIZE)
		return MVNC_INVALID_PARAMETERS;

	pthread_mutex_lock(&mm);
	if (!initialized)
		initialize();
	int rc = usb_find_device(index, name, nameSize, 0, 0, 0);
	pthread_mutex_unlock(&mm);

	return rc;
}

static int is_device_opened(const char *name)
{
	struct Device *d = devices;
	while (d) {
		if (strcmp(d->dev_addr, name) == 0)
			return 0;
		d = d->next;
	}
	return -1;
}

static mvncStatus load_fw_file(const char *name)
{
	int rc;
	FILE *fp;
	char *tx_buf;
	unsigned file_size;
	char mv_cmd_file[MAX_PATH_LENGTH], *p;

	// Search the mvnc executable in the same directory of this library, under mvnc
	Dl_info info;
	dladdr(mvncOpenDevice, &info);
	strncpy(mv_cmd_file, info.dli_fname, sizeof(mv_cmd_file) - 40);
	p = strrchr(mv_cmd_file, '/');
	if (p)
		strcpy(p + 1, "mvnc/MvNCAPI.mvcmd");
	else
		strcpy(mv_cmd_file, "mvnc/MvNCAPI.mvcmd");

	// Load the mvnc executable
	fp = fopen(mv_cmd_file, "rb");
	if (fp == NULL) {
		if (mvnc_loglevel)
			perror(mv_cmd_file);
		pthread_mutex_unlock(&mm);
		return MVNC_MVCMD_NOT_FOUND;
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);
	if (!(tx_buf = malloc(file_size))) {
		if (mvnc_loglevel)
			perror("buffer");
		fclose(fp);
		pthread_mutex_unlock(&mm);
		return MVNC_OUT_OF_MEMORY;
	}

	if (fread(tx_buf, 1, file_size, fp) != file_size) {
		if (mvnc_loglevel)
			perror(mv_cmd_file);
		fclose(fp);
		free(tx_buf);
		pthread_mutex_unlock(&mm);
		return MVNC_MVCMD_NOT_FOUND;
	}
	fclose(fp);

	// Boot it
	rc = usb_boot(name, tx_buf, file_size);
	free(tx_buf);
	if (rc) {
		pthread_mutex_unlock(&mm);
		return rc;
	}

	PRINT_DEBUG(stderr, "Boot successful, device address %s\n", name);
	return MVNC_OK;
}

static void allocate_device(const char* name, void **deviceHandle, void* f)
{
	struct Device *d = calloc(1, sizeof(*d));
	d->dev_addr = strdup(name);
	d->usb_link = f;
	d->next = devices;
	d->temp_lim_upper = 95;
	d->temp_lim_lower = 85;
	d->backoff_time_normal = 0;
	d->backoff_time_high = 100;
	d->backoff_time_critical = 10000;
	d->temperature_debug = 0;
	pthread_mutex_init(&d->mm, 0);
	devices = d;
	*deviceHandle = d;

	PRINT_DEBUG(stderr, "done\n");
	PRINT_INFO(stderr, "Booted %s -> %s\n",
		   d->dev_addr,
		   d->dev_file ? d->dev_file : "VSC");
}

mvncStatus mvncOpenDevice(const char *name, void **deviceHandle)
{
	int rc;
	char name2[MVNC_MAX_NAME_SIZE] = "";
	char* device_name;
	char* saved_name = NULL;
	char* temp = NULL; //save to be able to free memory
	int second_name_available = 0;

	if (!name || !deviceHandle)
		return MVNC_INVALID_PARAMETERS;

	temp = saved_name = strdup(name);

	device_name = strtok_r(saved_name, ":", &saved_name);
	if (device_name == NULL) {
		free(temp);
		return MVNC_INVALID_PARAMETERS;
	}

	pthread_mutex_lock(&mm);
	if (!initialized)
		initialize();


	rc = load_fw_file(device_name);
	if (rc != MVNC_OK) {
		free(temp);
		return rc;
	}
	if (saved_name && strlen(saved_name) > 0) {
		device_name = strtok_r(NULL, ":", &saved_name);
		second_name_available = 1;
	}

	// Now we should have a new /dev/ttyACM, try to open it
	double waittm = time_in_seconds() + STATUS_WAIT_TIMEOUT;
	while (time_in_seconds() < waittm) {
		void *f = usblink_open(device_name);

		//we might fail in case name changed after boot and we don't have it
		if (f == NULL && !second_name_available) {
			int count = 0;
			while (1) {
				name2[0] = '\0';
				rc = usb_find_device(count, name2,
						     sizeof(name2), NULL,
						     DEFAULT_OPEN_VID,
						     DEFAULT_OPEN_PID);
				if (rc < 0)	//Error or no more devices found
					break;

				//check if we already have name2 open
				// if not, check if it's not already busy
				if (is_device_opened(name2) < 0 &&
				    (f = usblink_open(name2)))
					break;
				count++;
			}
		}

		if (f) {
			myriadStatus_t status;

			if (!usblink_getmyriadstatus(f, &status) && status == MYRIAD_WAITING) {
				allocate_device(strlen(name2) > 0 ? name2 : device_name, deviceHandle, f);
				free(temp);
				pthread_mutex_unlock(&mm);
				return MVNC_OK;
			} else {
				PRINT_DEBUG(stderr,
					    "found, but cannot get status\n");
				usblink_close(f);
			}
		}
		// Error opening it, continue searching
		usleep(10000);
	}
	free(temp);
	pthread_mutex_unlock(&mm);
	return MVNC_ERROR;
}

static int find_device(void *deviceHandle)
{
	struct Device *d = devices;

	while (d) {
		if (d == deviceHandle)
			return 0;
		d = d->next;
	}

	return -1;
}

static int find_graph(void *graphHandle)
{
	struct Device *d = devices;

	while (d) {
		struct Graph *g = d->graphs;
		while (g) {
			if (g == graphHandle)
				return 0;
			g = g->next;
		}
		d = d->next;
	}

	return -1;
}

// Defined here as it will be used twice
static int deallocate_graph(struct Graph *g)
{
	int found = 0;

	// Remove it from the list of the associated device
	if (g->dev->graphs == g) {
		g->dev->graphs = g->next;
		found = 1;
	} else {
		struct Graph *gp = g->dev->graphs;
		while (gp->next) {
			if (gp->next == g) {
				found = 1;
				gp->next = gp->next->next;
				break;
			}
			gp = gp->next;
		}
	}

	// Free it with all its data
	if (found) {
		free(g->aux_buffer);
		free(g->output_data);
		g->dev->thermal_stats = 0;
		free(g);
	}

	return -!found;
}

mvncStatus mvncCloseDevice(void *deviceHandle)
{
	int found = 0;

	if (!deviceHandle)
		return MVNC_INVALID_PARAMETERS;

	pthread_mutex_lock(&mm);
	if (find_device(deviceHandle)) {
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}

	struct Device *d = (struct Device *) deviceHandle;
	// Remove it from our list
	if (devices == d) {
		devices = d->next;
		found = 1;
	} else {
		struct Device *dp = devices;
		while (dp->next) {
			if (dp->next == d) {
				found = 1;
				dp->next = dp->next->next;
				break;
			}
			dp = dp->next;
		}
	}

	if (!found) {
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}
	// Deallocate all associated graphs
	pthread_mutex_lock(&d->mm);
	while (d->graphs)
		deallocate_graph(d->graphs);

	// Reset
	usblink_resetmyriad(d->usb_link);
	usblink_close(d->usb_link);
	if (d->optimisation_list)
		free(d->optimisation_list);

	free(d->dev_addr);
	free(d->dev_file);
	pthread_mutex_unlock(&d->mm);
	pthread_mutex_destroy(&d->mm);
	free(d);
	pthread_mutex_unlock(&mm);

	usleep(500000);
	return MVNC_OK;
}

static unsigned read_32bits(const unsigned char *ptr)
{
	return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}


mvncStatus mvncAllocateGraph(void *deviceHandle, void **graphHandle,
                              const void *graphFile, unsigned int graphFileLength)
{
	if (!deviceHandle || !graphHandle || !graphFile)
		return MVNC_INVALID_PARAMETERS;

	if (graphFileLength < HEADER_LENGTH + STAGE_LENGTH ||
	    graphFileLength > 512 * 1024 * 1024)
		return MVNC_UNSUPPORTED_GRAPH_FILE;

	unsigned char *graph = (unsigned char *) graphFile;
	if (graph[VERSION_OFFSET] != GRAPH_VERSION)
		return MVNC_UNSUPPORTED_GRAPH_FILE;

	unsigned nstages = graph[N_STAGES_OFFSET] + (graph[N_STAGES_OFFSET + 1] << 8);
	unsigned noutputs = read_32bits(graph + N_OUTPUTS_OFFSET +
                                    (nstages - 1) * STAGE_LENGTH) *
						read_32bits(graph + N_OUTPUTS_OFFSET +
                                    (nstages - 1) * STAGE_LENGTH + 4) *
						read_32bits(graph + X_OUT_STRIDE_OFFSET +
                                    (nstages - 1) * STAGE_LENGTH) / 2;

	// A reasonable check on graph correctness
	if (noutputs > 64 * 1024 * 1024)
		return MVNC_UNSUPPORTED_GRAPH_FILE;

	pthread_mutex_lock(&mm);
	struct Device *d = devices;
	while (d) {
		if (d == deviceHandle)
			break;
		d = d->next;
	}

	if (!d) {
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}

	if (d->graphs) {
		pthread_mutex_unlock(&mm);
		return MVNC_BUSY;
	}

	myriadStatus_t status;
	double timeout = time_in_seconds() + 10;
	do {
		if (usblink_getmyriadstatus(d->usb_link, &status)) {
			pthread_mutex_unlock(&mm);
			return MVNC_ERROR;
		}
		usleep(10000);
	} while (status != MYRIAD_WAITING && time_in_seconds() < timeout);

	if (status != MYRIAD_WAITING) {
		pthread_mutex_unlock(&mm);
		return MVNC_ERROR;
	}

	if (usblink_setdata(d->usb_link, "blobFile", graphFile, graphFileLength, 0)) {
		pthread_mutex_unlock(&mm);
		return MVNC_ERROR;
	}

	struct Graph *g = calloc(1, sizeof(*g));
	g->dev = d;
	g->nstages = nstages;
	g->noutputs = noutputs;

	// aux_buffer
	g->aux_buffer = calloc(1, 224 + nstages * sizeof(*g->time_taken));
	if (!g->aux_buffer) {
		free(g);
		pthread_mutex_unlock(&mm);
		return MVNC_OUT_OF_MEMORY;
	}

	if (usblink_setdata(g->dev->usb_link, "auxBuffer", g->aux_buffer,
			    224 + nstages * sizeof(*g->time_taken), 0)) {
		free(g->aux_buffer);
		free(g);
		pthread_mutex_unlock(&mm);
		return MVNC_ERROR;
	}

	g->debug_buffer = g->aux_buffer;
	g->time_taken = (float *) (g->aux_buffer + 224);

	// output_data
	g->output_data = calloc(noutputs, 2);
	if (!g->output_data) {
		free(g->aux_buffer);
		free(g);
		pthread_mutex_unlock(&mm);
		return MVNC_OUT_OF_MEMORY;
	}

	g->dev->thermal_stats = (float *) (g->aux_buffer + DEBUG_BUFFER_SIZE);

	g->iterations = 1;
	g->network_throttle = 1;
	if (d->graphs)
		g->next = d->graphs;
	d->graphs = g;
	*graphHandle = g;
	pthread_mutex_unlock(&mm);
	return MVNC_OK;
}

mvncStatus mvncDeallocateGraph(void *graphHandle)
{
	if (!graphHandle)
		return MVNC_INVALID_PARAMETERS;

	pthread_mutex_lock(&mm);
	if (find_graph(graphHandle)) {
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}

	struct Device *d = ((struct Graph *) graphHandle)->dev;

	pthread_mutex_lock(&d->mm);
	if (deallocate_graph((struct Graph *) graphHandle)) {
		pthread_mutex_unlock(&d->mm);
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}

	pthread_mutex_unlock(&d->mm);
	pthread_mutex_unlock(&mm);
	return MVNC_OK;
}

mvncStatus mvncSetGraphOption(void *graphHandle, int option, const void *data,
			      unsigned int dataLength)
{
	if (!graphHandle || !data || dataLength != 4)
		return MVNC_INVALID_PARAMETERS;

	struct Graph *g = (struct Graph *) graphHandle;
	pthread_mutex_lock(&mm);
	if (find_graph(graphHandle)) {
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}

	pthread_mutex_lock(&g->dev->mm);
	pthread_mutex_unlock(&mm);
	switch (option) {
	case MVNC_ITERATIONS:
		g->iterations = *(int *) data;
		break;
	case MVNC_NETWORK_THROTTLE:
		g->network_throttle = *(int *) data;
		break;
	case MVNC_DONT_BLOCK:
		g->dont_block = *(int *) data;
		break;
	default:
		pthread_mutex_unlock(&g->dev->mm);
		return MVNC_INVALID_PARAMETERS;
	}

	pthread_mutex_unlock(&g->dev->mm);
	return MVNC_OK;
}

mvncStatus mvncGetGraphOption(void *graphHandle, int option, void *data,
			      unsigned int *dataLength)
{
	if (!graphHandle || !data || !dataLength)
		return MVNC_INVALID_PARAMETERS;

	struct Graph *g = (struct Graph *) graphHandle;
	pthread_mutex_lock(&mm);
	if (find_graph(graphHandle)) {
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}

	pthread_mutex_lock(&g->dev->mm);
	pthread_mutex_unlock(&mm);
	switch (option) {
	case MVNC_ITERATIONS:
		*(int *) data = g->iterations;
		*dataLength = sizeof(int);
		break;
	case MVNC_NETWORK_THROTTLE:
		*(int *) data = g->network_throttle;
		*dataLength = sizeof(int);
		break;
	case MVNC_DONT_BLOCK:
		*(int *) data = g->dont_block;

		*dataLength = sizeof(int);
		break;
	case MVNC_TIME_TAKEN:
		*(float **) data = g->time_taken;
		*dataLength = sizeof(*g->time_taken) * g->nstages;
		break;
	case MVNC_DEBUG_INFO:
		*(char **) data = g->debug_buffer;
		*dataLength = DEBUG_BUFFER_SIZE;
		break;
	default:
		pthread_mutex_unlock(&g->dev->mm);
		return MVNC_INVALID_PARAMETERS;
	}

	pthread_mutex_unlock(&g->dev->mm);
	return MVNC_OK;
}

mvncStatus mvncSetGlobalOption(int option, const void *data,
			       unsigned int dataLength)
{
	if (!data || dataLength != 4)
		return MVNC_INVALID_PARAMETERS;

	switch (option) {
	case MVNC_LOG_LEVEL:
		mvnc_loglevel = *(int *) data;
		break;
	default:
		return MVNC_INVALID_PARAMETERS;
	}

	return MVNC_OK;
}

mvncStatus mvncGetGlobalOption(int option, void *data, unsigned int *dataLength)
{
	if (!data || !dataLength)
		return MVNC_INVALID_PARAMETERS;

	switch (option) {
	case MVNC_LOG_LEVEL:
		*(int *) data = mvnc_loglevel;
		*dataLength = sizeof(mvnc_loglevel);
		break;
	default:
		return MVNC_INVALID_PARAMETERS;
	}
	return MVNC_OK;
}

mvncStatus mvncSetDeviceOption(void *deviceHandle, int option, const void *data,
			       unsigned int dataLength)
{
	if (deviceHandle == 0 && option == MVNC_LOG_LEVEL) {
		PRINT("Warning: MVNC_LOG_LEVEL is not a Device Option, \
                please use mvncSetGlobalOption()!\n");
		return mvncSetGlobalOption(option, data, dataLength);
	}

	if (!deviceHandle || !data || dataLength != 4)
		return MVNC_INVALID_PARAMETERS;

	struct Device *d = (struct Device *) deviceHandle;
	pthread_mutex_lock(&mm);
	if (find_device(d)) {
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}

	pthread_mutex_lock(&d->mm);
	pthread_mutex_unlock(&mm);
	switch (option) {
	case MVNC_TEMP_LIM_LOWER:
		d->temp_lim_lower = *(float *) data;
		break;
	case MVNC_TEMP_LIM_HIGHER:
		d->temp_lim_upper = *(float *) data;
		break;
	case MVNC_BACKOFF_TIME_NORMAL:
		d->backoff_time_normal = *(int *) data;
		break;
	case MVNC_BACKOFF_TIME_HIGH:
		d->backoff_time_high = *(int *) data;
		break;
	case MVNC_BACKOFF_TIME_CRITICAL:
		d->backoff_time_critical = *(int *) data;
		break;
	case MVNC_TEMPERATURE_DEBUG:
		d->temperature_debug = *(int *) data;
		break;
	default:
		pthread_mutex_unlock(&d->mm);
		return MVNC_INVALID_PARAMETERS;
	}
	pthread_mutex_unlock(&d->mm);

	return MVNC_OK;
}

static mvncStatus get_optimisation_list(struct Device *d)
{
	int i, config[10];
	double timeout;
	myriadStatus_t status;
	char *p;

	if (d->optimisation_list)
		return MVNC_OK;

	d->optimisation_list = calloc(OPTIMISATION_LIST_BUFFER_SIZE, 1);
	if (!d->optimisation_list)
		return MVNC_OUT_OF_MEMORY;

	memset(config, 0, sizeof(config));
	config[0] = 1;
	config[1] = 1;
	if (usblink_setdata(d->usb_link, "config", config, sizeof(config), 1))
		return MVNC_ERROR;

	timeout = time_in_seconds() + STATUS_WAIT_TIMEOUT;
	do {
		if (usblink_getmyriadstatus(d->usb_link, &status))
			return MVNC_ERROR;
		usleep(10000);
	} while (status != MYRIAD_WAITING &&
		 status != MYRIAD_FINISHED && time_in_seconds() < timeout);

	if (status != MYRIAD_WAITING && status != MYRIAD_FINISHED)
		return MVNC_TIMEOUT;

	if (usblink_getdata(d->usb_link, "optimizationList",
			    d->optimisation_list, OPTIMISATION_LIST_BUFFER_SIZE, 0, 0))
		return MVNC_ERROR;

	for (i = 0; i < MAX_OPTIMISATIONS; i++) {
		p = strchr(d->optimisation_list + i * OPTIMISATION_NAME_LEN, '~');
		if (p)
			*p = 0;
	}

	config[1] = 0;
	if (usblink_setdata(d->usb_link, "config", config, sizeof(config), 0))
		return MVNC_ERROR;
	return MVNC_OK;
}

mvncStatus mvncGetDeviceOption(void *deviceHandle, int option, void *data,
			       unsigned int *dataLength)
{
	mvncStatus rc;

	if (deviceHandle == 0 && option == MVNC_LOG_LEVEL) {
		PRINT("Warning: MVNC_LOG_LEVEL is not a Device Option, \
                 please use mvncGetGlobalOption()!\n");
		return mvncGetGlobalOption(option, data, dataLength);
	}

	if (!deviceHandle || !data || !dataLength)
		return MVNC_INVALID_PARAMETERS;

	struct Device *d = (struct Device *) deviceHandle;
	pthread_mutex_lock(&mm);
	if (find_device(d)) {
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}

	pthread_mutex_lock(&d->mm);
	pthread_mutex_unlock(&mm);
	switch (option) {
	case MVNC_TEMP_LIM_LOWER:
		*(float *) data = d->temp_lim_lower;
		*dataLength = sizeof(int);
		break;
	case MVNC_TEMP_LIM_HIGHER:
		*(float *) data = d->temp_lim_upper;
		*dataLength = sizeof(int);
		break;
	case MVNC_BACKOFF_TIME_NORMAL:
		*(int *) data = d->backoff_time_normal;
		*dataLength = sizeof(int);
		break;
	case MVNC_BACKOFF_TIME_HIGH:
		*(int *) data = d->backoff_time_high;
		*dataLength = sizeof(int);
		break;
	case MVNC_BACKOFF_TIME_CRITICAL:
		*(int *) data = d->backoff_time_critical;
		*dataLength = sizeof(int);
		break;
	case MVNC_TEMPERATURE_DEBUG:
		*(int *) data = d->temperature_debug;
		*dataLength = sizeof(int);
		break;
	case MVNC_THERMAL_STATS:
		if (!d->thermal_stats) {
			pthread_mutex_unlock(&d->mm);
			return MVNC_NO_DATA;
		}
		*(float **) data = d->thermal_stats;
		*dataLength = THERMAL_BUFFER_SIZE;
		break;
	case MVNC_OPTIMISATION_LIST:
		rc = get_optimisation_list(d);
		if (rc) {
			pthread_mutex_unlock(&d->mm);
			return rc;
		}
		*(char **) data = d->optimisation_list;
		*dataLength = OPTIMISATION_LIST_BUFFER_SIZE;
		break;
	case MVNC_THERMAL_THROTTLING_LEVEL:
		*(int *) data = d->throttle_happened;
		*dataLength = sizeof(int);
		break;
	default:
		pthread_mutex_unlock(&d->mm);
		return MVNC_INVALID_PARAMETERS;
	}
	pthread_mutex_unlock(&d->mm);

	return MVNC_OK;
}

static int send_opt_data(struct Graph *g)
{
	int config[10];

	config[0] = 1;		// Version
	config[1] = 0;		// Query disable
	config[2] = g->iterations;
	config[3] = g->dev->temp_lim_upper;
	config[4] = g->dev->temp_lim_lower;
	config[5] = g->dev->backoff_time_normal;
	config[6] = g->dev->backoff_time_high;
	config[7] = g->dev->backoff_time_critical;
	config[8] = g->dev->temperature_debug;
	config[9] = g->network_throttle;

	if (usblink_setdata(g->dev->usb_link, "config", config, sizeof(config), 0))
		return MVNC_ERROR;

	return MVNC_OK;
}

mvncStatus mvncLoadTensor(void *graphHandle, const void *inputTensor,
			  unsigned int inputTensorLength, void *userParam)
{
	if (!graphHandle || !inputTensor || inputTensorLength < 2)
		return MVNC_INVALID_PARAMETERS;

	struct Graph *g = (struct Graph *) graphHandle;
	pthread_mutex_lock(&mm);
	if (find_graph(graphHandle)) {
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}

	if (!g->started) {
		if (send_opt_data(g)) {
			pthread_mutex_unlock(&mm);
			return MVNC_ERROR;
		}
		g->started = 1;
	}

	while (g->have_data == 2) {
		if (g->dont_block) {
			pthread_mutex_unlock(&mm);
			return MVNC_BUSY;
		}
		if (g->failed) {
			pthread_mutex_unlock(&mm);
			return MVNC_ERROR;
		}
		pthread_mutex_unlock(&mm);
		usleep(1000);
		pthread_mutex_lock(&mm);
		if (find_graph(g)) {
			pthread_mutex_unlock(&mm);
			return MVNC_GONE;
		}
	}
	pthread_mutex_lock(&g->dev->mm);
	pthread_mutex_unlock(&mm);

	if (usblink_setdata(g->dev->usb_link, g->input_idx ? "input2" : "input1",
	     inputTensor, inputTensorLength, g->have_data == 0)) {
		pthread_mutex_unlock(&mm);
		return MVNC_ERROR;
	}

	g->user_param[g->input_idx] = userParam;
	g->input_idx = !g->input_idx;
	g->have_data++;
	pthread_mutex_unlock(&g->dev->mm);
	return MVNC_OK;
}

mvncStatus mvncGetResult(void *graphHandle, void **outputData,
			 unsigned int *outputDataLength, void **userParam)
{
	int rc, unlock_own = 0;

	if (!graphHandle || !outputData || !outputDataLength)
		return MVNC_INVALID_PARAMETERS;

	struct Graph *g = (struct Graph *) graphHandle;
	pthread_mutex_lock(&mm);
	if (find_graph(graphHandle)) {
		pthread_mutex_unlock(&mm);
		return MVNC_INVALID_PARAMETERS;
	}

	while (!g->have_data) {
		if (g->dont_block) {
			pthread_mutex_unlock(&mm);
			return MVNC_NO_DATA;
		}
		pthread_mutex_unlock(&mm);
		usleep(1000);
		pthread_mutex_lock(&mm);
		if (find_graph(g)) {
			pthread_mutex_unlock(&mm);
			return MVNC_GONE;
		}
	}

	double timeout = time_in_seconds() + STATUS_WAIT_TIMEOUT;
	do {
		pthread_mutex_lock(&g->dev->mm);
		pthread_mutex_unlock(&mm);
		if (!usblink_getdata(g->dev->usb_link, "output", g->output_data,
				     2 * g->noutputs, 0, 0)) {
			unsigned int length = DEBUG_BUFFER_SIZE + THERMAL_BUFFER_SIZE +
			     sizeof(int) + sizeof(*g->time_taken) * g->nstages;

			if (usblink_getdata(g->dev->usb_link, "auxBuffer", g->aux_buffer,
			     length, 0, g->have_data == 2)) {
				g->failed = 1;
				pthread_mutex_unlock(&g->dev->mm);
				return MVNC_ERROR;
			}
			unlock_own = 1;
			break;
		}
		pthread_mutex_unlock(&g->dev->mm);
		usleep(1000);
		pthread_mutex_lock(&mm);
		if (find_graph(g)) {
			pthread_mutex_unlock(&mm);
			return MVNC_GONE;
		}
	} while (time_in_seconds() < timeout);

	g->dev->throttle_happened = *(int *) (g->aux_buffer + DEBUG_BUFFER_SIZE
						+ THERMAL_BUFFER_SIZE);
	*outputData = g->output_data;
	*outputDataLength = 2 * g->noutputs;
	*userParam = g->user_param[g->output_idx];
	g->output_idx = !g->output_idx;
	g->have_data--;

	if (unlock_own) {
		rc = *g->debug_buffer ? MVNC_MYRIAD_ERROR : MVNC_OK;
		if (rc)
			g->failed = 1;
		pthread_mutex_unlock(&g->dev->mm);
	} else {
		rc = MVNC_TIMEOUT;
		g->failed = 1;
		pthread_mutex_unlock(&mm);
	}

	return rc;
}
