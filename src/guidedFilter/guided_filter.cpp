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

#ifdef ASGF_DEBUG
#include <stdio.h>
#define OUTPUT_PATH "../Debug_LOG"
#endif

MRESULT guidedFilter_Create(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter* pAgfInfo,
	const MByte *guidanceData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels)
{
	MHandle hMemMgr = pAgfInfo->hMemMgr;

	MInt32 gfRadius = pAgfInfo->gfRadius;
	MFloat gfEpsilon = pAgfInfo->gfEpsilon;

	if (!guidanceData || gfRadius <= 0 || gfEpsilon <= 0 || taskNum <= 0
		|| pitch <= 0 || height <= 0 || width <= 0 || channels > 3)
	{
		return MERR_INVALID_PARAM;
	}

	MRESULT res = MOK;

	MInt32 pitch_sc_uint8 = width * sizeof(MUInt8);
	MInt32 pitch_sc_uint16 = width * sizeof(MUInt16);
	MInt32 pitch_sc_float32 = width * sizeof(MFloat);

	MInt32 c1 = 0, c2 = 0, total = 0, n = 0;

	// split channels
	res = SplitChannels(threadEngine, taskNum, guidanceData, pitch, height, width, channels, pAgfInfo->pGuidanceCnOri);
	CHECK_ERROR(res);

	for (c1 = 0; c1 < channels; ++c1)
	{
		// mean filter for guidanceCn -> guidanceCnMean
		res = BoxFilter_U8C1toF32C1(hMemMgr, threadEngine, pAgfInfo->pGuidanceCnOri[c1], pitch_sc_uint8,
			pAgfInfo->pGuidanceCnMean[c1], pitch_sc_float32, height, width, gfRadius);
		CHECK_ERROR(res);
		for (c2 = c1; c2 < channels; ++c2)
		{
			// calculate corrGuidance
			res = DotMultiply_U8C1toU16C1(hMemMgr, threadEngine, taskNum, pAgfInfo->pGuidanceCnOri[c1], pAgfInfo->pGuidanceCnOri[c2],
				pitch_sc_uint8, pAgfInfo->pCorrGuidance[total], pitch_sc_uint16, height, width);
			CHECK_ERROR(res);
			// mean filter for corrGuidance -> corrGuidanceMean
			res = BoxFilter_U16C1toF32C1(hMemMgr, threadEngine, pAgfInfo->pCorrGuidance[total], pitch_sc_uint16,
				pAgfInfo->pCorrGuidanceMean[total], pitch_sc_float32, height, width, gfRadius);
			CHECK_ERROR(res);

			++total;
		}
	}

	// calculate covariance matrix
	for (n = 0; n < total; ++n)
	{
		GetCovGuidanceIdx(n, channels, c1, c2);
		if (c1 == c2)
		{
			res = CalcVarAddEpsilon_F32C1(threadEngine, taskNum,
				pAgfInfo->pGuidanceCnMean[c1], pAgfInfo->pCorrGuidanceMean[n], pAgfInfo->pCovGuidance[n],
				pitch_sc_float32, height, width, gfEpsilon);
			CHECK_ERROR(res);
		}
		else
		{
			res = CalcCovar_F32C1(threadEngine, taskNum,
				pAgfInfo->pGuidanceCnMean[c1], pAgfInfo->pGuidanceCnMean[c2], pAgfInfo->pCorrGuidanceMean[n],
				pAgfInfo->pCovGuidance[n], pitch_sc_float32, height, width);
			CHECK_ERROR(res);
		}
	}

	// calculate inverse of covariance matrix
	res = CalcCovarInv_F32(threadEngine, taskNum, pAgfInfo->pCovGuidance, 
		pitch_sc_float32, height, width, channels, pAgfInfo->pCovInvGuidance);
	CHECK_ERROR(res);

exit:
	return res;
}

