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
#include "guided_filter.h"
#include "guided_filter_imgproc.h"

#include "ammem.h"
#include "merror.h"
#include "mobilecv.h"
#include "defcompilesetting.h"

//#include "night_shot.h"

#ifdef ASGF_DEBUG
#include <stdio.h>
#define OUTPUT_PATH "../Debug_LOG"
#endif

//MRESULT fastGuidedFilter_C1_Create_False(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter_C1_False* pAgfInfo,
//	const MByte *guidanceData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels)
//{
//	MHandle hMemMgr = pAgfInfo->hMemMgr;
//
//	MInt32 gfRadius = pAgfInfo->gfRadius;
//	MFloat gfEpsilon = pAgfInfo->gfEpsilon;
//	MInt32 gfScale = pAgfInfo->gfScale;
//
//	if (!guidanceData || gfRadius <= 0 || gfEpsilon <= 0 || gfScale <= 0 || taskNum <= 0
//		|| pitch <= 0 || height <= 0 || width <= 0 || channels != 1)
//	{
//		return MERR_INVALID_PARAM;
//	}
//
//	MRESULT res = MOK;
//
//	// temp variable for calculate I .* I
//	BilinearResize_Param blParam = { 0 };
//	blParam.pBuf = MNull;
//
//	MInt32 heightSub = height / gfScale;
//	MInt32 widthSub = width / gfScale;
//	MInt32 gfRadiusSub = gfRadius / gfScale;
//
//	MInt32 pitchSub_sc_uint8 = widthSub * sizeof(MUInt8);
//	MInt32 pitchSub_sc_int32 = widthSub * sizeof(MInt32);
//	MInt32 pitchSub_sc_uint16 = widthSub * sizeof(MUInt16);
//
//	MInt32 lMinVal = 0, lMaxVal = 0;
//
//	// alloc memory
//	res = ImgBilinearResizeAllocMem_U8C1(hMemMgr, width, height, widthSub, heightSub, taskNum, &blParam);
//	CHECK_ERROR(res);
//
//	// down-sample
//	res = ImgBilinearResize_U8C1(threadEngine, taskNum, guidanceData, width, height, pitch,
//		pAgfInfo->pGuidanceCnSub, widthSub, heightSub, pitchSub_sc_uint8, &blParam);
//	CHECK_ERROR(res);
//
//	// mean filter: guidanceCn -> guidanceCnMean
//	res = BoxFilter_U8C1(hMemMgr, threadEngine, pAgfInfo->pGuidanceCnSub, pitchSub_sc_uint8, 
//			pAgfInfo->pGuidanceCnMean, pitchSub_sc_uint8, heightSub, widthSub, gfRadiusSub);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_0
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/guide_mean.bmp", OUTPUT_PATH);
//		SaveToBmp(pFile, pAgfInfo->pGuidanceCnMean, widthSub, heightSub, pitchSub_sc_uint8, 8);
//	}
//#endif
//
//	// calculate corrGuidance: guidanceCn .* guidanceCn
//	res = DotMultiply_U8C1toU16C1(threadEngine, taskNum, pAgfInfo->pGuidanceCnSub, pAgfInfo->pGuidanceCnSub, pitchSub_sc_uint8,
//								  pAgfInfo->pCorrGuidance, pitchSub_sc_uint16, heightSub, widthSub);
//	CHECK_ERROR(res);
//
//	// mean filter for corrGuidance -> corrGuidanceMean
//	res = BoxFilter_U16C1(hMemMgr, threadEngine, pAgfInfo->pCorrGuidance, pitchSub_sc_uint16,
//		pAgfInfo->pCorrGuidanceMean, pitchSub_sc_uint16, heightSub, widthSub, gfRadiusSub);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_0
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/corr_guide_mean.bmp", OUTPUT_PATH);
//		MByte *pBuffer = (MByte *)MMemAlloc(hMemMgr, heightSub * widthSub * sizeof(MByte));
//		for (MInt32 y = 0; y < heightSub; ++y)
//		{
//			MUInt16 *pSrc = pAgfInfo->pCorrGuidanceMean + widthSub * y;
//			MByte *pDst = pBuffer + widthSub * y;
//			for (MInt32 x = 0; x < widthSub; ++x)
//			{
//				pDst[x] = ET_CAST_BYTE(((pSrc[x] + (1 << 7)) >> 8));
//			}
//		}
//		SaveToBmp(pFile, pBuffer, widthSub, heightSub, pitchSub_sc_uint8, 8);
//		MMemFree(hMemMgr, pBuffer);
//	}
//#endif
//
//	res = CalcVarAddEpsilon_U16ToINT32C1(hMemMgr, threadEngine, pAgfInfo->pGuidanceCnMean, pitchSub_sc_uint8, pAgfInfo->pCorrGuidanceMean, pitchSub_sc_uint16,
//										 pAgfInfo->pCovGuidance, pitchSub_sc_int32, heightSub, widthSub, pAgfInfo->gfEpsilon, &lMinVal, &lMaxVal);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_0
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/cov_guide.bmp", OUTPUT_PATH);
//		MByte *pBuffer = (MByte *)MMemAlloc(hMemMgr, heightSub * widthSub * sizeof(MByte));
//		for (MInt32 y = 0; y < heightSub; ++y)
//		{
//			MInt32 *pSrc = pAgfInfo->pCovGuidance + widthSub * y;
//			MByte *pDst = pBuffer + widthSub * y;
//			for (MInt32 x = 0; x < widthSub; ++x)
//			{
//				pDst[x] = ET_CAST_BYTE(((pSrc[x] + (1 << 5)) >> 6));
//			}
//		}
//		SaveToBmp(pFile, pBuffer, widthSub, heightSub, pitchSub_sc_uint8, 8);
//		MMemFree(hMemMgr, pBuffer);
//	}
//#endif
//
//	res = CalcCovarInv_Int32C1(hMemMgr, threadEngine, pAgfInfo->pCovGuidance, pitchSub_sc_int32, lMinVal, lMaxVal, 
//							   pAgfInfo->pCovInvGuidance, pitchSub_sc_int32, widthSub, heightSub);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_0
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/cov_guide_inv.bmp", OUTPUT_PATH);
//		MByte *pBuffer = (MByte *)MMemAlloc(hMemMgr, heightSub * widthSub * sizeof(MByte));
//		for (MInt32 y = 0; y < heightSub; ++y)
//		{
//			MInt32 *pSrc = pAgfInfo->pCovInvGuidance + widthSub * y;
//			MByte *pDst = pBuffer + widthSub * y;
//			for (MInt32 x = 0; x < widthSub; ++x)
//			{
//				pDst[x] = ET_CAST_BYTE(((pSrc[x] + (1 << 11)) >> 12));
//			}
//		}
//		SaveToBmp(pFile, pBuffer, widthSub, heightSub, pitchSub_sc_uint8, 8);
//		MMemFree(hMemMgr, pBuffer);
//	}
//#endif
//
//exit:
//	// free memory
//	if (blParam.pBuf)
//	{
//		MMemFree(hMemMgr, blParam.pBuf);
//		blParam.pBuf = MNull;
//	}
//
//	return res;
//}
//
//MRESULT fastGuidedFilter_C1_Filter_False(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter_C1_False* pAgfInfo, const MByte *srcData, MInt32 lSrcPitch,
//								   MByte *dstData, MInt32 lDstPitch, MInt32 height, MInt32 width, MInt32 channels)
//{
//	MHandle hMemMgr = pAgfInfo->hMemMgr;
//
//	MInt32 gfRadius = pAgfInfo->gfRadius;
//	MInt32 gfScale = pAgfInfo->gfScale;
//	MInt32 guidanceHeight = pAgfInfo->height, guidanceWidth = pAgfInfo->width;
//
//	static MInt32 k = 1;
//
//	if (guidanceHeight != height || guidanceWidth != width || channels != 1
//		|| gfScale <= 0 || !srcData || lSrcPitch <= 0 || !dstData || lDstPitch <= 0 || taskNum <= 0)
//	{
//		return MERR_INVALID_PARAM;
//	}
//
//	MRESULT res = MOK;
//
//	MInt32 heightSub = height / gfScale;
//	MInt32 widthSub = width / gfScale;
//	MInt32 gfRadiusSub = gfRadius / gfScale;
//
//	MInt32 pitch_sc_uint16 = width * sizeof(MUInt16);
//
//	MInt32 pitchSub_sc_uint8 = widthSub * sizeof(MUInt8);
//	MInt32 pitchSub_sc_uint16 = widthSub * sizeof(MUInt16);
//	MInt32 pitchSub_sc_int32 = widthSub * sizeof(MInt32);
//
//	BilinearResize_Param blParam = { 0 };
//	blParam.pBuf = MNull;
//
//	// alloc memory
//	res = ImgBilinearResizeAllocMem_U8C1(hMemMgr, width, height, widthSub, heightSub, taskNum, &blParam);
//	CHECK_ERROR(res);
//
//	res = ImgBilinearResize_U8C1(threadEngine, taskNum, srcData, width, height, lSrcPitch,
//					pAgfInfo->pSrcCnSub, widthSub, heightSub, pitchSub_sc_uint8, &blParam);
//	CHECK_ERROR(res);
//
//	// mean filter for srcCn
//	res = BoxFilter_U8C1(hMemMgr, threadEngine, pAgfInfo->pSrcCnSub, pitchSub_sc_uint8, pAgfInfo->pSrcCnMean,
//						 pitchSub_sc_uint8, heightSub, widthSub, gfRadiusSub);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/src_mean.bmp", OUTPUT_PATH);
//		SaveToBmp(pFile, pAgfInfo->pSrcCnMean, widthSub, heightSub, pitchSub_sc_uint8, 8);
//	}
//#endif
//
//	// calculate corrGuidanceSrc
//	res = DotMultiply_U8C1toU16C1(threadEngine, taskNum, pAgfInfo->pSrcCnSub, pAgfInfo->pGuidanceCnSub, pitchSub_sc_uint8,
//								  pAgfInfo->pCorrGuidanceSrc, pitchSub_sc_uint16, heightSub, widthSub);
//	CHECK_ERROR(res);
//
//	// mean filter corrGuidanceSrc -> corrGuidanceSrcMean
//	res = BoxFilter_U16C1(hMemMgr, threadEngine, pAgfInfo->pCorrGuidanceSrc, pitchSub_sc_uint16,
//		pAgfInfo->pCorrGuidanceSrcMean, pitchSub_sc_uint16, heightSub, widthSub, gfRadiusSub);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/corr_guide_src_mean.bmp", OUTPUT_PATH);
//		MByte *pBuffer = (MByte *)MMemAlloc(hMemMgr, heightSub * widthSub * sizeof(MByte));
//		for (MInt32 y = 0; y < heightSub; ++y)
//		{
//			MUInt16 *pSrc = pAgfInfo->pCorrGuidanceSrcMean + widthSub * y;
//			MByte *pDst = pBuffer + widthSub * y;
//			for (MInt32 x = 0; x < widthSub; ++x)
//			{
//				pDst[x] = ET_CAST_BYTE(((pSrc[x] + (1 << 7)) >> 8));
//			}
//		}
//		SaveToBmp(pFile, pBuffer, widthSub, heightSub, pitchSub_sc_uint8, 8);
//		MMemFree(hMemMgr, pBuffer);
//	}
//#endif
//
//	// calculate alpha
//	res = CalcAlpha_S16(hMemMgr, threadEngine, pAgfInfo->pGuidanceCnMean, pitchSub_sc_uint8, pAgfInfo->pSrcCnMean, pitchSub_sc_uint8,
//						pAgfInfo->pCovInvGuidance, pitchSub_sc_int32, pAgfInfo->pCorrGuidanceSrcMean, pitchSub_sc_uint16, pAgfInfo->pAlphaSub,
//						pitchSub_sc_uint16, widthSub, heightSub);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/alpha_sub.bmp", OUTPUT_PATH);
//		MByte *pBuffer = (MByte *)MMemAlloc(hMemMgr, heightSub * widthSub * sizeof(MByte));
//		for (MInt32 y = 0; y < heightSub; ++y)
//		{
//			MInt16 *pSrc = pAgfInfo->pAlphaSub + widthSub * y;
//			MByte *pDst = pBuffer + widthSub * y;
//			for (MInt32 x = 0; x < widthSub; ++x)
//			{
//				pDst[x] = ET_CAST_BYTE(((pSrc[x] + (1 << 4)) >> 5));
//			}
//		}
//		SaveToBmp(pFile, pBuffer, widthSub, heightSub, pitchSub_sc_uint8, 8);
//		MMemFree(hMemMgr, pBuffer);
//	}
//#endif
//
//	// mean filter pAlphaSub --->>> pAlphaSubMean
//	res = BoxFilter_S16C1(hMemMgr, threadEngine, pAgfInfo->pAlphaSub, pitchSub_sc_uint16,
//		 pAgfInfo->pAlphaSubMean, pitchSub_sc_uint16, heightSub, widthSub, gfRadiusSub);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/alpha_sub_mean.bmp", OUTPUT_PATH);
//		MByte *pBuffer = (MByte *)MMemAlloc(hMemMgr, heightSub * widthSub * sizeof(MByte));
//		for (MInt32 y = 0; y < heightSub; ++y)
//		{
//			MInt16 *pSrc = pAgfInfo->pAlphaSubMean + widthSub * y;
//			MByte *pDst = pBuffer + widthSub * y;
//			for (MInt32 x = 0; x < widthSub; ++x)
//			{
//				pDst[x] = ET_CAST_BYTE(((pSrc[x] + (1 << 4)) >> 5));
//			}
//		}
//		SaveToBmp(pFile, pBuffer, widthSub, heightSub, pitchSub_sc_uint8, 8);
//		MMemFree(hMemMgr, pBuffer);
//	}
//#endif
//
//	// calc beta
//	res = CalcBeta_S16(hMemMgr, threadEngine, pAgfInfo->pAlphaSub, pitchSub_sc_uint16, pAgfInfo->pGuidanceCnMean, 
//					   pitchSub_sc_uint8, pAgfInfo->pSrcCnMean, pitchSub_sc_uint8, pAgfInfo->pBetaSub, 
//					   pitchSub_sc_uint16, widthSub, heightSub);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/beta_sub.bmp", OUTPUT_PATH);
//		MByte *pBuffer = (MByte *)MMemAlloc(hMemMgr, heightSub * widthSub * sizeof(MByte));
//		for (MInt32 y = 0; y < heightSub; ++y)
//		{
//			MInt16 *pSrc = pAgfInfo->pBetaSub + widthSub * y;
//			MByte *pDst = pBuffer + widthSub * y;
//			for (MInt32 x = 0; x < widthSub; ++x)
//			{
//				pDst[x] = ET_CAST_BYTE(pSrc[x]);
//			}
//		}
//		SaveToBmp(pFile, pBuffer, widthSub, heightSub, pitchSub_sc_uint8, 8);
//		MMemFree(hMemMgr, pBuffer);
//	}
//#endif
//
//	// mean filter pBetaSub --->>> pBetaSubMean
//	res = BoxFilter_S16C1(hMemMgr, threadEngine, pAgfInfo->pBetaSub, pitchSub_sc_uint16, 
//		pAgfInfo->pBetaSubMean, pitchSub_sc_uint16, heightSub, widthSub, gfRadiusSub);
//
//#ifdef _WIN32_DEBUG_
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/beta_sub_mean.bmp", OUTPUT_PATH);
//		MByte *pBuffer = (MByte *)MMemAlloc(hMemMgr, heightSub * widthSub * sizeof(MByte));
//		for (MInt32 y = 0; y < heightSub; ++y)
//		{
//			MInt16 *pSrc = pAgfInfo->pBetaSubMean + widthSub * y;
//			MByte *pDst = pBuffer + widthSub * y;
//			for (MInt32 x = 0; x < widthSub; ++x)
//			{
//				pDst[x] = ET_CAST_BYTE(pSrc[x]);
//			}
//		}
//		SaveToBmp(pFile, pBuffer, widthSub, heightSub, pitchSub_sc_uint8, 8);
//		MMemFree(hMemMgr, pBuffer);
//	}
//#endif
//
//	// alloc memory
//	res = ImgBilinearResizeAllocMem_S16C1(hMemMgr, widthSub, heightSub, width, height, &blParam);
//	CHECK_ERROR(res);
//
//	res = ImgBilinearResize_S16C1(hMemMgr, threadEngine, pAgfInfo->pAlphaSubMean, widthSub, heightSub, 
//					pitchSub_sc_uint16, pAgfInfo->pAlphaMean, width, height, pitch_sc_uint16, &blParam);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/alpha_mean.bmp", OUTPUT_PATH);
//		MByte *pBuffer = (MByte *)MMemAlloc(MNull, height * width * sizeof(MByte));
//		for (MInt32 y = 0; y < height; ++y)
//		{
//			MInt16 *pSrc = pAgfInfo->pAlphaMean + width * y;
//			MByte *pDst = pBuffer + width * y;
//			for (MInt32 x = 0; x < width; ++x)
//			{
//				pDst[x] = ET_CAST_BYTE(((pSrc[x] + (1 << 4)) >> 5));
//			}
//		}
//		SaveToBmp(pFile, pBuffer, width, height, width, 8);
//		MMemFree(MNull, pBuffer);
//	}
//#endif
//
//	res = ImgBilinearResize_S16C1(hMemMgr, threadEngine, pAgfInfo->pBetaSubMean, widthSub, heightSub, pitchSub_sc_uint16, 
//								  pAgfInfo->pBetaMean, width, height, pitch_sc_uint16, &blParam);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/beta_mean.bmp", OUTPUT_PATH);
//		MByte *pBuffer = (MByte *)MMemAlloc(MNull, height * width * sizeof(MByte));
//		for (MInt32 y = 0; y < height; ++y)
//		{
//			MInt16 *pSrc = pAgfInfo->pBetaMean + width * y;
//			MByte *pDst = pBuffer + width * y;
//			for (MInt32 x = 0; x < width; ++x)
//			{
//				pDst[x] = ET_CAST_BYTE(pSrc[x]);
//			}
//		}
//		SaveToBmp(pFile, pBuffer, width, height, width, 8);
//		MMemFree(MNull, pBuffer);
//	}
//#endif
//
//	res = ApplyTransform_ToDstU8C1(hMemMgr, threadEngine, pAgfInfo->pAlphaMean, pitch_sc_uint16, pAgfInfo->pBetaMean, pitch_sc_uint16,
//								   pAgfInfo->pGuidData, pAgfInfo->lGuidPitch, dstData, lDstPitch, width, height);
//	CHECK_ERROR(res);
//
//#ifdef _WIN32_DEBUG_
//	{
//		char pFile[100] = { 0 };
//		sprintf(pFile, "%s/gf_dst.bmp", OUTPUT_PATH);
//		SaveToBmp(pFile, dstData, width, height, lDstPitch, 8);
//	}
//#endif
//
//
//exit:
//	++k;
//
//	// free memory
//	if (blParam.pBuf)
//	{
//		MMemFree(hMemMgr, blParam.pBuf);
//		blParam.pBuf = MNull;
//	}
//
//#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
//	END_PROFILE(lTime);
//	PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter consume time = %d", k, lTime);
//	PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter end lret = %d", k, res);
//#endif
//
//	return res;
//}

