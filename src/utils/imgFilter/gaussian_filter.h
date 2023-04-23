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
#ifndef	_GAUSSIAN_FILTER_H_
#define _GAUSSIAN_FILTER_H_

#include "ammem.h"
#include "merror.h"
#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
#define PRE_TASK_NUM 16 //todo:暂时加在此处，by jck

#define  PX_GAU(FUNC)			arcgau##FUNC

#define  GaussPyrDown2		PX_GAU(GaussPyrDown2)
#define  GaussPyrDown2_C2	PX_GAU(GaussPyrDown2_C2)
#define  GaussPyrDown4		PX_GAU(GaussPyrDown4)



MInt32 GaussPyrDown2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte *dstdata, MInt32 srcW, MInt32 srcH,
	MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch);

MInt32 GaussPyrDown2_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte *dstdata, MInt32 srcW, MInt32 srcH,
	MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch);

MInt32 GaussPyrDown4(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte *dstdata, MInt32 srcW, MInt32 srcH,
	MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch);

MRESULT GaussianBlur3x3(MHandle hMemMgr, MHandle mcvParallelMonitor, MUInt8* src, MInt32 width, MInt32 height, MInt32 srcPitch,
	MUInt8* dst, MInt32 dstPitch, MInt32 cn);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif  //_GAUSSIAN_FILTER_H_