#include "merror.h"
#include "mobilecv.h"

#include "down16_fix.h"
#include "single_image_enhancement_define.h"

#if defined(USE_NEON) || defined(__ARM_NEON__)
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN


//a1 大图地址
//a2 小图地址
//a3 大图宽
//a4 大图高
//a6 小图step
//a5 大图step
//a7 channel 数
//__int64 __fastcall down16(__int64 a1, __int64 a2, int a3, int a4, unsigned int a5, unsigned int a6, int a7)
//{
//  int v7; // w8
//  int v8; // w7
//  __int128 v9; // ST20_16
//  __int128 v10; // ST10_16
//  __int64 result; // x0
//  __int128 v12; // ST40_16
//  __int128 v13; // ST30_16
//  __int128 v14; // [xsp+50h] [xbp-20h]
//  __int128 v15; // [xsp+60h] [xbp-10h]
//
//  v7 = a3 >> 1;
//  v8 = a4 >> 1;
//  if ( a7 == 1 )
//  {
//    *(_QWORD *)&v12 = a1;
//    *((_QWORD *)&v12 + 1) = a2;
//    v14 = v12;
//    *(_QWORD *)&v13 = __PAIR__(v8, v7);
//    *((_QWORD *)&v13 + 1) = __PAIR__(a6, a5);
//    v15 = v13;
//    result = GOMP_parallel(down16__omp_fn_3, (__int64)&v14, 0, 0);
//  }
//  else
//  {
//    *(_QWORD *)&v9 = a1;
//    *((_QWORD *)&v9 + 1) = a2;
//    v14 = v9;
//    *(_QWORD *)&v10 = __PAIR__(v8, v7);
//    *((_QWORD *)&v10 + 1) = __PAIR__(a6, a5);
//    v15 = v10;
//    result = GOMP_parallel(down16__omp_fn_4, (__int64)&v14, 0, 0);
//  }
//  return result;
//}


static MInt32 down16_cn1_stripe(MVoid *pSrc, MVoid *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep,
	MInt32 lStartLine, MInt32 lEndLine)
{
	MInt32 lret = 0;
	MInt32 x, y;

	for (y = lStartLine; y < lEndLine; ++y) // 0~lDstHeight
	{
		MInt16 *pSrcRow0 = (MInt16 *)pSrc + 2 * y * lSrcStep;
		MInt16 *pSrcRow1 = pSrcRow0 + lSrcStep;
		MInt16 *pDstRow = (MInt16 *)pDst + y * lDstStep;

		x = 0;
#ifdef __ARM_NEON__
		for (; x < lDstWidth - 7; x+=8)
		{
			MInt32 xx = 2 * x;
			int16x8x2_t vSrc0_s16x8x2 = vld2q_s16(pSrcRow0 + xx);
			int16x8x2_t vSrc1_s16x8x2 = vld2q_s16(pSrcRow1 + xx);
			int16x8_t vSum0, vSum1;
			vSum0 = vaddq_s16(vSrc0_s16x8x2.val[0], vSrc0_s16x8x2.val[1]);
			vSum1 = vaddq_s16(vSrc1_s16x8x2.val[0], vSrc1_s16x8x2.val[1]);
			vSum0 = vaddq_s16(vSum0, vSum1);
			vst1q_s16(pDstRow + x, vrshrq_n_s16(vSum0, 2));
		}
#endif
		for (; x < lDstWidth; ++x)
		{
			MInt32 xx = 2 * x;

			MInt32 val;
			val = (pSrcRow0[xx] + pSrcRow0[xx + 1] + pSrcRow1[xx] + pSrcRow1[xx + 1] + 2) >> 2;
			if (pDstRow[x] != val && x < lDstWidth - 7)
			{
				int a = 1;
			}

			pDstRow[x] = val;
		}
	}

	return lret;
}

typedef struct _tag_DOWN16
{
	MInt32      task_ID;
	MVoid*		pSrc;
	MVoid*		pDst;
	MInt32		lDstWidth;
	MInt32		lDstHeight;
	MInt32		lSrcStep;
	MInt32		lDstStep;
	MInt32		lStartLine;
	MInt32		lEndLine;
}PARAM_DOWN16;

MVoid thread_down16_cn1(MVoid* HParam)
{
	PARAM_DOWN16* pParam = (PARAM_DOWN16*)HParam;
	MInt32 lret = MOK;

	down16_cn1_stripe(pParam->pSrc, pParam->pDst, pParam->lDstWidth, pParam->lDstHeight, pParam->lSrcStep, pParam->lDstStep, 
		pParam->lStartLine, pParam->lEndLine);
	return;
}