MRESULT guidedFilter_Filter(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter* pAgfInfo,
	const MByte *srcData, MByte *dstData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels)
{
	MHandle hMemMgr = pAgfInfo->hMemMgr;

	MInt32 gfRadius = pAgfInfo->gfRadius;
	MInt32 guidanceHeight = pAgfInfo->height,
		guidanceWidth = pAgfInfo->width,
		guidanceCnNum = pAgfInfo->guidanceCnNum,
		srcCnNum = pAgfInfo->srcCnNum;

	if (guidanceHeight != height || guidanceWidth != width || channels != srcCnNum
		|| !srcData || !dstData || pitch <= 0 || taskNum <= 0)
	{
		return MERR_INVALID_PARAM;
	}

	MRESULT res = MOK;

	MInt32 pitch_sc_uint8 = width * sizeof(MUInt8);
	MInt32 pitch_sc_uint16 = width * sizeof(MUInt16);
	MInt32 pitch_sc_float32 = width * sizeof(MFloat);

	MInt32 c1 = 0, c2 = 0, idx = 0;

	// split channels
	res = SplitChannels(threadEngine, taskNum, srcData, pitch, height, width, channels, pAgfInfo->pSrcCnOri);
	CHECK_ERROR(res);

	for (c1 = 0; c1 < channels; ++c1)
	{
		// mean filter for srcCn
		res = BoxFilter_U8C1toF32C1(hMemMgr, threadEngine,
			pAgfInfo->pSrcCnOri[c1], pitch_sc_uint8, pAgfInfo->pSrcCnMean[c1], pitch_sc_float32, height, width, gfRadius);
		CHECK_ERROR(res);
		for (c2 = 0; c2 < guidanceCnNum; ++c2)
		{
			idx = c1 * channels + c2;
			// calculate corrGuidanceSrc
			res = DotMultiply_U8C1toU16C1(hMemMgr, threadEngine, taskNum, 
				pAgfInfo->pSrcCnOri[c1], pAgfInfo->pGuidanceCnOri[c2], pitch_sc_uint8, 
				pAgfInfo->pCorrGuidanceSrc[idx], pitch_sc_uint16, height, width);
			CHECK_ERROR(res);
			// mean filter corrGuidanceSrc
			res = BoxFilter_U16C1toF32C1(hMemMgr, threadEngine, pAgfInfo->pCorrGuidanceSrc[idx], pitch_sc_uint16,
				pAgfInfo->pCorrGuidanceSrcMean[idx], pitch_sc_float32, height, width, gfRadius);
			CHECK_ERROR(res);
			// calculate covGuidanceSrc
			res = CalcCovar_F32C1(threadEngine, taskNum, pAgfInfo->pSrcCnMean[c1], pAgfInfo->pGuidanceCnMean[c2],
				pAgfInfo->pCorrGuidanceSrcMean[idx], pAgfInfo->pCovGuidanceSrc[idx], pitch_sc_float32, height, width);
			CHECK_ERROR(res);
		}
	}

	// calculate alpha
	res = CalcAlpha_F32(threadEngine, taskNum, pAgfInfo->pCovGuidanceSrc, channels,
		pAgfInfo->pCovInvGuidance, guidanceCnNum, pitch_sc_float32, height, width, pAgfInfo->pAlpha);
	CHECK_ERROR(res);

	// calculate beta
	res = CalcBeta_F32(threadEngine, taskNum, pAgfInfo->pSrcCnMean, channels,
		pAgfInfo->pGuidanceCnMean, guidanceCnNum, pAgfInfo->pAlpha, pitch_sc_float32, height, width, pAgfInfo->pBeta);
	CHECK_ERROR(res);

	// mean filter for alpha && beta
	for (c1 = 0; c1 < channels; ++c1)
	{
		res = BoxFilter_F32C1(threadEngine, taskNum,
			pAgfInfo->pBeta[c1], pAgfInfo->pBetaMean[c1], pitch_sc_float32, height, width, gfRadius);
		CHECK_ERROR(res);
		for (c2 = 0; c2 < guidanceCnNum; ++c2)
		{
			res = BoxFilter_F32C1(threadEngine, taskNum,
				pAgfInfo->pAlpha[c1 * channels + c2], pAgfInfo->pAlphaMean[c1 * channels + c2],
				pitch_sc_float32, height, width, gfRadius);
			CHECK_ERROR(res);
		}
	}

	// apply transform to obtain the final results
	res = ApplyTransform_F32U8(threadEngine, taskNum, 
		pAgfInfo->pAlphaMean, pAgfInfo->pBetaMean, channels, pitch_sc_float32,
		pAgfInfo->pGuidanceCnOri, guidanceCnNum, pitch_sc_uint8, height, width, pAgfInfo->pDstCn);
	CHECK_ERROR(res);

	// merge channels and convert to UCHAR type
	res = MergeChannelsAndConvertToDstType(threadEngine, taskNum,
		pAgfInfo->pDstCn, pitch_sc_float32, dstData, pitch, height, width, channels);
	CHECK_ERROR(res);

exit:
	return res;
}

