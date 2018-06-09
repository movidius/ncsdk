#pragma once

#define CLOCK_REALTIME		0
#define CLOCK_MONOTONIC		1

void usleep(int waitTime);
int clock_gettime(int unused, struct timespec *tv);