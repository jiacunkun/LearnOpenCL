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
#include "defcompilesetting.h"

#include "merror.h"
#include "ammem.h"

#include <stdio.h>

MRESULT ARC_GuidedFilter_Init(MHandle hMemMgr, MHandle *phEngine, MInt32 gfMode)
{
	ARCGF_PARAM* pParam = MNull;

	if (phEngine == MNull)
		return MERR_INVALID_PARAM;
	if (gfMode != ARCGF_GUIDED_FILTER &&
		gfMode != ARCGF_FAST_GUIDED_FILTER &&
		gfMode != ARCGF_FAST_GUIDED_FILTER_C1)
		return MERR_INVALID_PARAM;

	if (gfMode != ARCGF_FAST_GUIDED_FILTER_C1)
	{
		ArcGuidedFilter* pEngine = (ArcGuidedFilter *)MMemAlloc(hMemMgr, sizeof(ArcGuidedFilter));
		if (!pEngine) return MERR_NO_MEMORY;

		MMemSet(pEngine, 0, sizeof(ArcGuidedFilter));
		pEngine->hMemMgr = hMemMgr;
		pEngine->gfMode = gfMode;

		*phEngine = pEngine;
	}
	else
	{
		ArcGuidedFilter_C1* pEngine = (ArcGuidedFilter_C1 *)MMemAlloc(hMemMgr, sizeof(ArcGuidedFilter_C1));
		if (!pEngine) return MERR_NO_MEMORY;

		MMemSet(pEngine, 0, sizeof(ArcGuidedFilter_C1));
		pEngine->hMemMgr = hMemMgr;
		pEngine->gfMode = gfMode;

		*phEngine = pEngine;
	}

	PrintfB(6, "PicZoom", "========== ArcGuidedFilter_Init   Finished ==========\n");
	return MOK;
}

