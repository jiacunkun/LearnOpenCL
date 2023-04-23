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

#include "ammem.h"
#include "amcomdef.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "math.h"
#include "mobilecv.h"
#include "img_dilate_erode.h"

#ifdef MCV_MULTI_THREAD
typedef struct __tag_FilterMophoOp_para{
	MByte *pBufImg;
	MByte* pSrc;
	MInt32 lSrcPitch;
	MByte* pDst;
	MInt32 lDstPitch;
	MInt32 lWidth;
	MInt32 lHeight;
	MInt32 lStartRow;
	MInt32 lEndRow;
}FilterMophoOp_Para;
#endif

MVoid ver_line_min_process_3x3(MByte* tmpSrc00, MByte* tmpSrc01, MByte* tmpSrc02, MByte* pTmpBuf, MInt32 lWidth)
{
	MInt32 x = 0;
	MInt32 lval = 0;

#ifdef __ARM_NEON__
	uint8x16_t srcData00, srcData01, srcData02, dstData;
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < lWidth - 15; x += 16)
	{
		srcData00 = vld1q_u8(tmpSrc00 + x);
		srcData01 = vld1q_u8(tmpSrc01 + x);
		srcData02 = vld1q_u8(tmpSrc02 + x);

		dstData = vminq_u8(srcData00, srcData01);
		dstData = vminq_u8(dstData, srcData02);
		vst1q_u8(pTmpBuf + x, dstData);
	}
#endif //__ARM_NEON__

	for (; x < lWidth; ++x)
	{
		lval = MIN(tmpSrc00[x], tmpSrc01[x]);
		lval = MIN(lval, tmpSrc02[x]);

		pTmpBuf[x] = lval;
	}
	return;
}

MVoid hor_line_min_process_3x3(MByte* pTmpBuf, MByte* tmpDst, MInt32 lWidth)
{
	MInt32 x = 0;
	MInt32 lval = 0;

#ifdef __ARM_NEON__
	uint8x16_t srcData00, srcData01, srcData02, dstData;
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < lWidth - 15; x += 16)
	{
		srcData00 = vld1q_u8(pTmpBuf + x - 1);
		srcData01 = vld1q_u8(pTmpBuf + x);
		srcData02 = vld1q_u8(pTmpBuf + x + 1);

		dstData = vminq_u8(srcData00, srcData01);
		dstData = vminq_u8(dstData, srcData02);
		vst1q_u8(tmpDst + x, dstData);
	}
#endif //__ARM_NEON__

	for (; x < lWidth; ++x)
	{
		lval = MIN(pTmpBuf[x - 1], pTmpBuf[x]);
		lval = MIN(lval, pTmpBuf[x + 1]);
		tmpDst[x] = lval;
	}
	return;
}

MVoid local_FilterErode3x3u8(MByte *pBufImg, MByte* pSrc, MInt32 lSrcPitch, MByte* pDst, MInt32 lDstPitch,
							 MInt32 lWidth, MInt32 lHeight, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 x, y;
	MByte* tmpSrc00 = MNull, *tmpSrc01 = MNull, *tmpSrc02 = MNull;
	MByte *pTmpBuf = pBufImg + 1;
	MByte* tmpDst = pDst + lStartRow * lDstPitch;

	tmpSrc01 = pSrc + lStartRow * lSrcPitch;
	tmpSrc00 = (lStartRow == 0) ? tmpSrc01 : (tmpSrc01 - lSrcPitch);
	tmpSrc02 = (lStartRow == lHeight - 1) ? tmpSrc01 : (tmpSrc01 + lSrcPitch);
	for (y = lStartRow; y < lEndRow; ++y, tmpDst += lDstPitch)
	{
		ver_line_min_process_3x3(tmpSrc00, tmpSrc01, tmpSrc02, pTmpBuf, lWidth);
		pTmpBuf[-1] = pTmpBuf[0];
		pTmpBuf[lWidth] = pTmpBuf[lWidth - 1];
		hor_line_min_process_3x3(pTmpBuf, tmpDst, lWidth);

		tmpSrc00 = tmpSrc01;
		tmpSrc01 = tmpSrc02;
		if (y + 1 < lHeight - 1)
		{
			tmpSrc02 += lSrcPitch;
		}
	}

	return;
}

#ifdef MCV_MULTI_THREAD
MVoid thread_FilterErode3x3u8(MVoid *para)
{
	if (MNull != para)
	{
		FilterMophoOp_Para *pPara = (FilterMophoOp_Para *)para;
		local_FilterErode3x3u8(pPara->pBufImg, pPara->pSrc, pPara->lSrcPitch, pPara->pDst, pPara->lDstPitch,
							   pPara->lWidth, pPara->lHeight, pPara->lStartRow, pPara->lEndRow);
	}
	return;
}
#endif


MInt32 FilterErode3x3u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch, 
						MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lret = MOK;
	MByte *pBufImg[16] = { MNull };

