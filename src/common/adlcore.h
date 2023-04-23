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
#ifndef _ADL_CORE_H_
#define _ADL_CORE_H_

#include "adlbase.h"
#include "merror.h"

// ====================================== //
// ====== Configuratino parameters ====== //
// ====================================== //
#if defined(__WIN32__) && defined(_DEBUG)
//	#define ADL_USE_DEBUG
	#include <stdio.h>
#endif

#ifdef _DEBUG
#define DEBUG_OUT
#endif // _DEBUG


#ifndef MAX
#  define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

#ifndef MIN
#  define MIN(a,b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef	ABS
#define ABS(a)		(((a) < 0) ? -(a) : (a))
#endif

#ifndef TRIMBYTE
#define TRIMBYTE(x)	(MUInt8)((x)<0?0:((x)>255?255:(x)))
#endif

#define OUTPUT_PATH		"G:/MFNS_Debug/"


#endif	// _ADL_CORE_H_