static MInt32 down16_cn1(MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep)
{
	MInt32 lRet = MOK;
	if (mcvParallelMonitor)
	{
		MInt32 lTaskNum = TASK_NUM;
		MInt32 lTaskHeight = lDstHeight / lTaskNum;
		MInt32 lTaskID[TASK_NUM] = {MNull };
		PARAM_DOWN16 pParam[TASK_NUM] = {MNull };
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
			lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_down16_cn1, (MVoid*)& pParam[lnum]);
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
		down16_cn1_stripe(pSrc, pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep, 0, lDstHeight);
	}
exit:
	return lRet;
}


static MInt32 down16_cn2_stripe(MVoid *pSrc, MVoid *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep,
	MInt32 lStartLine, MInt32 lEndLine)
{
	MInt32 lret = 0;
	MInt32 x, y;

	for (y = lStartLine; y < lEndLine; ++y)
	{
		MInt16 *pSrcRow0 = (MInt16 *)pSrc + 2 * y * lSrcStep;
		MInt16 *pSrcRow1 = pSrcRow0 + lSrcStep;
		MInt16 *pDstRow = (MInt16 *)pDst + y * lDstStep;
		x = 0;
#ifdef __ARM_NEON__
		for (x = 0; x < lDstWidth * 2 - 15; x += 16)
		{
			MInt32 xx = 2 * x;
			int16x8x4_t vSrc0 = vld4q_s16(pSrcRow0 + xx);
			int16x8x4_t vSrc1 = vld4q_s16(pSrcRow1 + xx);
			int16x8x2_t vRes;
			int16x8_t vTemp0, vTemp1;
			vTemp0 = vaddq_s16(vSrc0.val[0], vSrc0.val[2]);
			vTemp1 = vaddq_s16(vSrc1.val[0], vSrc1.val[2]);
			vRes.val[0] = vrshrq_n_s16(vaddq_s16(vTemp0, vTemp1), 2);

			vTemp0 = vaddq_s16(vSrc0.val[1], vSrc0.val[3]);
			vTemp1 = vaddq_s16(vSrc1.val[1], vSrc1.val[3]);
			vRes.val[1] = vrshrq_n_s16(vaddq_s16(vTemp0, vTemp1), 2);

			vst2q_s16(pDstRow + x, vRes);
		}
#endif

		for (; x < lDstWidth * 2; x += 2)
		{
			MInt32 xx = 2 * x;
			MInt32 val0, val1;
			val0 = (pSrcRow0[xx] + pSrcRow0[xx + 2] + pSrcRow1[xx] + pSrcRow1[xx + 2] + 2) >> 2;
			val1 = (pSrcRow0[xx + 1] + pSrcRow0[xx + 3] + pSrcRow1[xx + 1] + pSrcRow1[xx + 3] + 2) >> 2;

			if ((val0 != pDstRow[x] || val1 != pDstRow[x + 1]) && x < lDstWidth * 2 - 15)
			{
				int a = 1;
			}

			pDstRow[x] = val0;
			pDstRow[x + 1] = val1;
		}
	}

	return lret;
}

MVoid thread_down16_cn2(MVoid* HParam)
{
	PARAM_DOWN16* pParam = (PARAM_DOWN16*)HParam;
	MInt32 lret = MOK;

	down16_cn2_stripe(pParam->pSrc, pParam->pDst, pParam->lDstWidth, pParam->lDstHeight, pParam->lSrcStep, pParam->lDstStep,
		pParam->lStartLine, pParam->lEndLine);
	return;
}

static MInt32 down16_cn2(MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep)
{
	MInt32 lRet = MOK;
	if (mcvParallelMonitor)
	{
		MInt32 lTaskNum = TASK_NUM;
		MInt32 lTaskHeight = lDstHeight / lTaskNum;
		MInt32 lTaskID[TASK_NUM] = {MNull };
		PARAM_DOWN16 pParam[TASK_NUM] = {MNull };
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
			lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_down16_cn2, (MVoid*)& pParam[lnum]);
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
		down16_cn2_stripe(pSrc, pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep, 0, lDstHeight);
	}
exit:
	return lRet;
}

MInt32 down16(MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pDst, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcStep, MInt32 lDstStep, MInt32 cn)
{
	long lret = 0;

	MInt32 lDstWidth = lSrcWidth >> 1;
	MInt32 lDstHeight = lSrcHeight >> 1;

	if (cn == 1)
	{
		lret = down16_cn1(mcvParallelMonitor, (MVoid *)pSrc, (MVoid *)pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep);
	}
	else
	{
		lret = down16_cn2(mcvParallelMonitor, (MVoid *)pSrc, (MVoid *)pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep);
	}

	return lret;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END
