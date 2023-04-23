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
#include "arcsoft_guided_filter.h"
#include "guided_filter_common.h"
#include "guided_filter_imgproc.h"
#include "guided_filter.h"

#include "merror.h"
#include "ammem.h"
#include "ArcsoftLog.h"

#include <stdio.h>
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
MRESULT ARC_GuidedFilter_Init(MHandle hMemMgr, MHandle *phEngine, MInt32 gfMode)
{
	ArcGuidedFilter *pEngine = MNull;
	ARCGF_PARAM* pParam = MNull;

	if (phEngine == MNull)
		return MERR_INVALID_PARAM;
	if (gfMode != ARCGF_GUIDED_FILTER &&
		gfMode != ARCGF_FAST_GUIDED_FILTER)
		return MERR_INVALID_PARAM;

	pEngine = (ArcGuidedFilter *)MMemAlloc(hMemMgr, sizeof(ArcGuidedFilter));
	if (!pEngine) return MERR_NO_MEMORY;

	MMemSet(pEngine, 0, sizeof(ArcGuidedFilter));
	pEngine->hMemMgr = hMemMgr;
	pEngine->gfMode = gfMode;

	*phEngine = pEngine;
	LOGD("========== ArcGuidedFilter_Init   Finished ==========");
	return MOK;
}

MRESULT ARC_GuidedFilter_Uninit(MHandle hMemMgr, MHandle *phEngine)
{
	ArcGuidedFilter *pEngine;

	if (MNull == phEngine || MNull == *phEngine)
		return MOK;

	pEngine = (ArcGuidedFilter *)(*phEngine);

	// free data
	MMemFree_GuidedFilter(pEngine);

	MMemFree(hMemMgr, pEngine);
	phEngine = MNull;
	LOGD("========== ArcGuidedFilter_Uninit Finished ==========");
	return MOK;
}

MRESULT ARC_GuidedFilter_GetDefaultParam(LP_ARCGF_PARAM pParam)
{
	if (!pParam) return MERR_INVALID_PARAM;

	MMemSet(pParam, 0, sizeof(ARCGF_PARAM));

	pParam->gfRadius = 5;         // 11 x 11
	pParam->gfEpsilon = 650.25f;  // (0.1 * 255)^2
	pParam->gfScale = 4;          // 4x scale

	return MOK;
}

MRESULT ARC_GuidedFilter_Create(MHandle threadEngine, MHandle phEngine, 
	const LPASVLOFFSCREEN guidance, const LP_ARCGF_PARAM pParam, MInt32 srcCnNum)
{
	MRESULT res = MOK;

	ArcGuidedFilter *pEngine = MNull;
	MHandle hMemMgr = MNull;

	if (MNull == phEngine) 
		return MERR_INVALID_PARAM;
	if (MNull == pParam) 
		return MERR_INVALID_PARAM;

	if (!guidance || srcCnNum <= 0)
		return MERR_INVALID_PARAM;
	MUInt32 guidancePixelFormat = guidance->u32PixelArrayFormat;
	if (ASVL_PAF_RGB24_B8G8R8 != guidancePixelFormat && ASVL_PAF_GRAY != guidancePixelFormat)
		return MERR_UNSUPPORTED;
	MUInt8 *guidanceData = guidance->ppu8Plane[0];
	MInt32 height = guidance->i32Height, width = guidance->i32Width;
	MInt32 guidancePitch = guidance->pi32Pitch[0];
	if (!guidanceData || height <= 0 || width <= 0 || guidancePitch <= 0 ||
		pParam->gfEpsilon <= 0 || pParam->gfRadius <= 0)
		return MERR_INVALID_PARAM;

	pEngine = (ArcGuidedFilter*)phEngine;
	pEngine->gfRadius = pParam->gfRadius;
	pEngine->gfEpsilon = pParam->gfEpsilon;
	pEngine->gfScale = pParam->gfScale;

	MInt32 channels = guidancePixelFormat == ASVL_PAF_GRAY ? 1 : 3;
	pEngine->height = height;
	pEngine->width = width;
	pEngine->guidanceCnNum = channels;
	pEngine->srcCnNum = srcCnNum;

	// alloc memory according to the input guidance info
	res = MMemAlloc_GuidedFilter(pEngine);
	CHECK_ERROR(res);

	// create guided filter according to the input and mode
	if (pEngine->gfMode == ARCGF_GUIDED_FILTER)
	{
		res = guidedFilter_Create(threadEngine, GF_CREATE_TASK_NUM,
			pEngine, guidance->ppu8Plane[0], guidancePitch, height, width, channels);
		CHECK_ERROR(res);
	}
	else if (pEngine->gfMode == ARCGF_FAST_GUIDED_FILTER)
	{
		res = fastGuidedFilter_Create(threadEngine, GF_CREATE_TASK_NUM,
			pEngine, guidance->ppu8Plane[0], guidancePitch, height, width, channels);
		CHECK_ERROR(res);
	}

exit:
	if (res != MOK)
    {
        LOGD(".......... ArcGuidedFilter_Filtering Finished ..........");
    }
	else
    {
        LOGD(".......... ArcGuidedFilter_Create Finished ..........");
    }

	return res;
}