static MRESULT MMemAlloc_GuidedFilter_Sub_C1(ArcGuidedFilter_C1 *pAgfInfo)
{
	if (!pAgfInfo) return MERR_INVALID_PARAM;

	MRESULT res = MOK;

	MHandle hMemMgr = pAgfInfo->hMemMgr;

	//MInt32 singleCnPixelSize = pAgfInfo->height * pAgfInfo->width;
	MInt32 ucharBufferSize = 0, ushortBufferSize = 0, floatBufferSize = 0;
	MInt32 intBufferSize = 0;
	
	MInt32 gfScale = pAgfInfo->gfScale;
	MInt32 heightSub = pAgfInfo->height / gfScale;
	MInt32 widthSub = pAgfInfo->width / gfScale;
	MInt32 singleCnSubPixelSize = heightSub * widthSub;

	ucharBufferSize = singleCnSubPixelSize * sizeof(MByte) * 2;
	res = MMemAlloc_UCHAR(hMemMgr, &pAgfInfo->pUCHAR_Buffer, ucharBufferSize);
	CHECK_ERROR(res);
	//pGuidanceCnSub
	pAgfInfo->pGuidanceCnSub = pAgfInfo->pUCHAR_Buffer;
	//pSrcCnSub
	pAgfInfo->pSrcCnSub = pAgfInfo->pGuidanceCnSub + singleCnSubPixelSize;
	
	ushortBufferSize = singleCnSubPixelSize * sizeof(MUInt16) * 2;
	res = MMemAlloc_USHORT(hMemMgr, &pAgfInfo->pUSHORT_Buffer, ushortBufferSize);
	CHECK_ERROR(res);
	//pCorrGuidance
	pAgfInfo->pCorrGuidance = pAgfInfo->pUSHORT_Buffer;
	//pCorrGuidanceSrc
	pAgfInfo->pCorrGuidanceSrc = pAgfInfo->pCorrGuidance + singleCnSubPixelSize;

	floatBufferSize = singleCnSubPixelSize * sizeof(MFloat) * 4;
	res = MMemAlloc_FLOAT(hMemMgr, &pAgfInfo->pFLOAT_Buffer, floatBufferSize);
	CHECK_ERROR(res);
	//pGuidanceCnMean
	pAgfInfo->pGuidanceCnMean = pAgfInfo->pFLOAT_Buffer;
	//pCorrGuidanceMean/pBetaSub
	pAgfInfo->pCorrGuidanceMean = pAgfInfo->pGuidanceCnMean + singleCnSubPixelSize;
	pAgfInfo->pBetaSub = pAgfInfo->pCorrGuidanceMean;
	//pSrcCnMean
	pAgfInfo->pSrcCnMean = pAgfInfo->pCorrGuidanceMean + singleCnSubPixelSize;
	//pCorrGuidanceSrcMean/pAlphaSub
	pAgfInfo->pCorrGuidanceSrcMean = pAgfInfo->pSrcCnMean + singleCnSubPixelSize;
	pAgfInfo->pAlphaSub = pAgfInfo->pCorrGuidanceSrcMean;

	//shortBufferSize = singleCnSubPixelSize * sizeof(MInt16);
	////pFixBetaMean
	//pAgfInfo->pFixBetaSubMean = pAgfInfo->pSHORT_Buffer = (MInt16 *)MMemAlloc(hMemMgr, shortBufferSize);
	//if (!pAgfInfo->pSHORT_Buffer)
	//{
	//	res = MERR_NO_MEMORY;
	//	goto exit;
	//}

	intBufferSize = singleCnSubPixelSize * sizeof(MInt32) * 2;
	res = MMemAlloc_INT32(hMemMgr, &pAgfInfo->pINT_Buffer, intBufferSize);
	CHECK_ERROR(res);
	//pFixAlphaSubMean
	pAgfInfo->pFixAlphaSubMean = pAgfInfo->pINT_Buffer;
	//pFixBetaSubMean
	pAgfInfo->pFixBetaSubMean = pAgfInfo->pFixAlphaSubMean + singleCnSubPixelSize;

exit:
	return res;
}