MRESULT fastGuidedFilter_Create(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter* pAgfInfo,
	const MByte *guidanceData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels)
{
	MHandle hMemMgr = pAgfInfo->hMemMgr;

	MInt32 gfRadius = pAgfInfo->gfRadius;
	MFloat gfEpsilon = pAgfInfo->gfEpsilon;
	MInt32 gfScale = pAgfInfo->gfScale;

	if (!guidanceData || gfRadius <= 0 || gfEpsilon <= 0 || gfScale <= 0 || taskNum <= 0
		|| pitch <= 0 || height <= 0 || width <= 0 || channels > 3)
	{
		return MERR_INVALID_PARAM;
	}

	MRESULT res = MOK;

	// temp variable for calculate I .* I
	BilinearResize_Param blParam = { 0 };
	blParam.pBuf = MNull;

	MInt32 heightSub = height / gfScale;
	MInt32 widthSub = width / gfScale;
	MInt32 gfRadiusSub = gfRadius / gfScale;

	MInt32 pitch_sc_uint8 = width * sizeof(MUInt8);
	MInt32 pitchSub_sc_uint8 = widthSub * sizeof(MUInt8);
	MInt32 pitchSub_sc_float32 = widthSub * sizeof(MFloat);
	MInt32 pitchSub_sc_uint16 = widthSub * sizeof(MUInt16);

	MInt32 c1 = 0, c2 = 0, total = 0, n = 0;

	// split channels
	res = SplitChannels(threadEngine, taskNum,
		guidanceData, pitch, height, width, channels, pAgfInfo->pGuidanceCnOri);
	CHECK_ERROR(res);

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			sprintf(szName, "%s/guidanceCnOri_%d.txt", OUTPUT_PATH, c1);
			FILE *fp = fopen(szName, "w");
			if (fp)
			{
				MByte *tmpImg = pAgfInfo->pGuidanceCnOri[c1];
				for (MInt32 y = 0; y < height; ++y)
				{
					MByte *pD = tmpImg + y * pitch_sc_uint8;
					for (MInt32 x = 0; x < width; ++x)
						fprintf(fp, "%d ", (int)pD[x]);
					fprintf(fp, "\n");
				}

				fclose(fp);
			}
		}
	}
#endif

	// alloc memory
	res = ImgBilinearResizeAllocMem_U8C1(hMemMgr, width, height, widthSub, heightSub, taskNum, &blParam);
	CHECK_ERROR(res);

	// down-sampling
	for (c1 = 0; c1 < channels; ++c1)
	{
		res = ImgBilinearResize_U8C1(threadEngine, taskNum, pAgfInfo->pGuidanceCnOri[c1], width, height, pitch_sc_uint8,
			pAgfInfo->pGuidanceCnSub[c1], widthSub, heightSub, pitchSub_sc_uint8, &blParam);
		CHECK_ERROR(res);
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			sprintf(szName, "%s/guidanceCnSub_%d.txt", OUTPUT_PATH, c1);
			FILE *fp = fopen(szName, "w");
			if (fp)
			{
				MByte *tmpImg = pAgfInfo->pGuidanceCnSub[c1];
				for (MInt32 y = 0; y < heightSub; ++y)
				{
					MByte *pD = tmpImg + y * pitchSub_sc_uint8;
					for (MInt32 x = 0; x < widthSub; ++x)
						fprintf(fp, "%d ", (int)pD[x]);
					fprintf(fp, "\n");
				}

				fclose(fp);
			}
		}
	}
#endif

	total = 0;
	for (c1 = 0; c1 < channels; ++c1)
	{
		// mean filter: guidanceCn -> guidanceCnMean
		res = BoxFilter_U8C1toF32C1(hMemMgr, threadEngine, pAgfInfo->pGuidanceCnSub[c1], pitchSub_sc_uint8, 
			pAgfInfo->pGuidanceCnMean[c1], pitchSub_sc_float32, heightSub, widthSub, gfRadiusSub);
		CHECK_ERROR(res);
		for (c2 = c1; c2 < channels; ++c2)
		{
			// calculate corrGuidance
			res = DotMultiply_U8C1toU16C1(hMemMgr, threadEngine, taskNum, 
				pAgfInfo->pGuidanceCnSub[c1], pAgfInfo->pGuidanceCnSub[c2], pitchSub_sc_uint8,
				pAgfInfo->pCorrGuidance[total], pitchSub_sc_uint16, heightSub, widthSub);
			CHECK_ERROR(res);
			// mean filter for corrGuidance -> corrGuidanceMean
			res = BoxFilter_U16C1toF32C1(hMemMgr, threadEngine, pAgfInfo->pCorrGuidance[total], pitchSub_sc_uint16,
				pAgfInfo->pCorrGuidanceMean[total], pitchSub_sc_float32, heightSub, widthSub, gfRadiusSub);
			CHECK_ERROR(res);
			total++;
		}
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			sprintf(szName, "%s/guidanceCnMean_%d.txt", OUTPUT_PATH, c1);
			FILE *fp = fopen(szName, "w");
			if (fp)
			{
				MFloat *tmpImg = pAgfInfo->pGuidanceCnMean[c1];
				for (MInt32 y = 0; y < heightSub; ++y)
				{
					MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
					for (MInt32 x = 0; x < widthSub; ++x)
						fprintf(fp, "%f ", pD[x]);
					fprintf(fp, "\n");
				}

				fclose(fp);
			}
		}

		total = 0;
		for (c1 = 0; c1 < channels; ++c1)
		{
			for (c2 = c1; c2 < channels; ++c2)
			{
				sprintf(szName, "%s/corrGuidance_%d.txt", OUTPUT_PATH, total);
				FILE *fp = fopen(szName, "w");
				if (fp)
				{
					MUInt16 *tmpImg = pAgfInfo->pCorrGuidance[total];
					for (MInt32 y = 0; y < heightSub; ++y)
					{
						MUInt16 *pD = tmpImg + y * pitchSub_sc_uint8;
						for (MInt32 x = 0; x < widthSub; ++x)
							fprintf(fp, "%d ", (int)pD[x]);
						fprintf(fp, "\n");
					}

					fclose(fp);
				}

				total++;
			}
		}

		total = 0;
		for (c1 = 0; c1 < channels; ++c1)
		{
			for (c2 = c1; c2 < channels; ++c2)
			{
				sprintf(szName, "%s/corrGuidanceMean_%d.txt", OUTPUT_PATH, total);
				FILE *fp = fopen(szName, "w");
				if (fp)
				{
					MFloat *tmpImg = pAgfInfo->pCorrGuidanceMean[total];
					for (MInt32 y = 0; y < heightSub; ++y)
					{
						MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
						for (MInt32 x = 0; x < widthSub; ++x)
							fprintf(fp, "%f ", pD[x]);
						fprintf(fp, "\n");
					}

					fclose(fp);
				}

				total++;
			}
		}
	}
