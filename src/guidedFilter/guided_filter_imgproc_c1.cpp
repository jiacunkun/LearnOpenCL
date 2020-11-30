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

#define FIX_ALPHA_SCALE (14)
//#define FIX_BETA_SCALE (6)

MInt32 Get_Task_Num(MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lTaskNum = 1;
	MInt32 lMaxVal = MAX(lWidth, lHeight);

	if (lMaxVal >= 2000)
	{
		lTaskNum = 16;
	}
	else if (lMaxVal >= 800)
	{
		lTaskNum = 8;
	}
	else if (lMaxVal >= 400)
	{
		lTaskNum = 4;
	}
	else if (lMaxVal >= 200)
	{
		lTaskNum = 2;
	}
	else
	{
		lTaskNum = 1;
	}

	return lTaskNum;
}

MRESULT MMemAlloc_INT32(MHandle hMemMgr, MInt32 **pData, MInt32 size)
{
	if (pData == MNull) return MERR_INVALID_PARAM;

	MInt32 *pBuffer = (MInt32 *)MMemAlloc(hMemMgr, size);
	if (!pBuffer) return MERR_NO_MEMORY;

	*pData = pBuffer;

	return MOK;
}

//MRESULT MMemAlloc_GuidedFilter_C1_False(ArcGuidedFilter_C1_False *pAgfInfo)
//{
//	if (!pAgfInfo) return MERR_INVALID_PARAM;
//
//	MRESULT res = MOK;
//
//	MHandle hMemMgr = pAgfInfo->hMemMgr;
//
//	MInt32 singleCnPixelSize = pAgfInfo->height * pAgfInfo->width;
//	MInt32 ucharBufferSize = 0, ushortBufferSize = 0, intBufferSize = 0;
//	
//	MInt32 gfScale = pAgfInfo->gfScale;
//	MInt32 heightSub = pAgfInfo->height / gfScale;
//	MInt32 widthSub = pAgfInfo->width / gfScale;
//	MInt32 singleCnSubPixelSize = heightSub * widthSub;
//
//	ucharBufferSize = singleCnSubPixelSize * sizeof(MByte) * 4;
//	res = MMemAlloc_UCHAR(hMemMgr, &(pAgfInfo->pUCHAR_Buffer), ucharBufferSize);
//	CHECK_ERROR(res);
//	// pGuidanceCnSub
//	pAgfInfo->pGuidanceCnSub = pAgfInfo->pUCHAR_Buffer;
//	// pGuidanceCnMean
//	pAgfInfo->pGuidanceCnMean = pAgfInfo->pGuidanceCnSub + singleCnSubPixelSize;
//	// pSrcCnSub
//	pAgfInfo->pSrcCnSub = pAgfInfo->pGuidanceCnMean + singleCnSubPixelSize;
//	// pSrcCnMean
//	pAgfInfo->pSrcCnMean = pAgfInfo->pSrcCnSub + singleCnSubPixelSize;
//
//	ushortBufferSize = singleCnSubPixelSize * sizeof(MUInt16) * 3 + singleCnPixelSize * sizeof(MUInt16) * 2;
//	res = MMemAlloc_USHORT(hMemMgr, &(pAgfInfo->pUSHORT_Buffer), ushortBufferSize);
//	CHECK_ERROR(res);
//	// pCorrGuidance/pCorrGuidanceSrc/pAlphaSub/pBetaSub
//	pAgfInfo->pCorrGuidance = pAgfInfo->pCorrGuidanceSrc = pAgfInfo->pUSHORT_Buffer;
//	pAgfInfo->pBetaSub = pAgfInfo->pAlphaSub = (MInt16 *)pAgfInfo->pUSHORT_Buffer;
//	// pCorrGuidanceMean/pCorrGuidanceSrcMean/pAlphaSubMean
//	pAgfInfo->pCorrGuidanceMean = pAgfInfo->pCorrGuidanceSrcMean = pAgfInfo->pCorrGuidance + singleCnSubPixelSize;
//	pAgfInfo->pAlphaSubMean = (MInt16 *)pAgfInfo->pCorrGuidanceMean;
//	//pBetaSubMean
//	pAgfInfo->pBetaSubMean = (MInt16 *)pAgfInfo->pAlphaSubMean + singleCnSubPixelSize;
//	// pAlphaMean
//	pAgfInfo->pAlphaMean = (MInt16 *)pAgfInfo->pBetaSubMean + singleCnSubPixelSize;
//	// pBetaMean
//	pAgfInfo->pBetaMean = (MInt16 *)pAgfInfo->pAlphaMean + singleCnPixelSize;
//
//	intBufferSize = singleCnSubPixelSize * sizeof(MInt32) * 2;
//	res = MMemAlloc_INT32(hMemMgr, &(pAgfInfo->pINT_Buffer), intBufferSize);
//	CHECK_ERROR(res);
//	// pCovGuidance/pCovGuidanceSrc
//	pAgfInfo->pCovGuidance = pAgfInfo->pCovGuidanceSrc = pAgfInfo->pINT_Buffer;
//	// pCovInvGuidance
//	pAgfInfo->pCovInvGuidance = pAgfInfo->pCovGuidance + singleCnSubPixelSize;
//
//exit:
//	return res;
//}
//
//MVoid MMemFree_GuidedFilter_C1_False(ArcGuidedFilter_C1_False *pAgfInfo)
//{
//	if (pAgfInfo)
//	{
//		MHandle hMemMgr = pAgfInfo->hMemMgr;
//		if (pAgfInfo->pUCHAR_Buffer)
//			MMemFree(hMemMgr, pAgfInfo->pUCHAR_Buffer);
//		if (pAgfInfo->pUSHORT_Buffer)
//			MMemFree(hMemMgr, pAgfInfo->pUSHORT_Buffer);
//		if (pAgfInfo->pINT_Buffer)
//			MMemFree(hMemMgr, pAgfInfo->pINT_Buffer);
//
//		MMemSet(pAgfInfo, 0, sizeof(ArcGuidedFilter_C1_False));
//	}
//}

// box filter for UInt8 single channel
static MVoid boxBlurRowAdd_U8C1(MInt32* pSumLine, const MUInt8* addSrc, MInt32 width, MInt32 radius)
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

static MVoid boxBlurRowAddSub_U8C1(MInt32* pSumLine, const MUInt8* addSrc, const MUInt8* subSrc,
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

static MVoid boxBlurProcessRow_U8C1(MUInt8* pDst, MInt32* pBoxSumBuf, MInt32 width, MInt32 radius, MInt32 invWinArea)
{
	MInt32 x;
	MInt32 lbVal = 0;
	MInt32 lboxSize = radius * 2 + 1;

#ifdef __ARM_NEON__
	int32x4_t v_sum0, v_sum1;
	int32x4_t v_dst0, v_dst1;
	const int32x4_t v_invWinArea = vdupq_n_s32(invWinArea);
	int16x8_t resdata;
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < width - 7; x += 8)
	{
		v_sum1 = vld1q_s32(pBoxSumBuf + lboxSize);
		v_sum0 = vld1q_s32(pBoxSumBuf);
		v_dst0 = vsubq_s32(v_sum1, v_sum0);
		v_dst0 = vmulq_s32(v_dst0, v_invWinArea);
		v_dst0 = vrshrq_n_s32(v_dst0, 22);

		v_sum1 = vld1q_s32(pBoxSumBuf + lboxSize + 4);
		v_sum0 = vld1q_s32(pBoxSumBuf + 4);
		v_dst1 = vsubq_s32(v_sum1, v_sum0);
		v_dst1 = vmulq_s32(v_dst1, v_invWinArea);
		v_dst1 = vrshrq_n_s32(v_dst1, 22);

		resdata = vcombine_s16(vmovn_s32(v_dst0), vmovn_s32(v_dst1));
		vst1_u8(pDst, vqmovun_s16(resdata));

		pBoxSumBuf += 8;
		pDst += 8;
	}
#endif

	for (; x < width; x++)
	{
		lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
		lbVal = (lbVal * invWinArea + (1 << 21)) >> 22;
		(pDst++)[0] = ET_CAST_BYTE(lbVal);
		pBoxSumBuf++;
	}
}

static MVoid stripe_BoxBlur_U8C1(const MUInt8* pSrc, MInt32 srcPitch, MUInt8* pDst, MInt32 dstPitch, MInt32 height, MInt32 width,
	MInt32 lStartRow, MInt32 lEndRow, MInt32 radius, MInt32 *pBoxSumBuf)
{
	MInt32 srcStep = srcPitch, dstStep = dstPitch;

	MInt32 lPreLine = MIN(radius, lStartRow),
		lNexLine = MIN(radius, height - lEndRow);
	MInt32 lTop = lStartRow - lPreLine;
	MInt32 lBot = lEndRow + lNexLine;
	const MUInt8 *tmpaddSrc = pSrc + lTop * srcStep;
	const MUInt8 *tmpsubSrc = pSrc + lTop * srcStep;
	const MUInt8 *tmpSrc = pSrc + lStartRow * srcStep;
	MUInt8 *tmpDst = pDst + lStartRow * dstStep;
	MInt32 invWinArea = (1 << 22) / ((2 * radius + 1) * (2 * radius + 1));

	MInt32 line = lStartRow - radius;

	MMemSet(pBoxSumBuf, 0, (width + radius * 2 + 1 + 100) * sizeof(MInt32));

	for (; line < lTop; line++)
	{
		boxBlurRowAdd_U8C1(pBoxSumBuf, tmpaddSrc, width, radius);
	}
	for (; line < lStartRow + radius; line++)
	{
		boxBlurRowAdd_U8C1(pBoxSumBuf, tmpaddSrc, width, radius);
		tmpaddSrc += srcStep;
	}

	{
		boxBlurRowAdd_U8C1(pBoxSumBuf, tmpaddSrc, width, radius);
		boxBlurProcessRow_U8C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpaddSrc += srcStep;
		tmpSrc += srcStep;
		tmpDst += dstStep;
		line++;
	}

	for (; line < lBot; line++)
	{
		boxBlurRowAddSub_U8C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_U8C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
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
		boxBlurRowAddSub_U8C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_U8C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpsubSrc += srcStep;
		tmpSrc += srcStep;
		tmpDst += dstStep;
	}
}

struct BoxFilter_U8C1_MT
{
	MInt32			taskID;
	const MUInt8	*pSrc;
	MInt32			srcPitch;
	MUInt8			*pDst;
	MInt32			dstPitch;
	MInt32			width;
	MInt32			height;

	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32          radius;
	MInt32          *pBoxSumBuf;
};

MVoid thread_BoxFilter_U8C1(MVoid *info)
{
	BoxFilter_U8C1_MT *threadInfo = (BoxFilter_U8C1_MT *)info;
	stripe_BoxBlur_U8C1(threadInfo->pSrc, threadInfo->srcPitch, threadInfo->pDst, threadInfo->dstPitch,
		threadInfo->height, threadInfo->width, threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->radius,
		threadInfo->pBoxSumBuf);
}

MRESULT BoxFilter_U8C1(MHandle hMemMgr, MHandle parEngine, const MUInt8 *pSrc, MInt32 srcPitch,
					MUInt8 *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius)
{
	if (parEngine)
	{
		MInt32 taskNum = Get_Task_Num(width, height);

		MInt32 i = taskNum * (sizeof(BoxFilter_U8C1_MT) + sizeof(MInt32)
			+ (width + radius * 2 + 1 + 100) * sizeof(MInt32));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;

		BoxFilter_U8C1_MT *pParam = (BoxFilter_U8C1_MT *)buffer;
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
			iTask[i] = mcvAddTask(parEngine, thread_BoxFilter_U8C1, (MVoid *)&pParam[i]);
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

		stripe_BoxBlur_U8C1(pSrc, srcPitch, pDst, dstPitch, height, width, 0, height, radius, boxSumBuf);

		if (boxSumBuf) MMemFree(hMemMgr, boxSumBuf);
	}
	return MOK;
}