static MVoid Part_MMemFree_GuidedFilter_Sub_C1(ArcGuidedFilter_C1 *pAgfInfo)
{
	if (pAgfInfo)
	{
		MHandle hMemMgr = pAgfInfo->hMemMgr;

		if (pAgfInfo->pUCHAR_Buffer)
		{
			MMemFree(hMemMgr, pAgfInfo->pUCHAR_Buffer);
			pAgfInfo->pUCHAR_Buffer = MNull;
		}
		if (pAgfInfo->pUSHORT_Buffer)
		{
			MMemFree(hMemMgr, pAgfInfo->pUSHORT_Buffer);
			pAgfInfo->pUSHORT_Buffer = MNull;
		}
		if (pAgfInfo->pFLOAT_Buffer)
		{
			MMemFree(hMemMgr, pAgfInfo->pFLOAT_Buffer);
			pAgfInfo->pFLOAT_Buffer = MNull;
		}
	}

	return;
}

MRESULT fastGuidedFilter_C1_Create(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter_C1* pAgfInfo,
	const MByte *guidanceData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels)
{
	MRESULT res = MOK;
	MHandle hMemMgr = pAgfInfo->hMemMgr;

	MInt32 gfRadius = pAgfInfo->gfRadius;
	MFloat gfEpsilon = pAgfInfo->gfEpsilon;
	MInt32 gfScale = pAgfInfo->gfScale;

	if (!guidanceData || gfRadius <= 0 || gfEpsilon <= 0 || gfScale <= 0 || taskNum <= 0
		|| pitch <= 0 || height <= 0 || width <= 0 || channels != 1)
	{
		return MERR_INVALID_PARAM;
	}

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
	const MInt32 CYCLE = 3;
	static MInt32 k = 0;
	MInt32 lTime;
	//struct timeval time1,time2;	
	START_PROFILE();
    PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Create start", k%CYCLE);	

#endif

	// temp variable for calculate I .* I
	BilinearResize_Param blParam = { 0 };
	blParam.pBuf = MNull;

	MInt32 heightSub = height / gfScale;
	MInt32 widthSub = width / gfScale;
	MInt32 gfRadiusSub = gfRadius / gfScale;

	MInt32 pitchSub_sc_uint8 = widthSub * sizeof(MUInt8);
	MInt32 pitchSub_sc_float = widthSub * sizeof(MFloat);
	MInt32 pitchSub_sc_uint16 = widthSub * sizeof(MUInt16);

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Create---MMemAlloc_GuidedFilter_Sub_C1 start", k%CYCLE);		
#endif

		res = MMemAlloc_GuidedFilter_Sub_C1(pAgfInfo);
		CHECK_ERROR(res);

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Create---MMemAlloc_GuidedFilter_Sub_C1 consume time = %d", k%CYCLE, lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Create---MMemAlloc_GuidedFilter_Sub_C1 lret = %d", k%CYCLE, res);
#endif
	}

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Create---ResizeGuidAndGuidMean start", k%CYCLE);
	