#ifdef MCV_MULTI_THREAD
	const MInt32 lTaskNum = lHeight > 16 ? 16 : 8;;
	MInt32 lTaskId[16] = { MNull };
	FilterMophoOp_Para para[16] = { MNull };
	MInt32 lBlockH = (lHeight / lTaskNum) >> 1 << 1;
	MInt32 k = 0;

	pBufImg[0] = (MByte *)MMemAlloc(hMemMgr, lTaskNum * (lWidth + 2) * sizeof(MByte));
	if (!pBufImg[0])
	{
		return MERR_NO_MEMORY;
	}

	for (k = 1; k < lTaskNum; ++k)
	{
		pBufImg[k] = pBufImg[k - 1] + lWidth + 2;
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		para[k].pBufImg = pBufImg[k];
		para[k].pSrc = pSrc;
		para[k].lSrcPitch = lSrcPitch;
		para[k].pDst = pDst;
		para[k].lDstPitch = lDstPitch;
		para[k].lWidth = lWidth;
		para[k].lHeight = lHeight;
		para[k].lStartRow = k * lBlockH;
		para[k].lEndRow = (k + 1) * lBlockH;
	}
	para[lTaskNum - 1].lEndRow = lHeight;

	for (k = 0; k < lTaskNum; ++k)
	{
		lTaskId[k] = mcvAddTask(mcvParallelMonitor, thread_FilterErode3x3u8, (MVoid *)(para + k));
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		lret = mcvWaitTask(mcvParallelMonitor, lTaskId[k]);
		if (MOK != lret)
		{
			lret = MERR_BAD_STATE;
			goto exit;
		}
	}
#else
	pBufImg[0] = (MByte *)MMemAlloc(hMemMgr, (lWidth + 2) * sizeof(MByte));
	if (!pBufImg[0])
	{
		return MERR_NO_MEMORY;
	}

	local_FilterErode3x3u8(pBufImg[0], pSrc, lSrcPitch, pDst, lDstPitch, lWidth, lHeight, 0, lHeight);
#endif

exit:
	if (pBufImg[0])
	{
		MMemFree(hMemMgr, pBufImg[0]);
		pBufImg[0] = MNull;
	}
	return lret;
}

MVoid ver_line_max_process_3x3(MByte* tmpSrc00, MByte* tmpSrc01, MByte* tmpSrc02, MByte* pTmpBuf, MInt32 lWidth)
{
	MInt32 x = 0;
	MInt32 lval = 0;

#ifdef __ARM_NEON__
	uint8x16_t srcData00, srcData01, srcData02, dstData;
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < lWidth - 15; x += 16)
	{
		srcData00 = vld1q_u8(tmpSrc00 + x);
		srcData01 = vld1q_u8(tmpSrc01 + x);
		srcData02 = vld1q_u8(tmpSrc02 + x);

		dstData = vmaxq_u8(srcData00, srcData01);
		dstData = vmaxq_u8(dstData, srcData02);
		vst1q_u8(pTmpBuf + x, dstData);
	}
#endif //__ARM_NEON__

	for (; x < lWidth; ++x)
	{
		lval = MAX(tmpSrc00[x], tmpSrc01[x]);
		lval = MAX(lval, tmpSrc02[x]);

		pTmpBuf[x] = lval;
	}
	return;
}

MVoid hor_line_max_process_3x3(MByte* pTmpBuf, MByte* tmpDst, MInt32 lWidth)
{
	MInt32 x = 0;
	MInt32 lval = 0;
#ifdef __ARM_NEON__
	uint8x16_t srcData00, srcData01, srcData02, dstData;
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < lWidth - 15; x += 16)
	{
		srcData00 = vld1q_u8(pTmpBuf + x - 1);
		srcData01 = vld1q_u8(pTmpBuf + x);
		srcData02 = vld1q_u8(pTmpBuf + x + 1);

		dstData = vmaxq_u8(srcData00, srcData01);
		dstData = vmaxq_u8(dstData, srcData02);
		vst1q_u8(tmpDst + x, dstData);
	}
#endif //__ARM_NEON__

	for (; x < lWidth; ++x)
	{
		lval = MAX(pTmpBuf[x - 1], pTmpBuf[x]);
		lval = MAX(lval, pTmpBuf[x + 1]);
		tmpDst[x] = lval;
	}
	return;
}

MVoid local_FilterDilate3x3u8(MByte *pBufImg, MByte* pSrc, MInt32 lSrcPitch, MByte* pDst, MInt32 lDstPitch,
							  MInt32 lWidth, MInt32 lHeight, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 x, y;
	MByte* tmpSrc00 = MNull, *tmpSrc01 = MNull, *tmpSrc02 = MNull;
	MByte *pTmpBuf = pBufImg + 1;
	MByte* tmpDst = pDst + lStartRow * lDstPitch;

	tmpSrc01 = pSrc + lStartRow * lSrcPitch;
	tmpSrc00 = (lStartRow == 0) ? tmpSrc01 : (tmpSrc01 - lSrcPitch);
	tmpSrc02 = (lStartRow == lHeight - 1) ? tmpSrc01 : (tmpSrc01 + lSrcPitch);
	for (y = lStartRow; y < lEndRow; ++y, tmpDst += lDstPitch)
	{
		ver_line_max_process_3x3(tmpSrc00, tmpSrc01, tmpSrc02, pTmpBuf, lWidth);
		pTmpBuf[-1] = pTmpBuf[0];
		pTmpBuf[lWidth] = pTmpBuf[lWidth - 1];
		hor_line_max_process_3x3(pTmpBuf, tmpDst, lWidth);

		tmpSrc00 = tmpSrc01;
		tmpSrc01 = tmpSrc02;
		if (y + 1 < lHeight - 1)
		{
			tmpSrc02 += lSrcPitch;
		}
	}

	return;
}

#ifdef MCV_MULTI_THREAD
typedef struct __tag_FilterDilate3x3u8_para{
	MByte *pBufImg;
	MByte* pSrc;
	MInt32 lSrcPitch;
	MByte* pDst;
	MInt32 lDstPitch;
	MInt32 lWidth;
	MInt32 lHeight;
	MInt32 lStartRow;
	MInt32 lEndRow;
}FilterDilate3x3u8_Para;

