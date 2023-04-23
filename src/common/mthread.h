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
#ifndef _M_THREAD_H_
#define _M_THREAD_H_

#include "adlcore.h"
#define PX(FUN)		adl##FUN

#ifdef PLATFORM_LINUX
#include <sys/syscall.h>
#define CPU_SETSIZE       1024
#define __NCPUBITS        (8 * sizeof (unsigned long))
typedef struct
{
	unsigned long __bits[CPU_SETSIZE / __NCPUBITS];
} cpu_set_t;

#define CPU_SET(cpu, cpusetp) \
((cpusetp)->__bits[(cpu)/__NCPUBITS] |= (1UL << ((cpu) % __NCPUBITS)))
#define CPU_ZERO(cpusetp) \
memset((cpusetp), 0, sizeof(cpu_set_t))

static int
sched_setaffinity(pid_t pid, size_t len, cpu_set_t const * cpusetp)
{
	return syscall(__NR_sched_setaffinity, pid, len, cpusetp);
}
#endif 

#define MEventCreate		PX(MEventCreate)
#define MEventDestroy		PX(MEventDestroy)
#define MEventWait			PX(MEventWait)
#define MEventSignal		PX(MEventSignal)
#define MThreadCreate		PX(MThreadCreate)
#define MThreadExit			PX(MThreadExit)
#define MThreadDestory		PX(MThreadDestory)

#define MWAIT_INFINITE		(~0)

MHandle		MEventCreate(MBool bAutoReset);
MRESULT		MEventDestroy(MHandle hEvent);
MRESULT		MEventWait(MHandle hEvent, MDWord dwTimeOut);
MRESULT		MEventSignal(MHandle hEvent);
#ifdef PLATFORM_WIN32
typedef		MDWord (*MThreadProc)(MVoid* lpPara);
MHandle MThreadCreate(MThreadProc proc, MVoid* pParam);
#elif defined PLATFORM_LINUX
typedef		MVoid* MPVoidProc(MVoid* lpPara); 
MHandle 	MThreadCreate(MPVoidProc proc, MVoid* pParam);
#endif
MRESULT		MThreadExit(MHandle hThread);
MRESULT		MThreadDestory(MHandle hThread);

#ifdef PLATFORM_FUJITSU
extern MBool ArcSendMessage(MHandle pMessage, MUInt32 *parm);
extern MBool ArcReceiveMessage(MHandle pMessage, MUInt32 *parm);
extern MBool ArcWaitForEventFlag(MHandle pEventFlag);
#endif

#endif //_M_THREAD_H_