#endif

		// alloc memory
		res = ImgBilinearResizeAllocMem_U8C1(hMemMgr, width, height, widthSub, heightSub, taskNum, &blParam);
		CHECK_ERROR(res);


		// down-sample
		res = ImgBilinearResize_U8C1(threadEngine, taskNum, guidanceData, width, height, pitch,
					pAgfInfo->pGuidanceCnSub, widthSub, heightSub, pitchSub_sc_uint8, &blParam);
		CHECK_ERROR(res);

		// mean filter: guidanceCn -> guidanceCnMean
		res = BoxFilter_U8C1toF32C1(hMemMgr, threadEngine, pAgfInfo->pGuidanceCnSub, pitchSub_sc_uint8,
						pAgfInfo->pGuidanceCnMean, pitchSub_sc_float, heightSub, widthSub, gfRadiusSub);
		CHECK_ERROR(res);

#ifdef _WIN32_DEBUG_0
		{
			char pFile[100] = { 0 };
			MByte *pBuffer = (MByte *)MMemAlloc(MNull, heightSub * widthSub * sizeof(MByte));
			sprintf(pFile, "%s/sub_guidance_mean.bmp", OUTPUT_PATH);
			for (MInt32 y = 0; y < heightSub; ++y)
			{
				MFloat *pSrc = pAgfInfo->pGuidanceCnMean + y * widthSub;
				MByte *pDst = pBuffer + y * widthSub;
				for (MInt32 x = 0; x < widthSub; ++x)
				{
					MInt32 lVal = (MInt32)((pSrc[x] / 256.0) * 255.f + 0.5);
					pDst[x] = ET_CAST_BYTE(lVal);
				}
			}

			SaveToBmp(pFile, pBuffer, widthSub, heightSub, widthSub, 8);
			MMemFree(MNull, pBuffer);
		}