MVoid thread_FilterDilate3x3u8(MVoid *para)
{
	if (MNull != para)
	{
		FilterDilate3x3u8_Para *pPara = (FilterDilate3x3u8_Para *)para;
		local_FilterDilate3x3u8(pPara->pBufImg, pPara->pSrc, pPara->lSrcPitch, pPara->pDst, pPara->lDstPitch,
			pPara->lWidth, pPara->lHeight, pPara->lStartRow, pPara->lEndRow);
	}
	return;
}
#endif


MInt32 FilterDilate3x3u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch, 
						 MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lret = MOK;
	MByte *pBufImg[16] = { MNull };

#ifdef MCV_MULTI_THREAD
	const MInt32 lTaskNum = lHeight > 16 ? 16 : 8;;
	MInt32 lTaskId[16] = { MNull };
	FilterDilate3x3u8_Para para[16] = { MNull };
	MInt32 lBlockH = (lHeight / lTaskNum) >> 1 << 1;
	MInt32 k = 0;

	pBufImg[0] = (MByte *)MMemAlloc(hMemMgr, lTaskNum * (lWidth + 2) * sizeof(MByte));
	if (!pBufImg[0])
	{
		return MERR_NO_MEMORY;
	}

	for (k = 1; k < lTaskNum; ++k)
	{
		pBufImg[k] = pBufImg[k - 1] + lWidth + 2;
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		para[k].pBufImg = pBufImg[k];
		para[k].pSrc = pSrc;
		para[k].lSrcPitch = lSrcPitch;
		para[k].pDst = pDst;
		para[k].lDstPitch = lDstPitch;
		para[k].lWidth = lWidth;
		para[k].lHeight = lHeight;
		para[k].lStartRow = k * lBlockH;
		para[k].lEndRow = (k + 1) * lBlockH;
	}
	para[lTaskNum - 1].lEndRow = lHeight;

	for (k = 0; k < lTaskNum; ++k)
	{
		lTaskId[k] = mcvAddTask(mcvParallelMonitor, thread_FilterDilate3x3u8, (MVoid *)(para + k));
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		lret = mcvWaitTask(mcvParallelMonitor, lTaskId[k]);
		if (MOK != lret)
		{
			lret = MERR_BAD_STATE;
			goto exit;
		}
	}
#else
	pBufImg[0] = (MByte *)MMemAlloc(hMemMgr, (lWidth + 2) * sizeof(MByte));
	if (!pBufImg[0])
	{
		return MERR_NO_MEMORY;
	}

	local_FilterDilate3x3u8(pBufImg[0], pSrc, lSrcPitch, pDst, lDstPitch, lWidth, lHeight, 0, lHeight);
#endif

exit:
	if (pBufImg[0])
	{
		MMemFree(hMemMgr, pBufImg[0]);
		pBufImg[0] = MNull;
	}
	return lret;
}

MInt32 FilterOpenMopho3x3u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *pSrc, MInt32 lSrcPitch, 
							MByte *pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lret = MOK;
	MByte *pTmpBuf = (MByte *)MMemAlloc(hMemMgr, lHeight * lSrcPitch * sizeof(MByte));
	if (!pTmpBuf)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	lret = FilterErode3x3u8(hMemMgr, mcvParallelMonitor, pSrc, lSrcPitch, pTmpBuf, lSrcPitch, lWidth, lHeight);
	if (MOK != lret)
		goto exit;

	lret = FilterDilate3x3u8(hMemMgr, mcvParallelMonitor, pTmpBuf, lSrcPitch, pDst, lDstPitch, lWidth, lHeight);

exit:
	if (pTmpBuf)
	{
		MMemFree(hMemMgr, pTmpBuf);
		pTmpBuf = MNull;
	}
	return lret;
}

MInt32 FilterCloseMopho3x3u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *pSrc, MInt32 lSrcPitch,
							 MByte *pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lret = MOK;
	MByte *pTmpBuf = (MByte *)MMemAlloc(hMemMgr, lHeight * lSrcPitch * sizeof(MByte));
	if (!pTmpBuf)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	lret = FilterDilate3x3u8(hMemMgr, mcvParallelMonitor, pSrc, lSrcPitch, pTmpBuf, lSrcPitch, lWidth, lHeight);
	if (MOK != lret)
		goto exit;

	lret = FilterErode3x3u8(hMemMgr, mcvParallelMonitor, pTmpBuf, lSrcPitch, pDst, lDstPitch, lWidth, lHeight);

exit:
	if (pTmpBuf)
	{
		MMemFree(hMemMgr, pTmpBuf);
		pTmpBuf = MNull;
	}
	return lret;
}

MVoid ver_line_min_process_5x5(MByte* tmpSrc00, MByte* tmpSrc01, MByte* tmpSrc02, MByte *tmpSrc03, MByte *tmpSrc04, MByte* pTmpBuf, MInt32 lWidth)
{
	MInt32 x;
	MInt32 lval;

#ifdef __ARM_NEON__
	uint8x16_t srcData00, srcData01, srcData02, srcData03, srcData04, dstData;
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < lWidth - 15; x += 16)
	{
		srcData00 = vld1q_u8(tmpSrc00 + x);
		srcData01 = vld1q_u8(tmpSrc01 + x);
		srcData02 = vld1q_u8(tmpSrc02 + x);
		srcData03 = vld1q_u8(tmpSrc03 + x);
		srcData04 = vld1q_u8(tmpSrc04 + x);

		dstData = vminq_u8(srcData00, srcData01);
		dstData = vminq_u8(dstData, srcData02);
		dstData = vminq_u8(dstData, srcData03);
		dstData = vminq_u8(dstData, srcData04);
		vst1q_u8(pTmpBuf + x, dstData);
	}
