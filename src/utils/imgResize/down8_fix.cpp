#include <stdlib.h>
#include "merror.h"
#include "mobilecv.h"

#include "down8_fix.h"
#include "single_image_enhancement_define.h"

#if defined(USE_NEON) || defined(__ARM_NEON__)
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN


static MInt32 down8_cn1_stripe(MVoid *pSrc, MVoid *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep,
	MInt32 lStartLine, MInt32 lEndLine)
{
	MInt32 lret = 0;
	MInt32 x = 0, y = 0;

	for (y = lStartLine; y < lEndLine; ++y)
	{
		MUInt8*pSrcRow0 = (MUInt8*)pSrc + 2 * y * lSrcStep;
		MUInt8*pSrcRow1 = pSrcRow0 + lSrcStep;
		MInt16 *pDstRow = (MInt16 *)pDst + y * lDstStep;
		x = 0;
#ifdef __ARM_NEON__
		for (x = 0; x < lDstWidth - 7; x += 8)
		{
			MInt32 xx = 2 * x;
			uint8x8x2_t vSrc0_u8x8x2 = vld2_u8(pSrcRow0 + xx);
			uint8x8x2_t vSrc1_u8x8x2 = vld2_u8(pSrcRow1 + xx);
			int16x8_t vSum0, vSum1;
			vSum0 = vreinterpretq_s16_u16(vaddl_u8(vSrc0_u8x8x2.val[0], vSrc0_u8x8x2.val[1]));
			vSum1 = vreinterpretq_s16_u16(vaddl_u8(vSrc1_u8x8x2.val[0], vSrc1_u8x8x2.val[1]));
			vst1q_s16(pDstRow + x, vaddq_s16(vSum0, vSum1));
		}
#endif

		for (; x < lDstWidth; ++x)
		{
			MInt32 xx = 2 * x;
			MInt32 val = pSrcRow0[xx] + pSrcRow0[xx + 1] + pSrcRow1[xx] + pSrcRow1[xx + 1];
//			if (x < lDstWidth - 7 && (pDstRow[x] != val))
//			{
//				int a = 1;
//			}
			pDstRow[x] = val;
		}
	}

	return lret;
}

typedef struct _tag_DOWN8
{
	MInt32      task_ID;
	MVoid* pSrc;
	MVoid* pDst;
	MInt32		lDstWidth;
	MInt32		lDstHeight;
	MInt32		lSrcStep;
	MInt32		lDstStep;
	MInt32		lStartLine;
	MInt32		lEndLine;
}PARAM_DOWN8;

MVoid thread_down8_cn1(MVoid* HParam)
{
	PARAM_DOWN8* pParam = (PARAM_DOWN8*)HParam;
	MInt32 lret = MOK;

	down8_cn1_stripe(pParam->pSrc, pParam->pDst, pParam->lDstWidth, pParam->lDstHeight, pParam->lSrcStep, pParam->lDstStep,
		pParam->lStartLine, pParam->lEndLine);
	return;
}

static MInt32 down8_cn1(MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep)
{
	MInt32 lRet = MOK;
	if (mcvParallelMonitor)
	{
		MInt32 lTaskNum = TASK_NUM;
		MInt32 lTaskHeight = lDstHeight / lTaskNum;
		MInt32 lTaskID[TASK_NUM] = {MNull };
		PARAM_DOWN8 pParam[TASK_NUM] = {MNull };
		MInt32 lnum = 0;

		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].lStartLine = lTaskHeight * lnum;
			pParam[lnum].lEndLine = lTaskHeight * (lnum + 1);
		}
		pParam[lTaskNum - 1].lEndLine = lDstHeight;

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].pSrc = pSrc;
			pParam[lnum].pDst = pDst;
			pParam[lnum].lDstWidth = lDstWidth;
			pParam[lnum].lDstHeight = lDstHeight;
			pParam[lnum].lSrcStep = lSrcStep;
			pParam[lnum].lDstStep = lDstStep;
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_down8_cn1, (MVoid*)& pParam[lnum]);
			if (lTaskID[lnum] < 0)
			{
				lRet = MERR_BAD_STATE;
				goto exit;
			}
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
		}
	}
	else
	{
		down8_cn1_stripe(pSrc, pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep, 0, lDstHeight);
	}
exit:
	return lRet;
}