#endif

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Create---ResizeGuidAndGuidMean consume time = %d", k%CYCLE, lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Create---ResizeGuidAndGuidMean lret = %d", k%CYCLE, res);
#endif
	}

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Create---CalcCorrGuidMean start", k%CYCLE);
	
#endif

		//calculate corrGuidance : guidanceCn.*guidanceCn
		res = DotMultiply_U8C1toU16C1(hMemMgr, threadEngine, taskNum, pAgfInfo->pGuidanceCnSub, pAgfInfo->pGuidanceCnSub,
									 pitchSub_sc_uint8, pAgfInfo->pCorrGuidance, pitchSub_sc_uint16, heightSub, widthSub);
		CHECK_ERROR(res);

		//mean filter for corrGuidance -> corrGuidanceMean
		res = BoxFilter_U16C1toF32C1(hMemMgr, threadEngine, pAgfInfo->pCorrGuidance, pitchSub_sc_uint16, pAgfInfo->pCorrGuidanceMean,
									 pitchSub_sc_float, heightSub, widthSub, gfRadiusSub);
		CHECK_ERROR(res);

#ifdef _WIN32_DEBUG_0
		{
			char pFile[100] = { 0 };
			MByte *pBuffer = (MByte *)MMemAlloc(MNull, heightSub * widthSub * sizeof(MByte));
			sprintf(pFile, "%s/sub_corr_guidance_mean.bmp", OUTPUT_PATH);
			for (MInt32 y = 0; y < heightSub; ++y)
			{
				MFloat *pSrc = pAgfInfo->pCorrGuidanceMean + y * widthSub;
				MByte *pDst = pBuffer + y * widthSub;
				for (MInt32 x = 0; x < widthSub; ++x)
				{
					MInt32 lVal = (MInt32)((pSrc[x] / 256.0 / 256.0) * 255.f + 0.5);
					pDst[x] = ET_CAST_BYTE(lVal);
				}
			}

			SaveToBmp(pFile, pBuffer, widthSub, heightSub, widthSub, 8);
			MMemFree(MNull, pBuffer);
		}