#endif //__ARM_NEON__

	for (; x < lWidth; ++x)
	{
		lval = MIN(tmpSrc00[x], tmpSrc01[x]);
		lval = MIN(lval, tmpSrc02[x]);
		lval = MIN(lval, tmpSrc03[x]);
		lval = MIN(lval, tmpSrc04[x]);

		pTmpBuf[x] = lval;
	}
	return;
}

MVoid hor_line_min_process_5x5(MByte* pTmpBuf, MByte* tmpDst, MInt32 lWidth)
{
	MInt32 x;
	MInt32 lval;

#ifdef __ARM_NEON__
	uint8x16_t srcData00, srcData01, srcData02, srcData03, srcData04, dstData;
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < lWidth - 15; x += 16)
	{
		srcData00 = vld1q_u8(pTmpBuf + x - 2);
		srcData01 = vld1q_u8(pTmpBuf + x - 1);
		srcData02 = vld1q_u8(pTmpBuf + x);
		srcData03 = vld1q_u8(pTmpBuf + x + 1);
		srcData04 = vld1q_u8(pTmpBuf + x + 2);

		dstData = vminq_u8(srcData00, srcData01);
		dstData = vminq_u8(dstData, srcData02);
		dstData = vminq_u8(dstData, srcData03);
		dstData = vminq_u8(dstData, srcData04);
		vst1q_u8(tmpDst + x, dstData);
	}
#endif //__ARM_NEON__

	for (; x < lWidth; ++x)
	{
		lval = MIN(pTmpBuf[x - 2], pTmpBuf[x - 1]);
		lval = MIN(lval, pTmpBuf[x]);
		lval = MIN(lval, pTmpBuf[x + 1]);
		lval = MIN(lval, pTmpBuf[x + 2]);
		tmpDst[x] = lval;
	}
	return;
}

MVoid local_FilterErode5x5u8(MByte *pBufImg, MByte* pSrc, MInt32 lSrcPitch, MByte* pDst, MInt32 lDstPitch,
							 MInt32 lWidth, MInt32 lHeight, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 x, y;
	MByte* tmpSrc00 = MNull, *tmpSrc01 = MNull, *tmpSrc02 = MNull, *tmpSrc03, *tmpSrc04;
	MByte *pTmpBuf = pBufImg + 2;
	MByte* tmpDst = pDst + lStartRow * lDstPitch;

	tmpSrc02 = pSrc + lStartRow * lSrcPitch;
	tmpSrc01 = (lStartRow < 1) ? tmpSrc02 : (tmpSrc02 - lSrcPitch);
	tmpSrc00 = (lStartRow < 2) ? tmpSrc01 : (tmpSrc01 - lSrcPitch);
	tmpSrc03 = (lStartRow + 1 >= lHeight) ? tmpSrc02 : (tmpSrc02 + lSrcPitch);
	tmpSrc04 = (lStartRow + 2 >= lHeight) ? tmpSrc03 : (tmpSrc03 + lSrcPitch);
	for (y = lStartRow; y < lEndRow; ++y, tmpDst += lDstPitch)
	{
		ver_line_min_process_5x5(tmpSrc00, tmpSrc01, tmpSrc02, tmpSrc03, tmpSrc04, pTmpBuf, lWidth);
		pTmpBuf[-2] = pTmpBuf[-1] = pTmpBuf[0];
		pTmpBuf[lWidth + 1] = pTmpBuf[lWidth] = pTmpBuf[lWidth - 1];
		hor_line_min_process_5x5(pTmpBuf, tmpDst, lWidth);

		tmpSrc00 = tmpSrc01;
		tmpSrc01 = tmpSrc02;
		tmpSrc02 = tmpSrc03;
		tmpSrc03 = tmpSrc04;
		if (y + 2 < lHeight - 1)
		{
			tmpSrc04 += lSrcPitch;
		}
	}

	return;
}

#ifdef MCV_MULTI_THREAD
MVoid thread_FilterErode5x5u8(MVoid *para)
{
	if (MNull != para)
	{
		FilterMophoOp_Para *pPara = (FilterMophoOp_Para *)para;
		local_FilterErode5x5u8(pPara->pBufImg, pPara->pSrc, pPara->lSrcPitch, pPara->pDst, pPara->lDstPitch,
							   pPara->lWidth, pPara->lHeight, pPara->lStartRow, pPara->lEndRow);
	}
	return;
}
#endif


MInt32 FilterErode5x5u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch,
						MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lret = MOK;
	MByte *pBufImg[16] = { MNull };

#ifdef MCV_MULTI_THREAD
	const MInt32 lTaskNum = lHeight > 16 ? 16 : 8;;
	MInt32 lTaskId[16] = { MNull };
	FilterMophoOp_Para para[16] = { MNull };
	MInt32 lBlockH = (lHeight / lTaskNum) >> 1 << 1;
	MInt32 k = 0;

	pBufImg[0] = (MByte *)MMemAlloc(hMemMgr, lTaskNum * (lWidth + 4) * sizeof(MByte));
	if (!pBufImg[0])
	{
		return MERR_NO_MEMORY;
	}

	for (k = 1; k < lTaskNum; ++k)
	{
		pBufImg[k] = pBufImg[k - 1] + lWidth + 4;
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		para[k].pBufImg = pBufImg[k];
		para[k].pSrc = pSrc;
		para[k].lSrcPitch = lSrcPitch;
		para[k].pDst = pDst;
		para[k].lDstPitch = lDstPitch;
		para[k].lWidth = lWidth;
		para[k].lHeight = lHeight;
		para[k].lStartRow = k * lBlockH;
		para[k].lEndRow = (k + 1) * lBlockH;
	}
	para[lTaskNum - 1].lEndRow = lHeight;

	for (k = 0; k < lTaskNum; ++k)
	{
		lTaskId[k] = mcvAddTask(mcvParallelMonitor, thread_FilterErode5x5u8, (MVoid *)(para + k));
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		lret = mcvWaitTask(mcvParallelMonitor, lTaskId[k]);
		if (MOK != lret)
		{
			lret = MERR_BAD_STATE;
			goto exit;
		}
	}