MRESULT ARC_GuidedFilter_Uninit(MHandle *phEngine, MInt32 gfMode)
{
	MHandle hMemMgr;

	if (MNull == phEngine || MNull == *phEngine)
		return MOK;

	if (gfMode != ARCGF_FAST_GUIDED_FILTER_C1)
	{
		ArcGuidedFilter *pEngine = (ArcGuidedFilter *)(*phEngine);
		hMemMgr = pEngine->hMemMgr;

		// free data
		MMemFree_GuidedFilter(pEngine);

		MMemFree(hMemMgr, pEngine);
		phEngine = MNull;
	}
	else
	{
		ArcGuidedFilter_C1 *pEngine = (ArcGuidedFilter_C1 *)(*phEngine);
		hMemMgr = pEngine->hMemMgr;

		if (pEngine->pUCHAR_Buffer)
		{
			MMemFree(hMemMgr, pEngine->pUCHAR_Buffer);
			pEngine->pUCHAR_Buffer = MNull;
		}
		if (pEngine->pUSHORT_Buffer)
		{
			MMemFree(hMemMgr, pEngine->pUSHORT_Buffer);
			pEngine->pUSHORT_Buffer = MNull;
		}
		if (pEngine->pFLOAT_Buffer)
		{
			MMemFree(hMemMgr, pEngine->pFLOAT_Buffer);
			pEngine->pFLOAT_Buffer = MNull;
		}
		if (pEngine->pINT_Buffer)
		{
			MMemFree(hMemMgr, pEngine->pINT_Buffer);
			pEngine->pINT_Buffer = MNull;
		}

		MMemFree(hMemMgr, pEngine);
		phEngine = MNull;
	}
	PrintfB(6, "PicZoom", "========== ArcGuidedFilter_Uninit Finished ==========\n");
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

MRESULT ARC_GuidedFilter_Create(MHandle threadEngine, MHandle phEngine, MInt32 gfMode,
	const LPASVLOFFSCREEN guidance, const LP_ARCGF_PARAM pParam, MInt32 srcCnNum)
{
	MRESULT res = MOK;

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

	if (ARCGF_FAST_GUIDED_FILTER_C1 != gfMode)
	{
		ArcGuidedFilter *pEngine = (ArcGuidedFilter*)phEngine;
		MInt32 channels = guidancePixelFormat == ASVL_PAF_GRAY ? 1 : 3;
		pEngine->gfRadius = pParam->gfRadius;
		pEngine->gfEpsilon = pParam->gfEpsilon;
		pEngine->gfScale = pParam->gfScale;

		pEngine->height = height;
		pEngine->width = width;
		pEngine->guidanceCnNum = channels;
		pEngine->srcCnNum = srcCnNum;

		// alloc memory according to the input guidance info
		res = MMemAlloc_GuidedFilter(pEngine);
		CHECK_ERROR(res);

		// create guided filter according to the input and mode
		if (pEngine->gfMode == ARCGF_GUIDED_FILTER )//|| 1.0 == pEngine->gfScale
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
	}
	else
	{
		ArcGuidedFilter_C1 *pEngine = (ArcGuidedFilter_C1*)phEngine;
		MInt32 channels = 1;
		pEngine->gfRadius = pParam->gfRadius;
		pEngine->gfEpsilon = pParam->gfEpsilon;
		pEngine->gfScale = pParam->gfScale;

		pEngine->pGuidanceCnOri = guidanceData;
		pEngine->lGuidCnOriPitch = guidancePitch;

		pEngine->height = height;
		pEngine->width = width;
		pEngine->guidanceCnNum = channels;
		pEngine->srcCnNum = srcCnNum;

		res = fastGuidedFilter_C1_Create(threadEngine, GF_CREATE_TASK_NUM, pEngine, guidanceData, guidancePitch, height, width, 1);
		CHECK_ERROR(res);
	}

exit:
	if (res != MOK)
		PrintfB(6, "PicZoom", ".......... ArcGuidedFilter_Create Failed, ErrorCode(%d) ..........\n", res);
	else
		PrintfB(6, "PicZoom", ".......... ArcGuidedFilter_Create Finished ..........\n");
	return res;
}

MRESULT ARC_GuidedFilter_Filter(MHandle threadEngine, MHandle phEngine, MInt32 gfMode, const LPASVLOFFSCREEN src, LPASVLOFFSCREEN dst)
{
	MRESULT res = MOK;

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

	if (ARCGF_FAST_GUIDED_FILTER_C1 != gfMode)
	{
		ArcGuidedFilter *pEngine = (ArcGuidedFilter*)phEngine;

		MInt32 channels = srcPixelFormat == ASVL_PAF_GRAY ? 1 : 3;

		// apply guided filter according to the input and mode
		if (pEngine->gfMode == ARCGF_GUIDED_FILTER)
		{
			res = guidedFilter_Filter(threadEngine, GF_FILTER_TASK_NUM,
				pEngine, srcData, dstData, srcPitch, srcHeight, srcWidth, channels);
			CHECK_ERROR(res);
		}
		else if (pEngine->gfMode == ARCGF_FAST_GUIDED_FILTER)
		{
			res = fastGuidedFilter_Filter(threadEngine, GF_FILTER_TASK_NUM,
				pEngine, srcData, dstData, srcPitch, srcHeight, srcWidth, channels);
			CHECK_ERROR(res);
		}
	}
	else
	{
		ArcGuidedFilter_C1 *pEngine = (ArcGuidedFilter_C1 *)phEngine;

		pEngine->pSrcCnOri = srcData;
		pEngine->lSrcCnOriPitch = srcPitch;

		res = fastGuidedFilter_C1_Filter(threadEngine, GF_FILTER_TASK_NUM, pEngine, srcData, srcPitch,
										 dstData, dstPitch, srcHeight, srcWidth, 1);
	}

exit:
	if (res != MOK)
		PrintfB(6, "PicZoom", ".......... ArcGuidedFilter_Filter Failed, ErrorCode(%d) ..........\n", res);
	else
		PrintfB(6, "PicZoom", ".......... ArcGuidedFilter_Filter Finished ..........\n");
	return res;
}




MRESULT ARC_Image_GuidedFilter(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pGuideImg,
							   MInt32 lGRadius, MInt32 lGScale, MFloat fEpsilon)
{
	MInt32 lret = MOK;
	ARCGF_PARAM pParam;
	MHandle algorithmEngine = MNull;
	MInt32 gfMode = ARCGF_FAST_GUIDED_FILTER;
	ASVLOFFSCREEN guideImg, maskImg, binImg;
	MBool bDstMem = MFalse;
	ARC_GuidedFilter_GetDefaultParam(&pParam);
	pParam.gfRadius = lGRadius;
	pParam.gfEpsilon = fEpsilon * 255 * 255;
	pParam.gfScale = lGScale;

	//if (pSrcImg->i32Width * pSrcImg->i32Height < 10 * 1024 * 1024)
	//{
	//	pParam.gfRadius = 6;
	//	pParam.gfEpsilon = 0.005f * 255 * 255;
	//}

	if (pDstImg == MNull)
	{
		bDstMem = MTrue;
		pDstImg = pSrcImg;
	}
	else if (pSrcImg == pDstImg ||
		     pSrcImg->ppu8Plane[0] == pDstImg->ppu8Plane[0])
	{
		bDstMem = MTrue;
	}

	ARC_GuidedFilter_Init(hMemMgr, &algorithmEngine, gfMode);

	guideImg = pGuideImg[0];
	guideImg.u32PixelArrayFormat = ASVL_PAF_GRAY;
	binImg = guideImg;
	maskImg = guideImg;
	binImg.ppu8Plane[0] = pSrcImg->ppu8Plane[0];
	
	if (MTrue == bDstMem)
	{
		maskImg.ppu8Plane[0] = (MByte*)MMemAlloc(hMemMgr, guideImg.i32Height * guideImg.pi32Pitch[0]*sizeof(MByte));
		if (MNull ==  maskImg.ppu8Plane[0])
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}
	}
	else
	{
		maskImg.ppu8Plane[0] = pDstImg->ppu8Plane[0];
	}

	if (guideImg.i32Width != guideImg.pi32Pitch[0])
	{
		guideImg.i32Width = guideImg.pi32Pitch[0];
	}
	if (binImg.i32Width != binImg.pi32Pitch[0])
	{
		binImg.i32Width = binImg.pi32Pitch[0];
	}
	if (maskImg.i32Width != maskImg.pi32Pitch[0])
	{
		maskImg.i32Width = maskImg.pi32Pitch[0];
	}

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_) && defined(_EF_TIME_LOG_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
#endif

		lret = ARC_GuidedFilter_Create(mcvParallelMonitor, algorithmEngine, gfMode, &guideImg, &pParam, 1);
		if (MOK != lret)
		{
			goto exit;
		}
		lret = ARC_GuidedFilter_Filter(mcvParallelMonitor, algorithmEngine, gfMode, &binImg, &maskImg);
		if (MOK != lret)
		{
			goto exit;
		}


#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_) && defined(_EF_TIME_LOG_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "ARC_GuidedFilter consume time = %d", lTime);
#endif		
	}
	if (MTrue == bDstMem)
	{
		MMemCpy(pDstImg->ppu8Plane[0], maskImg.ppu8Plane[0], maskImg.i32Height*maskImg.pi32Pitch[0] * sizeof(MByte));
	}
	