#endif

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Create---CalcCorrGuidMean consume time = %d", k%CYCLE, lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Create---CalcCorrGuidMean lret = %d", k%CYCLE, res);
#endif
	}

exit:
	if (blParam.pBuf)
	{
		MMemFree(hMemMgr, blParam.pBuf);
		blParam.pBuf = MNull;
	}

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
	END_PROFILE(lTime);
	PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Create consume time = %d", k%CYCLE, lTime);
	PrintfB(6,"VNS","aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Create lret = %d", k%CYCLE, res);
	++k;
#endif

	return res;
}

MRESULT fastGuidedFilter_C1_Filter(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter_C1* pAgfInfo, const MByte *srcData,
							MInt32 lSrcPitch, MByte *dstData, MInt32 lDstPitch, MInt32 height, MInt32 width, MInt32 channels)
{
	MHandle hMemMgr = pAgfInfo->hMemMgr;

	//MInt32 gfRadius = pAgfInfo->gfRadius;
	MInt32 gfScale = pAgfInfo->gfScale;
	MInt32 guidanceHeight = pAgfInfo->height, guidanceWidth = pAgfInfo->width;

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
	const MInt32 CYCLE = 3;
	static MInt32 k = 0;
	MInt32 residue = k % CYCLE;
	MInt32 lTime;
	//struct timeval time1, time2;
	START_PROFILE();
	PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter start", residue);

#endif

	if (guidanceHeight != height || guidanceWidth != width || channels != 1
		|| gfScale <= 0 || !srcData || lSrcPitch <= 0 || !dstData || lDstPitch <= 0 || taskNum <= 0)
	{
		return MERR_INVALID_PARAM;
	}

	MRESULT res = MOK;

	MInt32 heightSub = height / gfScale;
	MInt32 widthSub = width / gfScale;
	MInt32 gfRadiusSub = pAgfInfo->gfRadius / gfScale;

	MInt32 pitch_sc_uint16 = width * sizeof(MUInt16);
	MInt32 pitch_sc_int32 = width * sizeof(MInt32);

	MInt32 pitchSub_sc_uint8 = widthSub * sizeof(MUInt8);
	MInt32 pitchSub_sc_uint16 = widthSub * sizeof(MUInt16);
	MInt32 pitchSub_sc_int32 = widthSub * sizeof(MInt32);
	MInt32 pitchSub_sc_float = widthSub * sizeof(MFloat);

	BilinearResize_Param blParam = { 0 };
	blParam.pBuf = MNull;

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---ResizeSrcAndSrcMean start", residue);	
#endif

		// alloc memory
		res = ImgBilinearResizeAllocMem_U8C1(hMemMgr, width, height, widthSub, heightSub, taskNum, &blParam);
		CHECK_ERROR(res);

		res = ImgBilinearResize_U8C1(threadEngine, taskNum, srcData, width, height, lSrcPitch,
						pAgfInfo->pSrcCnSub, widthSub, heightSub, pitchSub_sc_uint8, &blParam);
		CHECK_ERROR(res);

		// mean filter for srcCn
		res = BoxFilter_U8C1toF32C1(hMemMgr, threadEngine, pAgfInfo->pSrcCnSub, pitchSub_sc_uint8,
						pAgfInfo->pSrcCnMean, pitchSub_sc_float, heightSub, widthSub, gfRadiusSub);
		CHECK_ERROR(res);

#ifdef _WIN32_DEBUG_0
		{
			char pFile[100] = { 0 };
			MByte *pBuffer = (MByte *)MMemAlloc(MNull, heightSub * widthSub * sizeof(MByte));
			sprintf(pFile, "%s/sub_src_mean.bmp", OUTPUT_PATH);
			for (MInt32 y = 0; y < heightSub; ++y)
			{
				MFloat *pSrc = pAgfInfo->pSrcCnMean + y * widthSub;
				MByte *pDst = pBuffer + y * widthSub;
				for (MInt32 x = 0; x < widthSub; ++x)
				{
					MInt32 lVal = (MInt32)((pSrc[x] / 256.0) * 255.f + 0.5);
					pDst[x] = ET_CAST_BYTE(lVal);
				}
			}

			SaveToBmp(pFile, pBuffer, widthSub, heightSub, widthSub, 8);
			MMemFree(MNull, pBuffer);
		}
#endif

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---ResizeSrcAndSrcMean consume time = %d", residue, lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Filter---ResizeSrcAndSrcMean lret = %d", residue, res);
#endif
	}

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---CalcCorrGuidSrcMean start", residue);		
#endif

		// calculate corrGuidanceSrc
		res = DotMultiply_U8C1toU16C1(hMemMgr, threadEngine, taskNum, pAgfInfo->pSrcCnSub, pAgfInfo->pGuidanceCnSub, pitchSub_sc_uint8,
									  pAgfInfo->pCorrGuidanceSrc, pitchSub_sc_uint16, heightSub, widthSub);
		CHECK_ERROR(res);

		// mean filter corrGuidanceSrc -> corrGuidanceSrcMean
		res = BoxFilter_U16C1toF32C1(hMemMgr, threadEngine, pAgfInfo->pCorrGuidanceSrc, pitchSub_sc_uint16,
									 pAgfInfo->pCorrGuidanceSrcMean, pitchSub_sc_float, heightSub, widthSub, gfRadiusSub);
		CHECK_ERROR(res);