#else
	pBufImg[0] = (MByte *)MMemAlloc(hMemMgr, (lWidth + 4) * sizeof(MByte));
	if (!pBufImg[0])
	{
		return MERR_NO_MEMORY;
	}

	local_FilterErode5x5u8(pBufImg[0], pSrc, lSrcPitch, pDst, lDstPitch, lWidth, lHeight, 0, lHeight);
#endif

exit:
	if (pBufImg[0])
	{
		MMemFree(hMemMgr, pBufImg[0]);
		pBufImg[0] = MNull;
	}
	return lret;
}

MVoid ver_line_max_process_5x5(MByte* tmpSrc00, MByte* tmpSrc01, MByte* tmpSrc02, MByte *tmpSrc03, MByte *tmpSrc04, MByte* pTmpBuf, MInt32 lWidth)
{
	MInt32 x;
	MInt32 lval;

#ifdef __ARM_NEON__
	uint8x16_t srcData00, srcData01, srcData02, srcData03, srcData04, dstData;
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < lWidth - 15; x += 16)
	{
		srcData00 = vld1q_u8(tmpSrc00 + x);
		srcData01 = vld1q_u8(tmpSrc01 + x);
		srcData02 = vld1q_u8(tmpSrc02 + x);
		srcData03 = vld1q_u8(tmpSrc03 + x);
		srcData04 = vld1q_u8(tmpSrc04 + x);

		dstData = vmaxq_u8(srcData00, srcData01);
		dstData = vmaxq_u8(dstData, srcData02);
		dstData = vmaxq_u8(dstData, srcData03);
		dstData = vmaxq_u8(dstData, srcData04);
		vst1q_u8(pTmpBuf + x, dstData);
	}
#endif //__ARM_NEON__

	for (; x < lWidth; ++x)
	{
		lval = MAX(tmpSrc00[x], tmpSrc01[x]);
		lval = MAX(lval, tmpSrc02[x]);
		lval = MAX(lval, tmpSrc03[x]);
		lval = MAX(lval, tmpSrc04[x]);

		pTmpBuf[x] = lval;
	}
	return;
}

MVoid hor_line_max_process_5x5(MByte* pTmpBuf, MByte* tmpDst, MInt32 lWidth)
{
	MInt32 x;
	MInt32 lval;

#ifdef __ARM_NEON__
	uint8x16_t srcData00, srcData01, srcData02, srcData03, srcData04, dstData;
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < lWidth - 15; x += 16)
	{
		srcData00 = vld1q_u8(pTmpBuf + x - 2);
		srcData01 = vld1q_u8(pTmpBuf + x - 1);
		srcData02 = vld1q_u8(pTmpBuf + x);
		srcData03 = vld1q_u8(pTmpBuf + x + 1);
		srcData04 = vld1q_u8(pTmpBuf + x + 2);

		dstData = vmaxq_u8(srcData00, srcData01);
		dstData = vmaxq_u8(dstData, srcData02);
		dstData = vmaxq_u8(dstData, srcData03);
		dstData = vmaxq_u8(dstData, srcData04);
		vst1q_u8(tmpDst + x, dstData);
	}
#endif //__ARM_NEON__

	for (; x < lWidth; ++x)
	{
		lval = MAX(pTmpBuf[x - 2], pTmpBuf[x - 1]);
		lval = MAX(lval, pTmpBuf[x]);
		lval = MAX(lval, pTmpBuf[x + 1]);
		lval = MAX(lval, pTmpBuf[x + 2]);
		tmpDst[x] = lval;
	}
	return;
}

MVoid local_FilterDilate5x5u8(MByte *pBufImg, MByte* pSrc, MInt32 lSrcPitch, MByte* pDst, MInt32 lDstPitch,
							  MInt32 lWidth, MInt32 lHeight, MInt32 lStartRow, MInt32 lEndRow)
{
	MInt32 x, y;
	MByte* tmpSrc00 = MNull, *tmpSrc01 = MNull, *tmpSrc02 = MNull, *tmpSrc03, *tmpSrc04;
	MByte *pTmpBuf = pBufImg + 2;
	MByte* tmpDst = pDst + lStartRow * lDstPitch;

	tmpSrc02 = pSrc + lStartRow * lSrcPitch;
	tmpSrc01 = (lStartRow < 1) ? tmpSrc02 : (tmpSrc02 - lSrcPitch);
	tmpSrc00 = (lStartRow < 2) ? tmpSrc01 : (tmpSrc01 - lSrcPitch);
	tmpSrc03 = (lStartRow + 1 >= lHeight) ? tmpSrc02 : (tmpSrc02 + lSrcPitch);
	tmpSrc04 = (lStartRow + 2 >= lHeight) ? tmpSrc03 : (tmpSrc03 + lSrcPitch);
	for (y = lStartRow; y < lEndRow; ++y, tmpDst += lDstPitch)
	{
		ver_line_max_process_5x5(tmpSrc00, tmpSrc01, tmpSrc02, tmpSrc03, tmpSrc04, pTmpBuf, lWidth);
		pTmpBuf[-2] = pTmpBuf[-1] = pTmpBuf[0];
		pTmpBuf[lWidth + 1] = pTmpBuf[lWidth] = pTmpBuf[lWidth - 1];
		hor_line_max_process_5x5(pTmpBuf, tmpDst, lWidth);

		tmpSrc00 = tmpSrc01;
		tmpSrc01 = tmpSrc02;
		tmpSrc02 = tmpSrc03;
		tmpSrc03 = tmpSrc04;
		if (y + 2 < lHeight - 1)
		{
			tmpSrc04 += lSrcPitch;
		}
	}

	return;
}