// box filter for UInt16 single channel
static MVoid boxBlurRowAdd_U16C1(MInt32* pSumLine, const MUInt16* addSrc, MInt32 width, MInt32 radius)
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

static MVoid boxBlurRowAddSub_U16C1(MInt32* pSumLine, const MUInt16* addSrc, const MUInt16* subSrc,
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

static MVoid boxBlurProcessRow_U16C1(MUInt16* pDst, MInt32* pBoxSumBuf, MInt32 width, MInt32 radius, MInt32 invWinArea)
{
	MInt32 x;
	MInt32 lbVal = 0;
	MInt32 lboxSize = radius * 2 + 1;

#ifdef __ARM_NEON__
	int32x4_t v_sum0, v_sum1;
	int32x4_t v_dst;
	int32x4_t v_invWinArea = vdupq_n_s32(invWinArea);
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < width - 3; x += 4)
	{
		v_sum1 = vld1q_s32(pBoxSumBuf + lboxSize);
		v_sum0 = vld1q_s32(pBoxSumBuf);

		v_dst = vsubq_s32(v_sum1, v_sum0);
		v_dst = vmulq_s32(v_dst, v_invWinArea);
		v_dst = vrshrq_n_s32(v_dst, 14);
		
		vst1_u16(pDst, vqmovun_s32(v_dst));

		pBoxSumBuf += 4;
		pDst += 4;
	}
#endif

	for (; x < width; x++)
	{
		lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
		lbVal = (lbVal * invWinArea + (1 << 13)) >> 14;
		(pDst++)[0] = lbVal;
		pBoxSumBuf++;
	}
}

static MVoid stripe_BoxBlur_U16C1(const MUInt16* pSrc, MInt32 srcPitch, MUInt16* pDst, MInt32 dstPitch, MInt32 height, MInt32 width,
								  MInt32 lStartRow, MInt32 lEndRow, MInt32 radius, MInt32 *pBoxSumBuf)
{
	MInt32 srcStep = srcPitch / sizeof(MUInt16), dstStep = dstPitch / sizeof(MUInt16);

	MInt32 lPreLine = MIN(radius, lStartRow),
		lNexLine = MIN(radius, height - lEndRow);
	MInt32 lTop = lStartRow - lPreLine;
	MInt32 lBot = lEndRow + lNexLine;
	const MUInt16 *tmpaddSrc = pSrc + lTop * srcStep;
	const MUInt16 *tmpsubSrc = pSrc + lTop * srcStep;
	const MUInt16 *tmpSrc = pSrc + lStartRow * srcStep;
	MUInt16 *tmpDst = pDst + lStartRow * dstStep;
	MInt32 invWinArea = (1 << 14) / ((2 * radius + 1) * (2 * radius + 1));

	MInt32 line = lStartRow - radius;

	MMemSet(pBoxSumBuf, 0, (width + radius * 2 + 1 + 100) * sizeof(MInt32));

	for (; line < lTop; line++)
	{
		boxBlurRowAdd_U16C1(pBoxSumBuf, tmpaddSrc, width, radius);
	}
	for (; line < lStartRow + radius; line++)
	{
		boxBlurRowAdd_U16C1(pBoxSumBuf, tmpaddSrc, width, radius);
		tmpaddSrc += srcStep;
	}

	{
		boxBlurRowAdd_U16C1(pBoxSumBuf, tmpaddSrc, width, radius);
		boxBlurProcessRow_U16C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpaddSrc += srcStep;
		tmpSrc += srcStep;
		tmpDst += dstStep;
		line++;
	}

	for (; line < lBot; line++)
	{
		boxBlurRowAddSub_U16C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_U16C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
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
		boxBlurRowAddSub_U16C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_U16C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpsubSrc += srcStep;
		tmpSrc += srcStep;
		tmpDst += dstStep;
	}
}

struct BoxFilter_U16C1_MT
{
	MInt32			taskID;
	const MUInt16	*pSrc;
	MInt32			srcPitch;
	MUInt16			*pDst;
	MInt32			dstPitch;
	MInt32			width;
	MInt32			height;

	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32          radius;
	MInt32          *pBoxSumBuf;
};

MVoid thread_BoxFilter_U16C1(MVoid *info)
{
	BoxFilter_U16C1_MT *threadInfo = (BoxFilter_U16C1_MT *)info;
	stripe_BoxBlur_U16C1(threadInfo->pSrc, threadInfo->srcPitch, threadInfo->pDst, threadInfo->dstPitch,
		threadInfo->height, threadInfo->width, threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->radius,
		threadInfo->pBoxSumBuf);
}

MRESULT BoxFilter_U16C1(MHandle hMemMgr, MHandle parEngine, const MUInt16 *pSrc, MInt32 srcPitch,
						MUInt16 *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius)
{
	if (parEngine)
	{
		MInt32 taskNum = Get_Task_Num(width, height);

		MInt32 i = taskNum * (sizeof(BoxFilter_U16C1_MT) + sizeof(MInt32)
			+ (width + radius * 2 + 1 + 100) * sizeof(MInt32));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;

		BoxFilter_U16C1_MT *pParam = (BoxFilter_U16C1_MT *)buffer;
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
			iTask[i] = mcvAddTask(parEngine, thread_BoxFilter_U16C1, (MVoid *)&pParam[i]);
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

		stripe_BoxBlur_U16C1(pSrc, srcPitch, pDst, dstPitch, height, width, 0, height, radius, boxSumBuf);

		if (boxSumBuf) MMemFree(hMemMgr, boxSumBuf);
	}
	return MOK;
}

// box filter for signed Int16 single channel
static MVoid boxBlurRowAdd_S16C1(MInt32* pSumLine, const MInt16* addSrc, MInt32 width, MInt32 radius)
{
	MInt32 i;
	MInt32 nValue;
	MInt32 nSum = 0;

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

static MVoid boxBlurRowAddSub_S16C1(MInt32* pSumLine, const MInt16* addSrc, const MInt16* subSrc,
									MInt32 width, MInt32 radius)
{

	MInt32 i;
	MInt32 nValue;
	MInt32 nSum = 0;

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

static MVoid boxBlurProcessRow_S16C1(MInt16* pDst, MInt32* pBoxSumBuf, MInt32 width, MInt32 radius, MInt32 invWinArea)
{
	MInt32 x;
	MInt32 lbVal = 0;
	MInt32 lboxSize = radius * 2 + 1;

#ifdef __ARM_NEON__
	int32x4_t v_sum0, v_sum1;
	int32x4_t v_dst;
	int32x4_t v_invWinArea = vdupq_n_s32(invWinArea);
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < width - 3; x += 4)
	{
		v_sum1 = vld1q_s32(pBoxSumBuf + lboxSize);
		v_sum0 = vld1q_s32(pBoxSumBuf);

		v_dst = vsubq_s32(v_sum1, v_sum0);
		v_dst = vmulq_s32(v_dst, v_invWinArea);
		v_dst = vrshrq_n_s32(v_dst, 14);

		vst1_s16(pDst, vqmovn_s32(v_dst));

		pBoxSumBuf += 4;
		pDst += 4;
	}
#endif

	for (; x < width; x++)
	{
		lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
		lbVal = (lbVal * invWinArea + (1 << 13)) >> 14;
		(pDst++)[0] = lbVal;
		pBoxSumBuf++;
	}
}

static MVoid stripe_BoxBlur_S16C1(const MInt16* pSrc, MInt32 srcPitch, MInt16* pDst, MInt32 dstPitch, MInt32 height, MInt32 width,
								  MInt32 lStartRow, MInt32 lEndRow, MInt32 radius, MInt32 *pBoxSumBuf)
{
	MInt32 srcStep = srcPitch / sizeof(MInt16), dstStep = dstPitch / sizeof(MInt16);

	MInt32 lPreLine = MIN(radius, lStartRow),
		lNexLine = MIN(radius, height - lEndRow);
	MInt32 lTop = lStartRow - lPreLine;
	MInt32 lBot = lEndRow + lNexLine;
	const MInt16 *tmpaddSrc = pSrc + lTop * srcStep;
	const MInt16 *tmpsubSrc = pSrc + lTop * srcStep;
	const MInt16 *tmpSrc = pSrc + lStartRow * srcStep;
	MInt16 *tmpDst = pDst + lStartRow * dstStep;
	MInt32 invWinArea = (1 << 14) / ((2 * radius + 1) * (2 * radius + 1));

	MInt32 line = lStartRow - radius;

	MMemSet(pBoxSumBuf, 0, (width + radius * 2 + 1 + 100) * sizeof(MInt32));

	for (; line < lTop; line++)
	{
		boxBlurRowAdd_S16C1(pBoxSumBuf, tmpaddSrc, width, radius);
	}
	for (; line < lStartRow + radius; line++)
	{
		boxBlurRowAdd_S16C1(pBoxSumBuf, tmpaddSrc, width, radius);
		tmpaddSrc += srcStep;
	}

	{
		boxBlurRowAdd_S16C1(pBoxSumBuf, tmpaddSrc, width, radius);
		boxBlurProcessRow_S16C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpaddSrc += srcStep;
		tmpSrc += srcStep;
		tmpDst += dstStep;
		line++;
	}

	for (; line < lBot; line++)
	{
		boxBlurRowAddSub_S16C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_S16C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
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
		boxBlurRowAddSub_S16C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_S16C1(tmpDst, pBoxSumBuf, width, radius, invWinArea);
		tmpsubSrc += srcStep;
		tmpSrc += srcStep;
		tmpDst += dstStep;
	}
}

struct BoxFilter_S16C1_MT
{
	MInt32			taskID;
	const MInt16	*pSrc;
	MInt32			srcPitch;
	MInt16			*pDst;
	MInt32			dstPitch;
	MInt32			width;
	MInt32			height;

	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32          radius;
	MInt32          *pBoxSumBuf;
};

MVoid thread_BoxFilter_S16C1(MVoid *info)
{
	BoxFilter_S16C1_MT *threadInfo = (BoxFilter_S16C1_MT *)info;
	stripe_BoxBlur_S16C1(threadInfo->pSrc, threadInfo->srcPitch, threadInfo->pDst, threadInfo->dstPitch,
		threadInfo->height, threadInfo->width, threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->radius,
		threadInfo->pBoxSumBuf);
}

MRESULT BoxFilter_S16C1(MHandle hMemMgr, MHandle parEngine, const MInt16 *pSrc, MInt32 srcPitch,
						MInt16 *pDst, MInt32 dstPitch, MInt32 height, MInt32 width, MInt32 radius)
{
	if (parEngine)
	{
		MInt32 taskNum = Get_Task_Num(width, height);

		MInt32 i = taskNum * (sizeof(BoxFilter_S16C1_MT) + sizeof(MInt32)
			+ (width + radius * 2 + 1 + 100) * sizeof(MInt32));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;

		BoxFilter_S16C1_MT *pParam = (BoxFilter_S16C1_MT *)buffer;
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
			iTask[i] = mcvAddTask(parEngine, thread_BoxFilter_S16C1, (MVoid *)&pParam[i]);
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

		stripe_BoxBlur_S16C1(pSrc, srcPitch, pDst, dstPitch, height, width, 0, height, radius, boxSumBuf);

		if (boxSumBuf) MMemFree(hMemMgr, boxSumBuf);
	}
	return MOK;
}

static MVoid stripe_CalcVarAddEpsilon_U16ToINT32C1(const MUInt8 *pMeanI, MInt32 lPitchMeanI, const MUInt16 *pCorrII, MInt32 lPitchCorrII, 
												   MInt32 *pCovII, MInt32 lPitchCovII, MInt32 height, MInt32 width, MInt32 epsilon,
												   MInt32 *lMinVal, MInt32 *lMaxVal, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 i, j, k;
	MInt32 lStepMeanI = lPitchMeanI / sizeof(MUInt8), lStepCorrII = lPitchCorrII / sizeof(MUInt16), lStepCovII = lPitchCovII / sizeof(MInt32);
	const MUInt8 *pMI = pMeanI + lStartRow * lStepMeanI;
	const MUInt16 *pCorr = pCorrII + lStartRow * lStepCorrII;
	MInt32 *pCov = pCovII + lStartRow * lStepCovII;
	MInt32 tmpMI = 0, tmpVal = 0;
	const MInt32 lIntMax = 0x7FFFFFFF, lIntMin = 0x80000000;
	lMinVal[0] = lIntMax, lMaxVal[0] = lIntMin;

#ifdef __ARM_NEON__
	uint8x8_t meanI_8x8;
	uint16x8_t corrII_16x8, tmpdata_16x8;
	const uint32x4_t epsilon_32x4 = vdupq_n_s32(epsilon);
	int32x4_t resdata0_32x4, resdata1_32x4;
	int32x4_t mindata_32x4 = vdupq_n_s32(lIntMax), maxdata_32x4 = vdupq_n_s32(lIntMin);
#endif

	for (i = lStartRow; i < lEndRow - 1; ++i)
	{
		j = 0;
#ifdef __ARM_NEON__
		for (; j < width - 7; j += 8)
		{
			meanI_8x8 = vld1_u8(pMI + j);
			corrII_16x8 = vld1q_u16(pCorr + j);

			tmpdata_16x8 = vmull_u8(meanI_8x8, meanI_8x8);
			resdata0_32x4 = vreinterpretq_s32_u32(vaddw_u16(epsilon_32x4, vget_low_u16(corrII_16x8)));
			resdata1_32x4 = vreinterpretq_s32_u32(vaddw_u16(epsilon_32x4, vget_high_u16(corrII_16x8)));

			resdata0_32x4 = vsubq_s32(resdata0_32x4, vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(tmpdata_16x8))));
			resdata1_32x4 = vsubq_s32(resdata1_32x4, vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(tmpdata_16x8))));
			vst1q_s32(pCov + j, resdata0_32x4);
			vst1q_s32(pCov + j + 4, resdata1_32x4);

			mindata_32x4 = vminq_s32(mindata_32x4, resdata0_32x4);
			mindata_32x4 = vminq_s32(mindata_32x4, resdata1_32x4);

			maxdata_32x4 = vmaxq_s32(maxdata_32x4, resdata0_32x4);
			maxdata_32x4 = vmaxq_s32(maxdata_32x4, resdata1_32x4);
		}