static MInt32 down8_cn2_stripe(MVoid *pSrc, MVoid *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep,
	MInt32 lStartLine, MInt32 lEndLine)
{
	MInt32 lret = 0;
	MInt32 x = 0, y = 0;

	for (y = lStartLine; y < lEndLine; ++y)
	{
		MUInt8 *pSrcRow0 = (MUInt8*)pSrc + 2 * y * lSrcStep;
		MUInt8*pSrcRow1 = pSrcRow0 + lSrcStep;
		MInt16 *pDstRow = (MInt16 *)pDst + y * lDstStep;

		x = 0;
#ifdef __ARM_NEON__
		for (; x < lDstWidth * 2 - 15; x += 16)
		{
			MInt32 xx = 2 * x;
			uint8x8x4_t vSrc0 = vld4_u8(pSrcRow0 + xx);
			uint8x8x4_t vSrc1 = vld4_u8(pSrcRow1 + xx);
			int16x8x2_t vRes;
			int16x8_t vTemp0, vTemp1;
			vTemp0 = vreinterpretq_s16_u16(vaddl_u8(vSrc0.val[0], vSrc0.val[2]));
			vTemp1 = vreinterpretq_s16_u16(vaddl_u8(vSrc1.val[0], vSrc1.val[2]));
			vRes.val[0] = vaddq_s16(vTemp0, vTemp1);

			vTemp0 = vreinterpretq_s16_u16(vaddl_u8(vSrc0.val[1], vSrc0.val[3]));
			vTemp1 = vreinterpretq_s16_u16(vaddl_u8(vSrc1.val[1], vSrc1.val[3]));
			vRes.val[1] = vaddq_s16(vTemp0, vTemp1);

			vst2q_s16(pDstRow + x, vRes);
		}
#endif

		for (; x < lDstWidth * 2; x += 2)
		{
			MInt32 xx = 2 * x;
			MInt32 val0, val1;
			val0 = pSrcRow0[xx] + pSrcRow0[xx + 2] + pSrcRow1[xx] + pSrcRow1[xx + 2];
			val1 = pSrcRow0[xx + 1] + pSrcRow0[xx + 3] + pSrcRow1[xx + 1] + pSrcRow1[xx + 3];

			if ((pDstRow[x] != val0 || pDstRow[x + 1] != val1) && x < lDstWidth * 2 - 15)
			{
				int a = 1;
			}

			pDstRow[x] = val0;
			pDstRow[x + 1] = val1;
		}
	}

	return lret;
}

MVoid thread_down8_cn2(MVoid* HParam)
{
	PARAM_DOWN8* pParam = (PARAM_DOWN8*)HParam;
	MInt32 lret = MOK;

	down8_cn2_stripe(pParam->pSrc, pParam->pDst, pParam->lDstWidth, pParam->lDstHeight, pParam->lSrcStep, pParam->lDstStep,
		pParam->lStartLine, pParam->lEndLine);
	return;
}

static MInt32 down8_cn2(MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep)
{
	MInt32 lRet = MOK;
	if (mcvParallelMonitor)
	{
		MInt32 lTaskNum = TASK_NUM;
		MInt32 lTaskHeight = lDstHeight / lTaskNum;
		MInt32 lTaskID[TASK_NUM] = {MNull };
		PARAM_DOWN8 pParam[TASK_NUM] = {MNull };
		MInt32 lnum = 0;

		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].lStartLine = lTaskHeight * lnum;
			pParam[lnum].lEndLine = lTaskHeight * (lnum + 1);
		}
		pParam[lTaskNum - 1].lEndLine = lDstHeight;

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].pSrc = pSrc;
			pParam[lnum].pDst = pDst;
			pParam[lnum].lDstWidth = lDstWidth;
			pParam[lnum].lDstHeight = lDstHeight;
			pParam[lnum].lSrcStep = lSrcStep;
			pParam[lnum].lDstStep = lDstStep;
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_down8_cn2, (MVoid*)& pParam[lnum]);
			if (lTaskID[lnum] < 0)
			{
				lRet = MERR_BAD_STATE;
				goto exit;
			}
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
		}
	}
	else
	{
		down8_cn2_stripe(pSrc, pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep, 0, lDstHeight);
	}
exit:
	return lRet;
}

MInt32 down8(MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pDst, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcStep, MInt32 lDstStep, MInt32 cn)
{
//    printf("aaa = %d, %d, %d, %d\n", lSrcWidth, lSrcHeight, lSrcStep, lDstStep, cn);
	MInt32 lret = 0;

	MInt32 lDstWidth = lSrcWidth >> 1;
	MInt32 lDstHeight = lSrcHeight >> 1;

	if (cn == 1)
	{
		lret = down8_cn1(mcvParallelMonitor, pSrc, pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep);
	}
	else
	{
		lret = down8_cn2(mcvParallelMonitor, pSrc, pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep);
	}

	return lret;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END
