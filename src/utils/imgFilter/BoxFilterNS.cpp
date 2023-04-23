
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
#include "asvloffscreen.h"
#include "merror.h"
#include "mthread.h"
#include "math.h"
#include "mobilecv.h"
//#include <NEON_2_SSE.h>
#include "imagebase.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
const int DENOISE_TASK_NUM = 1; //todo: 未初始化变量值，暂且初始化，后续再看 4.9 by jck
#if defined(MCV_MULTI_THREAD)

typedef struct _tag_IMG_BOX_32I {
    MInt32			task_ID;
    MRESULT			errCode;
    MInt32			lImgWidth;
    MInt32			lImgHeight;
    MInt32			*pSrcImg;
    MInt32			lSrcPitch;
    MInt32			*pDstImg;
    MInt32			lDstPitch;
    MInt32			lStartRow;
    MInt32			lEndRow;
    MInt32          lRadius;
    MInt32          *pBoxSumBuf;
} Img_Box_32I, *LP_Img_Box_32I;

#endif

#if defined(MCV_MULTI_THREAD)

typedef struct _tag_IMG_BOX_32F {
    MInt32			task_ID;
    MRESULT			errCode;
    MInt32			lImgWidth;
    MInt32			lImgHeight;
    MFloat			*pSrcImg;
    MInt32			lSrcPitch;
    MFloat			*pDstImg;
    MInt32			lDstPitch;
    MInt32			lStartRow;
    MInt32			lEndRow;
    MInt32          lRadius;
    MFloat          *pBoxSumBuf;
} Img_Box_32F, *LP_Img_Box_32F;

#endif

// box filter for 32bit float
static MVoid boxBlurProcessRow_add_NS32F(MFloat* pSumLine, MFloat* addSrc, MInt32 lImgWidth, MInt32 lRadius)
{
    MInt32   i;
    MFloat  nValue;
    MInt32   DWidth = lImgWidth>>2;
    MFloat   nSum = 0;

    nValue = addSrc[0];
    pSumLine[0] = 0;
    pSumLine++;
    for (i = 0; i < lRadius; i++)
    {
        nSum += nValue;
        (pSumLine++)[0] += nSum;
    }
    for (i = 0; i < DWidth; i++)
    {
        nValue = addSrc[4*i];
        nSum += (nValue);
        (pSumLine++)[0] += nSum;

        nValue = addSrc[4 * i + 1];
        nSum += (nValue);
        (pSumLine++)[0] += nSum;

        nValue = addSrc[4 * i + 2];
        nSum += (nValue);
        (pSumLine++)[0] += nSum;

        nValue = addSrc[4 * i + 3];
        nSum += (nValue);
        (pSumLine++)[0] += nSum;
    }
    for (i = DWidth<<2; i < lImgWidth; i++)
    {
        nValue = addSrc[i];
        nSum +=  nValue;
        (pSumLine++)[0] += nSum;
    }
    nValue = addSrc[lImgWidth-1];
    for (i = 0; i < lRadius; i++)
    {
        nSum +=  nValue;
        (pSumLine++)[0] += nSum;
    }
    return;
}

static MVoid boxBlurProcessRow_add_sub_NS32F(MFloat* pSumLine, MFloat* addSrc, MFloat* subSrc, MInt32 lImgWidth, MInt32 lRadius)
{
    MInt32   i;
    MFloat  nAValue, nSValue;
    MFloat   lVal;
    MInt32   DWidth = lImgWidth>>2;
    MFloat   nSum = 0;

    lVal = addSrc[0] - subSrc[0];
    pSumLine[0] = 0;
    pSumLine++;
    for (i = 0; i < lRadius; i++)
    {
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }
    for (i = 0; i < DWidth; i++)
    {
        nAValue = addSrc[4*i];
        nSValue = subSrc[4*i];
        lVal = (nAValue) - (nSValue);
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        nAValue = addSrc[4 * i + 1];
        nSValue = subSrc[4 * i + 1];
        lVal =  (nAValue) - (nSValue);
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        nAValue = addSrc[4 * i + 2];
        nSValue = subSrc[4 * i + 2];
        lVal =  (nAValue) - (nSValue);
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        nAValue = addSrc[4 * i + 3];
        nSValue = subSrc[4 * i + 3];
        lVal = (nAValue) - (nSValue);
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }
    for (i = DWidth<<2; i < lImgWidth; i++)
    {
        lVal = addSrc[i] - subSrc[i];
        nSum +=  lVal;
        (pSumLine++)[0] += nSum;
    }
    for (i = 0; i < lRadius; i++)
    {
        nSum +=  lVal;
        (pSumLine++)[0] += nSum;
    }
    return;
}

static MVoid BoxFilterRow_NS32F(MFloat* pDst, MFloat* pBoxSumBuf, MInt32 lImgWidth, MInt32 lRadius)
{
    MInt32 x;
    MFloat lbVal   = 0.0;
    MInt32 lboxSize = lRadius*2+1;

    for (x = 0; x < lImgWidth; x++)
    {
        lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
        lbVal = lbVal / (lboxSize*lboxSize);
        pDst[x] = lbVal;
        pBoxSumBuf++;
    }
}

