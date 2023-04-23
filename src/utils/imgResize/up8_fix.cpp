#include <stdio.h>
#include <ArcsoftLog.h>

#include "mobilecv.h"
#include "ammem.h"

#include "imagebase.h"
#include "merror.h"
#include "up16_fix.h"
#include "up8_fix.h"
#include "single_image_enhancement_define.h"


//#if defined(USE_NEON) || defined(__ARM_NEON__)
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
//#endif


NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
static MVoid up8_cn1_stripe(MUInt16* pSrc, MUInt16* pSmooth, MUInt8* pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep,
                            MInt32 rowStart, MInt32 rowEnd)
{
    MInt32 lWidthSm = lDstWidth >> 1;
    MInt32 lHeightSm = lDstHeight >> 1;

    rowStart = MAX(rowStart, 1);
    rowEnd = MIN(rowEnd, lHeightSm - 2);

    MInt16 arrCubic[4] = { -2304, 28416, 7424, -768 };//{ 0xF700, 0x6F00, 0x1D00, 0xFD00 };//{-9,111,29,-3}
    int16x4_t vcubic = vld1_s16(arrCubic);

    MInt32 yl, ys, xl, xs;
    for (ys = rowStart; ys < rowEnd; ys++)
    {
        yl = (ys << 1) + 1;

        MUInt16* pSrc0 = pSrc + (ys - 1) * lSrcStep;
        MUInt16* pSrc1 = pSrc + (ys + 0) * lSrcStep;
        MUInt16* pSrc2 = pSrc + (ys + 1) * lSrcStep;
        MUInt16* pSrc3 = pSrc + (ys + 2) * lSrcStep;

        MUInt16* pSmooth0 = pSmooth + (ys - 1) * lSrcStep;
        MUInt16* pSmooth1 = pSmooth + (ys + 0) * lSrcStep;
        MUInt16* pSmooth2 = pSmooth + (ys + 1) * lSrcStep;
        MUInt16* pSmooth3 = pSmooth + (ys + 2) * lSrcStep;

        MUInt8* pDst0 = pDst + (yl + 0) * lDstStep;
        MUInt8* pDst1 = pDst + (yl + 1) * lDstStep;

        int16x8_t vdif00, vdif10, vdif20, vdif30; // 前8个
        int16x8_t vdif01, vdif11, vdif21, vdif31; // 后8个

        vdif00 = vsubq_s16(vld1q_s16((MInt16*)pSmooth0), vld1q_s16((MInt16*)pSrc0));
        vdif10 = vsubq_s16(vld1q_s16((MInt16*)pSmooth1), vld1q_s16((MInt16*)pSrc1));
        vdif20 = vsubq_s16(vld1q_s16((MInt16*)pSmooth2), vld1q_s16((MInt16*)pSrc2));
        vdif30 = vsubq_s16(vld1q_s16((MInt16*)pSmooth3), vld1q_s16((MInt16*)pSrc3));

        int16x8_t valPos0, valRev0, valPos1, valRev1;
        valPos0 = vaddq_s16(
                vaddq_s16(
                        vaddq_s16(
                                vqrdmulhq_lane_s16(vdif00, vcubic, 0),
                                vqrdmulhq_lane_s16(vdif10, vcubic, 1)),
                        vqrdmulhq_lane_s16(vdif20, vcubic, 2)),
                vqrdmulhq_lane_s16(vdif30, vcubic, 3));

        valRev0 = vaddq_s16(
                vaddq_s16(
                        vaddq_s16(
                                vqrdmulhq_lane_s16(vdif00, vcubic, 3),
                                vqrdmulhq_lane_s16(vdif10, vcubic, 2)),
                        vqrdmulhq_lane_s16(vdif20, vcubic, 1)),
                vqrdmulhq_lane_s16(vdif30, vcubic, 0));

        for (xs = 8, xl = 3; xl < lDstWidth - 15; xs += 8, xl+=16)
        {
            uint8x8x2_t vDst0 = vld2_u8(pDst0 + xl);
            uint8x8x2_t vDst1 = vld2_u8(pDst1 + xl);

            vdif01 = vsubq_s16(vld1q_s16((MInt16*)pSmooth0 + xs), vld1q_s16((MInt16*)pSrc0 + xs));
            vdif11 = vsubq_s16(vld1q_s16((MInt16*)pSmooth1 + xs), vld1q_s16((MInt16*)pSrc1 + xs));
            vdif21 = vsubq_s16(vld1q_s16((MInt16*)pSmooth2 + xs), vld1q_s16((MInt16*)pSrc2 + xs));
            vdif31 = vsubq_s16(vld1q_s16((MInt16*)pSmooth3 + xs), vld1q_s16((MInt16*)pSrc3 + xs));

            valPos1 = vaddq_s16(
                    vaddq_s16(
                            vaddq_s16(
                                    vqrdmulhq_lane_s16(vdif01, vcubic, 0),
                                    vqrdmulhq_lane_s16(vdif11, vcubic, 1)),
                            vqrdmulhq_lane_s16(vdif21, vcubic, 2)),
                    vqrdmulhq_lane_s16(vdif31, vcubic, 3));

            valRev1 = vaddq_s16(
                    vaddq_s16(
                            vaddq_s16(
                                    vqrdmulhq_lane_s16(vdif01, vcubic, 3),
                                    vqrdmulhq_lane_s16(vdif11, vcubic, 2)),
                            vqrdmulhq_lane_s16(vdif21, vcubic, 1)),
                    vqrdmulhq_lane_s16(vdif31, vcubic, 0));

            int16x8_t vHori0, vHori1, vHori2, vHori3;
            int16x8_t vDifPosPos, vDifPosRev, vDifRevPos, vDifRevRev;

            vHori0 = valPos0;
            vHori1 = vextq_s16(valPos0, valPos1, 1);
            vHori2 = vextq_s16(valPos0, valPos1, 2);
            vHori3 = vextq_s16(valPos0, valPos1, 3);

            vDifPosPos = vaddq_s16(
                    vaddq_s16(
                            vaddq_s16(
                                    vqrdmulhq_lane_s16(vHori0, vcubic, 0),
                                    vqrdmulhq_lane_s16(vHori1, vcubic, 1)),
                            vqrdmulhq_lane_s16(vHori2, vcubic, 2)),
                    vqrdmulhq_lane_s16(vHori3, vcubic, 3));

            vDifPosRev = vaddq_s16(
                    vaddq_s16(
                            vaddq_s16(
                                    vqrdmulhq_lane_s16(vHori0, vcubic, 3),
                                    vqrdmulhq_lane_s16(vHori1, vcubic, 2)),
                            vqrdmulhq_lane_s16(vHori2, vcubic, 1)),
                    vqrdmulhq_lane_s16(vHori3, vcubic, 0));

            vDst0.val[0] = vqrshrun_n_s16(vaddq_s16(vDifPosPos, vshll_n_u8(vDst0.val[0], 2)), 2);
            vDst0.val[1] = vqrshrun_n_s16(vaddq_s16(vDifPosRev, vshll_n_u8(vDst0.val[1], 2)), 2);
            vst2_u8(pDst0 + xl, vDst0);

            vHori0 = valRev0;
            vHori1 = vextq_s16(valRev0, valRev1, 1);
            vHori2 = vextq_s16(valRev0, valRev1, 2);
            vHori3 = vextq_s16(valRev0, valRev1, 3);

            vDifRevPos = vaddq_s16(
                    vaddq_s16(
                            vaddq_s16(
                                    vqrdmulhq_lane_s16(vHori0, vcubic, 0),
                                    vqrdmulhq_lane_s16(vHori1, vcubic, 1)),
                            vqrdmulhq_lane_s16(vHori2, vcubic, 2)),
                    vqrdmulhq_lane_s16(vHori3, vcubic, 3));

            vDifRevRev = vaddq_s16(
                    vaddq_s16(
                            vaddq_s16(
                                    vqrdmulhq_lane_s16(vHori0, vcubic, 3),
                                    vqrdmulhq_lane_s16(vHori1, vcubic, 2)),
                            vqrdmulhq_lane_s16(vHori2, vcubic, 1)),
                    vqrdmulhq_lane_s16(vHori3, vcubic, 0));

            vDst1.val[0] = vqrshrun_n_s16(vaddq_s16(vDifRevPos, vshll_n_u8(vDst1.val[0], 2)), 2);
            vDst1.val[1] = vqrshrun_n_s16(vaddq_s16(vDifRevRev, vshll_n_u8(vDst1.val[1], 2)), 2);
            vst2_u8(pDst1 + xl, vDst1);


            vdif00 = vdif01;
            vdif10 = vdif11;
            vdif20 = vdif21;
            vdif30 = vdif31;
            valPos0 = valPos1;
            valRev0 = valRev1;
        }
    }
}