#ifdef MCV_MULTI_THREAD
MVoid thread_FilterDilate5x5u8(MVoid *para)
{
	if (MNull != para)
	{
		FilterMophoOp_Para *pPara = (FilterMophoOp_Para *)para;
		local_FilterDilate5x5u8(pPara->pBufImg, pPara->pSrc, pPara->lSrcPitch, pPara->pDst, pPara->lDstPitch,
								pPara->lWidth, pPara->lHeight, pPara->lStartRow, pPara->lEndRow);
	}
	return;
}
#endif


MInt32 FilterDilate5x5u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch,
						 MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lret = MOK;
	MByte *pBufImg[16] = { MNull };

#ifdef MCV_MULTI_THREAD
	const MInt32 lTaskNum = lHeight > 16 ? 16 : 8;;
	MInt32 lTaskId[16] = { MNull };
	FilterMophoOp_Para para[16] = { MNull };
	MInt32 lBlockH = (lHeight / lTaskNum) >> 1 << 1;
	MInt32 k = 0;

	pBufImg[0] = (MByte *)MMemAlloc(hMemMgr, lTaskNum * (lWidth + 4) * sizeof(MByte));
	if (!pBufImg[0])
	{
		return MERR_NO_MEMORY;
	}

	for (k = 1; k < lTaskNum; ++k)
	{
		pBufImg[k] = pBufImg[k - 1] + lWidth + 4;
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		para[k].pBufImg = pBufImg[k];
		para[k].pSrc = pSrc;
		para[k].lSrcPitch = lSrcPitch;
		para[k].pDst = pDst;
		para[k].lDstPitch = lDstPitch;
		para[k].lWidth = lWidth;
		para[k].lHeight = lHeight;
		para[k].lStartRow = k * lBlockH;
		para[k].lEndRow = (k + 1) * lBlockH;
	}
	para[lTaskNum - 1].lEndRow = lHeight;

	for (k = 0; k < lTaskNum; ++k)
	{
		lTaskId[k] = mcvAddTask(mcvParallelMonitor, thread_FilterDilate5x5u8, (MVoid *)(para + k));
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		lret = mcvWaitTask(mcvParallelMonitor, lTaskId[k]);
		if (MOK != lret)
		{
			lret = MERR_BAD_STATE;
			goto exit;
		}
	}
#else
	pBufImg[0] = (MByte *)MMemAlloc(hMemMgr, (lWidth + 4) * sizeof(MByte));
	if (!pBufImg[0])
	{
		return MERR_NO_MEMORY;
	}

	local_FilterDilate5x5u8(pBufImg[0], pSrc, lSrcPitch, pDst, lDstPitch, lWidth, lHeight, 0, lHeight);
#endif

exit:
	if (pBufImg[0])
	{
		MMemFree(hMemMgr, pBufImg[0]);
		pBufImg[0] = MNull;
	}
	return lret;
}

MInt32 FilterOpenMopho5x5u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *pSrc, MInt32 lSrcPitch,
							MByte *pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lret = MOK;
	MByte *pTmpBuf = (MByte *)MMemAlloc(hMemMgr, lHeight * lSrcPitch * sizeof(MByte));
	if (!pTmpBuf)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	lret = FilterErode5x5u8(hMemMgr, mcvParallelMonitor, pSrc, lSrcPitch, pTmpBuf, lSrcPitch, lWidth, lHeight);
	if (MOK != lret)
		goto exit;

	lret = FilterDilate5x5u8(hMemMgr, mcvParallelMonitor, pTmpBuf, lSrcPitch, pDst, lDstPitch, lWidth, lHeight);

exit:
	if (pTmpBuf)
	{
		MMemFree(hMemMgr, pTmpBuf);
		pTmpBuf = MNull;
	}
	return lret;
}

MVoid ver_line_max_process_7x7(MByte* tmpSrc00, MByte* tmpSrc01, MByte* tmpSrc02, MByte* tmpSrc03, MByte* tmpSrc04, MByte* tmpSrc05, MByte* tmpSrc06, MByte* pTmpBuf, MInt32 lWidth)
{
	MInt32 x;
	for (x = 0; x < lWidth; x++)
	{
		MInt32 lval = MAX(tmpSrc00[x], tmpSrc01[x]);
		lval = MAX(lval, tmpSrc02[x]);
		lval = MAX(lval, tmpSrc03[x]);
		lval = MAX(lval, tmpSrc04[x]);
		lval = MAX(lval, tmpSrc05[x]);
		lval = MAX(lval, tmpSrc06[x]);
		pTmpBuf[x] = lval;
	}
	return;
}