#endif

	// calculate covariance matrix
	for (n = 0; n < total; ++n)
	{
		GetCovGuidanceIdx(n, channels, c1, c2);
		if (c1 == c2)
		{
			res = CalcVarAddEpsilon_F32C1(threadEngine, taskNum,
				pAgfInfo->pGuidanceCnMean[c1], pAgfInfo->pCorrGuidanceMean[n], pAgfInfo->pCovGuidance[n],
				pitchSub_sc_float32, heightSub, widthSub, gfEpsilon);
			CHECK_ERROR(res);
		}
		else
		{
			res = CalcCovar_F32C1(threadEngine, taskNum,
				pAgfInfo->pGuidanceCnMean[c1], pAgfInfo->pGuidanceCnMean[c2], pAgfInfo->pCorrGuidanceMean[n],
				pAgfInfo->pCovGuidance[n], pitchSub_sc_float32, heightSub, widthSub);
			CHECK_ERROR(res);
		}
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];

		total = 0;
		for (c1 = 0; c1 < channels; ++c1)
		{
			for (c2 = c1; c2 < channels; ++c2)
			{
				sprintf(szName, "%s/covGuidanceMean_%d.txt", OUTPUT_PATH, total);
				FILE *fp = fopen(szName, "w");
				if (fp)
				{
					MFloat *tmpImg = pAgfInfo->pCovGuidance[total];
					for (MInt32 y = 0; y < heightSub; ++y)
					{
						MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
						for (MInt32 x = 0; x < widthSub; ++x)
							fprintf(fp, "%f ", pD[x]);
						fprintf(fp, "\n");
					}

					fclose(fp);
				}

				total++;
			}
		}
	}
#endif

	// calculate inverse of covariance matrix
	res = CalcCovarInv_F32(threadEngine, taskNum, 
		pAgfInfo->pCovGuidance, pitchSub_sc_float32, heightSub, widthSub, channels, pAgfInfo->pCovInvGuidance);
	CHECK_ERROR(res);

#ifdef ASGF_DEBUG
	{
		char szName[200];

		total = 0;
		for (c1 = 0; c1 < channels; ++c1)
		{
			for (c2 = c1; c2 < channels; ++c2)
			{
				sprintf(szName, "%s/covInvGuidance_%d.txt", OUTPUT_PATH, total);
				FILE *fp = fopen(szName, "w");
				if (fp)
				{
					MFloat *tmpImg = pAgfInfo->pCovInvGuidance[total];
					for (MInt32 y = 0; y < heightSub; ++y)
					{
						MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
						for (MInt32 x = 0; x < widthSub; ++x)
							fprintf(fp, "%f ", pD[x]);
						fprintf(fp, "\n");
					}

					fclose(fp);
				}

				total++;
			}
		}
	}
#endif

exit:
	// free memory
	if (blParam.pBuf)
	{
		MMemFree(hMemMgr, blParam.pBuf);
		blParam.pBuf = MNull;
	}

	return res;
}