typedef struct _tag_UP8
{
    MInt32      task_ID;

    MUInt16*	pSrc;
    MUInt16*	pSmooth;
    MUInt8*		pDst;
    MInt32		lDstWidth;
    MInt32		lDstHeight;
    MInt32		lSrcStep;
    MInt32		lDstStep;

    MInt32		lStartLine;
    MInt32		lEndLine;
}PARAM_UP8;

MVoid thread_up8_cn1(MVoid* HParam)
{
    PARAM_UP8* pParam = (PARAM_UP8*)HParam;
    MInt32 lret = MOK;

    up8_cn1_stripe(pParam->pSrc, pParam->pSmooth, pParam->pDst, pParam->lDstWidth, pParam->lDstHeight, pParam->lSrcStep, pParam->lDstStep,
                   pParam->lStartLine, pParam->lEndLine);
    return;
}


MVoid up8_cn1(MHandle hMemMgr, MHandle mcvParallelMonitor, MUInt16* pSrc, MUInt16* pSmooth, MUInt8* pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep)
{
    MInt32 lWidthSm = lDstWidth >> 1;
    MInt32 lHeightSm = lDstHeight >> 1;
    MInt32 rowStart = 0, rowEnd = lHeightSm;

    if (mcvParallelMonitor)
    {
        MInt32 lTaskNum = TASK_NUM;
        MInt32 lTaskHeight = lHeightSm / lTaskNum;
        MInt32 lTaskID[TASK_NUM] = {MNull };
        PARAM_UP8 pParam[TASK_NUM] = {MNull };
        MInt32 lnum = 0;

        lTaskHeight = lTaskHeight >> 2 << 2;
        for (lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].lStartLine = lTaskHeight * lnum;
            pParam[lnum].lEndLine = lTaskHeight * (lnum + 1);
        }
        pParam[lTaskNum - 1].lEndLine = lHeightSm;

        for (lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].pSrc = pSrc;
            pParam[lnum].pDst = pDst;
            pParam[lnum].pSmooth = pSmooth;
            pParam[lnum].lDstWidth = lDstWidth;
            pParam[lnum].lDstHeight = lDstHeight;
            pParam[lnum].lSrcStep = lSrcStep;
            pParam[lnum].lDstStep = lDstStep;

        }

        for (lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_up8_cn1, (MVoid*)&pParam[lnum]);
        }

        for (lnum = 0; lnum < lTaskNum; lnum++)
        {
            mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
        }
    }
    else
    {
        up8_cn1_stripe(pSrc, pSmooth, pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep, 0, lHeightSm);
    }
}