#ifdef _WIN32_DEBUG_0
		{
			char pFile[100] = { 0 };
			MByte *pBuffer = (MByte *)MMemAlloc(MNull, heightSub * widthSub * sizeof(MByte));
			sprintf(pFile, "%s/sub_corr_guidance_src_mean.bmp", OUTPUT_PATH);
			for (MInt32 y = 0; y < heightSub; ++y)
			{
				MFloat *pSrc = pAgfInfo->pCorrGuidanceSrcMean + y * widthSub;
				MByte *pDst = pBuffer + y * widthSub;
				for (MInt32 x = 0; x < widthSub; ++x)
				{
					MInt32 lVal = (MInt32)((pSrc[x] / 256.0 / 256.0) * 255.f + 0.5);
					pDst[x] = ET_CAST_BYTE(lVal);
				}
			}

			SaveToBmp(pFile, pBuffer, widthSub, heightSub, widthSub, 8);
			MMemFree(MNull, pBuffer);
		}
#endif

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---CalcCorrGuidSrcMean consume time = %d", residue, lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Filter---CalcCorrGuidSrcMean lret = %d", residue, res);
#endif
	}

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---CalcAlphaAndBeta start", residue);	
#endif

		// calc alpha and beta
		res = Calc_AlphaAndBeta_F32_C1(hMemMgr, threadEngine, pAgfInfo->gfEpsilon, pAgfInfo->pGuidanceCnMean, widthSub,
									   pAgfInfo->pSrcCnMean, widthSub, pAgfInfo->pCorrGuidanceMean, widthSub,
									   pAgfInfo->pCorrGuidanceSrcMean, widthSub, pAgfInfo->pAlphaSub, widthSub,
									   pAgfInfo->pBetaSub, widthSub, widthSub, heightSub);
		CHECK_ERROR(res);

#ifdef _WIN32_DEBUG_0
		{
			char pFile1[100] = { 0 }, pFile2[100] = { 0 };
			MByte *pBuffer1 = (MByte *)MMemAlloc(MNull, 2 * heightSub * widthSub * sizeof(MByte));
			MByte *pBuffer2 = pBuffer1 + heightSub * widthSub;
			sprintf(pFile1, "%s/sub_alpha.bmp", OUTPUT_PATH);
			sprintf(pFile2, "%s/sub_beta.bmp", OUTPUT_PATH);
			for (MInt32 y = 0; y < heightSub; ++y)
			{
				MFloat *pSrc = pAgfInfo->pAlphaSub + y * widthSub;
				MFloat *pBeta = pAgfInfo->pBetaSub + y * widthSub;
				MByte *pDst1 = pBuffer1 + y * widthSub;
				MByte *pDst2 = pBuffer2 + y * widthSub;
				for (MInt32 x = 0; x < widthSub; ++x)
				{
					MInt32 lVal = (MInt32)(pSrc[x] * 255.f + 0.5);
					pDst1[x] = ET_CAST_BYTE(lVal);

					pDst2[x] = ET_CAST_BYTE((MInt32)pBeta[x]);
				}
			}

			SaveToBmp(pFile1, pBuffer1, widthSub, heightSub, widthSub, 8);
			SaveToBmp(pFile2, pBuffer2, widthSub, heightSub, widthSub, 8);
			MMemFree(MNull, pBuffer1);
		}
#endif

		//calc alpha --->>> box mean alpha
		res = BoxFilter_F32toI32_C1(hMemMgr, threadEngine, pAgfInfo->pAlphaSub, pitchSub_sc_float,
			     pAgfInfo->pFixAlphaSubMean, pitchSub_sc_int32, heightSub, widthSub, gfRadiusSub);
		CHECK_ERROR(res);

#ifdef _WIN32_DEBUG_0
		{
			char pFile[100] = { 0 };
			MByte *pBuffer = (MByte *)MMemAlloc(MNull, heightSub * widthSub * sizeof(MByte));
			sprintf(pFile, "%s/sub_alpha_mean.bmp", OUTPUT_PATH);
			for (MInt32 y = 0; y < heightSub; ++y)
			{
				MInt32 *pSrc = pAgfInfo->pFixAlphaSubMean + y * widthSub;
				MByte *pDst = pBuffer + y * widthSub;
				for (MInt32 x = 0; x < widthSub; ++x)
				{
					MInt32 lVal = (MInt32)((pSrc[x] / (MFloat)(1 << 14)) * 255.f + 0.5);
					pDst[x] = ET_CAST_BYTE(lVal);
				}
			}

			SaveToBmp(pFile, pBuffer, widthSub, heightSub, widthSub, 8);
			MMemFree(MNull, pBuffer);
		}
#endif

		//calc beta --->>> box mean beta
		res = BoxFilter_F32toI32_C1(hMemMgr, threadEngine, pAgfInfo->pBetaSub, pitchSub_sc_float,
			     pAgfInfo->pFixBetaSubMean, pitchSub_sc_int32, heightSub, widthSub, gfRadiusSub);
		CHECK_ERROR(res);

#ifdef _WIN32_DEBUG_0
		{
			char pFile[100] = { 0 };
			MByte *pBuffer = (MByte *)MMemAlloc(MNull, heightSub * widthSub * sizeof(MByte));
			sprintf(pFile, "%s/sub_beta_mean.bmp", OUTPUT_PATH);
			for (MInt32 y = 0; y < heightSub; ++y)
			{
				MInt32 *pSrc = pAgfInfo->pFixBetaSubMean + y * widthSub;
				MByte *pDst = pBuffer + y * widthSub;
				for (MInt32 x = 0; x < widthSub; ++x)
				{
					MInt32 lVal = (MInt32)((MFloat)pSrc[x] / (MFloat)(1 << 14));
					pDst[x] = ET_CAST_BYTE(lVal);
				}
			}

			SaveToBmp(pFile, pBuffer, widthSub, heightSub, widthSub, 8);
			MMemFree(MNull, pBuffer);
		}