MRESULT ARC_GuidedFilter_Filter(MHandle threadEngine, MHandle phEngine, const LPASVLOFFSCREEN src, LPASVLOFFSCREEN dst)
{
	MRESULT res = MOK;

	ArcGuidedFilter *pEngine = MNull;
	MHandle hMemMgr = MNull;

	if (MNull == phEngine)
		return MERR_INVALID_PARAM;

	if (!src || !dst)
		return MERR_INVALID_PARAM;
	MUInt32 srcPixelFormat = src->u32PixelArrayFormat, dstPixelFormat = dst->u32PixelArrayFormat;
	if (ASVL_PAF_RGB24_B8G8R8 != srcPixelFormat && ASVL_PAF_GRAY != srcPixelFormat &&
		ASVL_PAF_RGB24_B8G8R8 != dstPixelFormat && ASVL_PAF_GRAY != dstPixelFormat)
		return MERR_UNSUPPORTED;
	if (srcPixelFormat != dstPixelFormat)
		return MERR_INVALID_PARAM;
	MUInt8 *srcData = src->ppu8Plane[0], *dstData = dst->ppu8Plane[0];
	MInt32 srcHeight = src->i32Height, srcWidth = src->i32Width,
		dstHeight = dst->i32Height, dstWidth = dst->i32Width;
	MInt32 srcPitch = src->pi32Pitch[0], dstPitch = dst->pi32Pitch[0];
	if (!srcData || !dstData || srcHeight <= 0 || srcWidth <= 0 ||
		dstHeight <= 0 || dstWidth <= 0 || srcPitch <= 0 || dstPitch <= 0)
		return MERR_INVALID_PARAM;
	if (srcHeight != dstHeight || srcWidth != dstWidth || srcPitch != dstPitch)
		return MERR_INVALID_PARAM;
	pEngine = (ArcGuidedFilter*)phEngine;

	MInt32 channels = srcPixelFormat == ASVL_PAF_GRAY ? 1 : 3;

	// apply guided filter according to the input and mode
	if (pEngine->gfMode == ARCGF_GUIDED_FILTER)
	{
		res = guidedFilter_Filtering(threadEngine, GF_FILTER_TASK_NUM,
                                     pEngine, srcData, dstData, srcPitch, srcHeight, srcWidth, channels);
		CHECK_ERROR(res);
	}
	else if (pEngine->gfMode == ARCGF_FAST_GUIDED_FILTER)
	{
		res = fastGuidedFilter_Filtering(threadEngine, GF_FILTER_TASK_NUM,
                                         pEngine, srcData, dstData, srcPitch, srcHeight, srcWidth, channels);
		CHECK_ERROR(res);
	}

exit:
	if (res != MOK)
    {
        LOGD(".......... ArcGuidedFilter_Filtering Failed, ErrorCode(%d) ..........", res);
    }
	else
    {
        LOGD(".......... ArcGuidedFilter_Filtering Finished ..........");
    }
	return res;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END