MRESULT fastGuidedFilter_Filter(MHandle threadEngine, MInt32 taskNum, ArcGuidedFilter* pAgfInfo,
	const MByte *srcData, MByte *dstData, MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels)
{
	MHandle hMemMgr = pAgfInfo->hMemMgr;

	MInt32 gfRadius = pAgfInfo->gfRadius;
	MInt32 gfScale = pAgfInfo->gfScale;
	MInt32 guidanceHeight = pAgfInfo->height, guidanceWidth = pAgfInfo->width,
		guidanceCnNum = pAgfInfo->guidanceCnNum, srcCnNum = pAgfInfo->srcCnNum;

	static MInt32 k = 1;

	if (guidanceHeight != height || guidanceWidth != width || channels != srcCnNum
		|| gfScale <= 0 || !srcData || !dstData || pitch <= 0 || taskNum <= 0)
	{
		return MERR_INVALID_PARAM;
	}

	MRESULT res = MOK;

	MInt32 heightSub = height / gfScale;
	MInt32 widthSub = width / gfScale;
	MInt32 gfRadiusSub = gfRadius / gfScale;

	MInt32 pitch_sc_uint8 = width * sizeof(MUInt8);
	MInt32 pitch_sc_float32 = width * sizeof(MFloat);
	MInt32 pitchSub_sc_uint8 = widthSub * sizeof(MUInt8);
	MInt32 pitchSub_sc_uint16 = widthSub * sizeof(MUInt16);
	MInt32 pitchSub_sc_float32 = widthSub * sizeof(MFloat);

	BilinearResize_Param blParam = { 0 };
	blParam.pBuf = MNull;

	MInt32 c1 = 0, c2 = 0, idx = 0;

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
	MInt32 lTime;
	//struct timeval time1, time2;
	PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter start", k);
	START_PROFILE();
#endif

	// split channels and convert to FLOAT type
	res = SplitChannels(threadEngine, taskNum, srcData, pitch, height, width, channels, pAgfInfo->pSrcCnOri);
	CHECK_ERROR(res);

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			sprintf(szName, "%s/srcCnOri_%d.txt", OUTPUT_PATH, c1);
			FILE *fp = fopen(szName, "w");
			if (fp)
			{
				MByte *tmpImg = pAgfInfo->pSrcCnOri[c1];
				for (MInt32 y = 0; y < height; ++y)
				{
					MByte *pD = tmpImg + y * pitch_sc_uint8;
					for (MInt32 x = 0; x < width; ++x)
						fprintf(fp, "%d ", (int)pD[x]);
					fprintf(fp, "\n");
				}

				fclose(fp);
			}
		}
	}
#endif

	// alloc memory
	res = ImgBilinearResizeAllocMem_U8C1(hMemMgr, width, height, widthSub, heightSub, taskNum, &blParam);
	CHECK_ERROR(res);

	// down-sampling
	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1,time2;	
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---ImgBilinearResize_U8C1_DownSample start", k);	
		START_PROFILE();
#endif

		for (c1 = 0; c1 < channels; ++c1)
		{
			res = ImgBilinearResize_U8C1(threadEngine, taskNum, pAgfInfo->pSrcCnOri[c1], width, height, pitch_sc_uint8,
				pAgfInfo->pSrcCnSub[c1], widthSub, heightSub, pitchSub_sc_uint8, &blParam);
			CHECK_ERROR(res);
		}

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---ImgBilinearResize_U8C1_DownSample consume time = %d", k, lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---ImgBilinearResize_U8C1_DownSample end lret = %d", k, res);
#endif
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			sprintf(szName, "%s/srcCnSub_%d.txt", OUTPUT_PATH, c1);
			FILE *fp = fopen(szName, "w");
			if (fp)
			{
				MByte *tmpImg = pAgfInfo->pSrcCnSub[c1];
				for (MInt32 y = 0; y < heightSub; ++y)
				{
					MByte *pD = tmpImg + y * pitchSub_sc_uint8;
					for (MInt32 x = 0; x < widthSub; ++x)
						fprintf(fp, "%d ", (int)pD[x]);
					fprintf(fp, "\n");
				}

				fclose(fp);
			}
		}
	}
#endif

	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---calcSrcCorrCov start", k);
		START_PROFILE();