MVoid hor_line_max_process_7x7(MByte* pBufImg, MByte* tmpDst, MInt32 lWidth)
{
	MInt32 x;
	for (x = 0; x < lWidth;x++)
	{
		MInt32 lval = MAX(pBufImg[x-3], pBufImg[x-2]);
		lval = MAX(lval, pBufImg[x-1]);
		lval = MAX(lval, pBufImg[x]);
		lval = MAX(lval, pBufImg[x+1]);
		lval = MAX(lval, pBufImg[x+2]);
		lval = MAX(lval, pBufImg[x+3]);
		tmpDst[x] = lval;
	}
	return;
}


MInt32 FilterDilate7x7u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch, MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lret = MOK;
	MByte* pBufImg = MNull;
	MByte* pTmpBuf = MNull;
	MInt32 x, y;
	MByte* tmpSrc00, *tmpSrc01, *tmpSrc02, *tmpSrc03, *tmpSrc04, *tmpSrc05, *tmpSrc06;

	pBufImg = (MByte*)MMemAlloc(hMemMgr, (lWidth + 6)*sizeof(MByte));
	if (MNull == pBufImg)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	pTmpBuf = pBufImg + 3;

	tmpSrc00 = tmpSrc01 = tmpSrc02 = tmpSrc03 = pSrc;
	tmpSrc04 = tmpSrc03 + lSrcPitch;
	tmpSrc05 = tmpSrc04 + lSrcPitch;
	tmpSrc06 = tmpSrc05 + lSrcPitch;
	for (y = 0; y < lHeight; y++)
	{
		MByte* tmpDst = pDst + y * lDstPitch;
		ver_line_max_process_7x7(tmpSrc00, tmpSrc01, tmpSrc02, tmpSrc03, tmpSrc04, tmpSrc05, tmpSrc06, pTmpBuf, lWidth);
		pTmpBuf[-3] = pTmpBuf[-2] = pTmpBuf[-1] = pTmpBuf[0];
		pTmpBuf[lWidth] = pTmpBuf[lWidth + 1] = pTmpBuf[lWidth + 2] = pTmpBuf[lWidth - 1];
		hor_line_max_process_7x7(pTmpBuf, tmpDst, lWidth);
	
		tmpSrc00 = tmpSrc01;
		tmpSrc01 = tmpSrc02;
		tmpSrc02 = tmpSrc03;
		tmpSrc03 = tmpSrc04;
		tmpSrc04 = tmpSrc05;
		tmpSrc05 = tmpSrc06;
		if (y + 3 < lHeight-1)
		{
			tmpSrc06 += lSrcPitch;
		}
	}
exit:
	if (pBufImg)
	{
		MMemFree(hMemMgr, pBufImg);
		pBufImg = MNull;
	}
	return lret;
}

MVoid ver_line_max_process_9x9(MByte* tmpSrc00, MByte* tmpSrc01, MByte* tmpSrc02, MByte* tmpSrc03, MByte* tmpSrc04,
	MByte* tmpSrc05, MByte* tmpSrc06, MByte* tmpSrc07, MByte* tmpSrc08, MByte* pTmpBuf, MInt32 lWidth)
{
	MInt32 x;
	for (x = 0; x < lWidth; x++)
	{
		MInt32 lval = MAX(tmpSrc00[x], tmpSrc01[x]);
		lval = MAX(lval, tmpSrc02[x]);
		lval = MAX(lval, tmpSrc03[x]);
		lval = MAX(lval, tmpSrc04[x]);
		lval = MAX(lval, tmpSrc05[x]);
		lval = MAX(lval, tmpSrc06[x]);
		lval = MAX(lval, tmpSrc07[x]);
		lval = MAX(lval, tmpSrc08[x]);
		pTmpBuf[x] = lval;
	}
	return;
}

MVoid hor_line_max_process_9x9(MByte* pBufImg, MByte* tmpDst, MInt32 lWidth)
{
	MInt32 x;
	for (x = 0; x < lWidth; x++)
	{
		MInt32 lval = MAX(pBufImg[x - 4], pBufImg[x - 3]);
		lval = MAX(lval, pBufImg[x - 2]);
		lval = MAX(lval, pBufImg[x - 1]);
		lval = MAX(lval, pBufImg[x]);
		lval = MAX(lval, pBufImg[x + 1]);
		lval = MAX(lval, pBufImg[x + 2]);
		lval = MAX(lval, pBufImg[x + 3]);
		lval = MAX(lval, pBufImg[x + 4]);
		tmpDst[x] = lval;
	}
	return;
}

MInt32 FilterDilate9x9u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch, MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lret = MOK;
	MByte* pBufImg = MNull;
	MByte* pTmpBuf = MNull;
	MInt32 x, y;
	MByte* tmpSrc00, *tmpSrc01, *tmpSrc02, *tmpSrc03, *tmpSrc04, *tmpSrc05, *tmpSrc06, *tmpSrc07, *tmpSrc08;

	pBufImg = (MByte*)MMemAlloc(hMemMgr, (lWidth + 8)*sizeof(MByte));
	if (MNull == pBufImg)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	pTmpBuf = pBufImg + 4;

	tmpSrc00 = tmpSrc01 = tmpSrc02 = tmpSrc03 = tmpSrc04 = pSrc;
	tmpSrc05 = tmpSrc04 + lSrcPitch;
	tmpSrc06 = tmpSrc05 + lSrcPitch;
	tmpSrc07 = tmpSrc06 + lSrcPitch;
	tmpSrc08 = tmpSrc07 + lSrcPitch;
	for (y = 0; y < lHeight; y++)
	{
		MByte* tmpDst = pDst + y * lDstPitch;
		ver_line_max_process_9x9(tmpSrc00, tmpSrc01, tmpSrc02, tmpSrc03, tmpSrc04, tmpSrc05, tmpSrc06, tmpSrc07, tmpSrc08, pTmpBuf, lWidth);
		pTmpBuf[-4] = pTmpBuf[-3] = pTmpBuf[-2] = pTmpBuf[-1] = pTmpBuf[0];
		pTmpBuf[lWidth] = pTmpBuf[lWidth + 1] = pTmpBuf[lWidth + 2] = pTmpBuf[lWidth + 3] = pTmpBuf[lWidth - 1];
		hor_line_max_process_9x9(pTmpBuf, tmpDst, lWidth);

		tmpSrc00 = tmpSrc01;
		tmpSrc01 = tmpSrc02;
		tmpSrc02 = tmpSrc03;
		tmpSrc03 = tmpSrc04;
		tmpSrc04 = tmpSrc05;
		tmpSrc05 = tmpSrc06;
		tmpSrc06 = tmpSrc07;
		tmpSrc07 = tmpSrc08;
		if (y + 4 < lHeight - 1)
		{
			tmpSrc08 += lSrcPitch;
		}
	}
