#if (defined WIN32) || (defined _WIN64)
#include <Windows.h>

#include "usleep.h"
#include <time.h>

void usleep(int waitTime) {
	__int64 time1 = 0, time2 = 0, freq = 0;

	QueryPerformanceCounter((LARGE_INTEGER *)&time1);
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

	do {
		QueryPerformanceCounter((LARGE_INTEGER *)&time2);
	} while ((time2 - time1) < waitTime);
}

int clock_gettime(int unused, struct timespec *tv)
{
	return timespec_get(tv, TIME_UTC);
}

#endif