#endif

		for (c1 = 0; c1 < channels; ++c1)
		{
			// mean filter for srcCn
			res = BoxFilter_U8C1toF32C1(hMemMgr, threadEngine,
				pAgfInfo->pSrcCnSub[c1], pitchSub_sc_uint8, pAgfInfo->pSrcCnMean[c1], pitchSub_sc_float32, heightSub, widthSub, gfRadiusSub);
			CHECK_ERROR(res);
			for (c2 = 0; c2 < guidanceCnNum; ++c2)
			{
				idx = c1 * channels + c2;
				// calculate corrGuidanceSrc
				res = DotMultiply_U8C1toU16C1(hMemMgr, threadEngine, taskNum,
					pAgfInfo->pSrcCnSub[c1], pAgfInfo->pGuidanceCnSub[c2], pitchSub_sc_uint8,
					pAgfInfo->pCorrGuidanceSrc[idx], pitchSub_sc_uint16, heightSub, widthSub);
				CHECK_ERROR(res);
				// mean filter corrGuidanceSrc -> corrGuidanceSrcMean
				res = BoxFilter_U16C1toF32C1(hMemMgr, threadEngine,
					pAgfInfo->pCorrGuidanceSrc[idx], pitchSub_sc_uint16,
					pAgfInfo->pCorrGuidanceSrcMean[idx], pitchSub_sc_float32, heightSub, widthSub, gfRadiusSub);
				CHECK_ERROR(res);
				// calculate covGuidanceSrc
				res = CalcCovar_F32C1(threadEngine, taskNum, pAgfInfo->pSrcCnMean[c1], pAgfInfo->pGuidanceCnMean[c2],
					pAgfInfo->pCorrGuidanceSrcMean[idx], pAgfInfo->pCovGuidanceSrc[idx],
					pitchSub_sc_float32, heightSub, widthSub);
				CHECK_ERROR(res);
			}
		}

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---calcSrcCorrCov consume time = %d", k, lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---calcSrcCorrCov end lret = %d", k, res);
#endif
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			sprintf(szName, "%s/srcCnMean_%d.txt", OUTPUT_PATH, c1);
			FILE *fp = fopen(szName, "w");
			if (fp)
			{
				MFloat *tmpImg = pAgfInfo->pSrcCnMean[c1];
				for (MInt32 y = 0; y < heightSub; ++y)
				{
					MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
					for (MInt32 x = 0; x < widthSub; ++x)
						fprintf(fp, "%f ", pD[x]);
					fprintf(fp, "\n");
				}

				fclose(fp);
			}
		}

		for (c1 = 0; c1 < channels; ++c1)
		{
			for (c2 = 0; c2 < guidanceCnNum; ++c2)
			{
				sprintf(szName, "%s/corrGuidanceSrc_%d_%d.txt", OUTPUT_PATH, c1, c2);
				FILE *fp = fopen(szName, "w");
				if (fp)
				{
					MUInt16 *tmpImg = pAgfInfo->pCorrGuidanceSrc[c1 * channels + c2];
					for (MInt32 y = 0; y < heightSub; ++y)
					{
						MUInt16 *pD = tmpImg + y * pitchSub_sc_uint8;
						for (MInt32 x = 0; x < widthSub; ++x)
							fprintf(fp, "%d ", (int)pD[x]);
						fprintf(fp, "\n");
					}

					fclose(fp);
				}
			}
		}

		for (c1 = 0; c1 < channels; ++c1)
		{
			for (c2 = 0; c2 < guidanceCnNum; ++c2)
			{
				sprintf(szName, "%s/corrGuidanceSrcMean_%d_%d.txt", OUTPUT_PATH, c1, c2);
				FILE *fp = fopen(szName, "w");
				if (fp)
				{
					MFloat *tmpImg = pAgfInfo->pCorrGuidanceSrcMean[c1 * channels + c2];
					for (MInt32 y = 0; y < heightSub; ++y)
					{
						MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
						for (MInt32 x = 0; x < widthSub; ++x)
							fprintf(fp, "%f ", pD[x]);
						fprintf(fp, "\n");
					}

					fclose(fp);
				}
			}
		}

		for (c1 = 0; c1 < channels; ++c1)
		{
			for (c2 = 0; c2 < guidanceCnNum; ++c2)
			{
				sprintf(szName, "%s/covGuidanceSrc_%d_%d.txt", OUTPUT_PATH, c1, c2);
				FILE *fp = fopen(szName, "w");
				if (fp)
				{
					MFloat *tmpImg = pAgfInfo->pCovGuidanceSrc[c1 * channels + c2];
					for (MInt32 y = 0; y < heightSub; ++y)
					{
						MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
						for (MInt32 x = 0; x < widthSub; ++x)
							fprintf(fp, "%f ", pD[x]);
						fprintf(fp, "\n");
					}

					fclose(fp);
				}
			}
		}
	}
#endif

	// calculate alpha
	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---CalcAlpha_F32 start", k);
		START_PROFILE();
#endif

		res = CalcAlpha_F32(threadEngine, taskNum, pAgfInfo->pCovGuidanceSrc, channels,
			pAgfInfo->pCovInvGuidance, guidanceCnNum, pitchSub_sc_float32, heightSub, widthSub, pAgfInfo->pAlphaSub);
		CHECK_ERROR(res);

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---CalcAlpha_F32 consume time = %d", k, lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---CalcAlpha_F32 end lret = %d", k, res);
#endif
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			for (c2 = 0; c2 < guidanceCnNum; ++c2)
			{
				sprintf(szName, "%s/alphaSub_%d_%d.txt", OUTPUT_PATH, c1, c2);
				FILE *fp = fopen(szName, "w");
				if (fp)
				{
					MFloat *tmpImg = pAgfInfo->pAlphaSub[c1 * channels + c2];
					for (MInt32 y = 0; y < heightSub; ++y)
					{
						MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
						for (MInt32 x = 0; x < widthSub; ++x)
							fprintf(fp, "%f ", pD[x]);
						fprintf(fp, "\n");
					}

					fclose(fp);
				}
			}
		}
	}