static MVoid Image_Box_Stripe_NS32F(MFloat* pSrcImg, MInt32 lImgWidth, MInt32 lImgHeight, MInt32 lSrcPitch,
                                    MFloat* pDstImg, MInt32 lDstPitch, MInt32 lStartRow, MInt32 lEndRow, MInt32 lRadius, MFloat *pBoxSumBuf)
{
    MInt32 lPreLine=MIN(lRadius,lStartRow),
            lNexLine=MIN(lRadius,lImgHeight-lEndRow);
    MInt32 lTop = lStartRow - lPreLine;
    MInt32 lBot = lEndRow + lNexLine;
    MFloat *tmpaddSrc = pSrcImg + lTop * lSrcPitch;
    MFloat *tmpsubSrc = pSrcImg + lTop * lSrcPitch;
    MFloat *tmpSrc = pSrcImg + lStartRow * lSrcPitch;
    MFloat *tmpDst = pDstImg + lStartRow * lDstPitch;

    MInt32 line = lStartRow - lRadius;

    MInt32 lboxSize = lRadius*2+1;

    MMemSet(pBoxSumBuf, 0, (lImgWidth + lRadius*2 + 1 + 100) * sizeof(MFloat));

    for (; line < lTop; line++)
    {
        boxBlurProcessRow_add_NS32F(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
    }
    for (; line < lStartRow + lRadius; line++)
    {
        boxBlurProcessRow_add_NS32F(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
        tmpaddSrc += lSrcPitch;
    }
    {
        boxBlurProcessRow_add_NS32F(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
        BoxFilterRow_NS32F(tmpDst, pBoxSumBuf, lImgWidth, lRadius);
        tmpaddSrc += lSrcPitch;
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
        line++;
    }
    for (; line < lBot; line++)
    {
        boxBlurProcessRow_add_sub_NS32F(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
        BoxFilterRow_NS32F(tmpDst, pBoxSumBuf, lImgWidth, lRadius);
        tmpaddSrc += lSrcPitch;
        if (line >= lTop + lRadius * 2 + 1)
        {
            tmpsubSrc += lSrcPitch;
        }
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
    }
    tmpaddSrc -= lSrcPitch;
    for (; line < lEndRow + lRadius; line++)
    {
        boxBlurProcessRow_add_sub_NS32F(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
        BoxFilterRow_NS32F(tmpDst, pBoxSumBuf, lImgWidth, lRadius);
        tmpsubSrc += lSrcPitch;
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
    }
}

#if defined(MCV_MULTI_THREAD)
MVoid thread_box_filter_32f(MVoid* pParam)
{
    Img_Box_32F *Filter = (Img_Box_32F*)pParam;
    MInt32 lret = MOK;

    Image_Box_Stripe_NS32F(Filter->pSrcImg, Filter->lImgWidth, Filter->lImgHeight,
                           Filter->lSrcPitch, Filter->pDstImg, Filter->lDstPitch, Filter->lStartRow, Filter->lEndRow,
                           Filter->lRadius, Filter->pBoxSumBuf);

    Filter->errCode = lret;
}
#endif

MRESULT Box_Filter_NS32F(MHandle hMemMgr, MHandle mcvParallelMonitor, MFloat *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
                         MInt32 lSrcLineBytes,MFloat *pDstBuf,MInt32 lDstLineBytes, MInt32 lRadius)
{
    MRESULT lret = MOK;
    MInt32 i;
    MFloat *box_sum_buf[DENOISE_TASK_NUM] = { MNull };

    box_sum_buf[0] = (MFloat*)MMemAlloc(hMemMgr, (lWidth + lRadius * 2 + 1 + 100) * DENOISE_TASK_NUM * sizeof(MFloat));
    if (MNull == box_sum_buf[0])
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    for (i = 1; i < DENOISE_TASK_NUM; ++i)
    {
        box_sum_buf[i] = box_sum_buf[0] + (lWidth + lRadius * 2 + 1 + 100) * i;
    }

#ifdef MCV_MULTI_THREAD
    {
        MInt32 lSize;
        const MInt32 lTask_Num = DENOISE_TASK_NUM;
        MInt32 taskID[lTask_Num] = { 0 };
        Img_Box_32F pParams[lTask_Num] = { 0 };

        lSize = lHeight / lTask_Num;
        lSize = lSize >> 1 << 1;

        pParams[0].lStartRow = 0;
        pParams[0].lEndRow = lSize;
        for (i = 1; i < lTask_Num; i++)
        {
            pParams[i].lStartRow = i*lSize;
            pParams[i].lEndRow = (i + 1)*lSize;
        }
        pParams[i - 1].lEndRow = lHeight;

        for (i = 0; i < lTask_Num; i++)
        {
            pParams[i].task_ID = i;
            pParams[i].lImgWidth = lWidth;
            pParams[i].lImgHeight = lHeight;
            pParams[i].pSrcImg = pSrcBuf;
            pParams[i].lSrcPitch = lSrcLineBytes;
            pParams[i].pDstImg = pDstBuf;
            pParams[i].lDstPitch = lDstLineBytes;
            pParams[i].lRadius = lRadius;
            pParams[i].pBoxSumBuf = box_sum_buf[i];
        }

        for (i = 0; i < lTask_Num; i++)
        {
            taskID[i] = mcvAddTask(mcvParallelMonitor, thread_box_filter_32f, (MVoid*)&pParams[i]);
        }
        for (i = 0; i < lTask_Num; i++)
        {
            mcvWaitTask(mcvParallelMonitor, taskID[i]);
        }
    }
#else
    {
		Image_Box_Stripe_NS32F(pSrcBuf, lWidth, lHeight,
			lSrcLineBytes, pDstBuf, lDstLineBytes, 0, lHeight, lRadius, box_sum_buf[0]);
	}
#endif
    exit:

    if (box_sum_buf[0])
    {
        MMemFree(hMemMgr, box_sum_buf[0]);
        box_sum_buf[0] = MNull;
    }
    return lret;
}

//box filter for 32bit int
static MVoid boxBlurProcessRow_add_NS32I(MInt32* pSumLine, MInt32* addSrc, MInt32 lImgWidth, MInt32 lRadius)
{
    MInt32   i;
    MInt32   nValue;
    MInt32   DWidth = lImgWidth >> 2;
    MInt32   nSum = 0;

    nValue = addSrc[0];
    pSumLine[0] = 0;
    pSumLine++;
    for (i = 0; i < lRadius; i++)
    {
        nSum += nValue;
        (pSumLine++)[0] += nSum;
    }
    for (i = 0; i < DWidth; i++)
    {
        nValue = addSrc[4 * i];
        nSum += (nValue);
        (pSumLine++)[0] += nSum;

        nValue = addSrc[4 * i + 1];
        nSum += (nValue);
        (pSumLine++)[0] += nSum;

        nValue = addSrc[4 * i + 2];
        nSum += (nValue);
        (pSumLine++)[0] += nSum;

        nValue = addSrc[4 * i + 3];
        nSum += (nValue);
        (pSumLine++)[0] += nSum;
    }
    for (i = DWidth << 2; i < lImgWidth; i++)
    {
        nValue = addSrc[i];
        nSum += nValue;
        (pSumLine++)[0] += nSum;
    }
    nValue = addSrc[lImgWidth - 1];
    for (i = 0; i < lRadius; i++)
    {
        nSum += nValue;
        (pSumLine++)[0] += nSum;
    }
    return;
}

static MVoid boxBlurProcessRow_add_sub_NS32I(MInt32* pSumLine, MInt32* addSrc, MInt32* subSrc, MInt32 lImgWidth, MInt32 lRadius)
{
    MInt32   i;
    MInt32   nAValue, nSValue;
    MInt32   lVal;
    MInt32   DWidth = lImgWidth >> 2;
    MInt32   nSum = 0;

    lVal = addSrc[0] - subSrc[0];
    pSumLine[0] = 0;
    pSumLine++;
    for (i = 0; i < lRadius; i++)
    {
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }
    i = 0;
#if CV_NEON
    {
		int32x4x4_t tmpaddsrc, tmpsubsrc;
		int32x4_t sug_dif00, sug_dif01, sug_dif02, sug_dif03;
		int32x4x2_t tmpsum00, tmpsum01;
		int32x4x2_t dstsum00, dstsum01;
		int32x4_t currsum_32x4;
		int32x4_t pSum_32x4;
		currsum_32x4 = vdupq_n_s32(nSum);

		for (i = 0; i < DWidth - 4; i += 4)
		{
			tmpaddsrc = vld4q_s32(addSrc + 4*i);
			tmpsubsrc = vld4q_s32(subSrc + 4*i);

			sug_dif00 = vsubq_s32(tmpaddsrc.val[0], tmpsubsrc.val[0]);
			sug_dif01 = vsubq_s32(tmpaddsrc.val[1], tmpsubsrc.val[1]);
			sug_dif02 = vsubq_s32(tmpaddsrc.val[2], tmpsubsrc.val[2]);
			sug_dif03 = vsubq_s32(tmpaddsrc.val[3], tmpsubsrc.val[3]);

			sug_dif01 = vaddq_s32(sug_dif00, sug_dif01);
			sug_dif02 = vaddq_s32(sug_dif01, sug_dif02);
			sug_dif03 = vaddq_s32(sug_dif02, sug_dif03);

			tmpsum00 = vzipq_s32(sug_dif00, sug_dif02);
			tmpsum01 = vzipq_s32(sug_dif01, sug_dif03);
			dstsum00 = vzipq_s32(tmpsum00.val[0], tmpsum01.val[0]);
			dstsum01 = vzipq_s32(tmpsum00.val[1], tmpsum01.val[1]);

			currsum_32x4 = vdupq_lane_s32(vget_high_s32(currsum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			currsum_32x4 = vaddq_s32(currsum_32x4, dstsum00.val[0]);
			pSum_32x4 = vaddq_s32(pSum_32x4, currsum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);
			pSumLine += 4;

			currsum_32x4 = vdupq_lane_s32(vget_high_s32(currsum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			currsum_32x4 = vaddq_s32(currsum_32x4, dstsum00.val[1]);
			pSum_32x4 = vaddq_s32(pSum_32x4, currsum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);
			pSumLine += 4;

			currsum_32x4 = vdupq_lane_s32(vget_high_s32(currsum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			currsum_32x4 = vaddq_s32(currsum_32x4, dstsum01.val[0]);
			pSum_32x4 = vaddq_s32(pSum_32x4, currsum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);
			pSumLine += 4;

			currsum_32x4 = vdupq_lane_s32(vget_high_s32(currsum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			currsum_32x4 = vaddq_s32(currsum_32x4, dstsum01.val[1]);
			pSum_32x4 = vaddq_s32(pSum_32x4, currsum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);
			pSumLine += 4;
		}
		nSum = vgetq_lane_s32(currsum_32x4, 3);
	}
#endif
    for (; i < DWidth; i++)
    {
        nAValue = addSrc[4 * i];
        nSValue = subSrc[4 * i];
        lVal = (nAValue)-(nSValue);
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        nAValue = addSrc[4 * i + 1];
        nSValue = subSrc[4 * i + 1];
        lVal = (nAValue)-(nSValue);
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        nAValue = addSrc[4 * i + 2];
        nSValue = subSrc[4 * i + 2];
        lVal = (nAValue)-(nSValue);
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        nAValue = addSrc[4 * i + 3];
        nSValue = subSrc[4 * i + 3];
        lVal = (nAValue)-(nSValue);
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }
    for (i = DWidth << 2; i < lImgWidth; i++)
    {
        lVal = addSrc[i] - subSrc[i];
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }
    for (i = 0; i < lRadius; i++)
    {
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }
    return;
}

static MVoid BoxFilterRow_NS32I(MInt32* pDst, MInt32* pBoxSumBuf, MInt32 lImgWidth, MInt32 lRadius, MFloat invDivNum)
{
    MInt32 x;
    MInt32 lbVal = 0;
    MInt32 lboxSize = lRadius * 2 + 1;

    for (x = 0; x < lImgWidth; x++)
    {
        lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
        lbVal = lbVal * invDivNum;
        pDst[x] = lbVal;
        pBoxSumBuf++;
    }
}

static MVoid Image_Box_Stripe_NS32I(MInt32* pSrcImg, MInt32 lImgWidth, MInt32 lImgHeight, MInt32 lSrcPitch,
                                    MInt32* pDstImg, MInt32 lDstPitch, MInt32 lStartRow, MInt32 lEndRow, MInt32 lRadius, MInt32 *pBoxSumBuf)
{
    MInt32 lPreLine = MIN(lRadius, lStartRow),
            lNexLine = MIN(lRadius, lImgHeight - lEndRow);
    MInt32 lTop = lStartRow - lPreLine;
    MInt32 lBot = lEndRow + lNexLine;
    MInt32 *tmpaddSrc = pSrcImg + lTop * lSrcPitch;
    MInt32 *tmpsubSrc = pSrcImg + lTop * lSrcPitch;
    MInt32 *tmpSrc = pSrcImg + lStartRow * lSrcPitch;
    MInt32 *tmpDst = pDstImg + lStartRow * lDstPitch;

    MInt32 line = lStartRow - lRadius;

    MInt32 lboxSize = lRadius * 2 + 1;
    MFloat invDivNum = 1.0f / (lboxSize * lboxSize);

    MMemSet(pBoxSumBuf, 0, (lImgWidth + lRadius * 2 + 1 + 100) * sizeof(MInt32));

    for (; line < lTop; line++)
    {
        boxBlurProcessRow_add_NS32I(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
    }
    for (; line < lStartRow + lRadius; line++)
    {
        boxBlurProcessRow_add_NS32I(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
        tmpaddSrc += lSrcPitch;
    }
    {
        boxBlurProcessRow_add_NS32I(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
        BoxFilterRow_NS32I(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
        tmpaddSrc += lSrcPitch;
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
        line++;
    }
    for (; line < lBot; line++)
    {
        boxBlurProcessRow_add_sub_NS32I(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
        BoxFilterRow_NS32I(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
        tmpaddSrc += lSrcPitch;
        if (line >= lTop + lRadius * 2 + 1)
        {
            tmpsubSrc += lSrcPitch;
        }
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
    }
    tmpaddSrc -= lSrcPitch;
    for (; line < lEndRow + lRadius; line++)
    {
        boxBlurProcessRow_add_sub_NS32I(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
        BoxFilterRow_NS32I(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
        tmpsubSrc += lSrcPitch;
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
    }
}

#if defined(MCV_MULTI_THREAD)
MVoid thread_box_filter_32i(MVoid* pParam)
{
    Img_Box_32I *Filter = (Img_Box_32I*)pParam;
    MInt32 lret = MOK;

    Image_Box_Stripe_NS32I(Filter->pSrcImg, Filter->lImgWidth, Filter->lImgHeight,
                           Filter->lSrcPitch, Filter->pDstImg, Filter->lDstPitch, Filter->lStartRow, Filter->lEndRow,
                           Filter->lRadius, Filter->pBoxSumBuf);

    Filter->errCode = lret;
}
#endif
MRESULT Box_Filter_NS32I(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
                         MInt32 lSrcLineBytes, MInt32 *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius)
{
    MRESULT lret = MOK;
    MInt32 k;
    MInt32 *box_sum_buf[DENOISE_TASK_NUM] = { MNull };

    box_sum_buf[0] = (MInt32*)MMemAlloc(hMemMgr, (lWidth + lRadius * 2 + 1 + 100) * DENOISE_TASK_NUM * sizeof(MInt32));
    if (MNull == box_sum_buf[0])
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    for (k = 1; k < DENOISE_TASK_NUM; ++k)
    {
        box_sum_buf[k] = box_sum_buf[0] + (lWidth + lRadius * 2 + 1 + 100) * k;
    }

#ifdef MCV_MULTI_THREAD
    {
        MInt32 lSize, i;
        const MInt32 lTask_Num = DENOISE_TASK_NUM;
        MInt32 taskID[lTask_Num] = { 0 };
        Img_Box_32I pParams[lTask_Num] = { 0 };

        lSize = lHeight / lTask_Num;
        lSize = lSize >> 1 << 1;

        pParams[0].lStartRow = 0;
        pParams[0].lEndRow = lSize;
        for (i = 1; i < lTask_Num; i++)
        {
            pParams[i].lStartRow = i*lSize;
            pParams[i].lEndRow = (i + 1)*lSize;
        }
        pParams[i - 1].lEndRow = lHeight;

        for (i = 0; i < lTask_Num; i++)
        {
            pParams[i].task_ID = i;
            pParams[i].lImgWidth = lWidth;
            pParams[i].lImgHeight = lHeight;
            pParams[i].pSrcImg = pSrcBuf;
            pParams[i].lSrcPitch = lSrcLineBytes;
            pParams[i].pDstImg = pDstBuf;
            pParams[i].lDstPitch = lDstLineBytes;
            pParams[i].lRadius = lRadius;
            pParams[i].pBoxSumBuf = box_sum_buf[i];
        }

        for (i = 0; i < lTask_Num; i++)
        {
            taskID[i] = mcvAddTask(mcvParallelMonitor, thread_box_filter_32i, (MVoid*)&pParams[i]);
        }
        for (i = 0; i < lTask_Num; i++)
        {
            mcvWaitTask(mcvParallelMonitor, taskID[i]);
        }
    }
#else
    {
		Image_Box_Stripe_NS32I(pSrcBuf, lWidth, lHeight,
			lSrcLineBytes, pDstBuf, lDstLineBytes, 0, lHeight, lRadius, box_sum_buf[0]);
	}
#endif
    exit:

    if (box_sum_buf[0])
    {
        MMemFree(hMemMgr, box_sum_buf[0]);
        box_sum_buf[0] = MNull;
    }
    return lret;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END




////*******************************************************************************
//Copyright(c) ArcSoft, All right reserved.
//
//This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary
//and confidential information.
//
//The information and code contained in this file is only for authorized ArcSoft
//employees to design, create, modify, or review.
//
//DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
//AUTHORIZATION.
//
//If you are not an intended recipient of this file, you must not copy,
//distribute, modify, or take any action in reliance on it.
//
//If you have received this file in error, please immediately notify ArcSoft and
//permanently delete the original and any copy of any file and any printout
//thereof.
//*******************************************************************************/
//#include "BoxFilterNS.h"
//#include <stdio.h>
//#include <math.h>
//#include <string.h>
//#include <thread>
//#include <vector>
//// mpbase
//#include "merror.h"
//#include "ammem.h"
//#include "asvloffscreen.h"
//#include "amcomdef.h"
//
////#include "mthread.h"
////#include "mobilecv.h"
//#include <NEONvsSSE.h>
//#include "imagebase.h"
//
////#define USE_NEON
//
//#if defined(USE_NEON)
//#include "arm_neon.h"
//#endif
//
//
//// box filter for 32bit float
//MVoid BoxFilterNS::boxBlurProcessRow_add_NS32F(MFloat *pSumLine, MFloat *addSrc, MInt32 lImgWidth, MInt32 lRadius)
//{
//    MInt32 i;
//    MFloat nValue;
//    MInt32 DWidth = lImgWidth >> 2;
//    MFloat nSum = 0;
//
//    nValue = addSrc[ 0 ];
//    pSumLine[ 0 ] = 0;
//    pSumLine++;
//    for(i = 0; i < lRadius; i++)
//    {
//        nSum += nValue;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    for(i = 0; i < DWidth; i++)
//    {
//        nValue = addSrc[ 4 * i ];
//        nSum += ( nValue );
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nValue = addSrc[ 4 * i + 1 ];
//        nSum += ( nValue );
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nValue = addSrc[ 4 * i + 2 ];
//        nSum += ( nValue );
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nValue = addSrc[ 4 * i + 3 ];
//        nSum += ( nValue );
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    for(i = DWidth << 2; i < lImgWidth; i++)
//    {
//        nValue = addSrc[ i ];
//        nSum += nValue;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    nValue = addSrc[ lImgWidth - 1 ];
//    for(i = 0; i < lRadius; i++)
//    {
//        nSum += nValue;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    return;
//}
//
//MVoid BoxFilterNS::boxBlurProcessRow_add_sub_NS32F(MFloat *pSumLine, MFloat *addSrc, MFloat *subSrc, MInt32 lImgWidth, MInt32 lRadius)
//{
//    MInt32 i;
//    MFloat nAValue, nSValue;
//    MFloat lVal;
//    MInt32 DWidth = lImgWidth >> 2;
//    MFloat nSum = 0;
//
//    lVal = addSrc[ 0 ] - subSrc[ 0 ];
//    pSumLine[ 0 ] = 0;
//    pSumLine++;
//    for(i = 0; i < lRadius; i++)
//    {
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    for(i = 0; i < DWidth; i++)
//    {
//        nAValue = addSrc[ 4 * i ];
//        nSValue = subSrc[ 4 * i ];
//        lVal = ( nAValue ) - ( nSValue );
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nAValue = addSrc[ 4 * i + 1 ];
//        nSValue = subSrc[ 4 * i + 1 ];
//        lVal = ( nAValue ) - ( nSValue );
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nAValue = addSrc[ 4 * i + 2 ];
//        nSValue = subSrc[ 4 * i + 2 ];
//        lVal = ( nAValue ) - ( nSValue );
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nAValue = addSrc[ 4 * i + 3 ];
//        nSValue = subSrc[ 4 * i + 3 ];
//        lVal = ( nAValue ) - ( nSValue );
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    for(i = DWidth << 2; i < lImgWidth; i++)
//    {
//        lVal = addSrc[ i ] - subSrc[ i ];
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    for(i = 0; i < lRadius; i++)
//    {
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    return;
//}
//
//MVoid BoxFilterNS::BoxFilterRow_NS32F(MFloat *pDst, MFloat *pBoxSumBuf, MInt32 lImgWidth, MInt32 lRadius)
//{
//    MInt32 x;
//    MFloat lbVal = 0.0;
//    MInt32 lboxSize = lRadius * 2 + 1;
//
//    for(x = 0; x < lImgWidth; x++)
//    {
//        lbVal = pBoxSumBuf[ lboxSize ] - pBoxSumBuf[ 0 ];
//        lbVal = lbVal / ( lboxSize * lboxSize );
//        pDst[ x ] = lbVal;
//        pBoxSumBuf++;
//    }
//}
//
//
///// @brief
///// @param pSrcImg
///// @param lImgWidth
///// @param lImgHeight
///// @param lSrcPitch
///// @param pDstImg
///// @param lDstPitch
///// @param lStartRow
///// @param lEndRow
///// @param lRadius
///// @param pBoxSumBuf
///// @return
//MVoid BoxFilterNS::Image_Box_Stripe_NS32F(MFloat *pSrcImg,
//                                          MInt32 lImgWidth,
//                                          MInt32 lImgHeight,
//                                          MInt32 lSrcPitch,
//                                          MFloat *pDstImg,
//                                          MInt32 lDstPitch,
//                                          MInt32 lStartRow,
//                                          MInt32 lEndRow,
//                                          MInt32 lRadius,
//                                          MFloat *pBoxSumBuf)
//{
//    MInt32 lPreLine = MIN(lRadius, lStartRow);
//    MInt32 lNexLine = MIN(lRadius, lImgHeight - lEndRow);
//    MInt32 lTop = lStartRow - lPreLine;
//    MInt32 lBot = lEndRow + lNexLine;
//    MFloat *tmpaddSrc = pSrcImg + lTop * lSrcPitch;
//    MFloat *tmpsubSrc = pSrcImg + lTop * lSrcPitch;
//    MFloat *tmpSrc = pSrcImg + lStartRow * lSrcPitch;
//    MFloat *tmpDst = pDstImg + lStartRow * lDstPitch;
//
//    MInt32 line = lStartRow - lRadius;
//
//    MInt32 lboxSize = lRadius * 2 + 1;
//
//    MMemSet(pBoxSumBuf, 0, ( lImgWidth + lRadius * 2 + 1 + 100 ) * sizeof(MFloat));
//
//    for(; line < lTop; line++)
//    {
//        boxBlurProcessRow_add_NS32F(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
//    }
//    for(; line < lStartRow + lRadius; line++)
//    {
//        boxBlurProcessRow_add_NS32F(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
//        tmpaddSrc += lSrcPitch;
//    }
//    {
//        boxBlurProcessRow_add_NS32F(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
//        BoxFilterRow_NS32F(tmpDst, pBoxSumBuf, lImgWidth, lRadius);
//        tmpaddSrc += lSrcPitch;
//        tmpSrc += lSrcPitch;
//        tmpDst += lDstPitch;
//        line++;
//    }
//    for(; line < lBot; line++)
//    {
//        boxBlurProcessRow_add_sub_NS32F(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
//        BoxFilterRow_NS32F(tmpDst, pBoxSumBuf, lImgWidth, lRadius);
//        tmpaddSrc += lSrcPitch;
//        if( line >= lTop + lRadius * 2 + 1 )
//        {
//            tmpsubSrc += lSrcPitch;
//        }
//        tmpSrc += lSrcPitch;
//        tmpDst += lDstPitch;
//    }
//    tmpaddSrc -= lSrcPitch;
//    for(; line < lEndRow + lRadius; line++)
//    {
//        boxBlurProcessRow_add_sub_NS32F(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
//        BoxFilterRow_NS32F(tmpDst, pBoxSumBuf, lImgWidth, lRadius);
//        tmpsubSrc += lSrcPitch;
//        tmpSrc += lSrcPitch;
//        tmpDst += lDstPitch;
//    }
//}
//
//
//MRESULT BoxFilterNS::BoxFilterNS32F(MHandle hMemMgr,
//                                    MFloat *pSrcBuf,
//                                    MInt32 lWidth,
//                                    MInt32 lHeight,
//                                    MInt32 lSrcLineBytes,
//                                    MFloat *pDstBuf,
//                                    MInt32 lDstLineBytes,
//                                    MInt32 lRadius,
//                                    MInt32 nThreadCount)
//{
//    MRESULT lret = MOK;
//    std::vector<MFloat *> box_sum_buf;
//    const MInt32 threadCount = nThreadCount > 0 ? nThreadCount : lHeight >= 1024 ? 16 : 8;
//
//#if !defined(MCV_MULTI_THREAD)
//    const MInt32 threadCount = 1;
//#endif
//
//    MFloat *pBoxBuf = ( MFloat * ) MMemAlloc(hMemMgr, ( lWidth + lRadius * 2 + 1 + 100 ) * threadCount * sizeof(MFloat));
//    if( MNull == pBoxBuf )
//    {
//        MMemFree(hMemMgr, pBoxBuf);
//        return MERR_NO_MEMORY;
//    }
//
//    for(MInt32 i = 0; i < threadCount; ++i)
//    {
//        MFloat *p = box_sum_buf[ 0 ] + ( lWidth + lRadius * 2 + 1 + 100 ) * i;
//        box_sum_buf.push_back(p);
//    }
//
//    auto expand_functor = [&](int currentThreadId)
//    {
//        // 线程分割
//        int startHeight = 0;
//        int endHeight = lHeight;
//        if( threadCount > 1 )
//        {
//            int countStride = lHeight / threadCount;
//            countStride = ( countStride >> 1 ) << 1;
//
//            startHeight = currentThreadId * countStride;
//            if( currentThreadId != threadCount - 1 )
//            {
//                endHeight = startHeight + countStride;
//            }
//        }
//
//
//        Image_Box_Stripe_NS32F(pSrcBuf,
//                               lWidth,
//                               lHeight,
//                               lSrcLineBytes,
//                               pDstBuf,
//                               lDstLineBytes,
//                               startHeight,
//                               endHeight,
//                               lRadius,
//                               box_sum_buf[ currentThreadId ]);
//    };
//
//    std::thread *expand_thread = new std::thread[threadCount - 1];
//    for(int i = 0; i < threadCount - 1; ++i)
//    {
//        expand_thread[ i ] = std::thread(expand_functor, i);
//    }
//    expand_functor(threadCount - 1);
//    for(int i = 0; i < threadCount - 1; ++i)
//    {
//        expand_thread[ i ].join();
//    }
//    if( expand_thread )
//    {
//        delete[] expand_thread;
//        expand_thread = MNull;
//    }
//
//    if( pBoxBuf )
//    {
//        MMemFree(hMemMgr, pBoxBuf);
//        pBoxBuf = MNull;
//    }
//    return lret;
//}
//
//
////#pragma mark - NS32I
//
////box filter for 32bit int
//MVoid BoxFilterNS::boxBlurProcessRow_add_NS32I(MInt32 *pSumLine, MInt32 *addSrc, MInt32 lImgWidth, MInt32 lRadius)
//{
//    MInt32 i;
//    MInt32 nValue;
//    MInt32 DWidth = lImgWidth >> 2;
//    MInt32 nSum = 0;
//
//    nValue = addSrc[ 0 ];
//    pSumLine[ 0 ] = 0;
//    pSumLine++;
//    for(i = 0; i < lRadius; i++)
//    {
//        nSum += nValue;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    for(i = 0; i < DWidth; i++)
//    {
//        nValue = addSrc[ 4 * i ];
//        nSum += ( nValue );
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nValue = addSrc[ 4 * i + 1 ];
//        nSum += ( nValue );
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nValue = addSrc[ 4 * i + 2 ];
//        nSum += ( nValue );
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nValue = addSrc[ 4 * i + 3 ];
//        nSum += ( nValue );
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    for(i = DWidth << 2; i < lImgWidth; i++)
//    {
//        nValue = addSrc[ i ];
//        nSum += nValue;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    nValue = addSrc[ lImgWidth - 1 ];
//    for(i = 0; i < lRadius; i++)
//    {
//        nSum += nValue;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    return;
//}
//
//MVoid BoxFilterNS::boxBlurProcessRow_add_sub_NS32I(MInt32 *pSumLine, MInt32 *addSrc, MInt32 *subSrc, MInt32 lImgWidth, MInt32 lRadius)
//{
//    MInt32 i;
//    MInt32 nAValue, nSValue;
//    MInt32 lVal;
//    MInt32 DWidth = lImgWidth >> 2;
//    MInt32 nSum = 0;
//
//    lVal = addSrc[ 0 ] - subSrc[ 0 ];
//    pSumLine[ 0 ] = 0;
//    pSumLine++;
//    for(i = 0; i < lRadius; i++)
//    {
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    i = 0;
//#if defined(USE_NEON)
//    {
//        int32x4x4_t tmpaddsrc, tmpsubsrc;
//        int32x4_t sug_dif00, sug_dif01, sug_dif02, sug_dif03;
//        int32x4x2_t tmpsum00, tmpsum01;
//        int32x4x2_t dstsum00, dstsum01;
//        int32x4_t currsum_32x4;
//        int32x4_t pSum_32x4;
//        currsum_32x4 = vdupq_n_s32(nSum);
//
//        for (i = 0; i < DWidth - 4; i += 4)
//        {
//            tmpaddsrc = vld4q_s32(addSrc + 4*i);
//            tmpsubsrc = vld4q_s32(subSrc + 4*i);
//
//            sug_dif00 = vsubq_s32(tmpaddsrc.val[0], tmpsubsrc.val[0]);
//            sug_dif01 = vsubq_s32(tmpaddsrc.val[1], tmpsubsrc.val[1]);
//            sug_dif02 = vsubq_s32(tmpaddsrc.val[2], tmpsubsrc.val[2]);
//            sug_dif03 = vsubq_s32(tmpaddsrc.val[3], tmpsubsrc.val[3]);
//
//            sug_dif01 = vaddq_s32(sug_dif00, sug_dif01);
//            sug_dif02 = vaddq_s32(sug_dif01, sug_dif02);
//            sug_dif03 = vaddq_s32(sug_dif02, sug_dif03);
//
//            tmpsum00 = vzipq_s32(sug_dif00, sug_dif02);
//            tmpsum01 = vzipq_s32(sug_dif01, sug_dif03);
//            dstsum00 = vzipq_s32(tmpsum00.val[0], tmpsum01.val[0]);
//            dstsum01 = vzipq_s32(tmpsum00.val[1], tmpsum01.val[1]);
//
//            currsum_32x4 = vdupq_lane_s32(vget_high_s32(currsum_32x4), 1);
//            pSum_32x4 = vld1q_s32(pSumLine);
//            currsum_32x4 = vaddq_s32(currsum_32x4, dstsum00.val[0]);
//            pSum_32x4 = vaddq_s32(pSum_32x4, currsum_32x4);
//            vst1q_s32(pSumLine, pSum_32x4);
//            pSumLine += 4;
//
//            currsum_32x4 = vdupq_lane_s32(vget_high_s32(currsum_32x4), 1);
//            pSum_32x4 = vld1q_s32(pSumLine);
//            currsum_32x4 = vaddq_s32(currsum_32x4, dstsum00.val[1]);
//            pSum_32x4 = vaddq_s32(pSum_32x4, currsum_32x4);
//            vst1q_s32(pSumLine, pSum_32x4);
//            pSumLine += 4;
//
//            currsum_32x4 = vdupq_lane_s32(vget_high_s32(currsum_32x4), 1);
//            pSum_32x4 = vld1q_s32(pSumLine);
//            currsum_32x4 = vaddq_s32(currsum_32x4, dstsum01.val[0]);
//            pSum_32x4 = vaddq_s32(pSum_32x4, currsum_32x4);
//            vst1q_s32(pSumLine, pSum_32x4);
//            pSumLine += 4;
//
//            currsum_32x4 = vdupq_lane_s32(vget_high_s32(currsum_32x4), 1);
//            pSum_32x4 = vld1q_s32(pSumLine);
//            currsum_32x4 = vaddq_s32(currsum_32x4, dstsum01.val[1]);
//            pSum_32x4 = vaddq_s32(pSum_32x4, currsum_32x4);
//            vst1q_s32(pSumLine, pSum_32x4);
//            pSumLine += 4;
//        }
//        nSum = vgetq_lane_s32(currsum_32x4, 3);
//    }
//#endif
//    for(; i < DWidth; i++)
//    {
//        nAValue = addSrc[ 4 * i ];
//        nSValue = subSrc[ 4 * i ];
//        lVal = ( nAValue ) - ( nSValue );
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nAValue = addSrc[ 4 * i + 1 ];
//        nSValue = subSrc[ 4 * i + 1 ];
//        lVal = ( nAValue ) - ( nSValue );
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nAValue = addSrc[ 4 * i + 2 ];
//        nSValue = subSrc[ 4 * i + 2 ];
//        lVal = ( nAValue ) - ( nSValue );
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//
//        nAValue = addSrc[ 4 * i + 3 ];
//        nSValue = subSrc[ 4 * i + 3 ];
//        lVal = ( nAValue ) - ( nSValue );
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    for(i = DWidth << 2; i < lImgWidth; i++)
//    {
//        lVal = addSrc[ i ] - subSrc[ i ];
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    for(i = 0; i < lRadius; i++)
//    {
//        nSum += lVal;
//        ( pSumLine++ )[ 0 ] += nSum;
//    }
//    return;
//}
//
//MVoid BoxFilterNS::BoxFilterRow_NS32I(MInt32 *pDst, MInt32 *pBoxSumBuf, MInt32 lImgWidth, MInt32 lRadius, MFloat invDivNum)
//{
//    MInt32 x;
//    MInt32 lbVal = 0;
//    MInt32 lboxSize = lRadius * 2 + 1;
//
//    for(x = 0; x < lImgWidth; x++)
//    {
//        lbVal = pBoxSumBuf[ lboxSize ] - pBoxSumBuf[ 0 ];
//        lbVal = lbVal * invDivNum;
//        pDst[ x ] = lbVal;
//        pBoxSumBuf++;
//    }
//}
//
//
///// @brief
///// @param pSrcImg
///// @param lImgWidth
///// @param lImgHeight
///// @param lSrcPitch
///// @param pDstImg
///// @param lDstPitch
///// @param lStartRow
///// @param lEndRow
///// @param lRadius
///// @param pBoxSumBuf
///// @return
//MVoid BoxFilterNS::Image_Box_Stripe_NS32I(MInt32 *pSrcImg,
//                                          MInt32 lImgWidth,
//                                          MInt32 lImgHeight,
//                                          MInt32 lSrcPitch,
//                                          MInt32 *pDstImg,
//                                          MInt32 lDstPitch,
//                                          MInt32 lStartRow,
//                                          MInt32 lEndRow,
//                                          MInt32 lRadius,
//                                          MInt32 *pBoxSumBuf)
//{
//    MInt32 lPreLine = MIN(lRadius, lStartRow);
//    MInt32 lNexLine = MIN(lRadius, lImgHeight - lEndRow);
//    MInt32 lTop = lStartRow - lPreLine;
//    MInt32 lBot = lEndRow + lNexLine;
//    MInt32 *tmpaddSrc = pSrcImg + lTop * lSrcPitch;
//    MInt32 *tmpsubSrc = pSrcImg + lTop * lSrcPitch;
//    MInt32 *tmpSrc = pSrcImg + lStartRow * lSrcPitch;
//    MInt32 *tmpDst = pDstImg + lStartRow * lDstPitch;
//
//    MInt32 line = lStartRow - lRadius;
//
//    MInt32 lboxSize = lRadius * 2 + 1;
//    MFloat invDivNum = 1.0f / ( lboxSize * lboxSize );
//
//    MMemSet(pBoxSumBuf, 0, ( lImgWidth + lRadius * 2 + 1 + 100 ) * sizeof(MInt32));
//
//    for(; line < lTop; line++)
//    {
//        boxBlurProcessRow_add_NS32I(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
//    }
//    for(; line < lStartRow + lRadius; line++)
//    {
//        boxBlurProcessRow_add_NS32I(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
//        tmpaddSrc += lSrcPitch;
//    }
//    {
//        boxBlurProcessRow_add_NS32I(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
//        BoxFilterRow_NS32I(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
//        tmpaddSrc += lSrcPitch;
//        tmpSrc += lSrcPitch;
//        tmpDst += lDstPitch;
//        line++;
//    }
//    for(; line < lBot; line++)
//    {
//        boxBlurProcessRow_add_sub_NS32I(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
//        BoxFilterRow_NS32I(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
//        tmpaddSrc += lSrcPitch;
//        if( line >= lTop + lRadius * 2 + 1 )
//        {
//            tmpsubSrc += lSrcPitch;
//        }
//        tmpSrc += lSrcPitch;
//        tmpDst += lDstPitch;
//    }
//    tmpaddSrc -= lSrcPitch;
//    for(; line < lEndRow + lRadius; line++)
//    {
//        boxBlurProcessRow_add_sub_NS32I(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
//        BoxFilterRow_NS32I(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
//        tmpsubSrc += lSrcPitch;
//        tmpSrc += lSrcPitch;
//        tmpDst += lDstPitch;
//    }
//}
//
//
//MRESULT BoxFilterNS::BoxFilterNS32I(MHandle hMemMgr,
//                                    MInt32 *pSrcBuf,
//                                    MInt32 lWidth,
//                                    MInt32 lHeight,
//                                    MInt32 lSrcLineBytes,
//                                    MInt32 *pDstBuf,
//                                    MInt32 lDstLineBytes,
//                                    MInt32 lRadius,
//                                    MInt32 nThreadCount)
//{
//    MRESULT lret = MOK;
//    std::vector<MInt32 *> box_sum_buf;
//    MInt32 threadCount = nThreadCount > 0 ? nThreadCount : lHeight >= 1024 ? 16 : 8;
//#if !defined(MCV_MULTI_THREAD)
//    threadCount = 1;
//#endif
//    MInt32 *pBoxSum = ( MInt32 * ) MMemAlloc(hMemMgr, ( lWidth + lRadius * 2 + 1 + 100 ) * threadCount * sizeof(MInt32));
//    if( MNull == pBoxSum )
//    {
//        MMemFree(hMemMgr, pBoxSum);
//        return MERR_NO_MEMORY;
//    }
//
//    for(MInt32 k = 0; k < threadCount; ++k)
//    {
//        MInt32 *p = pBoxSum + ( lWidth + lRadius * 2 + 1 + 100 ) * k;
//        box_sum_buf.push_back(p);
//    }
//
//    auto expand_functor = [&](int currentThreadId)
//    {
//        // 线程分割
//        int startHeight = 0;
//        int endHeight = lHeight;
//        if( threadCount > 1 )
//        {
//            int countStride = lHeight / threadCount;
//            countStride = ( countStride >> 1 ) << 1;
//
//            startHeight = currentThreadId * countStride;
//            if( currentThreadId != threadCount - 1 )
//            {
//                endHeight = startHeight + countStride;
//            }
//        }
//
//
//        Image_Box_Stripe_NS32I(pSrcBuf,
//                               lWidth,
//                               lHeight,
//                               lSrcLineBytes,
//                               pDstBuf,
//                               lDstLineBytes,
//                               startHeight,
//                               endHeight,
//                               lRadius,
//                               box_sum_buf[ currentThreadId ]);
//    };
//
//    std::thread *expand_thread = new std::thread[threadCount - 1];
//    for(int i = 0; i < threadCount - 1; ++i)
//    {
//        expand_thread[ i ] = std::thread(expand_functor, i);
//    }
//    expand_functor(threadCount - 1);
//    for(int i = 0; i < threadCount - 1; ++i)
//    {
//        expand_thread[ i ].join();
//    }
//    if( expand_thread )
//    {
//        delete[] expand_thread;
//        expand_thread = MNull;
//    }
//
//
//    if( pBoxSum )
//    {
//        MMemFree(hMemMgr, pBoxSum);
//        pBoxSum = MNull;
//    }
//    box_sum_buf.clear();
//    return lret;
//}
//
//
//
//
//
//
//
//
//
//
//
//
//
//
