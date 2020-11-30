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
#include "guided_filter_imgproc.h"
#include "NEONvsSSE.h"
#include "ammem.h"
#include "merror.h"
#include "mobilecv.h"

#include <math.h>

MRESULT MMemAlloc_UCHAR(MHandle hMemMgr, MByte **pData, MInt32 size)
{
	if (pData == MNull) return MERR_INVALID_PARAM;

	MByte *pBuffer = (MByte *)MMemAlloc(hMemMgr, size);
	if (!pBuffer) return MERR_NO_MEMORY;

	*pData = pBuffer;

	return MOK;
}

MRESULT MMemAlloc_USHORT(MHandle hMemMgr, MUInt16 **pData, MInt32 size)
{
	if (pData == MNull) return MERR_INVALID_PARAM;

	MUInt16 *pBuffer = (MUInt16 *)MMemAlloc(hMemMgr, size);
	if (!pBuffer) return MERR_NO_MEMORY;

	*pData = pBuffer;

	return MOK;
}

MRESULT MMemAlloc_FLOAT(MHandle hMemMgr, MFloat **pData, MInt32 size)
{
	if (pData == MNull) return MERR_INVALID_PARAM;

	MFloat *pBuffer = (MFloat *)MMemAlloc(hMemMgr, size);
	if (!pBuffer) return MERR_NO_MEMORY;

	*pData = pBuffer;

	return MOK;
}

MRESULT MMemAlloc_GuidedFilter(ArcGuidedFilter *pAgfInfo)
{
	if (!pAgfInfo) return MERR_INVALID_PARAM;

	MRESULT res = MOK;

	MHandle hMemMgr = pAgfInfo->hMemMgr;
	MBool fastMode = pAgfInfo->gfMode & 0x02;
	MInt32 guidanceCnNum = pAgfInfo->guidanceCnNum;
	MInt32 srcCnNum = pAgfInfo->srcCnNum;
	MInt32 corrGuidanceCnNum = guidanceCnNum * (guidanceCnNum + 1) / 2;
	MInt32 corrGuidanceSrcCnNum = guidanceCnNum * srcCnNum;
	MInt32 shift = 0;

	MInt32 singleCnPixelSize = pAgfInfo->height * pAgfInfo->width;
	MInt32 ucharBufferSize = 0, ushortBufferSize = 0, floatBufferSize = 0;
	if (!fastMode)
	{
		ucharBufferSize = singleCnPixelSize * sizeof(MByte) 
			* (guidanceCnNum											// guidanceCnOri
			+ srcCnNum);												// srcCnOri
		res = MMemAlloc_UCHAR(hMemMgr, &(pAgfInfo->pUCHAR_Buffer), ucharBufferSize);
		CHECK_ERROR(res);
		MByte *ppGuidanceCnOri = pAgfInfo->pUCHAR_Buffer;
		shift = singleCnPixelSize * guidanceCnNum;
		MByte *ppSrcCnOri = ppGuidanceCnOri + shift;

		ushortBufferSize = singleCnPixelSize * sizeof(MUInt16) 
			* (corrGuidanceCnNum										// corrGuidance
			+ corrGuidanceSrcCnNum);									// corrGuidanceSrc
		res = MMemAlloc_USHORT(hMemMgr, &(pAgfInfo->pUSHORT_Buffer), ushortBufferSize);
		CHECK_ERROR(res);
		MUInt16 *ppCorrGuidance = pAgfInfo->pUSHORT_Buffer;
		shift = singleCnPixelSize * corrGuidanceCnNum;
		MUInt16 *ppCorrGuidanceSrc = ppCorrGuidance + shift;

		floatBufferSize = singleCnPixelSize * sizeof(MFloat)
			* (guidanceCnNum											// guidanceCnMean
			+ corrGuidanceCnNum											// corrGuidanceMean/covGuidance/covInvGuidance
			+ srcCnNum													// srcCnMean/beta
			+ srcCnNum													// betaMean/dstCn
			+ corrGuidanceSrcCnNum										// corrGuidanceSrcMean/covGuidanceSrc/alphaMean
			+ corrGuidanceSrcCnNum);									// alpha
		res = MMemAlloc_FLOAT(hMemMgr, &(pAgfInfo->pFLOAT_Buffer), floatBufferSize);
		CHECK_ERROR(res);
		MFloat *ppGuidanceCnMean = pAgfInfo->pFLOAT_Buffer;
		shift = singleCnPixelSize * guidanceCnNum;
		MFloat *ppCorrGuidanceMean = ppGuidanceCnMean + shift;
		shift = singleCnPixelSize * corrGuidanceCnNum;
		MFloat *ppSrcCnMean = ppCorrGuidanceMean + shift;
		shift = singleCnPixelSize * srcCnNum;
		MFloat *ppBetaMean = ppSrcCnMean + shift;
		MFloat *ppCorrGuidanceSrcMean = ppBetaMean + shift;
		shift = singleCnPixelSize * corrGuidanceSrcCnNum;
		MFloat *ppAlpha = ppCorrGuidanceSrcMean + shift;

		MInt32 total = 0;
		// assign each memory
		for (MInt32 sc = 0; sc < srcCnNum; ++sc)
		{
			// srcCnOri
			pAgfInfo->pSrcCnOri[sc] = ppSrcCnOri + sc * singleCnPixelSize;
			// srcCnMean / beta
			pAgfInfo->pSrcCnMean[sc] = ppSrcCnMean + sc * singleCnPixelSize;
			pAgfInfo->pBeta[sc] = pAgfInfo->pSrcCnMean[sc];
			// betaMean / dstCn
			pAgfInfo->pBetaMean[sc] = ppBetaMean + sc * singleCnPixelSize;
			pAgfInfo->pDstCn[sc] = pAgfInfo->pBetaMean[sc];
			for (MInt32 gc1 = 0; gc1 < guidanceCnNum; ++gc1)
			{
				MInt32 idxGuidanceSrc = sc * srcCnNum + gc1;
				// corrGuidanceSrc
				pAgfInfo->pCorrGuidanceSrc[idxGuidanceSrc] = ppCorrGuidanceSrc + total * singleCnPixelSize;
				// corrGuidanceSrcMean / covGuidanceSrc / alphaMean
				pAgfInfo->pCorrGuidanceSrcMean[idxGuidanceSrc] = ppCorrGuidanceSrcMean + total * singleCnPixelSize;
				pAgfInfo->pCovGuidanceSrc[idxGuidanceSrc] = pAgfInfo->pCorrGuidanceSrcMean[idxGuidanceSrc];
				pAgfInfo->pAlphaMean[idxGuidanceSrc] = pAgfInfo->pCorrGuidanceSrcMean[idxGuidanceSrc];
				// alpha
				pAgfInfo->pAlpha[idxGuidanceSrc] = ppAlpha + total * singleCnPixelSize;

				++total;
			}
		}

		total = 0;
		for (MInt32 gc1 = 0; gc1 < guidanceCnNum; ++gc1)
		{
			// guidanceCnOri
			pAgfInfo->pGuidanceCnOri[gc1] = ppGuidanceCnOri + gc1 * singleCnPixelSize;
			// guidanceCnMean
			pAgfInfo->pGuidanceCnMean[gc1] = ppGuidanceCnMean + gc1 * singleCnPixelSize;
			for (MInt32 gc2 = gc1; gc2 < guidanceCnNum; ++gc2)
			{
				// corrGuidance
				pAgfInfo->pCorrGuidance[total] = ppCorrGuidance + total * singleCnPixelSize;
				// corrGuidanceMean / covGuidance / covInvGuidance
				pAgfInfo->pCorrGuidanceMean[total] = ppCorrGuidanceMean + total * singleCnPixelSize;
				pAgfInfo->pCovGuidance[total] = pAgfInfo->pCorrGuidanceMean[total];
				pAgfInfo->pCovInvGuidance[total] = pAgfInfo->pCorrGuidanceMean[total];

				++total;
			}
		}
	}
	else
	{
		MInt32 gfScale = pAgfInfo->gfScale;
		MInt32 heightSub = pAgfInfo->height / gfScale;
		MInt32 widthSub = pAgfInfo->width / gfScale;
		MInt32 singleCnSubPixelSize = heightSub * widthSub;

		ucharBufferSize = singleCnPixelSize * sizeof(MByte)
			* (guidanceCnNum											// guidanceCnOri
			+ srcCnNum)													// srcCnOri
			+ singleCnSubPixelSize * sizeof(MByte)		
			* (guidanceCnNum											// guidanceCnSub
			+ srcCnNum);												// srcCnSub
		res = MMemAlloc_UCHAR(hMemMgr, &(pAgfInfo->pUCHAR_Buffer), ucharBufferSize);
		CHECK_ERROR(res);
		MByte *ppGuidanceCnOri = pAgfInfo->pUCHAR_Buffer;
		shift = singleCnPixelSize * guidanceCnNum;
		MByte *ppSrcCnOri = ppGuidanceCnOri + shift;
		shift = singleCnPixelSize * srcCnNum;
		MByte *ppGuidanceCnSub = ppSrcCnOri + shift;
		shift = singleCnSubPixelSize * guidanceCnNum;
		MByte *ppSrcCnSub = ppGuidanceCnSub + shift;

		ushortBufferSize = singleCnSubPixelSize * sizeof(MUInt16)
			* (corrGuidanceCnNum										// corrGuidance
			+ corrGuidanceSrcCnNum);									// corrGuidanceSrc
		res = MMemAlloc_USHORT(hMemMgr, &(pAgfInfo->pUSHORT_Buffer), ushortBufferSize);
		CHECK_ERROR(res);
		MUInt16 *ppCorrGuidance = pAgfInfo->pUSHORT_Buffer;
		shift = singleCnSubPixelSize * corrGuidanceCnNum;
		MUInt16 *ppCorrGuidanceSrc = ppCorrGuidance + shift;

		floatBufferSize = singleCnPixelSize * sizeof(MFloat)
			* (srcCnNum													// betaMean/dstCn 
			+ corrGuidanceSrcCnNum)										// alphaMean
			+ singleCnSubPixelSize * sizeof(MFloat)
			* (guidanceCnNum											// guidanceCnMean
			+ corrGuidanceCnNum											// corrGuidanceMean/covGuidance/covInvGuidance
			+ srcCnNum													// srcCnMean/betaSub
			+ srcCnNum													// betaSubMean
			+ corrGuidanceSrcCnNum										// corrGuidanceSrcMean/covGuidanceSrc/alphaSubMean
			+ corrGuidanceSrcCnNum);									// alphaSub
		res = MMemAlloc_FLOAT(hMemMgr, &(pAgfInfo->pFLOAT_Buffer), floatBufferSize);
		CHECK_ERROR(res);
		MFloat *ppBetaMean = pAgfInfo->pFLOAT_Buffer;
		shift = singleCnPixelSize * srcCnNum;
		MFloat *ppAlphaMean = ppBetaMean + shift;
		shift = singleCnPixelSize * corrGuidanceSrcCnNum;
		MFloat *ppGuidanceCnMean = ppAlphaMean + shift;
		shift = singleCnSubPixelSize * guidanceCnNum;
		MFloat *ppCorrGuidanceMean = ppGuidanceCnMean + shift;
		shift = singleCnSubPixelSize * corrGuidanceCnNum;
		MFloat *ppSrcCnMean = ppCorrGuidanceMean + shift;
		shift = singleCnSubPixelSize * srcCnNum;
		MFloat *ppBetaSubMean = ppSrcCnMean + shift;
		MFloat *ppCorrGuidanceSrcMean = ppBetaSubMean + shift;
		shift = singleCnSubPixelSize * corrGuidanceSrcCnNum;
		MFloat *ppAlphaSub = ppCorrGuidanceSrcMean + shift;

		MInt32 total = 0;
		// assign each memory
		for (MInt32 sc = 0; sc < srcCnNum; ++sc)
		{
			// srcCnOri
			pAgfInfo->pSrcCnOri[sc] = ppSrcCnOri + sc * singleCnPixelSize;
			// srcCnSub
			pAgfInfo->pSrcCnSub[sc] = ppSrcCnSub + sc * singleCnSubPixelSize;
			// srcCnMean / beta
			pAgfInfo->pSrcCnMean[sc] = ppSrcCnMean + sc * singleCnSubPixelSize;
			pAgfInfo->pBetaSub[sc] = pAgfInfo->pSrcCnMean[sc];
			// betaSubMean
			pAgfInfo->pBetaSubMean[sc] = ppBetaSubMean + sc * singleCnSubPixelSize;
			// betaMean / dstCn
			pAgfInfo->pBetaMean[sc] = ppBetaMean + sc * singleCnPixelSize;
			pAgfInfo->pDstCn[sc] = pAgfInfo->pBetaMean[sc];
			for (MInt32 gc1 = 0; gc1 < guidanceCnNum; ++gc1)
			{
				MInt32 idxGuidanceSrc = sc * srcCnNum + gc1;
				// corrGuidanceSrc
				pAgfInfo->pCorrGuidanceSrc[idxGuidanceSrc] = ppCorrGuidanceSrc + total * singleCnSubPixelSize;
				// corrGuidanceSrcMean / covGuidanceSrc / alphaSubMean
				pAgfInfo->pCorrGuidanceSrcMean[idxGuidanceSrc] = ppCorrGuidanceSrcMean + total * singleCnSubPixelSize;
				pAgfInfo->pCovGuidanceSrc[idxGuidanceSrc] = pAgfInfo->pCorrGuidanceSrcMean[idxGuidanceSrc];
				pAgfInfo->pAlphaSubMean[idxGuidanceSrc] = pAgfInfo->pCorrGuidanceSrcMean[idxGuidanceSrc];
				// alphaSub
				pAgfInfo->pAlphaSub[idxGuidanceSrc] = ppAlphaSub + total * singleCnSubPixelSize;
				// alphaMean
				pAgfInfo->pAlphaMean[idxGuidanceSrc] = ppAlphaMean + total * singleCnPixelSize;

				++total;
			}
		}

		total = 0;
		for (MInt32 gc1 = 0; gc1 < guidanceCnNum; ++gc1)
		{
			// guidanceCnOri
			pAgfInfo->pGuidanceCnOri[gc1] = ppGuidanceCnOri + gc1 * singleCnPixelSize;
			// guidanceCnSub
			pAgfInfo->pGuidanceCnSub[gc1] = ppGuidanceCnSub + gc1 * singleCnSubPixelSize;
			// guidanceCnMean
			pAgfInfo->pGuidanceCnMean[gc1] = ppGuidanceCnMean + gc1 * singleCnSubPixelSize;
			for (MInt32 gc2 = gc1; gc2 < guidanceCnNum; ++gc2)
			{
				// corrGuidance
				pAgfInfo->pCorrGuidance[total] = ppCorrGuidance + total * singleCnSubPixelSize;
				// corrGuidanceMean / covGuidance / covInvGuidance
				pAgfInfo->pCorrGuidanceMean[total] = ppCorrGuidanceMean + total * singleCnSubPixelSize;
				pAgfInfo->pCovGuidance[total] = pAgfInfo->pCorrGuidanceMean[total];
				pAgfInfo->pCovInvGuidance[total] = pAgfInfo->pCorrGuidanceMean[total];

				++total;
			}
		}
	}

exit:
	return res;
}