#endif

	// calculate beta
	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---CalcBeta_F32 start", k);
		START_PROFILE();
#endif

		res = CalcBeta_F32(threadEngine, taskNum, pAgfInfo->pSrcCnMean, channels,
			pAgfInfo->pGuidanceCnMean, guidanceCnNum, pAgfInfo->pAlphaSub, pitchSub_sc_float32, heightSub, widthSub, pAgfInfo->pBetaSub);

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---CalcBeta_F32 consume time = %d", k, lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---CalcBeta_F32 end lret = %d", k, res);
#endif
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			sprintf(szName, "%s/betaSub_%d.txt", OUTPUT_PATH, c1);
			FILE *fp = fopen(szName, "w");
			if (fp)
			{
				MFloat *tmpImg = pAgfInfo->pBetaSub[c1];
				for (MInt32 y = 0; y < heightSub; ++y)
				{
					MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
					for (MInt32 x = 0; x < widthSub; ++x)
						fprintf(fp, "%f ", pD[x]);
					fprintf(fp, "\n");
				}

				fclose(fp);
			}
		}
	}
#endif

	// mean filter for alpha && beta
	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---calcMeanOfAlphaAndBeta start", k);
		START_PROFILE();
#endif

		for (c1 = 0; c1 < channels; ++c1)
		{
			res = BoxFilter_F32C1(threadEngine, taskNum,
				pAgfInfo->pBetaSub[c1], pAgfInfo->pBetaSubMean[c1], pitchSub_sc_float32, heightSub, widthSub, gfRadiusSub);
			CHECK_ERROR(res);
			for (c2 = 0; c2 < guidanceCnNum; ++c2)
			{
				idx = c1 * channels + c2;
				res = BoxFilter_F32C1(threadEngine, taskNum, pAgfInfo->pAlphaSub[idx], pAgfInfo->pAlphaSubMean[idx],
					pitchSub_sc_float32, heightSub, widthSub, gfRadiusSub);
				CHECK_ERROR(res);
			}
		}

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---calcMeanOfAlphaAndBeta consume time = %d", k, lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---calcMeanOfAlphaAndBeta end lret = %d", k, res);
#endif
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			for (c2 = 0; c2 < guidanceCnNum; ++c2)
			{
				sprintf(szName, "%s/alphaSubMean_%d_%d.txt", OUTPUT_PATH, c1, c2);
				FILE *fp = fopen(szName, "w");
				if (fp)
				{
					MFloat *tmpImg = pAgfInfo->pAlphaSubMean[c1 * channels + c2];
					for (MInt32 y = 0; y < heightSub; ++y)
					{
						MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
						for (MInt32 x = 0; x < widthSub; ++x)
							fprintf(fp, "%f ", pD[x]);
						fprintf(fp, "\n");
					}

					fclose(fp);
				}
			}
		}

		for (c1 = 0; c1 < channels; ++c1)
		{
			sprintf(szName, "%s/betaSubMean_%d.txt", OUTPUT_PATH, c1);
			FILE *fp = fopen(szName, "w");
			if (fp)
			{
				MFloat *tmpImg = pAgfInfo->pBetaSubMean[c1];
				for (MInt32 y = 0; y < heightSub; ++y)
				{
					MFloat *pD = tmpImg + y * pitchSub_sc_uint8;
					for (MInt32 x = 0; x < widthSub; ++x)
						fprintf(fp, "%f ", pD[x]);
					fprintf(fp, "\n");
				}

				fclose(fp);
			}
		}
	}
#endif

	// alloc memory
	res = ImgBilinearResizeAllocMem_F32C1(hMemMgr, widthSub, heightSub, width, height, taskNum, &blParam);
	CHECK_ERROR(res);

	// up-sampling
	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---ImgBilinearResize_F32C1_UpSample start", k);
		START_PROFILE();