// 		for (k = 0; k < 4; ++k)
// 		{
// 			lMinVal[0] = MIN(vgetq_lane_s32(mindata_32x4, k), lMinVal[0]);
// 			lMaxVal[0] = MAX(vgetq_lane_s32(maxdata_32x4, k), lMaxVal[0]);
// 		}
#endif

		for (; j < width; ++j)
		{
			tmpMI = pMI[j];
			tmpVal = (MInt32)pCorr[j] - tmpMI * tmpMI + epsilon;

			lMinVal[0] = MIN(tmpVal, lMinVal[0]);
			lMaxVal[0] = MAX(tmpVal, lMaxVal[0]);

			/*if (lMinVal[0] == 3035)
			{
				j = j;
			}*/

			pCov[j] = tmpVal;
		}

		pMI += lStepMeanI;
		pCorr += lStepCorrII;
		pCov += lStepCovII;
	}

	j = 0;
#ifdef __ARM_NEON__
	for (; j < width - 7; j += 8)
	{
		meanI_8x8 = vld1_u8(pMI + j);
		corrII_16x8 = vld1q_u16(pCorr + j);

		tmpdata_16x8 = vmull_u8(meanI_8x8, meanI_8x8);
		resdata0_32x4 = vreinterpretq_s32_u32(vaddw_u16(epsilon_32x4, vget_low_u16(corrII_16x8)));
		resdata1_32x4 = vreinterpretq_s32_u32(vaddw_u16(epsilon_32x4, vget_high_u16(corrII_16x8)));

		resdata0_32x4 = vsubq_s32(resdata0_32x4, vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(tmpdata_16x8))));
		resdata1_32x4 = vsubq_s32(resdata1_32x4, vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(tmpdata_16x8))));
		vst1q_s32(pCov + j, resdata0_32x4);
		vst1q_s32(pCov + j + 4, resdata1_32x4);

		mindata_32x4 = vminq_s32(mindata_32x4, resdata0_32x4);
		mindata_32x4 = vminq_s32(mindata_32x4, resdata1_32x4);

		maxdata_32x4 = vmaxq_s32(maxdata_32x4, resdata0_32x4);
		maxdata_32x4 = vmaxq_s32(maxdata_32x4, resdata1_32x4);
	}
	{
		MInt32 lMinData[4];
		MInt32 lMaxData[4];
		vst1q_s32(lMinData, mindata_32x4);
		vst1q_s32(lMaxData, maxdata_32x4);
		for (k = 0; k < 4; ++k)
		{
			lMinVal[0] = MIN(lMinData[k], lMinVal[0]);
			lMaxVal[0] = MAX(lMaxData[k], lMaxVal[0]);
		}
	}
#endif

	for (; j < width; ++j)
	{
		tmpMI = pMI[j];
		tmpVal = (MInt32)pCorr[j] - tmpMI * tmpMI + epsilon;

		lMinVal[0] = MIN(tmpVal, lMinVal[0]);
		lMaxVal[0] = MAX(tmpVal, lMaxVal[0]);

		/*if (lMinVal[0] == 3035)
		{
		j = j;
		}*/

		pCov[j] = tmpVal;
	}
}

struct CalcVarAddEpsilon_U16ToINT32C1_MT
{
	MInt32 taskID;
	const MUInt8 *pMeanI;
	MInt32 lPitchMeanI;
	const MUInt16 *pCorrII;
	MInt32 lPitchCorrII;
	MInt32 *pCovII;
	MInt32 lPitchCovII;
	MInt32 height;
	MInt32 width;
	MInt32 epsilon;
	MInt32 *lMinVal;
	MInt32 *lMaxVal;
	MInt32 lStartRow;
	MInt32 lEndRow;
};

MVoid thread_CalcVarAddEpsilon_U16ToINT32C1(MVoid *info)
{
	CalcVarAddEpsilon_U16ToINT32C1_MT *threadInfo = (CalcVarAddEpsilon_U16ToINT32C1_MT *)info;
	stripe_CalcVarAddEpsilon_U16ToINT32C1(threadInfo->pMeanI, threadInfo->lPitchMeanI, threadInfo->pCorrII, threadInfo->lPitchCorrII, threadInfo->pCovII,
										  threadInfo->lPitchCovII, threadInfo->height, threadInfo->width, threadInfo->epsilon, threadInfo->lMinVal, 
										  threadInfo->lMaxVal, threadInfo->lStartRow, threadInfo->lEndRow);
}

