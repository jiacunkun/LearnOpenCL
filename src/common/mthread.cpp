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
#include "mthread.h"
#include "imagebase.h"
#include "single_image_enhancement_define.h"
#if defined(MULTI_THREAD) || defined(QUAD_MULTI_THREAD)

#ifdef PLATFORM_WIN32
#include <process.h>
#include <Windows.h>
MHandle MThreadCreate(MThreadProc proc, MVoid* pParam)
{
	unsigned long dwThreadId;
	if (NULL == proc)
	{
		return MNull;
	}
	return (MHandle)CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)proc, pParam, 0, &dwThreadId);
}
MRESULT		MThreadDestory(MHandle hThread)
{
	return MThreadExit(hThread);
}
MRESULT		MThreadExit(MHandle hThread)
{
	if (NULL != hThread)
	{
		CloseHandle(hThread);
	}
	return MOK;
}
#elif defined PLATFORM_LINUX
#include <pthread.h>
#include <unistd.h>
MHandle 	MThreadCreate(MThreadProc proc,  MVoid* pParam)
{
	if(MNull == proc)
		return MNull;
	
	pthread_t pthread;
	MInt32 nRe = pthread_create(&pthread, MNull, proc, pParam);
	return nRe ? MNull : (MVoid*)pthread;
}
MRESULT		MThreadExit(MHandle hThread)
{
	pthread_t pthread;
	if(MNull != hThread) 
	{
		pthread = (pthread_t)hThread;
		pthread_join(pthread,MNull);
	}
	return MOK;
}
MRESULT		MThreadDestory(MHandle hThread)
{
	return MThreadExit(hThread);
}
#endif

#endif	// MULTI_THREAD
