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
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "amcomdef.h"
#include "ammem.h"
#include "merror.h"
#include "mobilecv.h"
#include "imagebase.h"
#include "ArcsoftLog.h"

#include "gaussian_filter.h"
#include "convYUVToBGR.h"
#include "arcsoft_guided_filter.h"
#include "boxfilterC2.h"
#include "img_interpolation.h"
#include "imageproc.h"
#include "single_image_enhancement_define.h"
#include "Arcsoft_NV21_DownScale4_To_I444_RGB.h"
#include "imageproc.h"
#include "AnisotropicGuidedFiltering.h"
#include "Arcsoft_ReduceColorNoise.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

static MVoid MixImages_UV(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg1, LPASVLOFFSCREEN pSrcImg2, LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pAlphaMap)
{
	// 降彩噪是全局的，对结果需要根据mask，只保留需要降彩噪的部分

	auto *pTempSrc1 = pSrcImg1->ppu8Plane[1];
	auto *pTempSrc2 = pSrcImg2->ppu8Plane[1];
	auto *pTempDst = pDstImg->ppu8Plane[1];
	auto *pTempAlpha = pAlphaMap->ppu8Plane[0];

	auto tempHeight = pDstImg->i32Height / 2;
	auto tempWidth = pDstImg->i32Width;

	for (MInt32 y = 0; y < tempHeight; y++)
	{
		MInt32 x = 0;

#ifdef __ARM_NEON__


#endif

		for (; x < tempWidth; x++)
		{
			MUInt8 tempVal = pTempAlpha[x / 4] & 0x1;
			pTempDst[x] = pTempSrc1[x] * (1 - tempVal) + pTempSrc2[x] * tempVal;
		}

		pTempAlpha += pAlphaMap->pi32Pitch[0] * (y & 0x1); // 每两行进行递加
		pTempSrc1 += pSrcImg1->pi32Pitch[1];
		pTempSrc2 += pSrcImg2->pi32Pitch[1];
		pTempDst += pDstImg->pi32Pitch[1];

	}
}