exit:
	if (pBufImg)
	{
		MMemFree(hMemMgr, pBufImg);
		pBufImg = MNull;
	}
	return lret;
}

MVoid ver_line_min_process_9x9(MByte* tmpSrc00, MByte* tmpSrc01, MByte* tmpSrc02, MByte* tmpSrc03, MByte* tmpSrc04,
							   MByte* tmpSrc05, MByte* tmpSrc06, MByte* tmpSrc07, MByte* tmpSrc08, MByte* pTmpBuf, MInt32 lWidth)
{
	MInt32 x;
	for (x = 0; x < lWidth; x++)
	{
		MInt32 lval = MIN(tmpSrc00[x], tmpSrc01[x]);
		lval = MIN(lval, tmpSrc02[x]);
		lval = MIN(lval, tmpSrc03[x]);
		lval = MIN(lval, tmpSrc04[x]);
		lval = MIN(lval, tmpSrc05[x]);
		lval = MIN(lval, tmpSrc06[x]);
		lval = MIN(lval, tmpSrc07[x]);
		lval = MIN(lval, tmpSrc08[x]);
		pTmpBuf[x] = lval;
	}
	return;
}

MVoid hor_line_min_process_9x9(MByte* pBufImg, MByte* tmpDst, MInt32 lWidth)
{
	MInt32 x;
	for (x = 0; x < lWidth; x++)
	{
		MInt32 lval = MIN(pBufImg[x - 4], pBufImg[x - 3]);
		lval = MIN(lval, pBufImg[x - 2]);
		lval = MIN(lval, pBufImg[x - 1]);
		lval = MIN(lval, pBufImg[x]);
		lval = MIN(lval, pBufImg[x + 1]);
		lval = MIN(lval, pBufImg[x + 2]);
		lval = MIN(lval, pBufImg[x + 3]);
		lval = MIN(lval, pBufImg[x + 4]);
		tmpDst[x] = lval;
	}
	return;
}

MInt32 FilterErode9x9u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch, MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight)
{
	MInt32 lret = MOK;
	MByte* pBufImg = MNull;
	MByte* pTmpBuf = MNull;
	MInt32 x, y;
	MByte* tmpSrc00, *tmpSrc01, *tmpSrc02, *tmpSrc03, *tmpSrc04, *tmpSrc05, *tmpSrc06, *tmpSrc07, *tmpSrc08;

	pBufImg = (MByte*)MMemAlloc(hMemMgr, (lWidth + 8)*sizeof(MByte));
	if (MNull == pBufImg)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	pTmpBuf = pBufImg + 4;

	tmpSrc00 = tmpSrc01 = tmpSrc02 = tmpSrc03 = tmpSrc04 = pSrc;
	tmpSrc05 = tmpSrc04 + lSrcPitch;
	tmpSrc06 = tmpSrc05 + lSrcPitch;
	tmpSrc07 = tmpSrc06 + lSrcPitch;
	tmpSrc08 = tmpSrc07 + lSrcPitch;
	for (y = 0; y < lHeight; y++)
	{
		MByte* tmpDst = pDst + y * lDstPitch;
		ver_line_min_process_9x9(tmpSrc00, tmpSrc01, tmpSrc02, tmpSrc03, tmpSrc04, tmpSrc05, tmpSrc06, tmpSrc07, tmpSrc08, pTmpBuf, lWidth);
		pTmpBuf[-4] = pTmpBuf[-3] = pTmpBuf[-2] = pTmpBuf[-1] = pTmpBuf[0];
		pTmpBuf[lWidth] = pTmpBuf[lWidth + 1] = pTmpBuf[lWidth + 2] = pTmpBuf[lWidth + 3] = pTmpBuf[lWidth - 1];
		hor_line_min_process_9x9(pTmpBuf, tmpDst, lWidth);

		tmpSrc00 = tmpSrc01;
		tmpSrc01 = tmpSrc02;
		tmpSrc02 = tmpSrc03;
		tmpSrc03 = tmpSrc04;
		tmpSrc04 = tmpSrc05;
		tmpSrc05 = tmpSrc06;
		tmpSrc06 = tmpSrc07;
		tmpSrc07 = tmpSrc08;
		if (y + 4 < lHeight - 1)
		{
			tmpSrc08 += lSrcPitch;
		}
	}
exit:
	if (pBufImg)
	{
		MMemFree(hMemMgr, pBufImg);
		pBufImg = MNull;
	}
	return lret;
}