static MVoid conv8To16(MUInt8*pSrc, MInt32 lSrcStep, MInt16 *pDst, MInt32 lDstStep, MInt32 lWidth, MInt32 lHeight, MInt32 cn)
{
    MInt32 x, y;
    for (y = 0; y < lHeight; ++y)
    {
        MUInt8*pSrcRow = pSrc + y * lSrcStep;
        MInt16 *pDstRow = pDst + y * lDstStep;
        x = 0;
#ifdef __ARM_NEON__
        for (; x < lWidth * cn - 7; x += 8)
		{
			uint8x8_t vSrc = vld1_u8(pSrcRow + x);
			vst1q_s16(pDstRow + x, vreinterpretq_s16_u16(vshll_n_u8(vSrc, 2)));
		}
#endif


        for (x = 0; x < lWidth * cn; ++x)
        {
            pDstRow[x] = (MInt16)pSrcRow[x] << 2;
        }
    }
}

static MVoid conv16To8(MInt16 *pSrc, MInt32 lSrcStep, MUInt8*pDst, MInt32 lDstStep, MInt32 lWidth, MInt32 lHeight, MInt32 cn)
{
    MInt32 x, y;
    for (y = 0; y < lHeight; ++y)
    {
        MInt16 *pSrcRow = pSrc + y * lSrcStep;
        MUInt8*pDstRow = pDst + y * lDstStep;
        x = 0;
#ifdef __ARM_NEON__
        for (x = 0; x < lWidth * cn - 7; x += 8)
		{
			int16x8_t vSrc = vld1q_s16(pSrcRow + x);
			vst1_u8(pDstRow + x, vqrshrun_n_s16(vSrc, 2));
		}
#endif
        for (; x < lWidth * cn; ++x)
        {
            MInt16 res = (pSrcRow[x] + 2) >> 2;
            pDstRow[x] = (MUInt8)TRIMBYTE(res);
        }
    }
}

MInt32 up8(MHandle hMemMgr, MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pSmooth, MVoid* pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep, MInt32 cn)
{
    LOGD("up8++");
    MInt32 lret = 0;
    if (cn == 1)
    {
        up8_cn1(hMemMgr, mcvParallelMonitor, (MUInt16*)pSrc, (MUInt16*)pSmooth, (MUInt8*)pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep);
    }
    else
    {
        MInt16 *pDstBuf = (MInt16 *)MMemAlloc(hMemMgr, cn * lDstWidth * lDstHeight * sizeof(MInt16));
        if (!pDstBuf)
        {
            return MERR_NO_MEMORY;
        }

        conv8To16((MUInt8*)pDst, lDstStep, pDstBuf, cn * lDstWidth, lDstWidth, lDstHeight, cn);

        lret = up16(hMemMgr, mcvParallelMonitor, pSrc, pSmooth, pDstBuf, lDstWidth, lDstHeight, lSrcStep, cn * lDstWidth, cn);

        conv16To8(pDstBuf, cn * lDstWidth, (MUInt8*)pDst, lDstStep, lDstWidth, lDstHeight, cn);

        if (pDstBuf)
        {
            MMemFree(hMemMgr, pDstBuf);
            pDstBuf = NULL;
        }
    }
    LOGD("up8--");
    return lret;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END