exit:
	if (MTrue == bDstMem)
	{
		if (maskImg.ppu8Plane[0])
		{
			MMemFree(hMemMgr, maskImg.ppu8Plane[0]);
			maskImg.ppu8Plane[0] = MNull;
		}
	}
	ARC_GuidedFilter_Uninit(&algorithmEngine, gfMode);
	return lret;
}



MInt32 GuidedFilter_C1_C2(MHandle hMemMgr, MHandle mcvParallelMonitor,	MByte* pSrcData01, MByte* pSrcData02, MByte* pDstData01, MByte* pDstData02,
						  MInt32 lWidth, MInt32 lHeight, MInt32 lDataPitch, MByte* GuideData, MInt32 lGuidePitch, MFloat fISO)
{
	MInt32 lret = MOK;
	ARCGF_PARAM pParam;
	MHandle algorithmEngine = MNull;
	MInt32 gfMode = ARCGF_FAST_GUIDED_FILTER;
	ASVLOFFSCREEN guideImg, maskImg01, binImg01, maskImg02, binImg02;
	MFloat fEpsilon = 0.0025f;// (fISO >= 6400) ? 0.005f : 0.0025f;

	ARC_GuidedFilter_GetDefaultParam(&pParam);
	pParam.gfRadius = 8;
	pParam.gfEpsilon = fEpsilon * 255 * 255;
	pParam.gfScale = 2;

	//if (fISO > 10000)
	//{
	//	pParam.gfRadius = 16;
	//	pParam.gfScale = 4;
	//}


	ARC_GuidedFilter_Init(hMemMgr, &algorithmEngine, gfMode);

	guideImg.ppu8Plane[0] = GuideData;
	guideImg.u32PixelArrayFormat = ASVL_PAF_GRAY;
	guideImg.i32Width = lWidth;
	guideImg.i32Height = lHeight;
	guideImg.pi32Pitch[0] = lGuidePitch;

	binImg01 = guideImg;	binImg01.ppu8Plane[0] = pSrcData01;		binImg01.pi32Pitch[0] = lDataPitch;
	binImg02 = guideImg;	binImg02.ppu8Plane[0] = pSrcData02;		binImg02.pi32Pitch[0] = lDataPitch;
	maskImg01 = guideImg;	maskImg01.ppu8Plane[0] = pDstData01;	maskImg01.pi32Pitch[0] = lDataPitch;
	maskImg02 = guideImg;	maskImg02.ppu8Plane[0] = pDstData02;	maskImg02.pi32Pitch[0] = lDataPitch;


	
	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_) && defined(_EF_TIME_LOG_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
#endif

		lret = ARC_GuidedFilter_Create(mcvParallelMonitor, algorithmEngine, gfMode, &guideImg, &pParam, 1);
		if (MOK != lret)
		{
			goto exit;
		}
		lret = ARC_GuidedFilter_Filter(mcvParallelMonitor, algorithmEngine, gfMode, &binImg01, &maskImg01);
		if (MOK != lret)
		{
			goto exit;
		}

		lret = ARC_GuidedFilter_Filter(mcvParallelMonitor, algorithmEngine, gfMode, &binImg02, &maskImg02);
		if (MOK != lret)
		{
			goto exit;
		}


#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_) && defined(_EF_TIME_LOG_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "ARC_GuidedFilter consume time = %d", lTime);
#endif		
	}
	

exit:
	ARC_GuidedFilter_Uninit(&algorithmEngine, gfMode);
	return lret;
}