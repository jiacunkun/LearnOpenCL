/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary 
and confidential information. 

The information and code contained in this file is only for authorized ArcSoft 
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER 
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy, 
distribute, modify, or take any action in reliance on it. 

If you have received this file in error, please immediately notify ArcSoft and 
permanently delete the original and any copy of any file and any printout 
thereof.
*******************************************************************************/
#ifndef _COMPILE_SETTING_H_
#define _COMPILE_SETTING_H_

#include "amcomdef.h"

#ifdef PLATFORM_WIN32
#define WIN32_DEBUG_LOG
#endif

#ifdef WIN32_DEBUG_LOG
#include <windows.h>
#include <stdio.h>
#define START_PROFILE()						\
	LARGE_INTEGER freq, start, end;				\
	double dTime = 0;							\
	QueryPerformanceFrequency(&freq);			\
	QueryPerformanceCounter(&start);

#define END_PROFILE(lTime)							\
	QueryPerformanceCounter(&end);						\
	dTime = (double)(end.QuadPart - start.QuadPart);	\
	lTime = 1000 * dTime / freq.QuadPart;	

#define PrintfB(prio, tag, ...)		{ \
	char info[256];  \
	sprintf(info, __VA_ARGS__); \
	OutputDebugStringA(info);\
}

#elif  defined(_ARM_TIME_)

#include <android/log.h>
#include <time.h>

#define START_PROFILE()								\
	struct timeval time1,time2;						\
	gettimeofday(&time1, NULL);

#define END_PROFILE(lTime)							\
	gettimeofday(&time2, NULL);						\
	lTime = (time2.tv_sec * 1000 + time2.tv_usec / 1000)	\
	- (time1.tv_sec * 1000 + time1.tv_usec / 1000);

#define PrintfB		__android_log_print

#else
#define START_PROFILE()
#define END_PROFILE(lTime)
#define PrintfB(...)
#endif

#endif // _COMPILE_SETTING_H_