#endif

		for (c1 = 0; c1 < channels; ++c1)
		{
			res = ImgBilinearResize_F32C1(threadEngine, taskNum, pAgfInfo->pBetaSubMean[c1], widthSub, heightSub, pitchSub_sc_float32,
				pAgfInfo->pBetaMean[c1], width, height, pitch_sc_float32, &blParam);
			CHECK_ERROR(res);
			for (c2 = 0; c2 < guidanceCnNum; ++c2)
			{
				idx = c1 * channels + c2;
				res = ImgBilinearResize_F32C1(threadEngine, taskNum,
					pAgfInfo->pAlphaSubMean[idx], widthSub, heightSub, pitchSub_sc_float32,
					pAgfInfo->pAlphaMean[idx], width, height, pitch_sc_float32, &blParam);
				CHECK_ERROR(res);
			}
		}

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---ImgBilinearResize_F32C1_UpSample consume time = %d", k, lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---ImgBilinearResize_F32C1_UpSample end lret = %d", k, res);
#endif
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			for (c2 = 0; c2 < guidanceCnNum; ++c2)
			{
				sprintf(szName, "%s/alphaMean_%d_%d.txt", OUTPUT_PATH, c1, c2);
				FILE *fp = fopen(szName, "w");
				if (fp)
				{
					MFloat *tmpImg = pAgfInfo->pAlphaMean[c1 * channels + c2];
					for (MInt32 y = 0; y < height; ++y)
					{
						MFloat *pD = tmpImg + y * pitch_sc_uint8;
						for (MInt32 x = 0; x < width; ++x)
							fprintf(fp, "%f ", pD[x]);
						fprintf(fp, "\n");
					}

					fclose(fp);
				}
			}
		}

		for (c1 = 0; c1 < channels; ++c1)
		{
			sprintf(szName, "%s/betaMean_%d.txt", OUTPUT_PATH, c1);
			FILE *fp = fopen(szName, "w");
			if (fp)
			{
				MFloat *tmpImg = pAgfInfo->pBetaMean[c1];
				for (MInt32 y = 0; y < height; ++y)
				{
					MFloat *pD = tmpImg + y * pitch_sc_uint8;
					for (MInt32 x = 0; x < width; ++x)
						fprintf(fp, "%f ", pD[x]);
					fprintf(fp, "\n");
				}

				fclose(fp);
			}
		}
	}
#endif

	// apply transform to obtain the final results
	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---ApplyTransform_F32U8 start", k);
		START_PROFILE();
#endif

		res = ApplyTransform_F32U8(threadEngine, taskNum, pAgfInfo->pAlphaMean, pAgfInfo->pBetaMean, channels, pitch_sc_float32,
			pAgfInfo->pGuidanceCnOri, guidanceCnNum, pitch_sc_uint8, height, width, pAgfInfo->pDstCn);
		CHECK_ERROR(res);

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---ApplyTransform_F32U8 consume time = %d", k, lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---ApplyTransform_F32U8 end lret = %d", k, res);
#endif
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];
		for (c1 = 0; c1 < channels; ++c1)
		{
			sprintf(szName, "%s/dstCn_%d.txt", OUTPUT_PATH, c1);
			FILE *fp = fopen(szName, "w");
			if (fp)
			{
				MFloat *tmpImg = pAgfInfo->pDstCn[c1];
				for (MInt32 y = 0; y < height; ++y)
				{
					MFloat *pD = tmpImg + y * pitch_sc_uint8;
					for (MInt32 x = 0; x < width; ++x)
						fprintf(fp, "%f ", pD[x]);
					fprintf(fp, "\n");
				}

				fclose(fp);
			}
		}
	}
#endif

	// merge channels and convert to UCHAR type
	{
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		MInt32 lTime;
		//struct timeval time1, time2;
		PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---MergeChannelsAndConvertToDstType start", k);
		START_PROFILE();
#endif

		res = MergeChannelsAndConvertToDstType(threadEngine, taskNum,
			pAgfInfo->pDstCn, pitch_sc_float32, dstData, pitch, height, width, channels);
		CHECK_ERROR(res);

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
		END_PROFILE(lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---MergeChannelsAndConvertToDstType consume time = %d", k, lTime);
		PrintfB(6,"VNS","aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter---MergeChannelsAndConvertToDstType end lret = %d", k, res);
#endif
	}

#ifdef ASGF_DEBUG
	{
		char szName[200];
		sprintf(szName, "%s/dst.txt", OUTPUT_PATH);
		FILE *fp = fopen(szName, "w");
		if (fp)
		{
			for (MInt32 y = 0; y < height; ++y)
			{
				MByte *pD = dstData + y * pitch;
				for (MInt32 x = 0; x < pitch; ++x)
				{
					fprintf(fp, "%d ", (int)pD[x]);
				}
				fprintf(fp, "\n");
			}

			fclose(fp);
		}
	}
#endif

exit:
	++k;

	// free memory
	if (blParam.pBuf)
	{
		MMemFree(hMemMgr, blParam.pBuf);
		blParam.pBuf = MNull;
	}

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
	END_PROFILE(lTime);
	PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter consume time = %d", k, lTime);
	PrintfB(6, "VNS", "aveDoEnhancement enhance hdr %d_fastGuidedFilter_Filter end lret = %d", k, res);
#endif

	return MOK;
}