MVoid MMemFree_GuidedFilter(ArcGuidedFilter *pAgfInfo)
{
	if (pAgfInfo)
	{
		MHandle hMemMgr = pAgfInfo->hMemMgr;
		if (pAgfInfo->pUCHAR_Buffer)
			MMemFree(hMemMgr, pAgfInfo->pUCHAR_Buffer);
		if (pAgfInfo->pUSHORT_Buffer)
			MMemFree(hMemMgr, pAgfInfo->pUSHORT_Buffer);
		if (pAgfInfo->pFLOAT_Buffer)
			MMemFree(hMemMgr, pAgfInfo->pFLOAT_Buffer);

		MMemSet(pAgfInfo, 0, sizeof(ArcGuidedFilter));
	}
}

// =================================================================== //

struct SplitChannels_MT
{
	MInt32 taskID;
	const MByte* pSrc;
	MInt32 srcPitch;
	MInt32 height;
	MInt32 width;

	MUInt8 *pSrcCn0;
	MUInt8 *pSrcCn1;
	MUInt8 *pSrcCn2;
	MInt32 srcCnPitch;

	MInt32 lStartRow;
	MInt32 lEndRow;
};

static MVoid stripe_splitChannels_U8C1(const MByte* pSrc, MInt32 srcPitch, MInt32 height, MInt32 width,
	MUInt8 *pSrcCn, MInt32 srcCnPitch, MInt32 lStartRow, MInt32 lEndRow)
{
	const MByte* pS = pSrc + lStartRow * srcPitch;
	MUInt8 *pSCn = pSrcCn + lStartRow * srcCnPitch;

	MMemCpy(pSCn, pS, (lEndRow - lStartRow) * srcCnPitch);
}

static MVoid thread_splitChannels_U8C1(MVoid *info)
{
	SplitChannels_MT *threadInfo = (SplitChannels_MT *)info;
	stripe_splitChannels_U8C1(threadInfo->pSrc, threadInfo->srcPitch, threadInfo->height, threadInfo->width,
		threadInfo->pSrcCn0, threadInfo->srcCnPitch, threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT splitChannels_U8C1(MHandle parEngine, MInt32 taskNum,
	const MByte* pSrc, MInt32 srcPitch, MInt32 height, MInt32 width,
	MUInt8 *pSrcCn, MInt32 srcCnPitch)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;
		MInt32 i = taskNum * (sizeof(SplitChannels_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		SplitChannels_MT *pParam = (SplitChannels_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrc = pSrc;
			pParam[i].srcPitch = srcPitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].pSrcCn0 = pSrcCn;
			pParam[i].srcCnPitch = srcCnPitch;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_splitChannels_U8C1, (MVoid *)&pParam[i]);
		}
		
		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_splitChannels_U8C1(pSrc, srcPitch, height, width, pSrcCn, srcCnPitch, 0, height);
	}

	return MOK;
}

static MVoid stripe_splitChannels_U8C3(const MByte* pSrc, MInt32 srcPitch, MInt32 height, MInt32 width,
	MUInt8 *pSrcCn0, MUInt8 *pSrcCn1, MUInt8 *pSrcCn2, MInt32 srcCnPitch, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 i, j, idx;

	for (i = lStartRow; i < lEndRow; ++i)
	{
		const MByte* pS = pSrc + i * srcPitch;
		MUInt8 *pSCn0 = pSrcCn0 + i * srcCnPitch;
		MUInt8 *pSCn1 = pSrcCn1 + i * srcCnPitch;
		MUInt8 *pSCn2 = pSrcCn2 + i * srcCnPitch;

		j = 0, idx = 0;

#ifdef __ARM_NEON__
		for (; j < width - 15; j += 16, idx += 48)
		{
			uint8x16x3_t v_src;
			v_src = vld3q_u8(pS + idx);

			vst1q_u8(pSCn0 + j, v_src.val[0]);
			vst1q_u8(pSCn1 + j, v_src.val[1]);
			vst1q_u8(pSCn2 + j, v_src.val[2]);
		}
#endif

		for (; j < width; ++j, idx += 3)
		{
			pSCn0[j] = pS[idx];
			pSCn1[j] = pS[idx + 1];
			pSCn2[j] = pS[idx + 2];
		}
	}
}