MInt32 GuideFilterDenoise_UV(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, MInt32 kernelSizeUV)
{
	MInt32 lret = MOK, i, j;
	ASVLOFFSCREEN guideRGB;
	ASVLOFFSCREEN srcU, srcV;
	ASVLOFFSCREEN resU, resV;
	MInt32 lHeight, lWidth, lYPitch, lUVPitch;
	MInt32 smH, smW, smPitch;
	MByte *smoothY = MNull, *smoothUV = MNull, *uvBuf = MNull;

	lHeight = pSrcImg->i32Height;
	lWidth = pSrcImg->i32Width;
	lYPitch = lWidth;
	lUVPitch = lWidth;
	smH = lHeight >> 1, smW = lWidth >> 1;
	smPitch = smW;
	uvBuf = (MByte*)MMemAlloc(hMemMgr, smH*smPitch * 4);
	if (uvBuf == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	guideRGB.u32PixelArrayFormat = ASVL_PAF_RGB24_B8G8R8;
	guideRGB.i32Height = smH;
	guideRGB.i32Width = smW;
	guideRGB.pi32Pitch[0] = smW * 3 * sizeof(MByte);
	guideRGB.ppu8Plane[0] = (MByte*)MMemAlloc(hMemMgr, guideRGB.i32Height*guideRGB.pi32Pitch[0]);
	if (guideRGB.ppu8Plane[0] == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	smoothY = uvBuf;
	smoothUV = uvBuf + smH * smPitch;

	{
#if defined(_ARM_TIME_)
		MLong lTime;
		START_PROFILE();
#endif
		lret = GaussPyrDown2(hMemMgr, mcvParallelMonitor, pSrcImg->ppu8Plane[0], smoothY, lWidth, lHeight, lYPitch, smW, smH, smPitch);
		if (lret != MOK)
			goto exit;
#if defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement GaussPyrDown2_UV consume time = %d\r\n", lTime);
#endif
	}

#ifdef _WIN32_DEBUG_
	SaveToBMP("srcY.bmp", pSrcImg->ppu8Plane[0], lWidth, lHeight, lYPitch, 8);
	SaveToBMP("smoothY.bmp", smoothY, smW, smH, smPitch, 8);
#endif
	{
#if defined(_ARM_TIME_)
		MLong lTime;
		START_PROFILE();
#endif
		lret = Box_Filter_C2(hMemMgr, mcvParallelMonitor, pSrcImg->ppu8Plane[1], smW, smH, lUVPitch, smoothUV, lUVPitch, 1);
		if (lret != MOK)
			goto exit;
#if defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement Box_Filter_C2_UV consume time = %d\r\n", lTime);
#endif
	}

#ifdef _WIN32_DEBUG_
	SaveToBMP("srcUV.bmp", pSrcImg->ppu8Plane[1], lWidth, lHeight >> 1, lUVPitch, 8);
	SaveToBMP("smoothUV.bmp", smoothUV, lWidth, smH, lUVPitch, 8);
#endif

	{
#if defined(_ARM_TIME_)
		MLong lTime;
		START_PROFILE();
#endif
		lret = convYUVToBGR(mcvParallelMonitor, smoothY, smoothUV, smPitch, lUVPitch, &guideRGB);
		if (lret != MOK)
			goto exit;
#if defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6, "VNS", "aveDoEnhancement convYUVToBGR_UV consume time = %d\r\n", lTime);
#endif
	}

	srcU.u32PixelArrayFormat = ASVL_PAF_GRAY;
	srcU.i32Height = smH;
	srcU.i32Width = smW;
	srcU.pi32Pitch[0] = smW;
	srcU.ppu8Plane[0] = uvBuf;

	srcV.u32PixelArrayFormat = ASVL_PAF_GRAY;
	srcV.i32Height = smH;
	srcV.i32Width = smW;
	srcV.pi32Pitch[0] = smW;
	srcV.ppu8Plane[0] = uvBuf + smH * smW;

	resU.u32PixelArrayFormat = ASVL_PAF_GRAY;
	resU.i32Height = smH;
	resU.i32Width = smW;
	resU.pi32Pitch[0] = smW;
	resU.ppu8Plane[0] = uvBuf + smH * smW * 2;

	resV.u32PixelArrayFormat = ASVL_PAF_GRAY;
	resV.i32Height = smH;
	resV.i32Width = smW;
	resV.pi32Pitch[0] = smW;
	resV.ppu8Plane[0] = uvBuf + smH * smW * 3;

	for (i = 0; i < smH; i++)
	{
		MByte *uvptr = pSrcImg->ppu8Plane[1] + i * lUVPitch;
		MByte *uptr = srcU.ppu8Plane[0] + i * srcU.pi32Pitch[0];
		MByte *vptr = srcV.ppu8Plane[0] + i * srcV.pi32Pitch[0];
		for (j = 0; j < smW; j++)
		{
			uptr[j] = uvptr[2 * j];
			vptr[j] = uvptr[2 * j + 1];
		}
	}


	//RGB三通道的导向图滤波
	{
		ARCGF_PARAM pParam;
		ARC_GuidedFilter_GetDefaultParam(&pParam);
		pParam.gfRadius = kernelSizeUV;
		pParam.gfEpsilon = 0.001f * 255 * 255;
		pParam.gfScale = 2;

		MHandle algorithmEngine = MNull;
		MInt32 gfMode = ARCGF_FAST_GUIDED_FILTER;
		{
			lret = ARC_GuidedFilter_Init(hMemMgr, &algorithmEngine, gfMode);
			if (MOK != lret)
				goto exit;
		}

		{
			lret = ARC_GuidedFilter_Create(mcvParallelMonitor, algorithmEngine, &guideRGB, &pParam, 1);
			if (MOK != lret)
				goto exit;
		}

		{
			lret = ARC_GuidedFilter_Filter(mcvParallelMonitor, algorithmEngine, &srcU, &resU);
			if (MOK != lret)
				goto exit;
		}

		{
			lret = ARC_GuidedFilter_Filter(mcvParallelMonitor, algorithmEngine, &srcV, &resV);
			if (MOK != lret)
				goto exit;
		}

		lret = ARC_GuidedFilter_Uninit(hMemMgr, &algorithmEngine);
		if (MOK != lret)
			goto exit;

	}

	//copy to result image
	for (i = 0; i < smH; i++)
	{
		MByte *uvptr = pSrcImg->ppu8Plane[1] + i * pSrcImg->pi32Pitch[1];
		MByte *uptr = resU.ppu8Plane[0] + i * resU.pi32Pitch[0];
		MByte *vptr = resV.ppu8Plane[0] + i * resV.pi32Pitch[0];
		for (j = 0; j < smW; j++)
		{
			uvptr[2 * j] = uptr[j];
			uvptr[2 * j + 1] = vptr[j];
		}
	}

exit:
	if (uvBuf)
	{
		MMemFree(hMemMgr, uvBuf);
		uvBuf = MNull;
	}
	if (guideRGB.ppu8Plane[0])
	{
		MMemFree(hMemMgr, guideRGB.ppu8Plane[0]);
		guideRGB.ppu8Plane[0] = MNull;
	}
	return lret;
}

MInt32 BlendGuideFilterUV(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 lUVIntensity)
{
	MInt32 i, j, lret = MOK;
	MFloat scale;
	MInt32 lHeight = pSrcImg->i32Height;
	MInt32 lWidth = pSrcImg->i32Width;
	MByte  srcU, srcV, dstU, dstV, diffU, diffV, maxVal;
	MInt32 pBlendTable[129], diffThresh = 15, diffVar = 2;

	if (!(pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV21 || pSrcImg->u32PixelArrayFormat == ASVL_PAF_NV12)
		|| pSrcImg->u32PixelArrayFormat != pDstImg->u32PixelArrayFormat)
	{
		lret = MERR_UNSUPPORTED;
		goto exit;
	}

	if (lUVIntensity <= 10)
	{
		diffThresh = 15;
		diffVar = 2;
	}
	else
	{
		diffThresh = 25;
		diffVar = 3;
	}

	for (i = 0; i <= 128; i++)
	{
		if (i < diffThresh)
		{
			pBlendTable[i] = 256;
		}
		else
		{
			MFloat lVal = MFloat((i - diffThresh)*(i - diffThresh));
			MFloat lVar = (MFloat)(2 * diffVar * diffVar);
			scale = (256.0 * exp(-lVal / lVar) + 0.5f);
			pBlendTable[i] = (MInt32)(scale);
		}
	}

	for (i = 0; i < (lHeight >> 1); i++)
	{
		MByte *pSrcRowUV = pSrcImg->ppu8Plane[1] + i * pSrcImg->pi32Pitch[1];
		MByte *pDstRowUV = pDstImg->ppu8Plane[1] + i * pDstImg->pi32Pitch[1];
		for (j = 0; j < lWidth; j += 2)
		{
			srcU = pSrcRowUV[j];
			srcV = pSrcRowUV[j + 1];
			dstU = pDstRowUV[j];
			dstV = pDstRowUV[j + 1];
			diffU = ABS(srcU - 128);
			diffV = ABS(srcV - 128);
			maxVal = MAX(diffU, diffV);

			pDstRowUV[j] = (pBlendTable[maxVal] * dstU + (256 - pBlendTable[maxVal])*srcU) >> 8;
			pDstRowUV[j + 1] = (pBlendTable[maxVal] * dstV + (256 - pBlendTable[maxVal])*srcV) >> 8;
		}
	}
exit:
	return lret;
}


MInt32 ArcSoft_GuideFilter_For_DownSampleUV(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, MInt32 kernelSizeUV, MInt32 lUVIntensity, LPASVLOFFSCREEN pShadeMap, MBool isBF)
{

	 #define UV_GUIDED

	LOGD("ArcSoft_GuideFilter_For_DownSampleUV+");
	MInt32 lret = MOK, i, j;
	ASVLOFFSCREEN guideRGB;
	ASVLOFFSCREEN srcU, srcV;
	ASVLOFFSCREEN resU, resV;
	MByte *pMask = MNull;
	MInt32 lHeight, lWidth, lYPitch, lUVPitch;
	MInt32 smH, smW, smYPitch, smUVPitch;
	MByte *smoothY = MNull, *smoothUV = MNull, *uvBuf = MNull, *resUV = MNull;
	LPASVLOFFSCREEN pTmpSrc = MNull;

	ASVLOFFSCREEN srcRGB; // 1/2图像做
	ASVLOFFSCREEN dstNV21;

	pTmpSrc = (LPASVLOFFSCREEN)MMemAlloc(hMemMgr, sizeof(ASVLOFFSCREEN));
	if (pTmpSrc == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}



	pTmpSrc->u32PixelArrayFormat = pSrcImg->u32PixelArrayFormat;
	pTmpSrc->i32Height = pSrcImg->i32Height;
	pTmpSrc->i32Width = pSrcImg->i32Width;
	pTmpSrc->pi32Pitch[0] = pSrcImg->pi32Pitch[0];
	pTmpSrc->pi32Pitch[1] = pSrcImg->pi32Pitch[1];
	pTmpSrc->ppu8Plane[0] = (MByte*)MMemAlloc(hMemMgr, pTmpSrc->i32Height*pTmpSrc->pi32Pitch[0] * 3 / 2);
	pTmpSrc->ppu8Plane[1] = pTmpSrc->ppu8Plane[0] + pTmpSrc->i32Height*pTmpSrc->pi32Pitch[1];
	if (pTmpSrc->ppu8Plane[0] == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	MMemCpy(pTmpSrc->ppu8Plane[0], pSrcImg->ppu8Plane[0], pTmpSrc->i32Height*pTmpSrc->pi32Pitch[0]);
	MMemCpy(pTmpSrc->ppu8Plane[1], pSrcImg->ppu8Plane[1], pTmpSrc->i32Height * pTmpSrc->pi32Pitch[0] / 2);

	lHeight = pSrcImg->i32Height;
	lWidth = pSrcImg->i32Width;
	lYPitch = pSrcImg->pi32Pitch[0];
	lUVPitch = pSrcImg->pi32Pitch[1];
	smH = lHeight >> 2, smW = lWidth >> 2;
	smYPitch = (lYPitch + 3) >> 2;
	smUVPitch = (lWidth + 3) >> 2; // 中间结果不需要多分配padding
	uvBuf = (MByte*)MMemAlloc(hMemMgr, smH*smUVPitch * 5);
	resUV = (MByte*)MMemAlloc(hMemMgr, smH*smUVPitch * 2);
	pMask = (MByte*)MMemAlloc(hMemMgr, smH*smYPitch);
	if (uvBuf == MNull || resUV == MNull || pMask == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	guideRGB.u32PixelArrayFormat = ASVL_PAF_RGB24_B8G8R8;
	guideRGB.i32Height = smH;
	guideRGB.i32Width = smW;
	guideRGB.pi32Pitch[0] = smW * 3 * sizeof(MByte);
	guideRGB.ppu8Plane[0] = (MByte*)MMemAlloc(hMemMgr, guideRGB.i32Height*guideRGB.pi32Pitch[0]);
	if (guideRGB.ppu8Plane[0] == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}


#if 0
	smoothY = uvBuf;
	smoothUV = uvBuf + smH * smUVPitch;
	{
		lret = GaussPyrDown4(hMemMgr, mcvParallelMonitor, pSrcImg->ppu8Plane[0], smoothY, lWidth, lHeight, lYPitch, smW, smH, smYPitch);
		if (lret != MOK)
			goto exit;

		lret = GaussPyrDown2_C2(hMemMgr, mcvParallelMonitor, pSrcImg->ppu8Plane[1], smoothUV, lWidth, lHeight >> 1,
			lUVPitch, smW << 1, smH, smUVPitch);
		if (lret != MOK)
			goto exit;

		lret = convYUVToBGR(mcvParallelMonitor, smoothY, smoothUV, smYPitch, smUVPitch, &guideRGB);
		if (lret != MOK)
			goto exit;
	}
	srcU.u32PixelArrayFormat = ASVL_PAF_GRAY;
	srcU.i32Height = smH;
	srcU.i32Width = smW;
	srcU.pi32Pitch[0] = smW;
	srcU.ppu8Plane[0] = uvBuf;

	srcV.u32PixelArrayFormat = ASVL_PAF_GRAY;
	srcV.i32Height = smH;
	srcV.i32Width = smW;
	srcV.pi32Pitch[0] = smW;
	srcV.ppu8Plane[0] = uvBuf + smH * smW;

	for (i = 0; i < smH; i++)
	{
		MByte *uvptr = smoothUV + i * smUVPitch;
		MByte *uptr = srcU.ppu8Plane[0] + i * srcU.pi32Pitch[0];
		MByte *vptr = srcV.ppu8Plane[0] + i * srcV.pi32Pitch[0];
		for (j = 0; j < smW; j++)
		{
			uptr[j] = uvptr[2 * j];
			vptr[j] = uvptr[2 * j + 1];
		}
	}

	mat_write255(smH, smW, CV_8UC1, smoothY, "Y.jpg", 1.0);
	mat_write255(smH, smW, CV_8UC1, smoothUV, "U.jpg", 1.0);
	mat_write255(smH, smW, CV_8UC1, srcV.ppu8Plane[0], "V.jpg", 1.0);
	mat_write255(smH, smW, CV_8UC3, guideRGB.ppu8Plane[0], "rgb.jpg", 1.0);
#else
	ASVLOFFSCREEN smallSrcI444;
	{
		smallSrcI444.u32PixelArrayFormat = ASVL_PAF_I444;
		smallSrcI444.ppu8Plane[0] = uvBuf;
		smallSrcI444.ppu8Plane[1] = uvBuf + smH * smUVPitch;
		smallSrcI444.ppu8Plane[2] = uvBuf + smH * smUVPitch * 2;
		smallSrcI444.pi32Pitch[0] = smUVPitch;
		smallSrcI444.pi32Pitch[1] = smUVPitch;
		smallSrcI444.pi32Pitch[2] = smUVPitch;
		smallSrcI444.i32Height = smH;
		smallSrcI444.i32Width = smW;
	}

	Arcsoft_NV21_DownScale4_To_I444_RGB(hMemMgr, mcvParallelMonitor, pSrcImg, smallSrcI444, guideRGB);
	//mat_write255(smH, smW, CV_8UC3, guideRGB.ppu8Plane[0], "rgb.jpg", 1.0);
	{
		srcU.u32PixelArrayFormat = ASVL_PAF_GRAY;
		srcU.i32Height = smH;
		srcU.i32Width = smW;
		srcU.pi32Pitch[0] = smUVPitch;
		srcU.ppu8Plane[0] = smallSrcI444.ppu8Plane[1];
	}

	{
		srcV.u32PixelArrayFormat = ASVL_PAF_GRAY;
		srcV.i32Height = smH;
		srcV.i32Width = smW;
		srcV.pi32Pitch[0] = smUVPitch;
		srcV.ppu8Plane[0] = smallSrcI444.ppu8Plane[2];
	}

#endif
	resU.u32PixelArrayFormat = ASVL_PAF_GRAY;
	resU.i32Height = smH;
	resU.i32Width = smW;
	resU.pi32Pitch[0] = smUVPitch;
	resU.ppu8Plane[0] = uvBuf + smH * smUVPitch * 3;

	resV.u32PixelArrayFormat = ASVL_PAF_GRAY;
	resV.i32Height = smH;
	resV.i32Width = smW;
	resV.pi32Pitch[0] = smUVPitch;
	resV.ppu8Plane[0] = uvBuf + smH * smUVPitch * 4;

	if (isBF) //采用新的值域滤波降彩噪
	{
		DECOLORNOISE_PARAM decolornoiseParam;
		{
			decolornoiseParam.chroma = lUVIntensity;
			decolornoiseParam.smooth = lUVIntensity * lUVIntensity / 100;
			decolornoiseParam.detail = 50 - decolornoiseParam.smooth / 2;
			MInt32 lStep = (lUVIntensity * 2 / 25) + 1;
			CLAMP(lStep, 1, 4);
			decolornoiseParam.step = lStep;
		}
		Arcsoft_ReduceColorNoise(hMemMgr, mcvParallelMonitor, 16).runYUV444(&smallSrcI444, &smallSrcI444, decolornoiseParam);
		//Arcsoft_ReduceColorNoise(hMemMgr, mcvParallelMonitor).runBGR2YUV(&guideRGB, &smallSrcI444, decolornoiseParam);
		resU.ppu8Plane[0] = smallSrcI444.ppu8Plane[1];
		resV.ppu8Plane[0] = smallSrcI444.ppu8Plane[2];
	}
	else
	{
		LOGD("kernelSizeUV = %d", kernelSizeUV);
		//RGB三通道的导向图滤波
		{
			ARCGF_PARAM pParam;
			ARC_GuidedFilter_GetDefaultParam(&pParam);
			pParam.gfRadius = (kernelSizeUV >> 1) + 1;
			pParam.gfEpsilon = 0.0001f * 255 * 255 * lUVIntensity;
			pParam.gfScale = 2;
			MHandle algorithmEngine = MNull;
			MInt32 gfMode = ARCGF_FAST_GUIDED_FILTER;
			{
				lret = ARC_GuidedFilter_Init(hMemMgr, &algorithmEngine, gfMode);
				if (MOK != lret)
					goto exit;
			}

			{
#ifdef UV_GUIDED
				lret = ARC_GuidedFilter_Create(mcvParallelMonitor, algorithmEngine, &guideRGB, &pParam, 1);
#else
				lret = ARC_GuidedFilter_Create(mcvParallelMonitor, algorithmEngine, &guideRGB, &pParam, 3);
#endif
				if (MOK != lret)
					goto exit;
			}


#ifdef UV_GUIDED // 对UV分别进行引导滤波
			lret = ARC_GuidedFilter_Filter(mcvParallelMonitor, algorithmEngine, &srcU, &resU);
			if (MOK != lret)
				goto exit;

			lret = ARC_GuidedFilter_Filter(mcvParallelMonitor, algorithmEngine, &srcV, &resV);
			if (MOK != lret)
				goto exit;
#else // 对RGB分别进行引导滤波
			{
				srcRGB.u32PixelArrayFormat = ASVL_PAF_RGB24_B8G8R8;
				srcRGB.i32Height = smH;
				srcRGB.i32Width = smW;
				srcRGB.pi32Pitch[0] = smW * 3 * sizeof(MByte);
				srcRGB.ppu8Plane[0] = (MByte*)MMemAlloc(hMemMgr, srcRGB.i32Height * srcRGB.pi32Pitch[0]);
				if (srcRGB.ppu8Plane[0] == MNull)
				{
					lret = MERR_NO_MEMORY;
					goto exit;
				}
				MMemCpy(srcRGB.ppu8Plane[0], guideRGB.ppu8Plane[0], srcRGB.i32Height * srcRGB.pi32Pitch[0]);
				mat_write255(smH, smW, CV_8UC3, srcRGB.ppu8Plane[0], "rgb.jpg", 1.0);
			}
			{
				dstNV21.u32PixelArrayFormat = ASVL_PAF_NV21;
				dstNV21.i32Height = smH;
				dstNV21.i32Width = smW;
				dstNV21.pi32Pitch[0] = smW;
				dstNV21.pi32Pitch[1] = smW;
				dstNV21.ppu8Plane[0] = (MByte*)MMemAlloc(hMemMgr, dstNV21.i32Height * dstNV21.pi32Pitch[0] * 3 / 2);
				dstNV21.ppu8Plane[1] = dstNV21.ppu8Plane[0] + dstNV21.i32Height * dstNV21.pi32Pitch[0];
				dstNV21.ppu8Plane[2] = MNull;
				dstNV21.pi32Pitch[2] = 0;

				if (dstNV21.ppu8Plane[0] == MNull)
				{
					lret = MERR_NO_MEMORY;
					goto exit;
				}
			}
			lret = ARC_GuidedFilter_Filter(mcvParallelMonitor, algorithmEngine, &srcRGB, &srcRGB);
			mat_write255(smH, smW, CV_8UC3, srcRGB.ppu8Plane[0], "dstRGB.jpg", 1.0);
			if (MOK != lret)
				goto exit;

			BGRToYUV444Planar(srcRGB.ppu8Plane[0], srcRGB.i32Width, srcRGB.i32Height, srcRGB.pi32Pitch[0],
				MNull, 0, resU.ppu8Plane[0], resU.pi32Pitch[0], resV.ppu8Plane[0], resV.pi32Pitch[0]);

			if (srcRGB.ppu8Plane[0])
			{
				MMemFree(hMemMgr, srcRGB.ppu8Plane[0]);
				srcRGB.ppu8Plane[0] = MNull;
			}
			if (dstNV21.ppu8Plane[0])
			{
				MMemFree(hMemMgr, dstNV21.ppu8Plane[0]);
				dstNV21.ppu8Plane[0] = MNull;
			}
#endif

			lret = ARC_GuidedFilter_Uninit(hMemMgr, &algorithmEngine);
			if (MOK != lret)
				goto exit;

		}
	}

	//copy to result image
	for (i = 0; i < smH; i++)
	{
		MByte *uvptr = resUV + i * smUVPitch * 2;
		MByte *uptr = resU.ppu8Plane[0] + i * resU.pi32Pitch[0];
		MByte *vptr = resV.ppu8Plane[0] + i * resV.pi32Pitch[0];
		for (j = 0; j < smW; j++) //todo:必须是NV21或者NV12,其他格式可能或造成内存溢出
		{
			uvptr[2 * j] = uptr[j];
			uvptr[2 * j + 1] = vptr[j];
		}
	}

	//upSample UV

	lret = FastBilinear_2X_8UC2(mcvParallelMonitor, resUV, smW << 1, smH, smUVPitch << 1, pSrcImg->ppu8Plane[1],
		lWidth, lHeight >> 1, pSrcImg->pi32Pitch[1]);

	if (MOK != lret)
		goto exit;

#if 0 // TODO：对颜色鲜艳的地方进行了保护，但会噪声一些颜色去不掉，暂时关闭掉
	lret = BlendGuideFilterUV(pTmpSrc, pSrcImg, lUVIntensity);
#endif

	// 根据mask对前后图像进行融合
	if (pShadeMap != MNull)
	{
		MixImages_UV(hMemMgr, mcvParallelMonitor, pTmpSrc, pSrcImg, pSrcImg, pShadeMap);// Map的大小为原图宽高的1/4
	}

exit:
	if (pTmpSrc)
	{
		if (pTmpSrc->ppu8Plane[0])
		{
			MMemFree(hMemMgr, pTmpSrc->ppu8Plane[0]);
			pTmpSrc->ppu8Plane[0] = MNull;
		}
		MMemFree(hMemMgr, pTmpSrc);
		pTmpSrc = MNull;
	}
	if (pMask)
	{
		MMemFree(hMemMgr, pMask);
		pMask = MNull;
	}
	if (uvBuf)
	{
		MMemFree(hMemMgr, uvBuf);
		uvBuf = MNull;
	}
	if (resUV)
	{
		MMemFree(hMemMgr, resUV);
		resUV = MNull;
	}
	if (guideRGB.ppu8Plane[0])
	{
		MMemFree(hMemMgr, guideRGB.ppu8Plane[0]);
		guideRGB.ppu8Plane[0] = MNull;
	}

	LOGD("ArcSoft_GuideFilter_For_DownSampleUV-");
	return lret;

}

MInt32 ArcSoft_GuideFilter_For_DownSampleUV(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 kernelSizeUV, MInt32 lUVIntensity, LPASVLOFFSCREEN pShadeMap)
{

#define UV_GUIDED

	START_TIME;

	MInt32 lret = MOK, i, j;
	ASVLOFFSCREEN srcU, srcV;
	ASVLOFFSCREEN resU, resV;
	MInt32 lHeight, lWidth, lYPitch, lUVPitch;
	MInt32 smH, smW, smYPitch, smUVPitch;
	MByte* uvBuf = MNull, * resUV = MNull;


	lHeight = pSrcImg->i32Height;
	lWidth = pSrcImg->i32Width;
	lYPitch = pSrcImg->pi32Pitch[0];
	lUVPitch = pSrcImg->pi32Pitch[1];
	smH = lHeight >> 2, smW = lWidth >> 2;
	smYPitch = (lYPitch + 3) >> 2;
	smUVPitch = (lWidth + 3) >> 2; // 中间结果不需要多分配padding
	uvBuf = (MByte*)MMemAlloc(hMemMgr, smH * smUVPitch * 5);
	resUV = (MByte*)MMemAlloc(hMemMgr, smH * smUVPitch * 2);
	if (uvBuf == MNull || resUV == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	ASVLOFFSCREEN smallSrcI444;
	{
		smallSrcI444.u32PixelArrayFormat = ASVL_PAF_I444;
		smallSrcI444.ppu8Plane[0] = uvBuf;
		smallSrcI444.ppu8Plane[1] = uvBuf + smH * smUVPitch;
		smallSrcI444.ppu8Plane[2] = uvBuf + smH * smUVPitch * 2;
		smallSrcI444.pi32Pitch[0] = smUVPitch;
		smallSrcI444.pi32Pitch[1] = smUVPitch;
		smallSrcI444.pi32Pitch[2] = smUVPitch;
		smallSrcI444.i32Height = smH;
		smallSrcI444.i32Width = smW;
	}

	Arcsoft_NV21_DownScale4_To_I444(hMemMgr, mcvParallelMonitor, pSrcImg, smallSrcI444);
	{
		srcU.u32PixelArrayFormat = ASVL_PAF_GRAY;
		srcU.i32Height = smH;
		srcU.i32Width = smW;
		srcU.pi32Pitch[0] = smUVPitch;
		srcU.ppu8Plane[0] = smallSrcI444.ppu8Plane[1];
	}
	{
		srcV.u32PixelArrayFormat = ASVL_PAF_GRAY;
		srcV.i32Height = smH;
		srcV.i32Width = smW;
		srcV.pi32Pitch[0] = smUVPitch;
		srcV.ppu8Plane[0] = smallSrcI444.ppu8Plane[2];
	}


	resU.u32PixelArrayFormat = ASVL_PAF_GRAY;
	resU.i32Height = smH;
	resU.i32Width = smW;
	resU.pi32Pitch[0] = smUVPitch;
	resU.ppu8Plane[0] = uvBuf + smH * smUVPitch * 3;

	resV.u32PixelArrayFormat = ASVL_PAF_GRAY;
	resV.i32Height = smH;
	resV.i32Width = smW;
	resV.pi32Pitch[0] = smUVPitch;
	resV.ppu8Plane[0] = uvBuf + smH * smUVPitch * 4;

	DECOLORNOISE_PARAM decolornoiseParam;
	{
		decolornoiseParam.chroma = lUVIntensity;
		decolornoiseParam.smooth = lUVIntensity * lUVIntensity / 100;
		decolornoiseParam.detail = 50 - decolornoiseParam.smooth / 2;
		MInt32 lStep = (lUVIntensity * 2 / 25) + 1;
		CLAMP(lStep, 1, 4);
		decolornoiseParam.step = lStep;
	}
	Arcsoft_ReduceColorNoise(hMemMgr, mcvParallelMonitor, 16).runYUV444(&smallSrcI444, &smallSrcI444, decolornoiseParam, resUV);
	resU.ppu8Plane[0] = smallSrcI444.ppu8Plane[1];
	resV.ppu8Plane[0] = smallSrcI444.ppu8Plane[2];


	//copy to result image
	for (i = 0; i < smH; i++)
	{
		MByte* uvptr = resUV + i * smUVPitch * 2;
		MByte* uptr = resU.ppu8Plane[0] + i * resU.pi32Pitch[0];
		MByte* vptr = resV.ppu8Plane[0] + i * resV.pi32Pitch[0];
        j = 0;
#ifdef USE_NEON
        for (; j < smW - 16; j += 16)
        {
            uint8x16x2_t vdata_uv;
            vdata_uv.val[0] = vld1q_u8(uptr + j);
            vdata_uv.val[1] = vld1q_u8(vptr + j);
            vst2q_u8(uvptr + j * 2, vdata_uv);
        }
#endif
		for (; j < smW; j++)
		{
			uvptr[2 * j] = uptr[j];
			uvptr[2 * j + 1] = vptr[j];
		}
	}

	//upSample UV
	lret = FastBilinear_2X_8UC2(mcvParallelMonitor, resUV, smW << 1, smH, smUVPitch << 1, pDstImg->ppu8Plane[1],
		lWidth, lHeight >> 1, pDstImg->pi32Pitch[1]);
	if (MOK != lret)
		goto exit;

#if 0 // TODO：对颜色鲜艳的地方进行了保护，但会噪声一些颜色去不掉，暂时关闭掉
	lret = BlendGuideFilterUV(pTmpSrc, pSrcImg, lUVIntensity);
#endif


exit:
	if (uvBuf)
	{
		MMemFree(hMemMgr, uvBuf);
		uvBuf = MNull;
	}
	if (resUV)
	{
		MMemFree(hMemMgr, resUV);
		resUV = MNull;
	}

	END_TIME;
	return lret;

}
NS_SINFLE_IMAGE_ENHANCEMENT_END