#endif

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---CalcAlphaAndBeta consume time = %d", residue, lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Filter---CalcAlphaAndBeta lret = %d", residue, res);
#endif
	}

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---FreeAndAllocMemory start", residue);	
#endif

		//for reducing memory comsumption, free part of memory
		Part_MMemFree_GuidedFilter_Sub_C1(pAgfInfo);

		pAgfInfo->pFixAlphaMean = (MInt32 *)MMemAlloc(hMemMgr, width * height * sizeof(MInt32));
		pAgfInfo->pFixBetaMean = (MInt32 *)MMemAlloc(hMemMgr, width * height * sizeof(MInt32));
		if (!pAgfInfo->pFixAlphaMean || !pAgfInfo->pFixBetaMean)
		{
			res = MERR_NO_MEMORY;
			goto exit;
		}

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---FreeAndAllocMemory consume time = %d", residue, lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Filter---FreeAndAllocMemory lret = %d", residue, res);
#endif
	}

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---ResizeAlphaAndBeta start", residue);	
#endif

		// alloc memory
		res = ImgBilinearResizeAllocMem_S16C1(hMemMgr, widthSub, heightSub, width, height, &blParam);
		CHECK_ERROR(res);

		// resize alpha
		res = ImgBilinearResize_S32C1(hMemMgr, threadEngine, pAgfInfo->pFixAlphaSubMean, widthSub, heightSub, widthSub * sizeof(MInt32),
									  pAgfInfo->pFixAlphaMean, width, height, width * sizeof(MInt32), &blParam);
		CHECK_ERROR(res);

#ifdef _WIN32_DEBUG_0
		{
			char pFile[100] = { 0 };
			MByte *pBuffer = (MByte *)MMemAlloc(MNull, height * width * sizeof(MByte));
			sprintf(pFile, "%s/alpha_mean.bmp", OUTPUT_PATH);
			for (MInt32 y = 0; y < height; ++y)
			{
				MInt32 *pSrc = pAgfInfo->pFixAlphaMean + y * width;
				MByte *pDst = pBuffer + y * width;
				for (MInt32 x = 0; x < width; ++x)
				{
					MInt32 lVal = (MInt32)((pSrc[x] / (MFloat)(1 << 14)) * 255.f + 0.5);
					pDst[x] = ET_CAST_BYTE(lVal);
				}
			}

			SaveToBmp(pFile, pBuffer, width, height, width, 8);
			MMemFree(MNull, pBuffer);
	}
#endif

		//resize beta
		res = ImgBilinearResize_S32C1(hMemMgr, threadEngine, pAgfInfo->pFixBetaSubMean, widthSub, heightSub, widthSub * sizeof(MInt32),
									  pAgfInfo->pFixBetaMean, width, height, width * sizeof(MInt32), &blParam);
		CHECK_ERROR(res);

#ifdef _WIN32_DEBUG_0
		{
			char pFile[100] = { 0 };
			MByte *pBuffer = (MByte *)MMemAlloc(MNull, height * width * sizeof(MByte));
			sprintf(pFile, "%s/beta_mean.bmp", OUTPUT_PATH);
			for (MInt32 y = 0; y < height; ++y)
			{
				MInt32 *pSrc = pAgfInfo->pFixBetaMean + y * width;
				MByte *pDst = pBuffer + y * width;
				for (MInt32 x = 0; x < width; ++x)
				{
					MInt32 lVal = (MInt32)(pSrc[x] / (MFloat)(1<<14));
					pDst[x] = ET_CAST_BYTE(lVal);
				}
			}

			SaveToBmp(pFile, pBuffer, width, height, width, 8);
			MMemFree(MNull, pBuffer);
	}
#endif

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---ResizeAlphaAndBeta consume time = %d", residue, lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Filter---ResizeAlphaAndBeta lret = %d", residue, res);
#endif
	}

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		START_PROFILE();
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---ApplyTransform_ToDstU8C1 start", residue);		
#endif

		res = ApplyTransform_ToDstU8C1(hMemMgr, threadEngine, pAgfInfo->pFixAlphaMean, width * sizeof(MInt32), pAgfInfo->pFixBetaMean, width * sizeof(MInt32),
									   pAgfInfo->pGuidanceCnOri, pAgfInfo->lGuidCnOriPitch, dstData, lDstPitch, width, height);
		CHECK_ERROR(res);

#ifdef _WIN32_DEBUG_0
		{
			char pFile[100] = { 0 };
			sprintf(pFile, "%s/gf_dst.bmp", OUTPUT_PATH);
			SaveToBmp(pFile, dstData, width, height, lDstPitch, 8);
		}
#endif

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter---ApplyTransform_ToDstU8C1 consume time = %d", residue, lTime);
		PrintfB(6, "VNS", "aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Filter---ApplyTransform_ToDstU8C1 lret = %d", residue, res);
#endif
	}

exit:
	if (pAgfInfo->pFixBetaMean)
	{
		MMemFree(hMemMgr, pAgfInfo->pFixBetaMean);
		pAgfInfo->pFixBetaMean = MNull;
	}
	if (pAgfInfo->pFixAlphaMean)
	{
		MMemFree(hMemMgr, pAgfInfo->pFixAlphaMean);
		pAgfInfo->pFixAlphaMean = MNull;
	}

	// free memory
	if (blParam.pBuf)
	{
		MMemFree(hMemMgr, blParam.pBuf);
		blParam.pBuf = MNull;
	}

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
	END_PROFILE(lTime);
	PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_C1_Filter consume time = %d", residue, lTime);
	PrintfB(6, "VNS", "aveDoEnhancement enhance end hdr %d_fastGuidedFilter_C1_Filter lret = %d", residue, res);
	++k;
#endif

	return res;
}