static MVoid thread_splitChannels_U8C3(MVoid *info)
{
	SplitChannels_MT *threadInfo = (SplitChannels_MT *)info;
	stripe_splitChannels_U8C3(threadInfo->pSrc, threadInfo->srcPitch, threadInfo->height, threadInfo->width,
		threadInfo->pSrcCn0, threadInfo->pSrcCn1, threadInfo->pSrcCn2, threadInfo->srcCnPitch,
		threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT splitChannels_U8C3(MHandle parEngine, MInt32 taskNum, 
	const MByte* pSrc, MInt32 srcPitch, MInt32 height, MInt32 width,
	MUInt8 *pSrcCn0, MUInt8 *pSrcCn1, MUInt8 *pSrcCn2, MInt32 srcCnPitch)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;
		MInt32 i = taskNum * (sizeof(SplitChannels_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		SplitChannels_MT *pParam = (SplitChannels_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrc = pSrc;
			pParam[i].srcPitch = srcPitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].pSrcCn0 = pSrcCn0;
			pParam[i].pSrcCn1 = pSrcCn1;
			pParam[i].pSrcCn2 = pSrcCn2;
			pParam[i].srcCnPitch = srcCnPitch;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_splitChannels_U8C3, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_splitChannels_U8C3(pSrc, srcPitch, height, width, pSrcCn0, pSrcCn1, pSrcCn2, srcCnPitch, 0, height);
	}

	return MOK;
}

MRESULT SplitChannels(MHandle parEngine, MInt32 taskNum, const MByte* pSrc,
	MInt32 srcPitch, MInt32 height, MInt32 width, MInt32 channels, MByte **srcCn)
{
	if (channels == 1)
	{
		return splitChannels_U8C1(parEngine, taskNum, pSrc, srcPitch, height, width, 
			srcCn[0], srcPitch);
	}
	else if (channels == 3)
	{
		return splitChannels_U8C3(parEngine, taskNum, pSrc, srcPitch, height, width, 
			srcCn[0], srcCn[1], srcCn[2], srcPitch / 3);
	}
	else
	{
		return MERR_UNSUPPORTED;
	}
}

struct MergeChannelsAndConvertToDstType_MT
{
	MInt32 taskID;

	const MFloat *pDstCn0;
	const MFloat *pDstCn1;
	const MFloat *pDstCn2;
	MInt32 dstCnPitch;

	MByte *pDst;
	MInt32 dstPitch;
	MInt32 height;
	MInt32 width;

	MInt32 lStartRow;
	MInt32 lEndRow;
};

static MVoid stripe_MergeChannelsAndConvertToDstType_U8C1(const MFloat *pDstCn, MInt32 dstCnPitch,
	MByte* pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 i, j;
	MInt32 dstCnStep = dstCnPitch / sizeof(MFloat),
		dstStep = dstPitch;

	for (i = lStartRow; i < lEndRow; ++i)
	{
		const MFloat *pDCn = pDstCn + i * dstCnStep;
		MByte *pD = pDst + i * dstStep;

		j = 0;
#ifdef __ARM_NEON__
		float32x4_t v_const = vdupq_n_f32(0.5);
		for (; j < width - 7; j += 8)
		{
			float32x4_t v_DstLow, v_DstHigh;
			uint16x4_t v_tmp1, v_tmp2;
			uint8x8_t v_dst;

			v_DstLow = vld1q_f32(pDCn + j);
			v_DstHigh = vld1q_f32(pDCn + j + 4);

			v_DstLow = vaddq_f32(v_DstLow, v_const);
			v_DstHigh = vaddq_f32(v_DstHigh, v_const);

			v_tmp1 = vqmovun_s32(vcvtq_s32_f32(v_DstLow));
			v_tmp2 = vqmovun_s32(vcvtq_s32_f32(v_DstHigh));

			v_dst = vqmovn_u16(vcombine_u16(v_tmp1, v_tmp2));

			vst1_u8(pD + j, v_dst);
		}
#endif
		for (; j < width; ++j)
		{
			/*if (i == 808 && j == 806)
			{
				i = i;
			}*/
			ANYTYPE_CAST_8U(pDCn[j] + 0.5, pD[j]);
		}
	}
}

static MVoid thread_MergeChannelsAndConvertToDstType_U8C1(MVoid *info)
{
	MergeChannelsAndConvertToDstType_MT *threadInfo = (MergeChannelsAndConvertToDstType_MT *)info;
	stripe_MergeChannelsAndConvertToDstType_U8C1(threadInfo->pDstCn0, threadInfo->dstCnPitch,
		threadInfo->pDst, threadInfo->dstPitch, threadInfo->height, threadInfo->width, threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT MergeChannelsAndConvertToDstType_U8C1(MHandle parEngine, MInt32 taskNum,
	const MFloat *pDstCn, MInt32 dstCnPitch, MByte* pDst, MInt32 dstPitch, MInt32 height, MInt32 width)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;
		MInt32 i = taskNum * (sizeof(MergeChannelsAndConvertToDstType_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		MergeChannelsAndConvertToDstType_MT *pParam = (MergeChannelsAndConvertToDstType_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pDst = pDst;
			pParam[i].dstPitch = dstPitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].pDstCn0 = pDstCn;
			pParam[i].dstCnPitch = dstCnPitch;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_MergeChannelsAndConvertToDstType_U8C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_MergeChannelsAndConvertToDstType_U8C1(pDstCn, dstCnPitch, pDst, dstPitch, height, width, 0, height);
	}

	return MOK;
}

static MVoid stripe_MergeChannelsAndConvertToDstType_U8C3(const MFloat *pDstCn0,
	const MFloat *pDstCn1, const MFloat *pDstCn2, MInt32 dstCnPitch,
	MByte* pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 i, j, idx;
	MInt32 dstCnStep = dstCnPitch / sizeof(MFloat),
		dstStep = dstPitch;

	for (i = lStartRow; i < lEndRow; ++i)
	{
		const MFloat *pDCn0 = pDstCn0 + i * dstCnStep;
		const MFloat *pDCn1 = pDstCn1 + i * dstCnStep;
		const MFloat *pDCn2 = pDstCn2 + i * dstCnStep;
		MByte *pD = pDst + i * dstStep;

		j = 0, idx = 0;
#ifdef __ARM_NEON__
		for (; j < width - 7; j += 8, idx += 24)
		{
			float32x4_t v_DCn0_low, v_DCn0_high,
				v_DCn1_low, v_DCn1_high,
				v_DCn2_low, v_DCn2_high;
			uint16x4_t v_tmp1, v_tmp2;
			uint8x8x3_t v_dst;

			v_DCn0_low = vld1q_f32(pDCn0 + j);
			v_DCn0_high = vld1q_f32(pDCn0 + j + 4);

			v_DCn1_low = vld1q_f32(pDCn1 + j);
			v_DCn1_high = vld1q_f32(pDCn1 + j + 4);

			v_DCn2_low = vld1q_f32(pDCn2 + j);
			v_DCn2_high = vld1q_f32(pDCn2 + j + 4);

			v_tmp1 = vqmovun_s32(vcvtq_s32_f32(v_DCn0_low));
			v_tmp2 = vqmovun_s32(vcvtq_s32_f32(v_DCn0_high));
			v_dst.val[0] = vqmovn_u16(vcombine_u16(v_tmp1, v_tmp2));

			v_tmp1 = vqmovun_s32(vcvtq_s32_f32(v_DCn1_low));
			v_tmp2 = vqmovun_s32(vcvtq_s32_f32(v_DCn1_high));
			v_dst.val[1] = vqmovn_u16(vcombine_u16(v_tmp1, v_tmp2));

			v_tmp1 = vqmovun_s32(vcvtq_s32_f32(v_DCn2_low));
			v_tmp2 = vqmovun_s32(vcvtq_s32_f32(v_DCn2_high));
			v_dst.val[2] = vqmovn_u16(vcombine_u16(v_tmp1, v_tmp2));

			vst3_u8(pD + idx, v_dst);
		}
#endif
		for (; j < width; ++j, idx += 3)
		{
			ANYTYPE_CAST_8U(pDCn0[j], pD[idx]);
			ANYTYPE_CAST_8U(pDCn1[j], pD[idx + 1]);
			ANYTYPE_CAST_8U(pDCn2[j], pD[idx + 2]);
		}
	}
}

static MVoid thread_MergeChannelsAndConvertToDstType_U8C3(MVoid *info)
{
	MergeChannelsAndConvertToDstType_MT *threadInfo = (MergeChannelsAndConvertToDstType_MT *)info;
	stripe_MergeChannelsAndConvertToDstType_U8C3(threadInfo->pDstCn0, threadInfo->pDstCn1,
		threadInfo->pDstCn2, threadInfo->dstCnPitch, threadInfo->pDst, threadInfo->dstPitch,
		threadInfo->height, threadInfo->width, threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT MergeChannelsAndConvertToDstType_U8C3(MHandle parEngine, MInt32 taskNum,
	const MFloat *pDstCn0, const MFloat *pDstCn1, const MFloat *pDstCn2, MInt32 dstCnPitch,
	MByte* pDst, MInt32 dstPitch, MInt32 height, MInt32 width)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;
		MInt32 i = taskNum * (sizeof(MergeChannelsAndConvertToDstType_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		MergeChannelsAndConvertToDstType_MT *pParam = (MergeChannelsAndConvertToDstType_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pDst = pDst;
			pParam[i].dstPitch = dstPitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].pDstCn0 = pDstCn0;
			pParam[i].pDstCn1 = pDstCn1;
			pParam[i].pDstCn2 = pDstCn2;
			pParam[i].dstCnPitch = dstCnPitch;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine,
				thread_MergeChannelsAndConvertToDstType_U8C3, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_MergeChannelsAndConvertToDstType_U8C3(pDstCn0, pDstCn1, pDstCn2, dstCnPitch,
			pDst, dstPitch, height, width, 0, height);
	}

	return MOK;
}

MRESULT MergeChannelsAndConvertToDstType(MHandle parEngine, MInt32 taskNum,
	MFloat **dstCn, MInt32 dstCnPitch, MByte* pDst, MInt32 dstPitch,
	MInt32 height, MInt32 width, MInt32 channels)
{
	if (channels == 1)
	{
		return MergeChannelsAndConvertToDstType_U8C1(parEngine, taskNum,
			dstCn[0], dstCnPitch, pDst, dstPitch, height, width);
	}
	else if (channels == 3)
	{
		return MergeChannelsAndConvertToDstType_U8C3(parEngine, taskNum,
			dstCn[0], dstCn[1], dstCn[2], dstCnPitch, pDst, dstPitch, height, width);
	}
	else
	{
		return MERR_UNSUPPORTED;
	}
}

// box filter for float32 single channel
static MVoid boxBlurRowAdd_F32C1(MFloat* pSumLine, const MFloat* addSrc, MInt32 width, MInt32 radius)
{
	MInt32 i;
	MFloat nValue;
	MFloat nSum = 0;

	nValue = addSrc[0];
	pSumLine[0] = 0;
	pSumLine++;
	for (i = 0; i < radius; ++i)
	{
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}

	for (i = 0; i < width; ++i)
	{
		nValue = addSrc[i];
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}

	nValue = addSrc[width - 1];
	for (i = 0; i < radius; ++i)
	{
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}
}

static MVoid boxBlurRowAddSub_F32C1(MFloat* pSumLine, const MFloat* addSrc, const MFloat* subSrc,
	MInt32 width, MInt32 radius)
{
	MInt32 i;
	MFloat nValue;
	MFloat nSum = 0;

	nValue = addSrc[0] - subSrc[0];
	pSumLine[0] = 0;
	pSumLine++;
	for (i = 0; i < radius; ++i)
	{
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}

	for (i = 0; i < width; ++i)
	{
		nValue = addSrc[i] - subSrc[i];
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}

	nValue = addSrc[width - 1] - subSrc[width - 1];
	for (i = 0; i < radius; ++i)
	{
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}
}

static MVoid boxBlurProcessRow_F32C1(MFloat* pDst, MFloat* pBoxSumBuf, MInt32 width, MInt32 radius, MFloat invDivNum)
{
	MInt32 x;
	MFloat lbVal = 0;
	MInt32 lboxSize = radius * 2 + 1;

	x = 0;
#ifdef __ARM_NEON__
	float32x4_t v_invDiv = vdupq_n_f32(invDivNum);
	for (; x < width - 3; x += 4)
	{
		float32x4_t v_sum0, v_sum1;
		float32x4_t v_dst;
		v_sum1 = vld1q_f32(pBoxSumBuf + lboxSize);
		v_sum0 = vld1q_f32(pBoxSumBuf);
		v_dst = vsubq_f32(v_sum1, v_sum0);
		v_dst = vmulq_f32(v_dst, v_invDiv);
		vst1q_f32(pDst, v_dst);
		pBoxSumBuf += 4;
		pDst += 4;
	}
#endif
	for (; x < width; x++)
	{
		lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
		(pDst++)[0] = lbVal * invDivNum;
		pBoxSumBuf++;
	}
}

static MVoid stripe_BoxBlur_F32C1(const MFloat* pSrc, MFloat* pDst, MInt32 pitch, MInt32 height, MInt32 width,
	MInt32 lStartRow, MInt32 lEndRow, MInt32 radius, MFloat *pBoxSumBuf)
{
	MInt32 step = pitch / sizeof(MFloat);

	MInt32 lPreLine = MIN(radius, lStartRow),
		lNexLine = MIN(radius, height - lEndRow);
	MInt32 lTop = lStartRow - lPreLine;
	MInt32 lBot = lEndRow + lNexLine;
	const MFloat *tmpaddSrc = pSrc + lTop * step;
	const MFloat *tmpsubSrc = pSrc + lTop * step;
	const MFloat *tmpSrc = pSrc + lStartRow * step;
	MFloat *tmpDst = pDst + lStartRow * step;
	MFloat invArea = 1.f / ((radius * 2 + 1)*(radius * 2 + 1));

	MInt32 line = lStartRow - radius;

	MMemSet(pBoxSumBuf, 0, (width + radius * 2 + 1 + 100) * sizeof(MFloat));

	for (; line < lTop; line++)
	{
		boxBlurRowAdd_F32C1(pBoxSumBuf, tmpaddSrc, width, radius);
	}
	for (; line < lStartRow + radius; line++)
	{
		boxBlurRowAdd_F32C1(pBoxSumBuf, tmpaddSrc, width, radius);
		tmpaddSrc += step;
	}

	{
		boxBlurRowAdd_F32C1(pBoxSumBuf, tmpaddSrc, width, radius);
		boxBlurProcessRow_F32C1(tmpDst, pBoxSumBuf, width, radius, invArea);
		tmpaddSrc += step;
		tmpSrc += step;
		tmpDst += step;
		line++;
	}

	for (; line < lBot; line++)
	{
		boxBlurRowAddSub_F32C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_F32C1(tmpDst, pBoxSumBuf, width, radius, invArea);
		tmpaddSrc += step;
		if (line >= lTop + radius * 2 + 1)
		{
			tmpsubSrc += step;
		}
		tmpSrc += step;
		tmpDst += step;
	}

	tmpaddSrc -= step;
	for (; line < lEndRow + radius; line++)
	{
		boxBlurRowAddSub_F32C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_F32C1(tmpDst, pBoxSumBuf, width, radius, invArea);
		tmpsubSrc += step;
		tmpSrc += step;
		tmpDst += step;
	}
}

struct BoxFilter_F32C1_MT {
	MInt32			taskID;
	MInt32			width;
	MInt32			height;
	const MFloat	*pSrc;
	MFloat			*pDst;
	MInt32			pitch;
	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32          radius;
	MFloat          *pBoxSumBuf;
};

static MVoid thread_BoxFilter_F32C1(MVoid *info)
{
	BoxFilter_F32C1_MT *threadInfo = (BoxFilter_F32C1_MT *)info;
	stripe_BoxBlur_F32C1(threadInfo->pSrc, threadInfo->pDst, threadInfo->pitch,
		threadInfo->height, threadInfo->width, threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->radius, threadInfo->pBoxSumBuf);
}

MRESULT BoxFilter_F32C1(MHandle parEngine, MInt32 taskNum, const MFloat *pSrc, MFloat *pDst,
	MInt32 pitch, MInt32 height, MInt32 width, MInt32 radius)
{
	if (parEngine)
	{	
		taskNum = Get_Task_Num(width, height);
		if (taskNum > 8) taskNum = 8;	
		MInt32 i = taskNum * (sizeof(BoxFilter_F32C1_MT) + sizeof(MInt32)
			+ (width + radius * 2 + 1 + 100) * sizeof(MFloat));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;

		BoxFilter_F32C1_MT *pParam = (BoxFilter_F32C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);
		MFloat *boxSumBuf = (MFloat *)(iTask + taskNum);

		MInt32 rowStep = height / taskNum;
		MInt32 sumBufStep = (width + radius * 2 + 1 + 100);
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrc = pSrc;
			pParam[i].pDst = pDst;
			pParam[i].pitch = pitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].radius = radius;
			pParam[i].pBoxSumBuf = boxSumBuf + i * sumBufStep;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_BoxFilter_F32C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		MFloat *boxSumBuf = (MFloat*)MMemAlloc(MNull, (width + radius * 2 + 1 + 100) * sizeof(MFloat));
		if (!boxSumBuf) return MERR_NO_MEMORY;

		stripe_BoxBlur_F32C1(pSrc, pDst, pitch,
			height, width, 0, height, radius, boxSumBuf);

		if (boxSumBuf) MMemFree(MNull, boxSumBuf);
	}

	return MOK;
}

// box filter for UInt8 single channel
static MVoid boxBlurRowAdd_U8C1toF32C1(MInt32* pSumLine, const MUInt8* addSrc, MInt32 width, MInt32 radius)
{
	MInt32  i;
	MDWord* DaddSrc = (MDWord*)addSrc;
	MDWord  nValue;
	MInt32  DWidth = width >> 2;
	MInt32  nSum = 0;

	pSumLine[0] = 0;
	pSumLine++;

	nValue = addSrc[0];
	for (i = 0; i < radius; i++)
	{
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}

	for (i = 0; i < DWidth; i++)
	{
		nValue = DaddSrc[i];
		nSum += (nValue << 24 >> 24);
		(pSumLine++)[0] += nSum;
		nSum += (nValue << 16 >> 24);
		(pSumLine++)[0] += nSum;
		nSum += (nValue << 8 >> 24);
		(pSumLine++)[0] += nSum;
		nSum += (nValue >> 24);
		(pSumLine++)[0] += nSum;
	}

	for (i = DWidth << 2; i < width; i++)
	{
		nValue = addSrc[i];
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}

	nValue = addSrc[width - 1];
	for (i = 0; i < radius; i++)
	{
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}
}

static MVoid boxBlurRowAddSub_U8C1toF32C1(MInt32* pSumLine, const MUInt8* addSrc, const MUInt8* subSrc,
	MInt32 width, MInt32 radius)
{
	MInt32   i;
	MDWord* DaddSrc = (MDWord*)addSrc;
	MDWord* DsubSrc = (MDWord*)subSrc;
	MDWord  nAValue, nSValue;
	MInt32  lVal;
	MInt32  DWidth = width >> 2;
	MInt32  nSum = 0;

	pSumLine[0] = 0;
	pSumLine++;

	lVal = addSrc[0] - subSrc[0];
	for (i = 0; i < radius; i++)
	{
		nSum += lVal;
		(pSumLine++)[0] += nSum;
	}

	for (i = 0; i < DWidth; i++)
	{
		nAValue = DaddSrc[i];
		nSValue = DsubSrc[i];
		lVal = (nAValue << 24 >> 24) - (nSValue << 24 >> 24);
		nSum += lVal;
		(pSumLine++)[0] += nSum;

		lVal = (nAValue << 16 >> 24) - (nSValue << 16 >> 24);
		nSum += lVal;
		(pSumLine++)[0] += nSum;

		lVal = (nAValue << 8 >> 24) - (nSValue << 8 >> 24);
		nSum += lVal;
		(pSumLine++)[0] += nSum;

		lVal = (nAValue >> 24) - (nSValue >> 24);
		nSum += lVal;
		(pSumLine++)[0] += nSum;
	}

	for (i = DWidth << 2; i < width; i++)
	{
		lVal = addSrc[i] - subSrc[i];
		nSum += lVal;
		(pSumLine++)[0] += nSum;
	}

	for (i = 0; i < radius; i++)
	{
		nSum += lVal;
		(pSumLine++)[0] += nSum;
	}
}

static MVoid boxBlurProcessRow_U8C1toF32C1(MFloat* pDst, MInt32* pBoxSumBuf, MInt32 width, MInt32 radius, MFloat invWinArea)
{
	MInt32 x;
	MInt32 lbVal = 0;
	MInt32 lboxSize = radius * 2 + 1;

	x = 0;
#ifdef __ARM_NEON__
	float32x4_t v_invWinArea = vdupq_n_f32(invWinArea);
	for (; x < width - 3; x += 4)
	{
		int32x4_t v_sum0, v_sum1;
		float32x4_t v_dst;

		v_sum1 = vld1q_s32(pBoxSumBuf + lboxSize);
		v_sum0 = vld1q_s32(pBoxSumBuf);

		v_dst = vmulq_f32(vcvtq_f32_s32(vsubq_s32(v_sum1, v_sum0)), v_invWinArea);
		vst1q_f32(pDst, v_dst);

		pBoxSumBuf += 4;
		pDst += 4;
	}
#endif

	for (; x < width; x++)
	{
		lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
		(pDst++)[0] = lbVal * invWinArea;
		pBoxSumBuf++;
	}
}

static MVoid stripe_BoxBlur_U8C1toF32C1(const MUInt8* pSrc, MInt32 srcPitch, MFloat* pDst, MInt32 dstPitch, MInt32 height, MInt32 width,
	MInt32 lStartRow, MInt32 lEndRow, MInt32 radius, MInt32 *pBoxSumBuf)
{
	MInt32 srcStep = srcPitch, dstStep = dstPitch / sizeof(MFloat);

	MInt32 lPreLine = MIN(radius, lStartRow),
		lNexLine = MIN(radius, height - lEndRow);
	MInt32 lTop = lStartRow - lPreLine;
	MInt32 lBot = lEndRow + lNexLine;
	const MUInt8 *tmpaddSrc = pSrc + lTop * srcStep;
	const MUInt8 *tmpsubSrc = pSrc + lTop * srcStep;
	const MUInt8 *tmpSrc = pSrc + lStartRow * srcStep;
	MFloat *tmpDst = pDst + lStartRow * dstStep;
	MFloat invWinArea = 1.f / ((2 * radius + 1) * (2 * radius + 1));

	MInt32 line = lStartRow - radius;

	MMemSet(pBoxSumBuf, 0, (width + radius * 2 + 1 + 100) * sizeof(MInt32));

	for (; line < lTop; line++)
	{
		boxBlurRowAdd_U8C1toF32C1(pBoxSumBuf, tmpaddSrc, width, radius);
	}
	for (; line < lStartRow + radius; line++)
	{
		boxBlurRowAdd_U8C1toF32C1(pBoxSumBuf, tmpaddSrc, width, radius);
		tmpaddSrc += srcStep;
	}

	{
		boxBlurRowAdd_U8C1toF32C1(pBoxSumBuf, tmpaddSrc, width, radius);
		boxBlurProcessRow_U8C1toF32C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpaddSrc += srcStep;
		tmpSrc += srcStep;
		tmpDst += dstStep;
		line++;
	}

	for (; line < lBot; line++)
	{
		boxBlurRowAddSub_U8C1toF32C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_U8C1toF32C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpaddSrc += srcStep;
		if (line >= lTop + radius * 2 + 1)
		{
			tmpsubSrc += srcStep;
		}
		tmpSrc += srcStep;
		tmpDst += dstStep;
	}

	tmpaddSrc -= srcStep;
	for (; line < lEndRow + radius; line++)
	{
		boxBlurRowAddSub_U8C1toF32C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_U8C1toF32C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpsubSrc += srcStep;
		tmpSrc += srcStep;
		tmpDst += dstStep;
	}
}

struct BoxFilter_U8C1toF32C1_MT
{
	MInt32			taskID;
	const MUInt8	*pSrc;
	MInt32			srcPitch;
	MFloat			*pDst;
	MInt32			dstPitch;
	MInt32			width;
	MInt32			height;

	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32          radius;
	MInt32          *pBoxSumBuf;
};

static MVoid thread_BoxFilter_U8C1toF32C1(MVoid *info)
{
	BoxFilter_U8C1toF32C1_MT *threadInfo = (BoxFilter_U8C1toF32C1_MT *)info;
	stripe_BoxBlur_U8C1toF32C1(threadInfo->pSrc, threadInfo->srcPitch, threadInfo->pDst, threadInfo->dstPitch,
		threadInfo->height, threadInfo->width, threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->radius,
		threadInfo->pBoxSumBuf);
}

MRESULT BoxFilter_U8C1toF32C1(MHandle hMemMgr, MHandle parEngine, const MUInt8 *pSrc, MInt32 srcPitch, 
							  MFloat *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius)
{
	if (parEngine)
	{
		//if (taskNum > 8) taskNum = 8;
		MInt32 taskNum = Get_Task_Num(width, height);

		MInt32 i = taskNum * (sizeof(BoxFilter_U8C1toF32C1_MT) + sizeof(MInt32)
			+ (width + radius * 2 + 1 + 100) * sizeof(MInt32));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;

		BoxFilter_U8C1toF32C1_MT *pParam = (BoxFilter_U8C1toF32C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);
		MInt32 *boxSumBuf = (MInt32 *)(iTask + taskNum);

		MInt32 rowStep = height / taskNum;
		MInt32 sumBufStep = (width + radius * 2 + 1 + 100);
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrc = pSrc;
			pParam[i].srcPitch = srcPitch;
			pParam[i].pDst = pDst;
			pParam[i].dstPitch = dstPitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].radius = radius;
			pParam[i].pBoxSumBuf = boxSumBuf + i * sumBufStep;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_BoxFilter_U8C1toF32C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(hMemMgr, buffer);
	}
	else
	{
		MInt32 *boxSumBuf = (MInt32 *)MMemAlloc(hMemMgr, (width + radius * 2 + 1 + 100) * sizeof(MInt32));
		if (!boxSumBuf) return MERR_NO_MEMORY;

		stripe_BoxBlur_U8C1toF32C1(pSrc, srcPitch, pDst, dstPitch, height, width, 0, height, radius, boxSumBuf);

		if (boxSumBuf) MMemFree(hMemMgr, boxSumBuf);
	}
	return MOK;
}

// box filter for UInt16 single channel
static MVoid boxBlurRowAdd_U16C1toF32C1(MInt32* pSumLine, const MUInt16* addSrc, MInt32 width, MInt32 radius)
{
	MInt32  i;
	MDWord* DaddSrc = (MDWord*)addSrc;
	MDWord  nValue;
	MInt32  DWidth = width >> 1;
	MInt32  nSum = 0;

	pSumLine[0] = 0;
	pSumLine++;

	nValue = addSrc[0];
	for (i = 0; i < radius; i++)
	{
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}

	for (i = 0; i < DWidth; i++)
	{
		nValue = DaddSrc[i];
		nSum += (nValue << 16 >> 16);
		(pSumLine++)[0] += nSum;
		nSum += (nValue >> 16);
		(pSumLine++)[0] += nSum;
	}

	for (i = DWidth << 1; i < width; i++)
	{
		nValue = addSrc[i];
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}

	nValue = addSrc[width - 1];
	for (i = 0; i < radius; i++)
	{
		nSum += nValue;
		(pSumLine++)[0] += nSum;
	}
}

static MVoid boxBlurRowAddSub_U16C1toF32C1(MInt32* pSumLine, const MUInt16* addSrc, const MUInt16* subSrc,
	MInt32 width, MInt32 radius)
{
	MInt32   i;
	MDWord* DaddSrc = (MDWord*)addSrc;
	MDWord* DsubSrc = (MDWord*)subSrc;
	MDWord  nAValue, nSValue;
	MInt32  lVal;
	MInt32  DWidth = width >> 1;
	MInt32  nSum = 0;

	pSumLine[0] = 0;
	pSumLine++;

	lVal = addSrc[0] - subSrc[0];
	for (i = 0; i < radius; i++)
	{
		nSum += lVal;
		(pSumLine++)[0] += nSum;
	}

	for (i = 0; i < DWidth; i++)
	{
		nAValue = DaddSrc[i];
		nSValue = DsubSrc[i];
		lVal = (nAValue << 16 >> 16) - (nSValue << 16 >> 16);
		nSum += lVal;
		(pSumLine++)[0] += nSum;

		lVal = (nAValue >> 16) - (nSValue >> 16);
		nSum += lVal;
		(pSumLine++)[0] += nSum;
	}

	for (i = DWidth << 1; i < width; i++)
	{
		lVal = addSrc[i] - subSrc[i];
		nSum += lVal;
		(pSumLine++)[0] += nSum;
	}

	for (i = 0; i < radius; i++)
	{
		nSum += lVal;
		(pSumLine++)[0] += nSum;
	}
}

static MVoid boxBlurProcessRow_U16C1toF32C1(MFloat* pDst, MInt32* pBoxSumBuf, MInt32 width, MInt32 radius, MFloat invWinArea)
{
	MInt32 x;
	MInt32 lbVal = 0;
	MInt32 lboxSize = radius * 2 + 1;

	x = 0;
#ifdef __ARM_NEON__
	float32x4_t v_invWinArea = vdupq_n_f32(invWinArea);
	for (; x < width - 3; x += 4)
	{
		int32x4_t v_sum0, v_sum1;
		float32x4_t v_dst;

		v_sum1 = vld1q_s32(pBoxSumBuf + lboxSize);
		v_sum0 = vld1q_s32(pBoxSumBuf);

		v_dst = vmulq_f32(vcvtq_f32_s32(vsubq_s32(v_sum1, v_sum0)), v_invWinArea);
		vst1q_f32(pDst, v_dst);

		pBoxSumBuf += 4;
		pDst += 4;
	}
#endif

	for (; x < width; x++)
	{
		lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
		(pDst++)[0] = lbVal * invWinArea;
		pBoxSumBuf++;
	}
}

static MVoid stripe_BoxBlur_U16C1toF32C1(const MUInt16* pSrc, MInt32 srcPitch, MFloat* pDst, MInt32 dstPitch, MInt32 height, MInt32 width,
	MInt32 lStartRow, MInt32 lEndRow, MInt32 radius, MInt32 *pBoxSumBuf)
{
	MInt32 srcStep = srcPitch / sizeof(MUInt16), dstStep = dstPitch / sizeof(MFloat);

	MInt32 lPreLine = MIN(radius, lStartRow),
		lNexLine = MIN(radius, height - lEndRow);
	MInt32 lTop = lStartRow - lPreLine;
	MInt32 lBot = lEndRow + lNexLine;
	const MUInt16 *tmpaddSrc = pSrc + lTop * srcStep;
	const MUInt16 *tmpsubSrc = pSrc + lTop * srcStep;
	const MUInt16 *tmpSrc = pSrc + lStartRow * srcStep;
	MFloat *tmpDst = pDst + lStartRow * dstStep;
	MFloat invWinArea = 1.f / ((2 * radius + 1) * (2 * radius + 1));

	MInt32 line = lStartRow - radius;

	MMemSet(pBoxSumBuf, 0, (width + radius * 2 + 1 + 100) * sizeof(MInt32));

	for (; line < lTop; line++)
	{
		boxBlurRowAdd_U16C1toF32C1(pBoxSumBuf, tmpaddSrc, width, radius);
	}
	for (; line < lStartRow + radius; line++)
	{
		boxBlurRowAdd_U16C1toF32C1(pBoxSumBuf, tmpaddSrc, width, radius);
		tmpaddSrc += srcStep;
	}

	{
		boxBlurRowAdd_U16C1toF32C1(pBoxSumBuf, tmpaddSrc, width, radius);
		boxBlurProcessRow_U16C1toF32C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpaddSrc += srcStep;
		tmpSrc += srcStep;
		tmpDst += dstStep;
		line++;
	}

	for (; line < lBot; line++)
	{
		boxBlurRowAddSub_U16C1toF32C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_U16C1toF32C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpaddSrc += srcStep;
		if (line >= lTop + radius * 2 + 1)
		{
			tmpsubSrc += srcStep;
		}
		tmpSrc += srcStep;
		tmpDst += dstStep;
	}

	tmpaddSrc -= srcStep;
	for (; line < lEndRow + radius; line++)
	{
		boxBlurRowAddSub_U16C1toF32C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_U16C1toF32C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpsubSrc += srcStep;
		tmpSrc += srcStep;
		tmpDst += dstStep;
	}
}

struct BoxFilter_U16C1toF32C1_MT
{
	MInt32			taskID;
	const MUInt16	*pSrc;
	MInt32			srcPitch;
	MFloat			*pDst;
	MInt32			dstPitch;
	MInt32			width;
	MInt32			height;

	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32          radius;
	MInt32          *pBoxSumBuf;
};

static MVoid thread_BoxFilter_U16C1toF32C1(MVoid *info)
{
	BoxFilter_U16C1toF32C1_MT *threadInfo = (BoxFilter_U16C1toF32C1_MT *)info;
	stripe_BoxBlur_U16C1toF32C1(threadInfo->pSrc, threadInfo->srcPitch, threadInfo->pDst, threadInfo->dstPitch,
		threadInfo->height, threadInfo->width, threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->radius,
		threadInfo->pBoxSumBuf);
}

MRESULT BoxFilter_U16C1toF32C1(MHandle hMemMgr, MHandle parEngine, const MUInt16 *pSrc, MInt32 srcPitch,
	MFloat *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius)
{
	if (parEngine)
	{
		MInt32 taskNum = Get_Task_Num(width, height);
		MInt32 i = taskNum * (sizeof(BoxFilter_U16C1toF32C1_MT) + sizeof(MInt32)
			+ (width + radius * 2 + 1 + 100) * sizeof(MInt32));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;

		BoxFilter_U16C1toF32C1_MT *pParam = (BoxFilter_U16C1toF32C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);
		MInt32 *boxSumBuf = (MInt32 *)(iTask + taskNum);

		MInt32 rowStep = height / taskNum;
		MInt32 sumBufStep = (width + radius * 2 + 1 + 100);
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrc = pSrc;
			pParam[i].srcPitch = srcPitch;
			pParam[i].pDst = pDst;
			pParam[i].dstPitch = dstPitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].radius = radius;
			pParam[i].pBoxSumBuf = boxSumBuf + i * sumBufStep;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_BoxFilter_U16C1toF32C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(hMemMgr, buffer);
	}
	else
	{
		MInt32 *boxSumBuf = (MInt32 *)MMemAlloc(hMemMgr, (width + radius * 2 + 1 + 100) * sizeof(MInt32));
		if (!boxSumBuf) return MERR_NO_MEMORY;

		stripe_BoxBlur_U16C1toF32C1(pSrc, srcPitch, pDst, dstPitch, height, width, 0, height, radius, boxSumBuf);

		if (boxSumBuf) MMemFree(hMemMgr, boxSumBuf);
	}
	return MOK;
}

static MVoid stripe_DotMultiply_U8C1toU16C1(const MUInt8 *pSrc1, const MUInt8 *pSrc2, MInt32 srcPitch,
	MUInt16 *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 i, j;
	MInt32 srcStep = srcPitch, dstStep = dstPitch / sizeof(MUInt16);

	for (i = lStartRow; i < lEndRow; ++i)
	{
		const MUInt8 *pS1 = pSrc1 + i * srcStep;
		const MUInt8 *pS2 = pSrc2 + i * srcStep;
		MUInt16 *pD = pDst + i * dstStep;

		j = 0;
#ifdef __ARM_NEON__
		for (; j < width - 7; j += 8)
		{
			uint8x8_t v_src1, v_src2;
			uint16x8_t v_dst;

			v_src1 = vld1_u8(pS1 + j);
			v_src2 = vld1_u8(pS2 + j);
			v_dst = vmull_u8(v_src1, v_src2);

			vst1q_u16(pD + j, v_dst);
		}
#endif

		for (; j < width; ++j)
		{
			pD[j] = pS1[j] * pS2[j];
		}
	}
}

struct DotMultiply_U8C1toU16C1_MT
{
	MInt32 taskID;
	const MUInt8 *pSrc1;
	const MUInt8 *pSrc2;
	MInt32 srcPitch;
	MUInt16 *pDst;
	MInt32 dstPitch;
	MInt32 height;
	MInt32 width;
	MInt32 lStartRow;
	MInt32 lEndRow;
};

static MVoid thread_DotMultiply_U8C1toU16C1(MVoid *info)
{
	DotMultiply_U8C1toU16C1_MT *threadInfo = (DotMultiply_U8C1toU16C1_MT *)info;
	stripe_DotMultiply_U8C1toU16C1(threadInfo->pSrc1, threadInfo->pSrc2, threadInfo->srcPitch, 
		threadInfo->pDst, threadInfo->dstPitch, threadInfo->height, threadInfo->width,
		threadInfo->lStartRow, threadInfo->lEndRow);
}


MRESULT DotMultiply_U8C1toU16C1(MHandle hMemMgr, MHandle parEngine, MInt32 taskNum, const MUInt8 *pSrc1, const MUInt8 *pSrc2, 
								MInt32 srcPitch, MUInt16 *pDst, MInt32 dstPitch, MInt32 height, MInt32 width)
{	
	if (parEngine)
	{
		//if (taskNum > 8) taskNum = 8;
		MInt32 taskNum = Get_Task_Num(width, height);

		MInt32 i = taskNum * (sizeof(DotMultiply_U8C1toU16C1_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;
		DotMultiply_U8C1toU16C1_MT *pParam = (DotMultiply_U8C1toU16C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrc1 = pSrc1;
			pParam[i].pSrc2 = pSrc2;
			pParam[i].srcPitch = srcPitch;
			pParam[i].pDst = pDst;
			pParam[i].dstPitch = dstPitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_DotMultiply_U8C1toU16C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(hMemMgr, buffer);
	}
	else
	{
		stripe_DotMultiply_U8C1toU16C1(pSrc1, pSrc2, srcPitch, pDst, dstPitch, height, width, 0, height);
	}

	return MOK;
}

MVoid GetCovGuidanceIdx(MInt32 covIdx, MInt32 guidanceCnNum, MInt32 &guidanceIdx1, MInt32 &guidanceIdx2)
{
	static MInt32 guidanceIdxData[] = {
		0, -1, -1, -1, -1, -1,
		0, -1, -1, -1, -1, -1,

		0, 0, 1, -1, -1, -1,
		0, 1, 1, -1, -1, -1,

		0, 0, 0, 1, 1, 2,
		0, 1, 2, 1, 2, 2
	};

	MInt32 index = 12 * (guidanceCnNum - 1) + covIdx;
	guidanceIdx1 = guidanceIdxData[index];
	index += 6;
	guidanceIdx2 = guidanceIdxData[index];
}

static MVoid stripe_CalcCovar_F32C1(const MFloat *pMeanI1, const MFloat *pMeanI2,
	const MFloat *pCorrI1I2, MFloat *pCovI1I2, MInt32 pitch, MInt32 height, MInt32 width,
	MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 i, j;
	MInt32 step = pitch / sizeof(MFloat);

	for (i = lStartRow; i < lEndRow; ++i)
	{
		const MFloat *pMI1 = pMeanI1 + i * step;
		const MFloat *pMI2 = pMeanI2 + i * step;
		const MFloat *pCorr = pCorrI1I2 + i * step;
		MFloat *pCov = pCovI1I2 + i * step;

		j = 0;
#ifdef __ARM_NEON__
		for (; j < width - 3; j += 4)
		{
			float32x4_t v_src1, v_src2, v_src12;
			float32x4_t v_dst;

			v_src1 = vld1q_f32(pMI1 + j);
			v_src2 = vld1q_f32(pMI2 + j);
			v_src12 = vld1q_f32(pCorr + j);

			v_dst = vsubq_f32(v_src12, vmulq_f32(v_src1, v_src2));

			vst1q_f32(pCov + j, v_dst);
		}
#endif

		for (; j < width; ++j)
		{
			pCov[j] = pCorr[j] - pMI1[j] * pMI2[j];
		}
	}
}

struct CalcCovar_F32C1_MT
{
	MInt32 taskID;
	const MFloat *pMeanI1;
	const MFloat *pMeanI2;
	const MFloat *pCorrI1I2;
	MFloat *pCovI1I2;
	MInt32 pitch;
	MInt32 height; 
	MInt32 width;
	MInt32 lStartRow;
	MInt32 lEndRow;
};

static MVoid thread_CalcCovar_F32C1(MVoid *info)
{
	CalcCovar_F32C1_MT *threadInfo = (CalcCovar_F32C1_MT *)info;
	stripe_CalcCovar_F32C1(threadInfo->pMeanI1, threadInfo->pMeanI2, threadInfo->pCorrI1I2,
		threadInfo->pCovI1I2, threadInfo->pitch, threadInfo->height, threadInfo->width,
		threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT CalcCovar_F32C1(MHandle parEngine, MInt32 taskNum, const MFloat *pMeanI1, const MFloat *pMeanI2,
	const MFloat *pCorrI1I2, MFloat *pCovI1I2, MInt32 pitch, MInt32 height, MInt32 width)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;

		MInt32 i = taskNum * (sizeof(CalcCovar_F32C1_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		CalcCovar_F32C1_MT *pParam = (CalcCovar_F32C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pMeanI1 = pMeanI1;
			pParam[i].pMeanI2 = pMeanI2;
			pParam[i].pCorrI1I2 = pCorrI1I2;
			pParam[i].pCovI1I2 = pCovI1I2;
			pParam[i].pitch = pitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_CalcCovar_F32C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_CalcCovar_F32C1(pMeanI1, pMeanI2, pCorrI1I2, pCovI1I2, pitch, height, width, 0, height);
	}

	return MOK;
}

static MVoid stripe_CalcVarAddEpsilon_F32C1(const MFloat *pMeanI, const MFloat *pCorrII,
	MFloat *pCovII, MInt32 pitch, MInt32 height, MInt32 width, MFloat epsilon, 
	MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 i, j;
	MInt32 step = pitch / sizeof(MFloat);

#ifdef __ARM_NEON__
	float32x4_t v_epsilon = vdupq_n_f32(epsilon);
#endif

	for (i = lStartRow; i < lEndRow; ++i)
	{
		const MFloat *pMI = pMeanI + i * step;
		const MFloat *pCorr = pCorrII + i * step;
		MFloat *pCov = pCovII + i * step;

		j = 0;
#ifdef __ARM_NEON__
		for (; j < width - 3; j += 4)
		{
			float32x4_t v_src1, v_src11;
			float32x4_t v_dst;

			v_src1 = vld1q_f32(pMI + j);
			v_src11 = vld1q_f32(pCorr + j);
			v_src1 = vmulq_f32(v_src1, v_src1);
			v_dst = vaddq_f32(v_epsilon, vsubq_f32(v_src11, v_src1));

			vst1q_f32(pCov + j, v_dst);
		}
#endif

		for (; j < width; ++j)
		{
			MFloat tmpMI = pMI[j];
			pCov[j] = pCorr[j] - tmpMI * tmpMI + epsilon;
		}
	}
}

struct CalcVarAddEpsilon_F32C1_MT
{
	MInt32 taskID;
	const MFloat *pMeanI;
	const MFloat *pCorrII;
	MFloat *pCovII;
	MInt32 pitch;
	MInt32 height;
	MInt32 width;
	MFloat epsilon;
	MInt32 lStartRow;
	MInt32 lEndRow;
};

static MVoid thread_CalcVarAddEpsilon_F32C1(MVoid *info)
{
	CalcVarAddEpsilon_F32C1_MT *threadInfo = (CalcVarAddEpsilon_F32C1_MT *)info;
	stripe_CalcVarAddEpsilon_F32C1(threadInfo->pMeanI, threadInfo->pCorrII, threadInfo->pCovII,
		threadInfo->pitch, threadInfo->height, threadInfo->width, threadInfo->epsilon,
		threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT CalcVarAddEpsilon_F32C1(MHandle parEngine, MInt32 taskNum, const MFloat *pMeanI, const MFloat *pCorrII,
	MFloat *pCovII, MInt32 pitch, MInt32 height, MInt32 width, MFloat epsilon)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;

		MInt32 i = taskNum * (sizeof(CalcVarAddEpsilon_F32C1_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		CalcVarAddEpsilon_F32C1_MT *pParam = (CalcVarAddEpsilon_F32C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pMeanI = pMeanI;
			pParam[i].pCorrII = pCorrII;
			pParam[i].pCovII = pCovII;
			pParam[i].pitch = pitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].epsilon = epsilon;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_CalcVarAddEpsilon_F32C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_CalcVarAddEpsilon_F32C1(pMeanI, pCorrII, pCovII, pitch, height, width, epsilon, 0, height);
	}

	return MOK;
}

static MVoid stripe_CalcCovarInv_F32C1(MFloat **pCovar,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pCovarInv,
	MInt32 lStartRow, MInt32 lEndRow)
{
	const MFloat *pCovData = pCovar[0];
	MFloat *pCovInvData = pCovarInv[0];

	MInt32 i, j;
	MInt32 step = pitch / sizeof(MFloat);

	for (i = lStartRow; i < lEndRow; ++i)
	{
		const MFloat *pC = pCovData + i * step;
		MFloat *pCInv = pCovInvData + i * step;

		j = 0;
#ifdef __ARM_NEON__0
		for (; j < width - 3; j += 4)
		{
			float32x4_t v_src;
			float32x4_t v_dst;
			
			v_src = vld1q_f32(pC + j);
			v_dst = vrecpeq_f32(v_src);
			v_dst = vmulq_f32(vrecpsq_f32(v_src, v_dst), v_dst);
			v_dst = vmulq_f32(vrecpsq_f32(v_src, v_dst), v_dst);

			vst1q_f32(pCInv + j, v_dst);
		}
#endif

		for (; j < width; ++j)
			pCInv[j] = 1.f / pC[j];
	}
}


// https://www.wolframalpha.com/input/?i=inverse+%7B%7Ba,+b,+c%7D,+%7Bb,+d,+e%7D,+%7Bc,+e,+f%7D%7D
// variance of I in each local patch: the matrix Sigma
// Note the variance in each local patch is a 3x3 symmetric matrix:
//           RR, RG, RB
//   Sigma = RG, GG, GB
//           RB, GB, BB
static MVoid stripe_CalcCovarInv_F32C3(MFloat **pCovar,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pCovarInv,
	MInt32 lStartRow, MInt32 lEndRow)
{
	const MFloat *pCov00Data = pCovar[0];
	const MFloat *pCov01Data = pCovar[1];
	const MFloat *pCov02Data = pCovar[2];
	const MFloat *pCov11Data = pCovar[3];
	const MFloat *pCov12Data = pCovar[4];
	const MFloat *pCov22Data = pCovar[5];

	MFloat *pCovInv00Data = pCovarInv[0];
	MFloat *pCovInv01Data = pCovarInv[1];
	MFloat *pCovInv02Data = pCovarInv[2];
	MFloat *pCovInv11Data = pCovarInv[3];
	MFloat *pCovInv12Data = pCovarInv[4];
	MFloat *pCovInv22Data = pCovarInv[5];

	MInt32 i, j;
	MInt32 step = pitch / sizeof(MFloat);
	
	for (i = lStartRow; i < lEndRow; ++i)
	{
		const MFloat *pa00 = pCov00Data + i * step;
		const MFloat *pa01 = pCov01Data + i * step;
		const MFloat *pa02 = pCov02Data + i * step;
		const MFloat *pa11 = pCov11Data + i * step;
		const MFloat *pa12 = pCov12Data + i * step;
		const MFloat *pa22 = pCov22Data + i * step;

		MFloat *pa00Inv = pCovInv00Data + i * step;
		MFloat *pa01Inv = pCovInv01Data + i * step;
		MFloat *pa02Inv = pCovInv02Data + i * step;
		MFloat *pa11Inv = pCovInv11Data + i * step;
		MFloat *pa12Inv = pCovInv12Data + i * step;
		MFloat *pa22Inv = pCovInv22Data + i * step;

		j = 0;

#ifdef __ARM_NEON__0
		for (; j < width - 3; j += 4)
		{
			const float32x4_t v_a = vld1q_f32(pa00 + j);
			const float32x4_t v_b = vld1q_f32(pa01 + j);
			const float32x4_t v_c = vld1q_f32(pa02 + j);
			const float32x4_t v_d = vld1q_f32(pa11 + j);
			const float32x4_t v_e = vld1q_f32(pa12 + j);
			const float32x4_t v_f = vld1q_f32(pa22 + j);

			float32x4_t v_aInv = vmlsq_f32(vmulq_f32(v_d, v_f), v_e, v_e);
			float32x4_t v_bInv = vmlsq_f32(vmulq_f32(v_c, v_e), v_b, v_f);
			float32x4_t v_cInv = vmlsq_f32(vmulq_f32(v_b, v_e), v_c, v_d);
			float32x4_t v_dInv = vmlsq_f32(vmulq_f32(v_a, v_f), v_c, v_c);
			float32x4_t v_eInv = vmlsq_f32(vmulq_f32(v_b, v_c), v_a, v_e);
			float32x4_t v_fInv = vmlsq_f32(vmulq_f32(v_a, v_d), v_b, v_b);

			float32x4_t v_det = vmlaq_f32(vmlaq_f32(vmulq_f32(v_a, v_aInv), v_b, v_bInv), v_c, v_cInv);

			float32x4_t v_detInv = vrecpeq_f32(v_det);
			v_detInv = vmulq_f32(vrecpsq_f32(v_det, v_detInv), v_detInv);
			v_detInv = vmulq_f32(vrecpsq_f32(v_det, v_detInv), v_detInv);

			vst1q_f32(pa00Inv + j, vmulq_f32(v_aInv, v_detInv));
			vst1q_f32(pa01Inv + j, vmulq_f32(v_bInv, v_detInv));
			vst1q_f32(pa02Inv + j, vmulq_f32(v_cInv, v_detInv));
			vst1q_f32(pa11Inv + j, vmulq_f32(v_dInv, v_detInv));
			vst1q_f32(pa12Inv + j, vmulq_f32(v_eInv, v_detInv));
			vst1q_f32(pa22Inv + j, vmulq_f32(v_fInv, v_detInv));
		}
#endif

		for (; j < width; ++j)
		{
			const MFloat a = pa00[j];
			const MFloat b = pa01[j];
			const MFloat c = pa02[j];
			const MFloat d = pa11[j];
			const MFloat e = pa12[j];
			const MFloat f = pa22[j];

			MFloat aInv = d * f - e * e;
			MFloat bInv = c * e - b * f;
			MFloat cInv = b * e - c * d;
			MFloat dInv = a * f - c * c;
			MFloat eInv = b * c - a * e;
			MFloat fInv = a * d - b * b;

			MFloat det = a * aInv + b * bInv + c * cInv;
			MFloat detInv = fabs(det) > 1e-6f ? 1.f / det : 1.f;

			pa00Inv[j] = aInv * detInv;
			pa01Inv[j] = bInv * detInv;
			pa02Inv[j] = cInv * detInv;
			pa11Inv[j] = dInv * detInv;
			pa12Inv[j] = eInv * detInv;
			pa22Inv[j] = fInv * detInv;
		}
	}
}

struct CalcCovarInv_FLOAT32_MT
{
	MInt32 taskID;
	MFloat **pCovar;
	MInt32 pitch;
	MInt32 height;
	MInt32 width;
	MFloat **pCovarInv;
	MInt32 lStartRow;
	MInt32 lEndRow;
};

static MVoid thread_CalcCovarInv_F32C1(MVoid *info)
{
	CalcCovarInv_FLOAT32_MT *threadInfo = (CalcCovarInv_FLOAT32_MT *)info;
	stripe_CalcCovarInv_F32C1(threadInfo->pCovar, threadInfo->pitch,
		threadInfo->height, threadInfo->width, threadInfo->pCovarInv, threadInfo->lStartRow, threadInfo->lEndRow);
}


MRESULT CalcCovarInv_F32C1(MHandle parEngine, MInt32 taskNum, MFloat **pCovar,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pCovarInv)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;
		MInt32 i = taskNum * (sizeof(CalcCovarInv_FLOAT32_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		CalcCovarInv_FLOAT32_MT *pParam = (CalcCovarInv_FLOAT32_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);
		
		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pCovar = pCovar;
			pParam[i].pitch = pitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].pCovarInv = pCovarInv;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_CalcCovarInv_F32C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_CalcCovarInv_F32C1(pCovar, pitch, height, width, pCovarInv, 0, height);
	}

	return MOK;
}

static MVoid thread_CalcCovarInv_F32C3(MVoid *info)
{
	CalcCovarInv_FLOAT32_MT *threadInfo = (CalcCovarInv_FLOAT32_MT *)info;
	stripe_CalcCovarInv_F32C3(threadInfo->pCovar, threadInfo->pitch,
		threadInfo->height, threadInfo->width, threadInfo->pCovarInv, threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT CalcCovarInv_F32C3(MHandle parEngine, MInt32 taskNum, MFloat **pCovar,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pCovarInv)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;
		MInt32 i = taskNum * (sizeof(CalcCovarInv_FLOAT32_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		CalcCovarInv_FLOAT32_MT *pParam = (CalcCovarInv_FLOAT32_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pCovar = pCovar;
			pParam[i].pitch = pitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].pCovarInv = pCovarInv;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_CalcCovarInv_F32C3, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_CalcCovarInv_F32C3(pCovar, pitch, height, width, pCovarInv, 0, height);
	}

	return MOK;
}

MRESULT CalcCovarInv_F32(MHandle parEngine, MInt32 taskNum, MFloat **pCovar,
	MInt32 pitch, MInt32 height, MInt32 width, MInt32 channels, MFloat **pCovarInv)
{
	if (channels == 1)
	{
		return CalcCovarInv_F32C1(parEngine, taskNum, pCovar, pitch, height, width, pCovarInv);
	}
	else if (channels == 3)
	{
		return CalcCovarInv_F32C3(parEngine, taskNum, pCovar, pitch, height, width, pCovarInv);
	}
	else
	{
		return MERR_UNSUPPORTED;
	}
}

static MInt32 getCovGuidanceInvIdx(MInt32 guidanceIdx1, MInt32 guidanceIdx2, MInt32 guidanceCnNum)
{
	static MInt32 covGuidanceInvIdxData[] = {
		0, -1, -1, -1, -1, -1, -1, -1, -1,
		0, 1, -1, 1, 2, -1, -1, -1, -1,
		0, 1, 2, 1, 3, 4, 2, 4, 5
	};

	MInt32 idx = (guidanceCnNum - 1) * 9;
	idx += guidanceIdx1 * guidanceCnNum + guidanceIdx2;
	return covGuidanceInvIdxData[idx];
}

struct CalcAlpha_F32_MT
{
	MInt32 taskID;
	MFloat **pCovGuidanceSrc;
	MInt32 srcCnNum;
	MFloat **pCovGuidanceInv;
	MInt32 guidanceCnNum;

	MInt32 pitch;
	MInt32 height;
	MInt32 width;

	MFloat **pAlpha;

	MInt32 lStartRow;
	MInt32 lEndRow;
};

static MVoid stripe_CalcAlpha_F32(MFloat **pCovGuidanceSrc, MInt32 srcCnNum,
	MFloat **pCovGuidanceInv, MInt32 guidanceCnNum,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pAlpha,
	MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 i, j, sn, gn1, gn2, covGuidanceInvIdx;
	MInt32 step = pitch / sizeof(MFloat);
	MFloat *pDstA_Data;
	const MFloat *pCGS_Data, *pCGI_Data;
	MFloat *pDstA;
	const MFloat *pCGS, *pCGI;

	for (sn = 0; sn < srcCnNum; ++sn)
	{
		for (gn1 = 0; gn1 < guidanceCnNum; ++gn1)
		{
			pDstA_Data = pAlpha[sn * srcCnNum + gn1];
			for (gn2 = 0; gn2 < guidanceCnNum; ++gn2)
			{
				pCGS_Data = pCovGuidanceSrc[sn * srcCnNum + gn2];
				covGuidanceInvIdx = getCovGuidanceInvIdx(gn1, gn2, guidanceCnNum);
				pCGI_Data = pCovGuidanceInv[covGuidanceInvIdx];
				if (gn2 == 0)
				{
					for (i = lStartRow; i < lEndRow; ++i)
					{
						pCGS = pCGS_Data + i * step;
						pCGI = pCGI_Data + i * step;
						pDstA = pDstA_Data + i * step;

						j = 0;
#ifdef __ARM_NEON__
						for (; j < width - 3; j += 4)
						{
							float32x4_t v_src1, v_src2;
							float32x4_t v_dst;

							v_src1 = vld1q_f32(pCGS + j);
							v_src2 = vld1q_f32(pCGI + j);
							v_dst = vmulq_f32(v_src1, v_src2);
						
							vst1q_f32(pDstA + j, v_dst);
						}
#endif					

						for (; j < width; ++j)
						{
							pDstA[j] = pCGS[j] * pCGI[j];
						}
					}
				}
				else
				{
					for (i = lStartRow; i < lEndRow; ++i)
					{
						pCGS = pCGS_Data + i * step;
						pCGI = pCGI_Data + i * step;
						pDstA = pDstA_Data + i * step;

						j = 0;
#ifdef __ARM_NEON__
						for (; j < width - 3; j += 4)
						{
							float32x4_t v_src1, v_src2;
							float32x4_t v_dst;

							v_src1 = vld1q_f32(pCGS + j);
							v_src2 = vld1q_f32(pCGI + j);
							v_dst = vld1q_f32(pDstA + j);
							v_dst = vaddq_f32(v_dst, vmulq_f32(v_src1, v_src2));

							vst1q_f32(pDstA + j, v_dst);
						}
#endif					

						for (; j < width; ++j)
						{
							pDstA[j] += pCGS[j] * pCGI[j];
						}
					}
				}
			}
		}
	}
}

static MVoid thread_CalcAlpha_F32(MVoid *info)
{
	CalcAlpha_F32_MT *threadInfo = (CalcAlpha_F32_MT *)info;
	stripe_CalcAlpha_F32(threadInfo->pCovGuidanceSrc, threadInfo->srcCnNum,
		threadInfo->pCovGuidanceInv, threadInfo->guidanceCnNum,
		threadInfo->pitch, threadInfo->height, threadInfo->width, threadInfo->pAlpha,
		threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT CalcAlpha_F32(MHandle parEngine, MInt32 taskNum,
	MFloat **pCovGuidanceSrc, MInt32 srcCnNum,
	MFloat **pCovGuidanceInv, MInt32 guidanceCnNum,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pAlpha)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;
		MInt32 i = taskNum * (sizeof(CalcAlpha_F32_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		CalcAlpha_F32_MT *pParam = (CalcAlpha_F32_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pCovGuidanceSrc = pCovGuidanceSrc;
			pParam[i].srcCnNum = srcCnNum;
			pParam[i].pCovGuidanceInv = pCovGuidanceInv;
			pParam[i].guidanceCnNum = guidanceCnNum;
			pParam[i].pitch = pitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].pAlpha = pAlpha;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_CalcAlpha_F32, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_CalcAlpha_F32(pCovGuidanceSrc, srcCnNum, pCovGuidanceInv, guidanceCnNum, 
			pitch, height, width, pAlpha, 0, height);
	}

	return MOK;
}

struct CalcBeta_F32_MT
{
	MInt32 taskID;
	MFloat **pSrcMean;
	MInt32 srcCnNum;
	MFloat **pGuidanceMean;
	MInt32 guidanceCnNum;
	MFloat **pAlpha;

	MInt32 pitch;
	MInt32 height;
	MInt32 width;

	MFloat **pBeta;

	MInt32 lStartRow;
	MInt32 lEndRow;
};

static MVoid stripe_CalcBeta_F32(MFloat **pSrcMean, MInt32 srcCnNum,
	MFloat **pGuidanceMean, MInt32 guidanceCnNum,
	MFloat **pAlpha,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pBeta,
	MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 i, j, sn, gn;
	MInt32 step = pitch / sizeof(MFloat);
	MFloat *pDstB_Data;
	const MFloat *pA_Data, *pGM_Data;
	MFloat *pDst;
	const MFloat *pA, *pGM;

	for (sn = 0; sn < srcCnNum; ++sn)
	{
		pDstB_Data = pBeta[sn];
		for (gn = 0; gn < guidanceCnNum; ++gn)
		{
			pA_Data = pAlpha[sn * srcCnNum + gn];
			pGM_Data = pGuidanceMean[gn];

			for (i = lStartRow; i < lEndRow; ++i)
			{
				pDst = pDstB_Data + i * step;
				pA = pA_Data + i * step;
				pGM = pGM_Data + i * step;

				j = 0;
#ifdef __ARM_NEON__
				for (; j < width - 3; j += 4)
				{
					float32x4_t v_srcA, v_srcGM;
					float32x4_t v_dst;

					v_srcA = vld1q_f32(pA + j);
					v_srcGM = vld1q_f32(pGM + j);
					v_dst = vld1q_f32(pDst + j);

					v_dst = vsubq_f32(v_dst, vmulq_f32(v_srcA, v_srcGM));
					vst1q_f32(pDst + j, v_dst);
				}
#endif

				for (; j < width; ++j)
				{
					pDst[j] -= pA[j] * pGM[j];
				}
			}
		}
	}
}

static MVoid thread_CalcBeta_F32(MVoid *info)
{
	CalcBeta_F32_MT *threadInfo = (CalcBeta_F32_MT *)info;
	stripe_CalcBeta_F32(threadInfo->pSrcMean, threadInfo->srcCnNum, 
		threadInfo->pGuidanceMean, threadInfo->guidanceCnNum, threadInfo->pAlpha, 
		threadInfo->pitch, threadInfo->height, threadInfo->width, threadInfo->pBeta, 
		threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT CalcBeta_F32(MHandle parEngine, MInt32 taskNum,
	MFloat **pSrcMean, MInt32 srcCnNum,
	MFloat **pGuidanceMean, MInt32 guidanceCnNum,
	MFloat **pAlpha,
	MInt32 pitch, MInt32 height, MInt32 width, MFloat **pBeta)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;
		MInt32 i = taskNum * (sizeof(CalcBeta_F32_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		CalcBeta_F32_MT *pParam = (CalcBeta_F32_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrcMean = pSrcMean;
			pParam[i].srcCnNum = srcCnNum;
			pParam[i].pGuidanceMean = pGuidanceMean;
			pParam[i].guidanceCnNum = guidanceCnNum;
			pParam[i].pAlpha = pAlpha;
			pParam[i].pitch = pitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].pBeta = pBeta;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_CalcBeta_F32, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_CalcBeta_F32(pSrcMean, srcCnNum, pGuidanceMean, guidanceCnNum, pAlpha, 
			pitch, height, width, pBeta, 0, height);
	}

	return MOK;
}

struct ApplyTransform_F32U8_MT
{
	MInt32 taskID;
	MFloat **pAlphaMean;
	MFloat **pBetaMean;
	MInt32 srcCnNum;
	MInt32 f32Pitch;
	MByte **pGuidanceCn;
	MInt32 guidanceCnNum;
	MInt32 u8Pitch;

	MInt32 height;
	MInt32 width;

	MFloat **pDst;

	MInt32 lStartRow;
	MInt32 lEndRow;
};

static MVoid stripe_ApplyTransform_F32U8(MFloat **pAlphaMean,
	MFloat **pBetaMean, MInt32 srcCnNum, MInt32 f32Pitch,
	MByte **pGuidanceCn, MInt32 guidanceCnNum, MInt32 u8Pitch, 
	MInt32 height, MInt32 width, MFloat **pDst, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 i, j, sn, gn;
	MInt32 u8Step = u8Pitch, f32Step = f32Pitch / sizeof(MFloat);

	const MFloat *pAM_Data;
	const MUInt8 *pGCn_Data;
	MFloat *pD_Data;
	const MFloat *pAM;
	const MUInt8 *pGCn;
	MFloat *pD;

	for (sn = 0; sn < srcCnNum; ++sn)
	{
		pD_Data = pDst[sn];

		for (gn = 0; gn < guidanceCnNum; ++gn)
		{
			pAM_Data = pAlphaMean[sn * srcCnNum + gn];
			pGCn_Data = pGuidanceCn[gn];

			for (i = lStartRow; i < lEndRow; ++i)
			{
				pD = pD_Data + i * f32Step;
				pAM = pAM_Data + i * f32Step;
				pGCn = pGCn_Data + i * u8Step;

				j = 0;
#ifdef __ARM_NEON__
				for (; j < width - 7; j += 8)
				{
					uint16x8_t v_GCn;
					float32x4_t v_AM, v_dst;

					v_GCn = vmovl_u8(vld1_u8(pGCn + j));
					v_AM = vld1q_f32(pAM + j);
					v_dst = vld1q_f32(pD + j);
					
					v_dst = vmlaq_f32(v_dst, v_AM, vcvtq_f32_u32(vmovl_u16(vget_low_u16(v_GCn))));
					vst1q_f32(pD + j, v_dst);

					v_AM = vld1q_f32(pAM + j + 4);
					v_dst = vld1q_f32(pD + j + 4);

					v_dst = vmlaq_f32(v_dst, v_AM, vcvtq_f32_u32(vmovl_u16(vget_high_u16(v_GCn))));
					vst1q_f32(pD + j + 4, v_dst);
				}
#endif
				for (; j < width; ++j)
				{
					/*if (i == 808 && j == 806)
					{
						i = i;
					}*/
					pD[j] += pAM[j] * pGCn[j];
				}
			}
		}
	}
}

static MVoid thread_ApplyTransform_F32U8(MVoid *info)
{
	ApplyTransform_F32U8_MT *threadInfo = (ApplyTransform_F32U8_MT *)info;
	stripe_ApplyTransform_F32U8(threadInfo->pAlphaMean, threadInfo->pBetaMean, threadInfo->srcCnNum, threadInfo->f32Pitch,
		threadInfo->pGuidanceCn, threadInfo->guidanceCnNum, threadInfo->u8Pitch, threadInfo->height, threadInfo->width,
		threadInfo->pDst, threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT ApplyTransform_F32U8(MHandle parEngine, MInt32 taskNum,
	MFloat **pAlphaMean,
	MFloat **pBetaMean, MInt32 srcCnNum, MInt32 f32Pitch,
	MByte **pGuidanceCn, MInt32 guidanceCnNum, MInt32 u8Pitch,
	MInt32 height, MInt32 width, MFloat **pDst)
{
	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;
		MInt32 i = taskNum * (sizeof(ApplyTransform_F32U8_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		ApplyTransform_F32U8_MT *pParam = (ApplyTransform_F32U8_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pAlphaMean = pAlphaMean;
			pParam[i].pBetaMean = pBetaMean;
			pParam[i].srcCnNum = srcCnNum;
			pParam[i].f32Pitch = f32Pitch;
			pParam[i].pGuidanceCn = pGuidanceCn;
			pParam[i].guidanceCnNum = guidanceCnNum;
			pParam[i].u8Pitch = u8Pitch;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].pDst = pDst;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_ApplyTransform_F32U8, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_ApplyTransform_F32U8(pAlphaMean, pBetaMean, srcCnNum, f32Pitch, 
			pGuidanceCn, guidanceCnNum, u8Pitch, height, width, pDst, 0, height);
	}

	return MOK;
}

#ifndef SWAP_BUFFER
#define SWAP_BUFFER(a, b, t)  ((t) = (a), (a) = (b), (b) = (t))
#endif

#define FLT_TO_FIX(x,n)				(MInt32)((x)*(1<<(n))+0.5f)
#define DESCALE(x,n)				(((x) + (1 << ((n)-1))) >> (n))

// for bilinear interpolation
#define BL_WARP_SHIFT				7
#define BL_WARP_MUL_ONE_8U(x)		((x) << BL_WARP_SHIFT)
#define BL_WARP_DESCALE_8U(x)		DESCALE((x), BL_WARP_SHIFT)
#define BL_WARP_DESCALE2_8U(x)		DESCALE((x), (BL_WARP_SHIFT<<1))

MRESULT ImgBilinearResizeAllocMem_F32C1(MHandle hMemMgr, MInt32 srcWidth, MInt32 srcHeight,
	MInt32 dstWidth, MInt32 dstHeight, MInt32 taskNum, BilinearResize_Param *pParam)
{
	if (!pParam) return MERR_INVALID_PARAM;

	MFloat scaleX = (MFloat)srcWidth / dstWidth,
		scaleY = (MFloat)srcHeight / dstHeight;

	pParam->channelsNum = 1;
	MInt32 bufSize = (dstWidth * 2 * taskNum) * sizeof(MFloat) 
		+ sizeof(MInt32) 
		+ (dstWidth + dstHeight) * (sizeof(MFloat) + sizeof(MInt32));
	if (pParam->pBuf) 
		MMemFree(hMemMgr, pParam->pBuf);
	pParam->pBuf = MMemAlloc(hMemMgr, bufSize);
	if (!pParam->pBuf) return MERR_NO_MEMORY;

	MInt32 *xCoor = (MInt32 *)(pParam->pBuf) + 1;
	MFloat *xWeight = (MFloat *)(xCoor + dstWidth);
	MInt32 *yCoor = (MInt32 *)(xWeight + dstWidth);
	MFloat *yWeight = (MFloat *)(yCoor + dstHeight);

	MInt32 xMaxCoor = dstWidth;
	MFloat fx, fy;
	MInt32 sx, sy;

	for (MInt32 dx = 0; dx < dstWidth; ++dx)
	{
		fx = (dx + 0.5f) * scaleX - 0.5f;
		sx = (MInt32)fx;
		fx -= sx;

		if (fx < 0)
			fx = 0, sx = 0;
		if (sx >= srcWidth - 1)
		{
			fx = 0, sx = srcWidth - 1;
			if (xMaxCoor >= dstWidth)
				xMaxCoor = dx;
		}

		xCoor[dx] = sx;
		xWeight[dx] = fx;
	}

	*((MInt32 *)pParam->pBuf) = xMaxCoor;

	for (MInt32 dy = 0; dy < dstHeight; ++dy)
	{
		fy = (dy + 0.5f) * scaleY - 0.5f;
		sy = (MInt32)fy;
		fy -= sy;

		if (fy < 0)
			fy = 0, sy = 0;
		if (fy >= srcHeight - 1)
			fy = 0, sy = srcHeight - 1;

		yCoor[dy] = sy;
		yWeight[dy] = fy;
	}

	return MOK;
}

struct ImgBilinearResize_F32C1_MT
{
	MInt32 taskID;
	const MFloat *pSrc;
	MInt32 srcWidth;
	MInt32 srcHeight;
	MInt32 srcPitch;

	MFloat *pDst;
	MInt32 dstWidth;
	MInt32 dstHeight;
	MInt32 dstPitch;

	MInt32 lStartRow;
	MInt32 lEndRow;

	const BilinearResize_Param *pBLParam;
};

static MVoid stripe_ImgBilinearResize_F32C1(const MFloat *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
	MFloat *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch, 
	MInt32 lStartRow, MInt32 lEndRow, MInt32 taskID, const BilinearResize_Param *pBLParam)
{
	MInt32 srcStep = srcPitch / sizeof(MFloat), dstStep = dstPitch / sizeof(MFloat);

	// get info and buffer for bilinear resize
	MInt32 xMaxCoor = *((MInt32 *)pBLParam->pBuf);
	MInt32 *xCoor = (MInt32 *)pBLParam->pBuf + 1;
	MFloat *xWeight = (MFloat *)(xCoor + dstWidth);
	MInt32 *yCoor = (MInt32 *)(xWeight + dstWidth);
	MFloat *yWeight = (MFloat *)(yCoor + dstHeight);
	MFloat *buf0 = yWeight + dstHeight + taskID * dstWidth * 2;
	MFloat *buf1 = buf0 + dstWidth;

	// do bilinear resize
	MInt32 prev_sy0 = -1, prev_sy1 = -1;
	MInt32 k, dx, dy;

	MFloat *pD = pDst + lStartRow * dstStep;
	for (dy = lStartRow; dy < lEndRow; ++dy, pD += dstStep)
	{
		MInt32 sy0 = yCoor[dy];
		MFloat fy = yWeight[dy];
		MInt32 sy1 = sy0 + (fy > 0 && sy0 < srcHeight - 1);

		if (sy0 == prev_sy0 && sy1 == prev_sy1)
			k = 2;
		else if (sy0 == prev_sy1)
		{
			MFloat *swap_t;
			// swap buffer
			SWAP_BUFFER(buf0, buf1, swap_t);
			k = 1;
		}
		else
			k = 0;

		for (; k < 2; k++)
		{
			MFloat* tmpBuf = k == 0 ? buf0 : buf1;
			MInt32 sy = k == 0 ? sy0 : sy1;
			if (k == 1 && sy1 == sy0)
			{
				MMemCpy(buf1, buf0, dstWidth * sizeof(buf0[0]));
				continue;
			}

			const MFloat *pS = pSrc + sy * srcStep;
			for (dx = 0; dx < xMaxCoor; dx++)
			{
				MInt32 sx = xCoor[dx];
				MFloat fx = xWeight[dx];

				MFloat sVal0 = pS[sx];
				MFloat sVal1 = pS[sx + 1];
				MFloat val = sVal0 + fx * (sVal1 - sVal0);

				tmpBuf[dx] = val;
			}

			for (; dx < dstWidth; dx++)
			{
				MInt32 sx = xCoor[dx];
				tmpBuf[dx] = pS[sx];
			}
		}

		prev_sy0 = sy0;
		prev_sy1 = sy1;

		if (sy0 == sy1)
		{
			MMemCpy(pD, buf0, dstWidth * sizeof(MFloat));
		}
		else
		{
			dx = 0;
#ifdef __ARM_NEON__
			float32x4_t v_fy = vdupq_n_f32(fy);
			for (; dx < dstWidth - 3; dx += 4)
			{
				float32x4_t v_src0, v_src1;
				float32x4_t v_dst;

				v_src0 = vld1q_f32(buf0 + dx);
				v_src1 = vld1q_f32(buf1 + dx);
				v_src1 = vsubq_f32(v_src1, v_src0);
				v_dst = vaddq_f32(v_src0, vmulq_f32(v_fy, v_src1));

				vst1q_f32(pD + dx, v_dst);
			}
#endif
			for (; dx < dstWidth; ++dx)
			{
				MFloat sVal0 = buf0[dx];
				MFloat sVal1 = buf1[dx];

				pD[dx] = sVal0 + fy * (sVal1 - sVal0);
			}
		}
	}
}

static MVoid thread_ImgBilinearResize_F32C1(MVoid *info)
{
	ImgBilinearResize_F32C1_MT *threadInfo = (ImgBilinearResize_F32C1_MT *)info;
	stripe_ImgBilinearResize_F32C1(threadInfo->pSrc, threadInfo->srcWidth, threadInfo->srcHeight, threadInfo->srcPitch,
		threadInfo->pDst, threadInfo->dstWidth, threadInfo->dstHeight, threadInfo->dstPitch, 
		threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->taskID, threadInfo->pBLParam);
}

MRESULT ImgBilinearResize_F32C1(MHandle parEngine, MInt32 taskNum,
	const MFloat *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
	MFloat *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch, 
	const BilinearResize_Param *pBLParam)
{
	// same size, only copy
	if (srcWidth == dstWidth && srcHeight == dstHeight)
	{
		MMemCpy(pDst, pSrc, dstHeight * dstPitch);
		return MOK;
	}

	if (!pBLParam)
		return MERR_INVALID_PARAM;

	if (parEngine)
	{
		if (taskNum > 8) taskNum = 8;
		MInt32 i = taskNum * (sizeof(ImgBilinearResize_F32C1_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		ImgBilinearResize_F32C1_MT *pParam = (ImgBilinearResize_F32C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = dstHeight / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrc = pSrc;
			pParam[i].srcHeight = srcHeight;
			pParam[i].srcWidth = srcWidth;
			pParam[i].srcPitch = srcPitch;
			pParam[i].pDst = pDst;
			pParam[i].dstHeight = dstHeight;
			pParam[i].dstWidth = dstWidth;
			pParam[i].dstPitch = dstPitch;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
			pParam[i].pBLParam = pBLParam;
		}
		pParam[taskNum - 1].lEndRow = dstHeight;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_ImgBilinearResize_F32C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_ImgBilinearResize_F32C1(pSrc, srcWidth, srcHeight, srcPitch, 
			pDst, dstWidth, dstHeight, dstPitch, 0, dstHeight, 0, pBLParam);
	}

	return MOK;
}

MRESULT ImgBilinearResizeAllocMem_U8C1(MHandle hMemMgr, MInt32 srcWidth, MInt32 srcHeight,
	MInt32 dstWidth, MInt32 dstHeight, MInt32 taskNum, BilinearResize_Param *pParam)
{
	if (!pParam)  return MERR_INVALID_PARAM;

	MFloat scale_x, scale_y, fx, fy;
	MInt32 buf_size, xmax = dstWidth, sx, sy, dx, dy;
	MDWord *xofs, *yofs;
	MInt32 lTaskNum = Get_Task_Num(dstWidth, dstHeight);

	scale_x = (MFloat)srcWidth / dstWidth;
	scale_y = (MFloat)srcHeight / dstHeight;
	buf_size = dstWidth * 2 * lTaskNum * sizeof(MInt16)
		+ sizeof(MInt32) 
		+ (dstWidth + dstHeight)*sizeof(MDWord);

	if (pParam->pBuf)
		MMemFree(hMemMgr, pParam->pBuf);
	pParam->pBuf = MMemAlloc(hMemMgr, buf_size);
	if (!pParam->pBuf) return MERR_NO_MEMORY;

	pParam->channelsNum = 1;

	xofs = (MDWord*)((MInt32*)pParam->pBuf + 1);
	yofs = xofs + dstWidth;

	for (dy = 0; dy < dstHeight; dy++)
	{
		MDWord val;
		//fy = dy*scale_y; 
		fy = (dy + 0.5f)*scale_y - 0.5f;
		sy = (MInt32)fy;
		fy -= sy;
		if (fy < 0)
			sy = 0, fy = 0;
		if (sy >= srcHeight - 1)
			fy = 0, sy = srcHeight - 1;

		val = FLT_TO_FIX(fy, BL_WARP_SHIFT);
		yofs[dy] = sy | (val << 16);
	}

	for (dx = 0; dx < dstWidth; dx++)
	{
		MDWord val;
		//fx = dx*scale_x;
		fx = ((dx + 0.5f)*scale_x - 0.5f);
		sx = (MInt32)fx;
		fx -= sx;
		if (fx < 0)
			fx = 0, sx = 0;

		if (sx >= srcWidth - 1)
		{
			fx = 0, sx = srcWidth - 1;
			if (xmax >= dstWidth)
				xmax = dx;
		}

		val = FLT_TO_FIX(fx, BL_WARP_SHIFT);
		xofs[dx] = sx | (val << 16);
	}

	*((MInt32*)pParam->pBuf) = xmax;

	return MOK;
}

struct ImgBilinearResize_U8C1_MT
{
	MInt32 taskID;
	const MUInt8 *pSrc;
	MInt32 srcWidth;
	MInt32 srcHeight;
	MInt32 srcPitch;

	MUInt8 *pDst;
	MInt32 dstWidth;
	MInt32 dstHeight;
	MInt32 dstPitch;

	MInt32 lStartRow;
	MInt32 lEndRow;

	const BilinearResize_Param *pBLParam;
};

static MVoid stripe_ImgBilinearResize_U8C1(const MUInt8 *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
	MUInt8 *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch,
	MInt32 lStartRow, MInt32 lEndRow, MInt32 taskID, const BilinearResize_Param *pBLParam)
{
	MInt32 srcStep = srcPitch, dstStep = dstPitch;

	// get info and buffer for bilinear resize
	MInt32 xMaxCoor = *((MInt32 *)pBLParam->pBuf);
	MDWord *xofs = (MDWord *)pBLParam->pBuf + 1;
	MDWord *yofs = xofs + dstWidth;
	MInt16 *buf0 = (MInt16 *)(yofs + dstHeight) + taskID * dstWidth * 2;
	MInt16 *buf1 = buf0 + dstWidth;

	// do bilinear resize
	MInt32 prev_sy0 = -1, prev_sy1 = -1;
	MInt32 k, dx, dy;

	MUInt8 *pD = pDst + lStartRow * dstStep;
	for (dy = lStartRow; dy < lEndRow; ++dy, pD += dstStep)
	{
		MInt32 sy0 = yofs[dy];
		MInt32 fy = sy0 >> 16;
		sy0 = sy0 & 0xffff;
		MInt32 sy1 = sy0 + (fy > 0 && sy0 < srcHeight - 1);

		if (sy0 == prev_sy0 && sy1 == prev_sy1)
			k = 2;
		else if (sy0 == prev_sy1)
		{
			MInt16 *swap_t;
			// swap buffer
			SWAP_BUFFER(buf0, buf1, swap_t);
			k = 1;
		}
		else
			k = 0;

		for (; k < 2; k++)
		{
			MInt16* tmpBuf = k == 0 ? buf0 : buf1;
			MInt32 sy = k == 0 ? sy0 : sy1;
			if (k == 1 && sy1 == sy0)
			{
				MMemCpy(buf1, buf0, dstWidth * sizeof(buf0[0]));
				continue;
			}

			const MUInt8 *pS = pSrc + sy * srcStep;
			for (dx = 0; dx < xMaxCoor; dx++)
			{
				MInt32 sx = xofs[dx];
				MInt32 fx = sx >> 16;
				sx = sx & 0xffff;

				MInt32 sVal0 = pS[sx];
				MInt32 sVal1 = pS[sx + 1];
				MInt32 val = BL_WARP_MUL_ONE_8U(sVal0);
				val += fx * (sVal1 - sVal0);

				tmpBuf[dx] = val;
			}

			for (; dx < dstWidth; dx++)
			{
				MInt32 sx = xofs[dx];
				sx = sx & 0xffff;

				tmpBuf[dx] = BL_WARP_MUL_ONE_8U(pS[sx]);
			}
		}

		prev_sy0 = sy0;
		prev_sy1 = sy1;

		if (sy0 == sy1)
		{
			dx = 0;
#ifdef __ARM_NEON__
			for (; dx < dstWidth - 7; dx += 8)
			{
				int16x8_t v_buf;
				v_buf = vld1q_s16(buf0 + dx);
				vst1_u8(pD + dx, vqrshrun_n_s16(v_buf, 7));
			}
#endif
			for (; dx < dstWidth; ++dx)
				pD[dx] = (MUInt8)BL_WARP_DESCALE_8U(buf0[dx]);
		}
		else
		{
			dx = 0;
#ifdef __ARM_NEON__
			int16x4_t v_fy = vdup_n_s16(fy);
			for (; dx < dstWidth - 7; dx += 8)
			{
				int16x8_t v_sx0, v_sx1, v_sub;
				int32x4_t v_val;
				uint16x4_t v_dst0, v_dst1;
				uint16x8_t v_dst;
				v_sx0 = vld1q_s16(buf0 + dx);
				v_sx1 = vld1q_s16(buf1 + dx);
				v_sub = vsubq_s16(v_sx1, v_sx0);
				v_val = vshll_n_s16(vget_low_s16(v_sx0), 7);
				v_val = vmlal_s16(v_val, v_fy, vget_low_s16(v_sub));
				v_dst0 = vqrshrun_n_s32(v_val, 14);

				v_val = vshll_n_s16(vget_high_s16(v_sx0), 7);
				v_val = vmlal_s16(v_val, v_fy, vget_high_s16(v_sub));
				v_dst1 = vqrshrun_n_s32(v_val, 14);
				v_dst = vcombine_u16(v_dst0, v_dst1);
				vst1_u8(pD + dx, vmovn_u16(v_dst));
			}
#endif
			for (; dx < dstWidth; ++dx)
			{
				MInt32 sx0 = buf0[dx];
				MInt32 sx = buf1[dx];
				MInt32 val = BL_WARP_MUL_ONE_8U(sx0);
				val += fy * (sx - sx0);
				pD[dx] = (MByte)BL_WARP_DESCALE2_8U(val);
			}
		}
	}
}

static MVoid thread_ImgBilinearResize_U8C1(MVoid *info)
{
	ImgBilinearResize_U8C1_MT *threadInfo = (ImgBilinearResize_U8C1_MT *)info;
	stripe_ImgBilinearResize_U8C1(threadInfo->pSrc, threadInfo->srcWidth, threadInfo->srcHeight, threadInfo->srcPitch,
		threadInfo->pDst, threadInfo->dstWidth, threadInfo->dstHeight, threadInfo->dstPitch,
		threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->taskID, threadInfo->pBLParam);
}

MRESULT ImgBilinearResize_U8C1(MHandle parEngine, MInt32 taskNum,
	const MUInt8 *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
	MUInt8 *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch, const BilinearResize_Param *pBLParam)
{
	// same size, only copy
	if (srcWidth == dstWidth && srcHeight == dstHeight)
	{
		//MMemCpy(pDst, pSrc, dstHeight * dstPitch);
		const MUInt8 *pSrcRow = pSrc;
		MUInt8 *pDstRow = pDst;
		for (MInt32 y = 0; y < dstHeight; ++y)
		{
			MMemCpy(pDstRow, pSrcRow, dstWidth * sizeof(MUInt8));
			pSrcRow += srcPitch;
			pDstRow += dstPitch;
		}
		return MOK;
	}

	if (!pBLParam) return MERR_INVALID_PARAM;

	if (parEngine)
	{
		//if (taskNum > 8) taskNum = 8;
		MInt32 taskNum = Get_Task_Num(dstWidth, dstHeight);

		MInt32 i = taskNum * (sizeof(ImgBilinearResize_U8C1_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(MNull, i);
		if (!buffer) return MERR_NO_MEMORY;
		ImgBilinearResize_U8C1_MT *pParam = (ImgBilinearResize_U8C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);

		MInt32 rowStep = (dstHeight / taskNum) >> 2 << 2;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrc = pSrc;
			pParam[i].srcHeight = srcHeight;
			pParam[i].srcWidth = srcWidth;
			pParam[i].srcPitch = srcPitch;
			pParam[i].pDst = pDst;
			pParam[i].dstHeight = dstHeight;
			pParam[i].dstWidth = dstWidth;
			pParam[i].dstPitch = dstPitch;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
			pParam[i].pBLParam = pBLParam;
		}
		pParam[taskNum - 1].lEndRow = dstHeight;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_ImgBilinearResize_U8C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(MNull, buffer);
	}
	else
	{
		stripe_ImgBilinearResize_U8C1(pSrc, srcWidth, srcHeight, srcPitch,
			pDst, dstWidth, dstHeight, dstPitch, 0, dstHeight, 0, pBLParam);
	}

	return MOK;
}