MRESULT CalcVarAddEpsilon_U16ToINT32C1(MHandle hMemMgr, MHandle parEngine, const MUInt8 *pMeanI, MInt32 lPitchMeanI, const MUInt16 *pCorrII, 
									   MInt32 lPitchCorrII, MInt32 *pCovII, MInt32 lPitchCovII, MInt32 height, MInt32 width, MInt32 epsilon,
									   MInt32 *lMinVal, MInt32 *lMaxVal)
{
	if (parEngine)
	{
		MInt32 taskNum = Get_Task_Num(width, height);

		MInt32 i = taskNum * (sizeof(CalcVarAddEpsilon_U16ToINT32C1_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;
		CalcVarAddEpsilon_U16ToINT32C1_MT *pParam = (CalcVarAddEpsilon_U16ToINT32C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);
		MInt32 lMin[16] = { 0 }, lMax[16] = { 0 };

		MInt32 rowStep = height / taskNum;
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pMeanI = pMeanI;
			pParam[i].lPitchMeanI = lPitchMeanI;
			pParam[i].pCorrII = pCorrII;
			pParam[i].lPitchCorrII = lPitchCorrII;
			pParam[i].pCovII = pCovII;
			pParam[i].lPitchCovII = lPitchCovII;
			pParam[i].height = height;
			pParam[i].width = width;
			pParam[i].epsilon = epsilon;
			pParam[i].lMinVal = lMin + i;
			pParam[i].lMaxVal = lMax + i;
			pParam[i].lStartRow = i * rowStep;
			pParam[i].lEndRow = (i + 1) * rowStep;
		}
		pParam[taskNum - 1].lEndRow = height;

		for (i = 0; i < taskNum; ++i)
		{
			iTask[i] = mcvAddTask(parEngine, thread_CalcVarAddEpsilon_U16ToINT32C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		lMinVal[0] = lMin[0];
		lMaxVal[0] = lMax[0];
		for (i = 1; i < taskNum; ++i)
		{
			lMinVal[0] = MIN(lMinVal[0], lMin[i]);
			lMaxVal[0] = MAX(lMaxVal[0], lMax[i]);
		}

		if (buffer) MMemFree(hMemMgr, buffer);
	}
	else
	{
		stripe_CalcVarAddEpsilon_U16ToINT32C1(pMeanI, lPitchMeanI, pCorrII, lPitchCorrII, pCovII, lPitchCovII, 
											  height, width, epsilon, lMinVal, lMaxVal, 0, height);
	}

	return MOK;
}

MVoid stripe_CalcCovarInv_Int32C1(MInt32 *pInvNum, MInt32 lMinVal, MInt32 *pCovar, MInt32 lPitchCov, MInt32 *pCovarInv, 
								  MInt32 lPitchInvCov, MInt32 lWidth, MInt32 lHeight, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 x = 0, y = 0;
	MInt32 lStepCov = lPitchCov / sizeof(MInt32), lStepInvCov = lPitchInvCov / sizeof(MInt32);
	MInt32 *pCovRow = pCovar + lStartRow * lStepCov;
	MInt32 *pInvCovRow = pCovarInv + lStartRow * lStepInvCov;

	for (y = lStartRow; y < lEndRow; ++y)
	{
		for (x = 0; x < lWidth; ++x)
		{
			pInvCovRow[x] = pInvNum[pCovRow[x] - lMinVal];
		}

		pCovRow += lStepCov;
		pInvCovRow += lStepInvCov;
	}

	return;
}

struct CalcCovarInv_Int32C1_MT{
	MInt32 *pInvNum;
	MInt32 lMinVal;
	MInt32 *pCovar;
	MInt32 lPitchCov;
	MInt32 *pCovarInv;
	MInt32 lPitchInvCov;
	MInt32 lWidth;
	MInt32 lHeight;
	MInt32 lStartRow;
	MInt32 lEndRow;
};

MVoid thread_CalcCovarInv_Int32C1(MVoid *para)
{
	if (MNull != para)
	{
		CalcCovarInv_Int32C1_MT *pPara = (CalcCovarInv_Int32C1_MT *)para;
		stripe_CalcCovarInv_Int32C1(pPara->pInvNum, pPara->lMinVal, pPara->pCovar, pPara->lPitchCov, pPara->pCovarInv, pPara->lPitchInvCov,
									pPara->lWidth, pPara->lHeight, pPara->lStartRow, pPara->lEndRow);
	}
	return;
}

MRESULT CalcCovarInv_Int32C1(MHandle hMemMgr, MHandle parEngine, MInt32 *pCovar, MInt32 lPitchCov, MInt32 lMinVal, 
							MInt32 lMaxVal, MInt32 *pCovarInv, MInt32 lPitchInvCov, MInt32 lWidth, MInt32 lHeight)
{
	MRESULT lret = MOK;
	MInt32 *pInvNum = (MInt32 *)MMemAlloc(hMemMgr, (lMaxVal - lMinVal + 1) * sizeof(MInt32));
	if (!pInvNum)
		return MERR_NO_MEMORY;

	for (MInt32 k = lMinVal; k <= lMaxVal; ++k)
	{
		pInvNum[k - lMinVal] = ((1 << 30) + (k >> 1)) / k;
	}

	if (parEngine)
	{
		MInt32 lTaskNum = Get_Task_Num(lWidth, lHeight);
		MInt32 k = 0;
		CalcCovarInv_Int32C1_MT para[16] = { 0 };
		MInt32 lTaskId[16] = { 0 };
		MInt32 lBlockH = lHeight / lTaskNum;

		for (k = 0; k < lTaskNum; ++k)
		{
			para[k].pInvNum = pInvNum;
			para[k].lMinVal = lMinVal;
			para[k].pCovar = pCovar;
			para[k].lPitchCov = lPitchCov;
			para[k].pCovarInv = pCovarInv;
			para[k].lPitchInvCov = lPitchInvCov;
			para[k].lWidth = lWidth;
			para[k].lHeight = lHeight;
			para[k].lStartRow = k * lBlockH;
			para[k].lEndRow = (k + 1) * lBlockH;
		}
		para[lTaskNum - 1].lEndRow = lHeight;

		for (k = 0; k < lTaskNum; ++k)
		{
			lTaskId[k] = mcvAddTask(parEngine, thread_CalcCovarInv_Int32C1, (MVoid *)(para + k));
		}

		for (k = 0; k < lTaskNum; ++k)
		{
			lret = mcvWaitTask(parEngine, lTaskId[k]);
		}

	}
	else
	{
		stripe_CalcCovarInv_Int32C1(pInvNum, lMinVal, pCovar, lPitchCov, pCovarInv, lPitchInvCov, lWidth, lHeight, 0, lHeight);
	}

	if (pInvNum)
	{
		MMemFree(hMemMgr, pInvNum);
		pInvNum = MNull;
	}
	return MOK;
}

static MVoid stripe_CalcAlpha_U16(MByte *pGuidMean, MInt32 lGuidMeanStep, MByte *pSrcMean, MInt32 lSrcStep,
								  MInt32 *pInvCovar, MInt32 lInvCovarStep, MUInt16 *pCorrGuidSrcMean, 
								  MInt32 lCorrStep, MInt16 *pAlpha, MInt32 lAlphaStep, MInt32 lWidth, 
								  MInt32 lHeight, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 x = 0, y = 0;
	MByte *pGuidRow = pGuidMean + lStartRow * lGuidMeanStep;
	MByte *pSrcRow = pSrcMean + lStartRow * lSrcStep;
	MInt32 *pInvCovRow = pInvCovar + lStartRow * lInvCovarStep;
	MUInt16 *pCorrRow = pCorrGuidSrcMean + lStartRow * lCorrStep;
	MInt16 *pAlphaRow = pAlpha + lStartRow * lAlphaStep;
	MInt32 lCovGuiSrcVal = 0;
	const MInt32 lShortMax = (1 << 15) - 1;
	const MInt32 lShortMin = -lShortMax;

	for (y = lStartRow; y < lEndRow; ++y)
	{
		for (x = 0; x < lWidth; ++x)
		{
			if (y == 218 && x == 89)
			{
				x = x;
			}
			lCovGuiSrcVal = (MInt32)pCorrRow[x] - (MInt32)pGuidRow[x] * pSrcRow[x];
			lCovGuiSrcVal *= pInvCovRow[x];
			lCovGuiSrcVal = (lCovGuiSrcVal + (1 << 14)) >> 15;

			lCovGuiSrcVal = lCovGuiSrcVal > lShortMax ? lShortMax : lCovGuiSrcVal; //Constrained to 1
			lCovGuiSrcVal = lCovGuiSrcVal < lShortMin ? lShortMin : lCovGuiSrcVal; //Constrained to 1

			pAlphaRow[x] = (MInt16)lCovGuiSrcVal;
		}

		pGuidRow += lGuidMeanStep;
		pSrcRow += lSrcStep;
		pInvCovRow += lInvCovarStep;
		pCorrRow += lCorrStep;
		pAlphaRow += lAlphaStep;
	}

	return;
}

struct CalcAlpha_U16_MT{
	MByte *pGuidMean;
	MInt32 lGuidMeanStep;
	MByte *pSrcMean;
	MInt32 lSrcStep;
	MInt32 *pInvCovar;
	MInt32 lInvCovarStep;
	MUInt16 *pCorrGuidSrcMean;
	MInt32 lCorrStep;
	MInt16 *pAlpha;
	MInt32 lAlphaStep;
	MInt32 lWidth;
	MInt32 lHeight;
	MInt32 lStartRow;
	MInt32 lEndRow;
};

MVoid thread_CalcAlpha_U16(MVoid *para)
{
	if(MNull != para)
	{
		CalcAlpha_U16_MT *pPara = (CalcAlpha_U16_MT *)para;

		stripe_CalcAlpha_U16(pPara->pGuidMean, pPara->lGuidMeanStep, pPara->pSrcMean, pPara->lSrcStep,
							 pPara->pInvCovar, pPara->lInvCovarStep, pPara->pCorrGuidSrcMean, pPara->lCorrStep,
							 pPara->pAlpha, pPara->lAlphaStep, pPara->lWidth, pPara->lHeight, pPara->lStartRow, pPara->lEndRow);
	}
	return;
}

MRESULT CalcAlpha_S16(MHandle hMemMgr, MHandle parEngine, MByte *pGuidMean, MInt32 lGuidMeanPitch, MByte *pSrcMean, MInt32 lSrcPitch, 
					  MInt32 *pInvCovar, MInt32 lInvCovarPitch, MUInt16 *pCorrGuidSrcMean, MInt32 lCorrPitch, MInt16 *pAlpha,
					  MInt32 lAlphaPitch, MInt32 lWidth, MInt32 lHeight)
{
	MRESULT lret = MOK;
	MInt32 lGuidMeanStep = lGuidMeanPitch / sizeof(MByte);
	MInt32 lSrcStep = lSrcPitch / sizeof(MByte);
	MInt32 lInvCovarStep = lInvCovarPitch / sizeof(MInt32);
	MInt32 lCorrStep = lCorrPitch / sizeof(MUInt16);
	MInt32 lAlphaStep = lAlphaPitch / sizeof(MInt16);

	if (parEngine)
	{
		MInt32 lTaskNum = Get_Task_Num(lWidth, lHeight);
		CalcAlpha_U16_MT para[16] = { 0 };
		MInt32 lTaskId[16] = { 0 };
		MInt32 lBlockH = lHeight / lTaskNum;
		MInt32 k = 0;

		for (k = 0; k < lTaskNum; ++k)
		{
			para[k].pGuidMean = pGuidMean;
			para[k].lGuidMeanStep = lGuidMeanStep;
			para[k].pSrcMean = pSrcMean;
			para[k].lSrcStep = lSrcStep;
			para[k].pInvCovar = pInvCovar;
			para[k].lInvCovarStep = lInvCovarStep;
			para[k].pCorrGuidSrcMean = pCorrGuidSrcMean;
			para[k].lCorrStep = lCorrStep;
			para[k].pAlpha = pAlpha;
			para[k].lAlphaStep = lAlphaStep;
			para[k].lWidth = lWidth;
			para[k].lHeight = lHeight;
			para[k].lStartRow = k * lBlockH;
			para[k].lEndRow = (k + 1) * lBlockH;
		}
		para[lTaskNum - 1].lEndRow = lHeight;

		for (k = 0; k < lTaskNum; ++k)
		{
			lTaskId[k] = mcvAddTask(parEngine, thread_CalcAlpha_U16, (MVoid *)(para + k));
		}

		for (k = 0; k < lTaskNum; ++k)
		{
			lret = mcvWaitTask(parEngine, lTaskId[k]);
			if (MOK != lret)
			{
				lret = MERR_BAD_STATE;
				return lret;
			}
		}
	}
	else
	{
		stripe_CalcAlpha_U16(pGuidMean, lGuidMeanStep, pSrcMean, lSrcStep, pInvCovar, lInvCovarStep, 
					 pCorrGuidSrcMean, lCorrStep, pAlpha, lAlphaStep, lWidth, lHeight, 0, lHeight);
	}

	return lret;
}

static MVoid stripe_CalcBeta_S16(MInt16 *pAlpha, MInt32 lAlphaStep, MByte *pGuidMean, MInt32 lGuidStep, MByte *pSrcMean,
	MInt32 lSrcStep, MInt16 *pBeta, MInt32 lBetaStep, MInt32 lWidth, MInt32 lHeight, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 x = 0, y = 0;
	MInt16 *pAlphaRow = pAlpha + lStartRow * lAlphaStep;
	MByte *pGuidRow = pGuidMean + lStartRow * lGuidStep;
	MByte *pSrcRow = pSrcMean + lStartRow * lSrcStep;
	MInt16 *pBetaRow = pBeta + lStartRow * lBetaStep;
	MInt32 lVal = 0;

	for (y = lStartRow; y < lEndRow; ++y)
	{
		for (x = 0; x < lWidth; ++x)
		{
			lVal = ((MInt32)pSrcRow[x] << 15) - (MInt32)pAlphaRow[x] * pGuidRow[x];
			/*if (lVal < 0)
				x = x;*/
			lVal = (lVal + (1 << 14)) >> 15;

			/*if (lVal < 0 || lVal > 255)
				x = x;*/

			pBetaRow[x] = (MInt16)lVal;
		}

		pAlphaRow += lAlphaStep;
		pGuidRow += lGuidStep;
		pSrcRow += lSrcStep;
		pBetaRow += lBetaStep;
	}
}

struct CalcBeta_S16_MT{
	MInt16 *pAlpha;
	MInt32 lAlphaStep;
	MByte *pGuidMean;
	MInt32 lGuidStep;
	MByte *pSrcMean;
	MInt32 lSrcStep;
	MInt16 *pBeta;
	MInt32 lBetaStep;
	MInt32 lWidth;
	MInt32 lHeight;
	MInt32 lStartRow;
	MInt32 lEndRow;
};

MVoid thread_CalcBeta_S16(MVoid *para)
{
	if (MNull != para)
	{
		CalcBeta_S16_MT *pPara = (CalcBeta_S16_MT *)para;
		stripe_CalcBeta_S16(pPara->pAlpha, pPara->lAlphaStep, pPara->pGuidMean, pPara->lGuidStep, pPara->pSrcMean, pPara->lSrcStep, 
							pPara->pBeta, pPara->lBetaStep, pPara->lWidth, pPara->lHeight, pPara->lStartRow, pPara->lEndRow);
	}
	return;
}

MRESULT CalcBeta_S16(MHandle hMemMgr, MHandle parEngine, MInt16 *pAlpha, MInt32 lAlphaPitch, MByte *pGuidMean,
					 MInt32 lGuidPitch, MByte *pSrcMean, MInt32 lSrcPitch, MInt16 *pBeta, MInt32 lBetaPitch,
					 MInt32 lWidth, MInt32 lHeight)
{
	MRESULT lret = MOK;
	MInt32 lAlphaStep = lAlphaPitch / sizeof(MInt16);
	MInt32 lGuidStep = lGuidPitch / sizeof(MByte);
	MInt32 lSrcStep = lSrcPitch / sizeof(MByte);
	MInt32 lBetaStep = lBetaPitch / sizeof(MInt16);

	if (parEngine)
	{
		MInt32 lTaskNum = Get_Task_Num(lWidth, lHeight);
		CalcBeta_S16_MT para[16] = { 0 };
		MInt32 lTaskId[16] = { 0 };
		MInt32 lBlockH = lHeight / lTaskNum;
		MInt32 k = 0;

		for (k = 0; k < lTaskNum; ++k)
		{
			para[k].pAlpha = pAlpha;
			para[k].lAlphaStep = lAlphaStep;
			para[k].pGuidMean = pGuidMean;
			para[k].lGuidStep = lGuidStep;
			para[k].pSrcMean = pSrcMean;
			para[k].lSrcStep = lSrcStep;
			para[k].pBeta = pBeta;
			para[k].lBetaStep = lBetaStep;
			para[k].lWidth = lWidth;
			para[k].lHeight = lHeight;
			para[k].lStartRow = k * lBlockH;
			para[k].lEndRow = (k + 1) * lBlockH;
		}
		para[lTaskNum - 1].lEndRow = lHeight;

		for (k = 0; k < lTaskNum; ++k)
		{
			lTaskId[k] = mcvAddTask(parEngine, thread_CalcBeta_S16, (MVoid *)(para + k));
		}

		for (k = 0; k < lTaskNum; ++k)
		{
			lret = mcvWaitTask(parEngine, lTaskId[k]);
			if (MOK != lret)
			{
				lret = MERR_BAD_STATE;
				return lret;
			}
		}
	}
	else
	{
		stripe_CalcBeta_S16(pAlpha, lAlphaStep, pGuidMean, lGuidStep, pSrcMean, lSrcStep,
							pBeta, lBetaStep, lWidth, lHeight, 0, lHeight);
	}

	return lret;
}

#ifndef SWAP_BUFFER
#define SWAP_BUFFER(a, b, t)  ((t) = (a), (a) = (b), (b) = (t))
#endif

#ifndef FLT_TO_FIX
#define FLT_TO_FIX(x,n)				(MInt32)((x)*(1<<(n))+0.5f)
#endif

#ifndef DESCALE_S
#define DESCALE_S(x,n)				(((x) + (1 << ((n) - 1))) >> (n))
#endif

// for bilinear interpolation
#ifndef BL_WARP_SHIFT
#define BL_WARP_SHIFT				7
#endif

#ifndef BL_WARP_MUL_ONE_S
#define BL_WARP_MUL_ONE_S(x)		((x) << BL_WARP_SHIFT)
#endif

#ifndef BL_WARP_DESCALE_S
#define BL_WARP_DESCALE_S(x)		DESCALE_S((x), BL_WARP_SHIFT)
#endif

#ifndef BL_WARP_DESCALE2_S
#define BL_WARP_DESCALE2_S(x)		DESCALE_S((x), (BL_WARP_SHIFT<<1))
#endif

MRESULT ImgBilinearResizeAllocMem_S16C1(MHandle hMemMgr, MInt32 srcWidth, MInt32 srcHeight,
							MInt32 dstWidth, MInt32 dstHeight, BilinearResize_Param *pParam)
{
	if (!pParam)  return MERR_INVALID_PARAM;

	MFloat scale_x, scale_y, fx, fy;
	MInt32 buf_size, xmax = dstWidth, sx, sy, dx, dy;
	MDWord *xofs, *yofs;
	MInt32 lTaskNum = Get_Task_Num(dstWidth, dstHeight);

	scale_x = (MFloat)srcWidth / dstWidth;
	scale_y = (MFloat)srcHeight / dstHeight;
	buf_size = dstWidth * 2 * lTaskNum * sizeof(MInt32)
		+ sizeof(MInt32)
		+ (dstWidth + dstHeight)*sizeof(MDWord);

	if (pParam->pBuf)
	{
		MMemFree(hMemMgr, pParam->pBuf);
		pParam->pBuf = MNull;
	}
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

struct ImgBilinearResize_S16C1_MT
{
	MInt32 taskID;
	const MInt16 *pSrc;
	MInt32 srcWidth;
	MInt32 srcHeight;
	MInt32 srcPitch;

	MInt16 *pDst;
	MInt32 dstWidth;
	MInt32 dstHeight;
	MInt32 dstPitch;

	MInt32 lStartRow;
	MInt32 lEndRow;

	const BilinearResize_Param *pBLParam;
};

static MVoid stripe_ImgBilinearResize_S16C1(const MInt16 *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
											MInt16 *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch,
											MInt32 lStartRow, MInt32 lEndRow, MInt32 taskID, const BilinearResize_Param *pBLParam)
{
	MInt32 srcStep = srcPitch / sizeof(MInt16), dstStep = dstPitch / sizeof(MInt16);

	// get info and buffer for bilinear resize
	MInt32 xMaxCoor = *((MInt32 *)pBLParam->pBuf);
	MDWord *xofs = (MDWord *)pBLParam->pBuf + 1;
	MDWord *yofs = xofs + dstWidth;
	MInt32 *buf0 = (MInt32 *)(yofs + dstHeight) + taskID * dstWidth * 2;
	MInt32 *buf1 = buf0 + dstWidth;

	// do bilinear resize
	MInt32 prev_sy0 = -1, prev_sy1 = -1;
	MInt32 k, dx, dy;

	MInt16 *pD = pDst + lStartRow * dstStep;
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
			MInt32 *swap_t;
			// swap buffer
			SWAP_BUFFER(buf0, buf1, swap_t);
			k = 1;
		}
		else
			k = 0;

		for (; k < 2; k++)
		{
			MInt32* tmpBuf = k == 0 ? buf0 : buf1;
			MInt32 sy = k == 0 ? sy0 : sy1;
			if (k == 1 && sy1 == sy0)
			{
				MMemCpy(buf1, buf0, dstWidth * sizeof(buf0[0]));
				continue;
			}

			const MInt16 *pS = pSrc + sy * srcStep;
			for (dx = 0; dx < xMaxCoor; dx++)
			{
				MInt32 sx = xofs[dx];
				MInt32 fx = sx >> 16;
				sx = sx & 0xffff;

				MInt32 sVal0 = pS[sx];
				MInt32 sVal1 = pS[sx + 1];
				MInt32 val = BL_WARP_MUL_ONE_S(sVal0);
				val += fx * (sVal1 - sVal0);

				tmpBuf[dx] = val;
			}

			for (; dx < dstWidth; dx++)
			{
				MInt32 sx = xofs[dx];
				sx = sx & 0xffff;

				tmpBuf[dx] = BL_WARP_MUL_ONE_S(pS[sx]);
			}
		}

		prev_sy0 = sy0;
		prev_sy1 = sy1;

		if (sy0 == sy1)
		{
			dx = 0;
#ifdef __ARM_NEON__0
			for (; dx < dstWidth - 7; dx += 8)
			{
				int16x8_t v_buf;
				v_buf = vld1q_s16(buf0 + dx);
				vst1_u8(pD + dx, vqrshrun_n_s16(v_buf, 7));
			}
#endif
			for (; dx < dstWidth; ++dx)
				pD[dx] = (MInt16)BL_WARP_DESCALE_S(buf0[dx]);
		}
		else
		{
			dx = 0;
#ifdef __ARM_NEON__0
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
				MInt32 val = BL_WARP_MUL_ONE_S(sx0);
				val += fy * (sx - sx0);
				pD[dx] = (MInt16)BL_WARP_DESCALE2_S(val);
			}
		}
	}
}

MVoid thread_ImgBilinearResize_S16C1(MVoid *info)
{
	ImgBilinearResize_S16C1_MT *threadInfo = (ImgBilinearResize_S16C1_MT *)info;
	stripe_ImgBilinearResize_S16C1(threadInfo->pSrc, threadInfo->srcWidth, threadInfo->srcHeight, threadInfo->srcPitch,
		threadInfo->pDst, threadInfo->dstWidth, threadInfo->dstHeight, threadInfo->dstPitch,
		threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->taskID, threadInfo->pBLParam);
}

MRESULT ImgBilinearResize_S16C1(MHandle hMemMgr, MHandle parEngine, const MInt16 *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
								MInt16 *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch, const BilinearResize_Param *pBLParam)
{
	// same size, only copy
	if (srcWidth == dstWidth && srcHeight == dstHeight)
	{
		//MMemCpy(pDst, pSrc, dstHeight * dstPitch);
		const MInt16 *pSrcRow = pSrc;
		MInt16 *pDstRow = pDst;
		MInt32 lSrcStep = srcPitch / sizeof(MInt16);
		MInt32 lDstStep = dstPitch / sizeof(MInt16);

		for (MInt32 y = 0; y < dstHeight; ++y)
		{
			MMemCpy(pDstRow, pSrcRow, dstWidth * sizeof(MInt16));
			pDstRow += lDstStep;
			pSrcRow += lSrcStep;
		}

		return MOK;
	}

	if (!pBLParam) return MERR_INVALID_PARAM;

	if (parEngine)
	{
		MInt32 taskNum = Get_Task_Num(dstWidth, dstHeight);

		MInt32 i = taskNum * (sizeof(ImgBilinearResize_S16C1_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;
		ImgBilinearResize_S16C1_MT *pParam = (ImgBilinearResize_S16C1_MT *)buffer;
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
			iTask[i] = mcvAddTask(parEngine, thread_ImgBilinearResize_S16C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(hMemMgr, buffer);
	}
	else
	{
		stripe_ImgBilinearResize_S16C1(pSrc, srcWidth, srcHeight, srcPitch,
			pDst, dstWidth, dstHeight, dstPitch, 0, dstHeight, 0, pBLParam);
	}

	return MOK;
}

struct ImgBilinearResize_S32C1_MT
{
	MInt32 taskID;
	const MInt32 *pSrc;
	MInt32 srcWidth;
	MInt32 srcHeight;
	MInt32 srcPitch;

	MInt32 *pDst;
	MInt32 dstWidth;
	MInt32 dstHeight;
	MInt32 dstPitch;

	MInt32 lStartRow;
	MInt32 lEndRow;

	const BilinearResize_Param *pBLParam;
};

static MVoid stripe_ImgBilinearResize_S32C1(const MInt32 *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
											MInt32 *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch,
											MInt32 lStartRow, MInt32 lEndRow, MInt32 taskID, const BilinearResize_Param *pBLParam)
{
	MInt32 srcStep = srcPitch / sizeof(MInt32), dstStep = dstPitch / sizeof(MInt32);

	// get info and buffer for bilinear resize
	MInt32 xMaxCoor = *((MInt32 *)pBLParam->pBuf);
	MDWord *xofs = (MDWord *)pBLParam->pBuf + 1;
	MDWord *yofs = xofs + dstWidth;
	MInt32 *buf0 = (MInt32 *)(yofs + dstHeight) + taskID * dstWidth * 2;
	MInt32 *buf1 = buf0 + dstWidth;

	// do bilinear resize
	MInt32 prev_sy0 = -1, prev_sy1 = -1;
	MInt32 k, dx, dy;

	MInt32 *pD = pDst + lStartRow * dstStep;
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
			MInt32 *swap_t;
			// swap buffer
			SWAP_BUFFER(buf0, buf1, swap_t);
			k = 1;
		}
		else
			k = 0;

		for (; k < 2; k++)
		{
			MInt32* tmpBuf = k == 0 ? buf0 : buf1;
			MInt32 sy = k == 0 ? sy0 : sy1;
			if (k == 1 && sy1 == sy0)
			{
				MMemCpy(buf1, buf0, dstWidth * sizeof(buf0[0]));
				continue;
			}

			const MInt32 *pS = pSrc + sy * srcStep;
			for (dx = 0; dx < xMaxCoor; dx++)
			{
				MInt32 sx = xofs[dx];
				MInt32 fx = sx >> 16;
				sx = sx & 0xffff;

				MInt32 sVal0 = pS[sx];
				MInt32 sVal1 = pS[sx + 1];
				MInt32 val = BL_WARP_MUL_ONE_S(sVal0);
				val += fx * (sVal1 - sVal0);

				tmpBuf[dx] = BL_WARP_DESCALE_S(val);
			}

			for (; dx < dstWidth; dx++)
			{
				MInt32 sx = xofs[dx];
				sx = sx & 0xffff;

				tmpBuf[dx] = pS[sx];
			}
		}

		prev_sy0 = sy0;
		prev_sy1 = sy1;

		if (sy0 == sy1)
		{
			dx = 0;
#ifdef __ARM_NEON__
			int32x4_t v_buf;
			for (; dx < dstWidth - 3; dx += 4)
			{
				v_buf = vld1q_s32(buf0 + dx);
				vst1q_s32(pD + dx, v_buf);
			}
#endif
			for (; dx < dstWidth; ++dx)
				pD[dx] = buf0[dx];
		}
		else
		{
			dx = 0;
#ifdef __ARM_NEON__
			int32x4_t v_fy = vdupq_n_s32(fy);
			int32x4_t v_sx0, v_sx1, v_sub;
			int32x4_t v_dst;
			for (; dx < dstWidth - 3; dx += 4)
			{
				v_sx0 = vld1q_s32(buf0 + dx);
				v_sx1 = vld1q_s32(buf1 + dx);

				v_sub = vsubq_s32(v_sx1, v_sx0);
				v_dst = vshlq_n_s32(v_sx0, BL_WARP_SHIFT);
				v_dst = vmlaq_s32(v_dst, v_fy, v_sub);
				v_dst = vrshrq_n_s32(v_dst, BL_WARP_SHIFT);

				vst1q_s32(pD + dx, v_dst);
			}
#endif
			for (; dx < dstWidth; ++dx)
			{
				MInt32 sx0 = buf0[dx];
				MInt32 sx = buf1[dx];
				MInt32 val = BL_WARP_MUL_ONE_S(sx0);
				val += fy * (sx - sx0);
				pD[dx] = BL_WARP_DESCALE_S(val);
			}
		}
	}
}

MVoid thread_ImgBilinearResize_S32C1(MVoid *info)
{
	ImgBilinearResize_S32C1_MT *threadInfo = (ImgBilinearResize_S32C1_MT *)info;
	stripe_ImgBilinearResize_S32C1(threadInfo->pSrc, threadInfo->srcWidth, threadInfo->srcHeight, threadInfo->srcPitch,
		threadInfo->pDst, threadInfo->dstWidth, threadInfo->dstHeight, threadInfo->dstPitch,
		threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->taskID, threadInfo->pBLParam);
}

MRESULT ImgBilinearResize_S32C1(MHandle hMemMgr, MHandle parEngine, const MInt32 *pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
								MInt32 *pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch, const BilinearResize_Param *pBLParam)
{
	// same size, only copy
	if (srcWidth == dstWidth && srcHeight == dstHeight)
	{
		//MMemCpy(pDst, pSrc, dstHeight * dstPitch);
		const MInt32 *pSrcRow = pSrc;
		MInt32 *pDstRow = pDst;
		MInt32 lSrcStep = srcPitch / sizeof(MInt32);
		MInt32 lDstStep = dstPitch / sizeof(MInt32);

		for (MInt32 y = 0; y < dstHeight; ++y)
		{
			MMemCpy(pDstRow, pSrcRow, dstWidth * sizeof(MInt32));
			pDstRow += lDstStep;
			pSrcRow += lSrcStep;
		}
		return MOK;
	}

	if (!pBLParam) return MERR_INVALID_PARAM;

	if (parEngine)
	{
		MInt32 taskNum = Get_Task_Num(dstWidth, dstHeight);

		MInt32 i = taskNum * (sizeof(ImgBilinearResize_S32C1_MT) + sizeof(MInt32));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;
		ImgBilinearResize_S32C1_MT *pParam = (ImgBilinearResize_S32C1_MT *)buffer;
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
			iTask[i] = mcvAddTask(parEngine, thread_ImgBilinearResize_S32C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(hMemMgr, buffer);
	}
	else
	{
		stripe_ImgBilinearResize_S32C1(pSrc, srcWidth, srcHeight, srcPitch,
			pDst, dstWidth, dstHeight, dstPitch, 0, dstHeight, 0, pBLParam);
	}

	return MOK;
}


static MVoid stripe_ApplyTransform_ToDstU8C1(MInt32 *pAlphaMean, MInt32 lAlphaStep, MInt32 *pBetaMean, MInt32 lBetaStep, 
											 MByte *pGuidData, MInt32 lGuidStep, MByte *pDstData, MInt32 lDstStep, 
											 MInt32 lWidth, MInt32 lHeight, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 x = 0, y = 0;
	MInt32 *pAlphaRow = pAlphaMean + lStartRow * lAlphaStep;
	MInt32 *pBetaRow = pBetaMean + lStartRow * lBetaStep;
	MByte *pGuidRow = pGuidData + lStartRow * lGuidStep;
	MByte *pDstRow = pDstData + lStartRow * lDstStep;
	MInt32 lVal = 0;

#ifdef __ARM_NEON__
	uint8x8_t guidData_u8x8, dstData_u8x8;
	int32x4_t alphaDataLow_s32x4, betaDataLow_s32x4;
	int32x4_t alphaDataHigh_s32x4, betaDataHigh_s32x4;
	int16x8_t tmpData_s16x8;
	int32x4_t dstLow_s32x4, dstHigh_s32x4;
#endif //__ARM_NEON__

	for (y = lStartRow; y < lEndRow; ++y)
	{
		x = 0;
#ifdef __ARM_NEON__
		for (; x < lWidth - 7; x += 8)
		{
			guidData_u8x8 = vld1_u8(pGuidRow + x);
			alphaDataLow_s32x4 = vld1q_s32(pAlphaRow + x);
			alphaDataHigh_s32x4 = vld1q_s32(pAlphaRow + x + 4);
			betaDataLow_s32x4 = vld1q_s32(pBetaRow + x);
			betaDataHigh_s32x4 = vld1q_s32(pBetaRow + x + 4);

			tmpData_s16x8 = vreinterpretq_s16_u16(vmovl_u8(guidData_u8x8));
			dstLow_s32x4 = vmlaq_s32(betaDataLow_s32x4, alphaDataLow_s32x4, vmovl_s16(vget_low_s16(tmpData_s16x8)));
			dstHigh_s32x4 = vmlaq_s32(betaDataHigh_s32x4, alphaDataHigh_s32x4, vmovl_s16(vget_high_s16(tmpData_s16x8)));

			tmpData_s16x8 = vcombine_s16(vrshrn_n_s32(dstLow_s32x4, FIX_ALPHA_SCALE), vrshrn_n_s32(dstHigh_s32x4, FIX_ALPHA_SCALE));
			dstData_u8x8 = vqmovun_s16(tmpData_s16x8);
			vst1_u8(pDstRow + x, dstData_u8x8);
		}
#endif //__ARM_NEON__

		for (; x < lWidth; ++x)
		{
			/*if (y == 808 && x == 806)
			{
				x = x;
			}*/
			lVal = pAlphaRow[x] * (MInt32)pGuidRow[x] + pBetaRow[x];
			lVal = (lVal + (1 << (FIX_ALPHA_SCALE - 1))) >> FIX_ALPHA_SCALE;
			pDstRow[x] = ET_CAST_BYTE(lVal);
		}

		pAlphaRow += lAlphaStep;
		pBetaRow += lBetaStep;
		pGuidRow += lGuidStep;
		pDstRow += lDstStep;
	}

	return;
}

struct ApplyTransform_ToDstU8C1_MT{
	MInt32 *pAlphaMean;
	MInt32 lAlphaStep;
	MInt32 *pBetaMean;
	MInt32 lBetaStep;
	MByte *pGuidData;
	MInt32 lGuidStep;
	MByte *pDstData;
	MInt32 lDstStep;
	MInt32 lWidth;
	MInt32 lHeight;
	MInt32 lStartRow;
	MInt32 lEndRow;
};

MVoid thread_ApplyTransform_ToDstU8C1(MVoid *para)
{
	if (MNull != para)
	{
		ApplyTransform_ToDstU8C1_MT *pPara = (ApplyTransform_ToDstU8C1_MT *)para;
		stripe_ApplyTransform_ToDstU8C1(pPara->pAlphaMean, pPara->lAlphaStep, pPara->pBetaMean, pPara->lBetaStep,
										pPara->pGuidData, pPara->lGuidStep, pPara->pDstData, pPara->lDstStep,
										pPara->lWidth, pPara->lHeight, pPara->lStartRow, pPara->lEndRow);
	}
	return;
}

MRESULT ApplyTransform_ToDstU8C1(MHandle hMemMgr, MHandle parEngine, MInt32 *pAlphaMean, MInt32 lAlphaPitch,
								 MInt32 *pBetaMean, MInt32 lBetaPitch, MByte *pGuidData, MInt32 lGuidPitch, 
								 MByte *pDstData, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MRESULT lret = MOK;
	MInt32 lAlphaStep = lAlphaPitch / sizeof(MInt32);
	MInt32 lBetaStep = lBetaPitch / sizeof(MInt32);
	MInt32 lGuidStep = lGuidPitch / sizeof(MByte);
	MInt32 lDstStep = lDstPitch / sizeof(MByte);

	if (parEngine)
	{
		MInt32 lTaskNum = Get_Task_Num(lWidth, lHeight);
		ApplyTransform_ToDstU8C1_MT para[16] = { 0 };
		MInt32 lTaskId[16] = { 0 };
		MInt32 lBlockH = lHeight / lTaskNum;
		MInt32 k = 0;

		for (k = 0; k < lTaskNum; ++k)
		{
			para[k].pAlphaMean = pAlphaMean;
			para[k].lAlphaStep = lAlphaStep;
			para[k].pBetaMean = pBetaMean;
			para[k].lBetaStep = lBetaStep;
			para[k].pGuidData = pGuidData;
			para[k].lGuidStep = lGuidStep;
			para[k].pDstData = pDstData;
			para[k].lDstStep = lDstStep;
			para[k].lWidth = lWidth;
			para[k].lHeight = lHeight;
			para[k].lStartRow = k * lBlockH;
			para[k].lEndRow = (k + 1) * lBlockH;
		}
		para[lTaskNum - 1].lEndRow = lHeight;

		for (k = 0; k < lTaskNum; ++k)
		{
			lTaskId[k] = mcvAddTask(parEngine, thread_ApplyTransform_ToDstU8C1, (MVoid *)(para + k));
		}

		for (k = 0; k < lTaskNum; ++k)
		{
			lret = mcvWaitTask(parEngine, lTaskId[k]);
			if (MOK != lret)
			{
				lret = MERR_BAD_STATE;
				return lret;
			}
		}
	}
	else
	{
		stripe_ApplyTransform_ToDstU8C1(pAlphaMean, lAlphaStep, pBetaMean, lBetaStep, pGuidData, lGuidStep,
										pDstData, lDstStep, lWidth, lHeight, 0, lHeight);
	}

	return lret;
}

static MVoid stripe_Calc_AlphaAndBeta_F32_C1(MFloat epsilon, MFloat *pGuidMean, MInt32 lGuidMeanStep,
									MFloat *pSrcMean, MInt32 lSrcMeanStep, MFloat *pCorrGuidMean, MInt32 lCorrGuidMeanStep,
									MFloat *pCorrGuidSrcMean, MInt32 lCorrGuidSrcMeanStep, MFloat *pAlpha, MInt32 lAlphaStep,
									MFloat *pBeta, MInt32 lBetaStep, MInt32 lWidth, MInt32 lHeight, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 x = 0, y = 0;
	MFloat *pGuidMeanRow = pGuidMean + lStartRow * lGuidMeanStep;
	MFloat *pSrcMeanRow = pSrcMean + lStartRow * lSrcMeanStep;
	MFloat *pCorrGuidMeanRow = pCorrGuidMean + lStartRow * lCorrGuidMeanStep;
	MFloat *pCorrGuidSrcMeanRow = pCorrGuidSrcMean + lStartRow * lCorrGuidSrcMeanStep;
	MFloat *pAlphaRow = pAlpha + lStartRow * lAlphaStep;
	MFloat *pBetaRow = pBeta + lStartRow * lBetaStep;
	MFloat lCovGuidSrc = 0.0, lVarGuid = 0.0, lAlphaVal = 0.0;

#ifdef __ARM_NEON__
	float32x4_t guidData_f32x4, srcData_f32x4;
	float32x4_t corrGuid_f32x4, corrGuidSrc_f32x4, invCovGuid_f32x4;
	float32x4_t alphaData_f32x4, betaData_f32x4;
	const float32x4_t epsilonData_f32x4 = vdupq_n_f32(epsilon);
#endif //__ARM_NEON__

	for (y = lStartRow; y < lEndRow; ++y)
	{
		x = 0;
#ifdef __ARM_NEON__
		for (; x < lWidth - 3; x += 4)
		{
			guidData_f32x4 = vld1q_f32(pGuidMeanRow + x);
			srcData_f32x4 = vld1q_f32(pSrcMeanRow + x);
			corrGuid_f32x4 = vld1q_f32(pCorrGuidMeanRow + x);
			corrGuidSrc_f32x4 = vld1q_f32(pCorrGuidSrcMeanRow + x);

			corrGuid_f32x4 = vmlsq_f32(corrGuid_f32x4, guidData_f32x4, guidData_f32x4);
			corrGuid_f32x4 = vaddq_f32(corrGuid_f32x4, epsilonData_f32x4);

			//Newton-Raphson iteration
			invCovGuid_f32x4 = vrecpeq_f32(corrGuid_f32x4);
			invCovGuid_f32x4 = vmulq_f32(vrecpsq_f32(corrGuid_f32x4, invCovGuid_f32x4), invCovGuid_f32x4);
			invCovGuid_f32x4 = vmulq_f32(vrecpsq_f32(corrGuid_f32x4, invCovGuid_f32x4), invCovGuid_f32x4);

			corrGuidSrc_f32x4 = vmlsq_f32(corrGuidSrc_f32x4, guidData_f32x4, srcData_f32x4);

			alphaData_f32x4 = vmulq_f32(corrGuidSrc_f32x4, invCovGuid_f32x4);
			betaData_f32x4 = vmlsq_f32(srcData_f32x4, alphaData_f32x4, guidData_f32x4);

			vst1q_f32(pAlphaRow + x, alphaData_f32x4);
			vst1q_f32(pBetaRow + x, betaData_f32x4);
		}
#endif //__ARM_NEON__

		for (; x < lWidth; ++x)
		{
			lVarGuid = pCorrGuidMeanRow[x] - pGuidMeanRow[x] * pGuidMeanRow[x] + epsilon;
			lCovGuidSrc = pCorrGuidSrcMeanRow[x] - pGuidMeanRow[x] * pSrcMeanRow[x];

			pAlphaRow[x] = lAlphaVal = lCovGuidSrc / lVarGuid;
			pBetaRow[x] = pSrcMeanRow[x] - lAlphaVal * pGuidMeanRow[x];
		}

		pGuidMeanRow += lGuidMeanStep;
		pSrcMeanRow += lSrcMeanStep;
		pCorrGuidMeanRow += lCorrGuidMeanStep;
		pCorrGuidSrcMeanRow += lCorrGuidSrcMeanStep;
		pAlphaRow += lAlphaStep;
		pBetaRow += lBetaStep;
	}

	return;
}

struct Calc_AlphaAndBeta_F32_C1_MT{
	MFloat epsilon;
	MFloat *pGuidMean;
	MInt32 lGuidMeanStep;
	MFloat *pSrcMean;
	MInt32 lSrcMeanStep;
	MFloat *pCorrGuidMean;
	MInt32 lCorrGuidMeanStep;
	MFloat *pCorrGuidSrcMean;
	MInt32 lCorrGuidSrcMeanStep;
	MFloat *pAlpha;
	MInt32 lAlphaStep;
	MFloat *pBeta;
	MInt32 lBetaStep;
	MInt32 lWidth;
	MInt32 lHeight;
	MInt32 lStartRow;
	MInt32 lEndRow;
};

MVoid thread_Calc_AlphaAndBeta_F32_C1(MVoid *para)
{
	if (MNull != para)
	{
		Calc_AlphaAndBeta_F32_C1_MT *pPara = (Calc_AlphaAndBeta_F32_C1_MT *)para;
		stripe_Calc_AlphaAndBeta_F32_C1(pPara->epsilon, pPara->pGuidMean, pPara->lGuidMeanStep, pPara->pSrcMean, pPara->lSrcMeanStep,
										pPara->pCorrGuidMean, pPara->lCorrGuidMeanStep, pPara->pCorrGuidSrcMean, pPara->lCorrGuidSrcMeanStep,
										pPara->pAlpha, pPara->lAlphaStep, pPara->pBeta, pPara->lBetaStep, pPara->lWidth, pPara->lHeight, pPara->lStartRow, pPara->lEndRow);
	}
	return;
}

MRESULT Calc_AlphaAndBeta_F32_C1(MHandle hMemMgr, MHandle parEngine, MFloat epsilon, MFloat *pGuidMean, MInt32 lGuidMeanStep,
								 MFloat *pSrcMean, MInt32 lSrcMeanStep, MFloat *pCorrGuidMean, MInt32 lCorrGuidMeanStep,
								 MFloat *pCorrGuidSrcMean, MInt32 lCorrGuidSrcMeanStep, MFloat *pAlpha, MInt32 lAlphaStep,
								 MFloat *pBeta, MInt32 lBetaStep, MInt32 lWidth, MInt32 lHeight)
{
	MRESULT lret = MOK;

	if (parEngine)
	{
		const MInt32 lTaskNum = Get_Task_Num(lWidth, lHeight);
		Calc_AlphaAndBeta_F32_C1_MT *para = (Calc_AlphaAndBeta_F32_C1_MT *)MMemAlloc(hMemMgr, lTaskNum * sizeof(Calc_AlphaAndBeta_F32_C1_MT));
		MInt32 lTaskId[16] = { 0 };
		MInt32 lBlockH = lHeight / lTaskNum;
		MInt32 k = 0;
		if (!para)
		{
			lret = MERR_NO_MEMORY;
			return lret;
		}

		for (k = 0; k < lTaskNum; ++k)
		{
			para[k].epsilon = epsilon;
			para[k].pGuidMean = pGuidMean;
			para[k].lGuidMeanStep = lGuidMeanStep;
			para[k].pSrcMean = pSrcMean;
			para[k].lSrcMeanStep = lSrcMeanStep;
			para[k].pCorrGuidMean = pCorrGuidMean;
			para[k].lCorrGuidMeanStep = lCorrGuidMeanStep;
			para[k].pCorrGuidSrcMean = pCorrGuidSrcMean;
			para[k].lCorrGuidSrcMeanStep = lCorrGuidSrcMeanStep;
			para[k].pAlpha = pAlpha;
			para[k].lAlphaStep = lAlphaStep;
			para[k].pBeta = pBeta;
			para[k].lBetaStep = lBetaStep;
			para[k].lWidth = lWidth;
			para[k].lHeight = lHeight;
			para[k].lStartRow = k * lBlockH;
			para[k].lEndRow = (k + 1) * lBlockH;
		}
		para[lTaskNum - 1].lEndRow = lHeight;

		for (k = 0; k < lTaskNum; ++k)
		{
			lTaskId[k] = mcvAddTask(parEngine, thread_Calc_AlphaAndBeta_F32_C1, (MVoid *)(para + k));
		}

		for (k = 0; k < lTaskNum; ++k)
		{
			lret = mcvWaitTask(parEngine, lTaskId[k]);
			if (MOK != lret)
			{
				lret = MERR_BAD_STATE;
				goto exit_par;
			}
		}

	exit_par:
		if (para)
		{
			MMemFree(hMemMgr, para);
			para = MNull;
			return lret;
		}

	}
	else
	{
		stripe_Calc_AlphaAndBeta_F32_C1(epsilon, pGuidMean, lGuidMeanStep, pSrcMean, lSrcMeanStep, pCorrGuidMean, lCorrGuidMeanStep,
						pCorrGuidSrcMean, lCorrGuidSrcMeanStep, pAlpha, lAlphaStep, pBeta, lBetaStep, lWidth, lHeight, 0, lHeight);
	}

	return lret;
}

// box filter for float32 single channel
static MVoid boxBlurRowAdd_F32toI32_C1(MFloat* pSumLine, const MFloat* addSrc, MInt32 width, MInt32 radius)
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

static MVoid boxBlurRowAddSub_F32toI32_C1(MFloat* pSumLine, const MFloat* addSrc, const MFloat* subSrc,
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

static MVoid boxBlurProcessRow_F32toI32_C1(MInt32* pDst, MFloat* pBoxSumBuf, MInt32 width, MInt32 radius, MFloat invDivNum)
{
	MInt32 x;
	MFloat lbVal = 0;
	MInt32 lboxSize = radius * 2 + 1;

	x = 0;
#ifdef __ARM_NEON__
	float32x4_t v_invDiv = vdupq_n_f32(invDivNum);
	float32x4_t v_sum0, v_sum1;
	float32x4_t v_dst;
	const float32x4_t v_const = vdupq_n_f32(0.5);
	for (; x < width - 3; x += 4)
	{
		v_sum1 = vld1q_f32(pBoxSumBuf + lboxSize);
		v_sum0 = vld1q_f32(pBoxSumBuf);
		v_dst = vsubq_f32(v_sum1, v_sum0);
		v_dst = vmulq_f32(v_dst, v_invDiv);
		v_dst = vaddq_f32(v_dst, v_const);
		vst1q_s32(pDst, vcvtq_s32_f32(v_dst));
		pBoxSumBuf += 4;
		pDst += 4;
	}
#endif
	for (; x < width; x++)
	{
		lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
		(pDst++)[0] = lbVal * invDivNum + 0.5;
		pBoxSumBuf++;
	}
}

MVoid stripe_BoxBlur_F32toI32C1(const MFloat* pSrc, MInt32 lSrcPitch, MInt32* pDst, MInt32 lDstPitch, MInt32 height, MInt32 width,
	MInt32 lStartRow, MInt32 lEndRow, MInt32 radius, MFloat *pBoxSumBuf)
{
	MInt32 lSrcStep = lSrcPitch / sizeof(MFloat);
	MInt32 lDstStep = lDstPitch / sizeof(MInt32);

	MInt32 lPreLine = MIN(radius, lStartRow),
		lNexLine = MIN(radius, height - lEndRow);
	MInt32 lTop = lStartRow - lPreLine;
	MInt32 lBot = lEndRow + lNexLine;
	const MFloat *tmpaddSrc = pSrc + lTop * lSrcStep;
	const MFloat *tmpsubSrc = pSrc + lTop * lSrcStep;
	const MFloat *tmpSrc = pSrc + lStartRow * lSrcStep;
	MInt32 *tmpDst = pDst + lStartRow * lDstStep;
	MFloat invArea = (MFloat)(1 << FIX_ALPHA_SCALE) / ((radius * 2 + 1)*(radius * 2 + 1));

	MInt32 line = lStartRow - radius;

	MMemSet(pBoxSumBuf, 0, (width + radius * 2 + 1 + 100) * sizeof(MFloat));

	for (; line < lTop; line++)
	{
		boxBlurRowAdd_F32toI32_C1(pBoxSumBuf, tmpaddSrc, width, radius);
	}
	for (; line < lStartRow + radius; line++)
	{
		boxBlurRowAdd_F32toI32_C1(pBoxSumBuf, tmpaddSrc, width, radius);
		tmpaddSrc += lSrcStep;
	}

	{
		boxBlurRowAdd_F32toI32_C1(pBoxSumBuf, tmpaddSrc, width, radius);
		boxBlurProcessRow_F32toI32_C1(tmpDst, pBoxSumBuf, width, radius, invArea);
		tmpaddSrc += lSrcStep;
		tmpSrc += lSrcStep;
		tmpDst += lDstStep;
		line++;
	}

	for (; line < lBot; line++)
	{
		boxBlurRowAddSub_F32toI32_C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_F32toI32_C1(tmpDst, pBoxSumBuf, width, radius, invArea);
		tmpaddSrc += lSrcStep;
		if (line >= lTop + radius * 2 + 1)
		{
			tmpsubSrc += lSrcStep;
		}
		tmpSrc += lSrcStep;
		tmpDst += lDstStep;
	}

	tmpaddSrc -= lSrcStep;
	for (; line < lEndRow + radius; line++)
	{
		boxBlurRowAddSub_F32toI32_C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_F32toI32_C1(tmpDst, pBoxSumBuf, width, radius, invArea);
		tmpsubSrc += lSrcStep;
		tmpSrc += lSrcStep;
		tmpDst += lDstStep;
	}
}

struct BoxFilter_F32toI32C1_MT {
	MInt32			taskID;
	MInt32			width;
	MInt32			height;
	const MFloat	*pSrc;
	MInt32			lSrcPitch;
	MInt32			*pDst;
	MInt32			lDstPitch;
	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32          radius;
	MFloat          *pBoxSumBuf;
};

MVoid thread_BoxFilter_F32toI32C1(MVoid *info)
{
	BoxFilter_F32toI32C1_MT *threadInfo = (BoxFilter_F32toI32C1_MT *)info;
	stripe_BoxBlur_F32toI32C1(threadInfo->pSrc, threadInfo->lSrcPitch, threadInfo->pDst, threadInfo->lDstPitch,
		threadInfo->height, threadInfo->width, threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->radius, threadInfo->pBoxSumBuf);
}

MRESULT BoxFilter_F32toI32_C1(MHandle hMemMgr, MHandle parEngine, const MFloat *pSrc, MInt32 lSrcPitch, MInt32 *pDst,
	MInt32 lDstPitch, MInt32 height, MInt32 width, MInt32 radius)
{
	if (parEngine)
	{
		MInt32 taskNum = Get_Task_Num(width, height);
		MInt32 i = taskNum * (sizeof(BoxFilter_F32toI32C1_MT) + sizeof(MInt32)
			+ (width + radius * 2 + 1 + 100) * sizeof(MFloat));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;

		BoxFilter_F32toI32C1_MT *pParam = (BoxFilter_F32toI32C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);
		MFloat *boxSumBuf = (MFloat *)(iTask + taskNum);

		MInt32 rowStep = height / taskNum;
		MInt32 sumBufStep = (width + radius * 2 + 1 + 100);
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrc = pSrc;
			pParam[i].lSrcPitch = lSrcPitch;
			pParam[i].pDst = pDst;
			pParam[i].lDstPitch = lDstPitch;
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
			iTask[i] = mcvAddTask(parEngine, thread_BoxFilter_F32toI32C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(hMemMgr, buffer);
	}
	else
	{
		MFloat *boxSumBuf = (MFloat*)MMemAlloc(hMemMgr, (width + radius * 2 + 1 + 100) * sizeof(MFloat));
		if (!boxSumBuf) return MERR_NO_MEMORY;

		stripe_BoxBlur_F32toI32C1(pSrc, lSrcPitch, pDst, lDstPitch,
			height, width, 0, height, radius, boxSumBuf);

		if (boxSumBuf) MMemFree(hMemMgr, boxSumBuf);
	}

	return MOK;
}

// box filter for float32 single channel
static MVoid boxBlurRowAdd_F32toI16_C1(MFloat* pSumLine, const MFloat* addSrc, MInt32 width, MInt32 radius)
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

static MVoid boxBlurRowAddSub_F32toI16_C1(MFloat* pSumLine, const MFloat* addSrc, const MFloat* subSrc,
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

static MVoid boxBlurProcessRow_F32toI16_C1(MInt16* pDst, MFloat* pBoxSumBuf, MInt32 width, MInt32 radius, MFloat invDivNum)
{
	MInt32 x;
	MFloat lbVal = 0;
	MInt32 lboxSize = radius * 2 + 1;

	x = 0;
#ifdef __ARM_NEON__
	float32x4_t v_invDiv = vdupq_n_f32(invDivNum);
	float32x4_t v_sum0, v_sum1;
	float32x4_t v_dst;
	const float32x4_t v_const = vdupq_n_f32(0.5);
	for (; x < width - 3; x += 4)
	{
		v_sum1 = vld1q_f32(pBoxSumBuf + lboxSize);
		v_sum0 = vld1q_f32(pBoxSumBuf);
		v_dst = vsubq_f32(v_sum1, v_sum0);
		v_dst = vmulq_f32(v_dst, v_invDiv);
		v_dst = vaddq_f32(v_dst, v_const);
		vst1_s16(pDst, vmovn_s32(vcvtq_s32_f32(v_dst)));
		pBoxSumBuf += 4;
		pDst += 4;
	}
#endif
	for (; x < width; x++)
	{
		lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
		(pDst++)[0] = lbVal * invDivNum + 0.5;
		pBoxSumBuf++;
	}
}

MVoid stripe_BoxBlur_F32toI16C1(const MFloat* pSrc, MInt32 lSrcPitch, MInt16* pDst, MInt32 lDstPitch, MInt32 height, MInt32 width,
								MInt32 lStartRow, MInt32 lEndRow, MInt32 radius, MFloat *pBoxSumBuf)
{
	MInt32 lSrcStep = lSrcPitch / sizeof(MFloat);
	MInt32 lDstStep = lDstPitch / sizeof(MInt16);

	MInt32 lPreLine = MIN(radius, lStartRow),
		lNexLine = MIN(radius, height - lEndRow);
	MInt32 lTop = lStartRow - lPreLine;
	MInt32 lBot = lEndRow + lNexLine;
	const MFloat *tmpaddSrc = pSrc + lTop * lSrcStep;
	const MFloat *tmpsubSrc = pSrc + lTop * lSrcStep;
	const MFloat *tmpSrc = pSrc + lStartRow * lSrcStep;
	MInt16 *tmpDst = pDst + lStartRow * lDstStep;
	MFloat invArea = 1.0 / ((radius * 2 + 1)*(radius * 2 + 1));

	MInt32 line = lStartRow - radius;

	MMemSet(pBoxSumBuf, 0, (width + radius * 2 + 1 + 100) * sizeof(MFloat));

	for (; line < lTop; line++)
	{
		boxBlurRowAdd_F32toI16_C1(pBoxSumBuf, tmpaddSrc, width, radius);
	}
	for (; line < lStartRow + radius; line++)
	{
		boxBlurRowAdd_F32toI16_C1(pBoxSumBuf, tmpaddSrc, width, radius);
		tmpaddSrc += lSrcStep;
	}

	{
		boxBlurRowAdd_F32toI16_C1(pBoxSumBuf, tmpaddSrc, width, radius);
		boxBlurProcessRow_F32toI16_C1(tmpDst, pBoxSumBuf, width, radius, invArea);
		tmpaddSrc += lSrcStep;
		tmpSrc += lSrcStep;
		tmpDst += lDstStep;
		line++;
	}

	for (; line < lBot; line++)
	{
		boxBlurRowAddSub_F32toI16_C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_F32toI16_C1(tmpDst, pBoxSumBuf, width, radius, invArea);
		tmpaddSrc += lSrcStep;
		if (line >= lTop + radius * 2 + 1)
		{
			tmpsubSrc += lSrcStep;
		}
		tmpSrc += lSrcStep;
		tmpDst += lDstStep;
	}

	tmpaddSrc -= lSrcStep;
	for (; line < lEndRow + radius; line++)
	{
		boxBlurRowAddSub_F32toI16_C1(pBoxSumBuf, tmpaddSrc, tmpsubSrc, width, radius);
		boxBlurProcessRow_F32toI16_C1(tmpDst, pBoxSumBuf, width, radius, invArea);
		tmpsubSrc += lSrcStep;
		tmpSrc += lSrcStep;
		tmpDst += lDstStep;
	}
}

struct BoxFilter_F32toI16C1_MT {
	MInt32			taskID;
	MInt32			width;
	MInt32			height;
	const MFloat	*pSrc;
	MInt32			lSrcPitch;
	MInt16			*pDst;
	MInt32			lDstPitch;
	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32          radius;
	MFloat          *pBoxSumBuf;
};

MVoid thread_BoxFilter_F32toI16C1(MVoid *info)
{
	BoxFilter_F32toI16C1_MT *threadInfo = (BoxFilter_F32toI16C1_MT *)info;
	stripe_BoxBlur_F32toI16C1(threadInfo->pSrc, threadInfo->lSrcPitch, threadInfo->pDst, threadInfo->lDstPitch,
		threadInfo->height, threadInfo->width, threadInfo->lStartRow, threadInfo->lEndRow, threadInfo->radius, threadInfo->pBoxSumBuf);
}

MRESULT BoxFilter_F32toI16_C1(MHandle hMemMgr, MHandle parEngine, const MFloat *pSrc, MInt32 lSrcPitch, MInt16 *pDst,
							  MInt32 lDstPitch, MInt32 height, MInt32 width, MInt32 radius)
{
	if (parEngine)
	{
		MInt32 taskNum = Get_Task_Num(width, height);
		MInt32 i = taskNum * (sizeof(BoxFilter_F32toI16C1_MT) + sizeof(MInt32)
			+ (width + radius * 2 + 1 + 100) * sizeof(MFloat));
		MVoid *buffer = MMemAlloc(hMemMgr, i);
		if (!buffer) return MERR_NO_MEMORY;

		BoxFilter_F32toI16C1_MT *pParam = (BoxFilter_F32toI16C1_MT *)buffer;
		MInt32 *iTask = (MInt32 *)(pParam + taskNum);
		MFloat *boxSumBuf = (MFloat *)(iTask + taskNum);

		MInt32 rowStep = height / taskNum;
		MInt32 sumBufStep = (width + radius * 2 + 1 + 100);
		for (i = 0; i < taskNum; ++i)
		{
			pParam[i].taskID = i;
			pParam[i].pSrc = pSrc;
			pParam[i].lSrcPitch = lSrcPitch;
			pParam[i].pDst = pDst;
			pParam[i].lDstPitch = lDstPitch;
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
			iTask[i] = mcvAddTask(parEngine, thread_BoxFilter_F32toI16C1, (MVoid *)&pParam[i]);
		}

		for (i = 0; i < taskNum; ++i)
		{
			mcvWaitTask(parEngine, iTask[i]);
		}

		if (buffer) MMemFree(hMemMgr, buffer);
	}
	else
	{
		MFloat *boxSumBuf = (MFloat*)MMemAlloc(hMemMgr, (width + radius * 2 + 1 + 100) * sizeof(MFloat));
		if (!boxSumBuf) return MERR_NO_MEMORY;

		stripe_BoxBlur_F32toI16C1(pSrc, lSrcPitch, pDst, lDstPitch,
			height, width, 0, height, radius, boxSumBuf);

		if (boxSumBuf) MMemFree(hMemMgr, boxSumBuf);
	}

	return MOK;
}