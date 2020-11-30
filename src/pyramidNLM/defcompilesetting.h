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

// ******************* Platform setting ***************** //
//#define _COMPILE_WIN32_
//#define _COMPILE_ADS_
//#define _COMPILE_COACH_
//#define _COMPILE_ARM_
//#define _COMPILE_FUJITSU_
//#define _COMPILE_ANDROID_
//#define _COMPILE_DRIME3_
//#define _ANDROID_PROFILE_
//#define _WIN32_PROFILE_
// ******************* Version Setting ****************** //
//#define _COMPILE_DEBUG_VERSION_

// ******************** Debug option ******************** //

#if defined(_COMPILE_ANDROID_) && defined(_COMPILE_DEBUG_VERSION_)

#include <log.h>

#define	 Printf						LOGV

#endif

#if defined(_COMPILE_COACH_) && defined(_COMPILE_DEBUG_VERSION_)

#ifdef __cplusplus
extern "C" {
#endif

void Printf_B(const char *fmt,...);

unsigned long gulChipFreq;
unsigned int _tx_counter_get(void);
#define GET_COUNTER_FREQ_MHZ()  (gulChipFreq / 2000000)

#ifdef __cplusplus
}
#endif

#define  Printf						Printf_B
#define  GetTime					_tx_counter_get

#endif

#if defined(_COMPILE_FUJITSU_) && defined(_COMPILE_DEBUG_VERSION_)

extern MUInt32 GET_TICK_COUNT(MVoid);
extern MVoid FUJI_SubCore_printf(MUInt32 *parm);

#define	 Printf						printf

#endif

//#define  _ARM_TIME_

#ifdef _ANDROID_PROFILE_
#include <android/log.h>
#include <time.h>

#define START_PROFILE()								\
	struct timeval time1,time2;						\
	gettimeofday(&time1, NULL);

#define END_PROFILE(lTime)							\
	gettimeofday(&time2, NULL);						\
	lTime = (time2.tv_sec*1000+time2.tv_usec/1000)	\
		  - (time1.tv_sec*1000+time1.tv_usec/1000);

#define PrintfB		__android_log_print
//#define PrintfB

#define PrintfB_CUST	__android_log_print

#elif defined _WIN32_PROFILE_

#include <stdio.h>
#include <windows.h>

#define START_PROFILE()								\
	LARGE_INTEGER start, end, freq;					\
	QueryPerformanceFrequency(&freq);				\
	QueryPerformanceCounter(&start);


#define END_PROFILE(lTime)							\
	QueryPerformanceCounter(&end);					\
	double etime = 0;								\
	etime = (double)(end.QuadPart - start.QuadPart);\
	etime /= freq.QuadPart;                         \
	lTime = etime*1000;

#define PrintfB(a,b,c,d)                          \
{   char info[256];                               \
	sprintf(info, c, d);                          \
	OutputDebugStringA(info);}

#define PrintfB_CUST(a,b,c,d)                     \
{   char info[256];                               \
	sprintf(info, c, d);                          \
	OutputDebugStringA(info);}

#else

#define START_PROFILE()
#define END_PROFILE(lTime)
#define PrintfB
#define PrintfB_CUST

#endif // _ANDROID_PROFILE_


#endif // _COMPILE_SETTING_H_
