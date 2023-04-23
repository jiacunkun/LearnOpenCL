#include "mobilecv.h"
#include "up_down_scale.h"
#include "imageproc.h"
#include "merror.h"
#include "SetLPASVLOFFSCREEN.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

static MVoid ver_smooth(MByte* tmpSrc00, MByte* tmpSrc01, MByte* tmpSrc02, MShort* pSmoothBuf, MInt32 lWidth, MInt32 cn)
{
	MInt32 x = 0;
#ifdef USE_NEON

	uint8x8_t tmpSrc00_8x8;
	uint8x8_t tmpSrc01_8x8;
	uint8x8_t tmpSrc02_8x8;

	int16x8_t smoothBuf_16x8;

	for (; x < lWidth * cn - 7; x += 8)
	{
		tmpSrc00_8x8 = vld1_u8((MUInt8*)tmpSrc00 + x);
		tmpSrc01_8x8 = vld1_u8((MUInt8*)tmpSrc01 + x);
		tmpSrc02_8x8 = vld1_u8((MUInt8*)tmpSrc02 + x);

		smoothBuf_16x8 = vreinterpretq_s16_u16(vaddq_u16(vaddl_u8(tmpSrc00_8x8, tmpSrc02_8x8), vshll_n_u8(tmpSrc01_8x8, 1)));

		vst1q_s16(pSmoothBuf + x, smoothBuf_16x8);
	}

#endif
	for (; x < lWidth * cn; x++)
	{
		pSmoothBuf[x] = tmpSrc00[x] + tmpSrc02[x] + (tmpSrc01[x] << 1);
	}
	return;
}

static MVoid hor_smooth(MShort* pSmoothBuf, MByte* tmpDst, MInt32 lWidth, MInt32 cn)
{
	MInt32 x = 0;
	for (x = 0; x < lWidth * cn; x++)
	{
		MInt32 lval = (pSmoothBuf[x] << 1) + pSmoothBuf[x - cn] + pSmoothBuf[x + cn] + 8 >> 4;
		tmpDst[x] = lval;
	}
	return;
}


static MVoid hor_smooth_down2_C1(MShort* SmoothBuf, MByte* tmpDst, MInt32 lSrcWidth, MInt32 lDstWidth)
{
	MInt32 x = 0;
#ifdef USE_NEON
	for (; x < lDstWidth - 7; x += 8)
	{
		int16x8x2_t lS_16x8x2 = vld2q_s16(SmoothBuf + 2 * x - 1);
		int16x8_t lS0_16x8 = lS_16x8x2.val[0];
		int16x8_t lS1_16x8 = lS_16x8x2.val[1];
		lS_16x8x2 = vld2q_s16(SmoothBuf + 2 * x + 1);
		int16x8_t lS2_16x8 = lS_16x8x2.val[0];

		lS1_16x8 = vshlq_n_s16(lS1_16x8, 1);
		lS1_16x8 = vaddq_s16(lS1_16x8, lS0_16x8);
		lS1_16x8 = vaddq_s16(lS1_16x8, lS2_16x8);
		vst1_u8(tmpDst + x, vqrshrun_n_s16(lS1_16x8, 4));
	}
#endif
	for (; x < lDstWidth; x++)
	{
		MShort lval = (SmoothBuf[2 * x] << 1) + SmoothBuf[2 * x - 1] + SmoothBuf[2 * x + 1] + 8 >> 4;
		tmpDst[x] = lval;
	}
}

static MVoid hor_smooth_down2_C2(MShort* SmoothBuf, MByte* tmpDst, MInt32 lSrcWidth, MInt32 lDstWidth)
{
	MInt32 x = 0, k = 0;
	for (x = 0; x < lDstWidth * 2; x += 2, k += 4)
	{
		MInt32 lval;
		lval = (SmoothBuf[k] << 1) + SmoothBuf[k - 2] + SmoothBuf[k + 2] + 8 >> 4;
		tmpDst[x] = lval;

		lval = (SmoothBuf[k + 1] << 1) + SmoothBuf[k - 1] + SmoothBuf[k + 3] + 8 >> 4;
		tmpDst[x + 1] = lval;
	}
	return;
}

MInt32 Img_Guass3x3_Down2_Range(MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch,
	MInt32 lTopLine, MInt32 lBotLine, MShort* SmoothBuf, MInt32 cn)
{
	MInt32 lret = MOK;
	MInt32 y;
	SmoothBuf += cn;
	if (cn == 1)
	{
		for (y = lTopLine; y < lBotLine; y++)
		{
			MByte* tmpSrc01 = pSrcImg + y * 2 * lSrcPitch;
			MByte* tmpSrc00 = (0 == y) ? tmpSrc01 : tmpSrc01 - lSrcPitch;
			MByte* tmpSrc02 = (y * 2 == lSrcHeight - 1) ? tmpSrc01 : tmpSrc01 + lSrcPitch;
			MByte* tmpDst = pDstImg + y * lDstPitch;

			ver_smooth(tmpSrc00, tmpSrc01, tmpSrc02, SmoothBuf, lSrcWidth, cn);

			SmoothBuf[-1] = SmoothBuf[0];
			SmoothBuf[lSrcWidth] = SmoothBuf[lSrcWidth - 1];
			hor_smooth_down2_C1(SmoothBuf, tmpDst, lSrcWidth, lDstWidth);

		}
	}
	else if (cn == 2)
	{
		for (y = lTopLine; y < lBotLine; y++)
		{
			MByte* tmpSrc01 = pSrcImg + y * 2 * lSrcPitch;
			MByte* tmpSrc00 = (0 == y) ? tmpSrc01 : tmpSrc01 - lSrcPitch;
			MByte* tmpSrc02 = (y * 2 == lSrcHeight - 1) ? tmpSrc01 : tmpSrc01 + lSrcPitch;
			MByte* tmpDst = pDstImg + y * lDstPitch;

			ver_smooth(tmpSrc00, tmpSrc01, tmpSrc02, SmoothBuf, lSrcWidth, cn);

			SmoothBuf[-2] = SmoothBuf[0];
			SmoothBuf[-1] = SmoothBuf[1];

			SmoothBuf[lSrcWidth] = SmoothBuf[lSrcWidth - 2];
			SmoothBuf[lSrcWidth + 1] = SmoothBuf[lSrcWidth - 1];
			hor_smooth_down2_C2(SmoothBuf, tmpDst, lSrcWidth, lDstWidth);
		}
	}
exit:
	return lret;
}

typedef struct _tag_IMG_DOWN_SCALE_ST {
	MByte* pSrcImg;
	MInt32 lSrcWidth;
	MInt32 lSrcHeight;
	MInt32 lSrcPitch;
	MByte* pDstImg;
	MInt32 lDstWidth;
	MInt32 lDstHeight;
	MInt32 lDstPitch;
	MInt32 cn;
	MInt32  topline;
	MInt32  botline;
	MShort* SmoothBuf;

	MInt32  lTaskHeight;
	MInt32  lret;
	MInt32   thread_ID;
} Img_Down_Scale, * LpImg_Down_Scale;

#ifdef MCV_MULTI_THREAD
static MVoid thread_Img_GuassDownScale(MVoid* pParam)
{
	Img_Down_Scale* Filter = (Img_Down_Scale*)pParam;
	MInt32 lret = MOK;

	Filter->lret = Img_Guass3x3_Down2_Range(Filter->pSrcImg, Filter->lSrcWidth, Filter->lSrcHeight, Filter->lSrcPitch,
		Filter->pDstImg, Filter->lDstWidth, Filter->lDstPitch, Filter->lDstPitch, Filter->topline, Filter->botline, Filter->SmoothBuf, Filter->cn);
	return;
}
#endif

MInt32 Img_Guass3x3_Down2_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 cn)
{
	START_TIME;
	MInt32 lret = MOK;

	lret = Img_Guass3x3_Down2(hMemMgr, mcvParallelMonitor, 
							  pSrcImg->ppu8Plane[0], pSrcImg->i32Width, pSrcImg->i32Height, pSrcImg->pi32Pitch[0],
							  pDstImg->ppu8Plane[0], pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], cn);
	END_TIME;
	return lret;
}

#define MAX_TASK_NUM 16
MInt32 Img_Guass3x3_Down2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch, MInt32 cn)
{
	MInt32 lnum, lret = MOK;
	Img_Down_Scale pParams[MAX_TASK_NUM] = { MNull };
	MShort* SmoothBuf[MAX_TASK_NUM] = { MNull };
	MInt32 lTask_Num = lDstHeight >= 1024 ? 16 : 8;
	MInt32 lTaskHeight = lDstHeight / lTask_Num;
	MInt32 taskID[MAX_TASK_NUM] = { 0 };

	MShort* pMemory = MNull;
	pMemory = (MShort*)MMemAlloc(hMemMgr, (lSrcWidth + 2) * sizeof(MShort) * cn * lTask_Num);
	if (MNull == pMemory)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	for (MInt32 lnum = 0; lnum < lTask_Num; lnum++)
	{
		SmoothBuf[lnum] = pMemory + (lSrcWidth + 2) * cn * lnum;
	}

#ifdef MCV_MULTI_THREAD

	for (lnum = 0; lnum < lTask_Num; lnum++)
	{
		pParams[lnum].topline = lTaskHeight * lnum;
		pParams[lnum].botline = lTaskHeight * (lnum + 1);
	}
	pParams[lTask_Num - 1].botline = lDstHeight;

	for (lnum = 0; lnum < lTask_Num; lnum++)
	{
		pParams[lnum].thread_ID = lnum;

		pParams[lnum].pSrcImg = pSrcImg;
		pParams[lnum].lSrcWidth = lSrcWidth;
		pParams[lnum].lSrcHeight = lSrcHeight;
		pParams[lnum].lSrcPitch = lSrcPitch;

		pParams[lnum].pDstImg = pDstImg;
		pParams[lnum].lDstWidth = lDstWidth;
		pParams[lnum].lDstHeight = lDstHeight;
		pParams[lnum].lDstPitch = lDstPitch;

		pParams[lnum].cn = cn;
		pParams[lnum].SmoothBuf = SmoothBuf[lnum];
	}

	for (lnum = 0; lnum < lTask_Num; lnum++)
	{
		taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_GuassDownScale, (MVoid*)&pParams[lnum]);
	}

	for (lnum = 0; lnum < lTask_Num; lnum++)
	{
		mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
	}

	for (lnum = 0; lnum < lTask_Num; lnum++)
	{
		if (MOK != pParams[lnum].lret)
		{
			lret = pParams[lnum].lret;
			goto exit;
		}
	}


#else
	{

#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
		for (MInt32 lTask = 0; lTask < lTask_Num; lTask++)
		{
			MInt32 rowBegin = lTask * lTaskHeight;
			MInt32 rowEnd = (lTask == lTask_Num - 1) ? lDstHeight : (rowBegin + lTaskHeight);
			MInt32 curThread = 0;
#ifdef NH_ENABLE_OPENMP
			curThread = omp_get_thread_num();
#endif

			Img_Guass3x3_Down2_Range(pSrcImg, lSrcWidth, lSrcHeight, lSrcPitch,
				pDstImg, lDstWidth, lDstHeight, lDstPitch,
				rowBegin, rowEnd, SmoothBuf[curThread], cn);
		}
	}
#endif


exit:
	if (pMemory)
	{
		MMemFree(hMemMgr, pMemory);
		pMemory = MNull;
	}
	return lret;
}


typedef struct _tag_fix_img_smooth_para {
	MInt32* tmpBuf;
	MInt32* tmpMaskBuf;
	LPASVLOFFSCREEN  pSrcImg;
	LPASVLOFFSCREEN  pDstImg;
	LPASVLOFFSCREEN  pMaskImg;
	MInt32*			 Inv_Data;
	MInt32			 lStart;
	MInt32			 lEnd;
}Fix_Img_Smooth_Para, * Lp_Fix_Img_Smooth_Para;


static MVoid ver_line_process_guass_3x3_s16(MInt16* pSrc00, MInt16* pSrc01, MInt16* pSrc02, MInt32* tmpRow, MInt32 lWidth)
{
	MInt32 x = 0;
	for (x = 0; x < lWidth; x++)
	{
		tmpRow[x] = pSrc00[x] + pSrc02[x] + pSrc01[x] * 2;
	}
}


static MVoid hor_line_process_guass_3x3_s16(MInt16* tmpDst, MInt32* tmpRow, MInt32 lWidth)
{
	MInt32 x = 0;
	for (x = 0; x < lWidth; x++)
	{
		MInt32 lVal = tmpRow[x - 1] + tmpRow[x + 1] + tmpRow[x] * 2;
		tmpDst[x] = lVal + 8 >> 4;
	}
}

MVoid Img_Guass3x3_S16_range(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32* tmpBuf, MInt32 lTopLine, MInt32 lBotLine)
{
	MInt16 lWidth = pSrcImg->i32Width;
	MInt16 lHeight = pSrcImg->i32Height;
	MInt32 lStepSrc = pSrcImg->pi32Pitch[0] >> 1;
	MInt32 lStepDst = pDstImg->pi32Pitch[0] >> 1;
	MInt16* pSrcData = (MInt16 *)pSrcImg->ppu8Plane[0];
	MInt16* pDstData = (MInt16 *)pDstImg->ppu8Plane[0];
	MInt16* pSrc00, * pSrc01, * pSrc02;
	MInt32* tmpRow;
	MInt32  y;

	if (0 == lTopLine)
	{
		pSrc00 = pSrcData;
		pSrc01 = pSrc00;
		pSrc02 = pSrc01 + lStepSrc;
	}
	else
	{
		pSrc00 = pSrcData + (lTopLine - 1) * lStepSrc;
		pSrc01 = pSrc00 + lStepSrc;
		pSrc02 = pSrc01 + lStepSrc;
	}

	tmpRow = tmpBuf + 1;
	for (y = lTopLine; y < lBotLine; y++)
	{
		MInt16* tmpDst = pDstData + y * lStepDst;
		ver_line_process_guass_3x3_s16(pSrc00, pSrc01, pSrc02, tmpRow, lWidth);

		tmpRow[-1] = tmpRow[0];
		tmpRow[lWidth] = tmpRow[lWidth - 1];

		hor_line_process_guass_3x3_s16(tmpDst, tmpRow, lWidth);
		pSrc00 = pSrc01;
		pSrc01 = pSrc02;
		if (y < lHeight - 2)
		{
			pSrc02 += lStepSrc;
		}
	}

}

static MVoid thread_fix_img_gausmooth_3x3_range(MVoid* para)
{
	if (MNull != para)
	{
		Fix_Img_Smooth_Para* pPara = (Fix_Img_Smooth_Para*)para;
		Img_Guass3x3_S16_range(pPara->pSrcImg, pPara->pDstImg, pPara->tmpBuf, pPara->lStart, pPara->lEnd);
	}
	return;
}


MInt32 Img_Guass3x3_s16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg)
{
	MInt32 lret = MOK;
	MInt16 lWidth = pSrcImg->i32Width;
	MInt16 lHeight = pSrcImg->i32Height;
	MInt16 lStep = pSrcImg->pi32Pitch[0];
	MInt32 lTaskNum = 1, lTaskH = 0;
	MInt32 lnum = 0;
	MInt32* tmpBuf[8] = { MNull };
	MBool bAllocDst = MFalse;
	ASVLOFFSCREEN  tmpDstImg = { MNull };

	if (MNull == pDstImg || pSrcImg == pDstImg)
	{
		lret = AllocOffscreenMemory(hMemMgr, lWidth, lHeight, ASVL_PAF_RAW10_GRAY_16B, &tmpDstImg);
		if (MOK != lret)
		{
			goto exit;
		}
		pDstImg = &tmpDstImg;
		bAllocDst = MTrue;
	}

	lTaskNum = (lHeight > 1024) ? 8 : (lHeight > 64 ? 4 : 1);
	lTaskH = lHeight / lTaskNum >> 2 << 2;

	for (lnum = 0; lnum < lTaskNum; lnum++)
	{
		tmpBuf[lnum] = (MInt32*)MMemAlloc(hMemMgr, (lWidth + 2) * sizeof(MInt32));
		if (MNull == tmpBuf[lnum])
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}
	}

#ifdef MCV_MULTI_THREAD
	{
		Fix_Img_Smooth_Para pPara[8] = { MNull };
		MInt32 taskId[8] = { MNull };

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pPara[lnum].lStart = lTaskH * lnum;
			pPara[lnum].lEnd = lTaskH * (lnum + 1);
		}
		pPara[lTaskNum - 1].lEnd = lHeight;

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pPara[lnum].pSrcImg = pSrcImg;
			pPara[lnum].pDstImg = pDstImg;
			pPara[lnum].tmpBuf = tmpBuf[lnum];
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			taskId[lnum] = mcvAddTask(mcvParallelMonitor, thread_fix_img_gausmooth_3x3_range, (MVoid*)&pPara[lnum]);
			if (taskId[lnum] < 0)
			{
				lret = MERR_BAD_STATE;
				goto exit;
			}
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			lret = mcvWaitTask(mcvParallelMonitor, taskId[lnum]);
			if (MOK != lret)
			{
				lret = MERR_BAD_STATE;
				goto exit;
			}
		}
	}
#else
	{

#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
		for (MInt32 lTask = 0; lTask < lTaskNum; lTask++)
		{
			MInt32 rowBegin = lTask * lTaskH;
			MInt32 rowEnd = (lTask == lTaskNum - 1) ? lHeight : (rowBegin + lTaskH);
			MInt32 curThread = 0;
#ifdef NH_ENABLE_OPENMP
			curThread = omp_get_thread_num();
#endif	
	
			Img_Guass3x3_S16_range(pSrcImg, pDstImg, tmpBuf[curThread], rowBegin, rowEnd);
		}
	}
#endif


	if (MTrue == bAllocDst)
	{
		CopyOffscreen(pSrcImg, &tmpDstImg);
	}

exit:
	if (MTrue == bAllocDst)
	{
		FreeOffscreenMemory(hMemMgr, &tmpDstImg);
	}
	for (lnum = 0; lnum < lTaskNum; lnum++)
	{
		if (tmpBuf[lnum])
		{
			MMemFree(hMemMgr, tmpBuf[lnum]);
			tmpBuf[lnum] = MNull;
		}
	}
	return lret;
}



static MVoid Odd_Line_UpScaele_C1(MUInt8* FullLine, MUInt8* pHalfLine, MInt32 lFullWidth, MInt32 lHalfWidth)
{
	MInt32 Max = 255;
	FullLine[0] = pHalfLine[0];
	MInt32 k = 1;
	for (MInt32 x = 1; x < lHalfWidth; x++, k += 2)
	{
		FullLine[k] = (pHalfLine[x] + pHalfLine[x - 1] + 1) >> 1;
		FullLine[k + 1] = pHalfLine[x];
		CLAMP(FullLine[k], -128, Max);
		CLAMP(FullLine[k + 1], -128, Max);
	}
	for (; k < lFullWidth; k++)
	{
		FullLine[k] = FullLine[k - 1];
		CLAMP(FullLine[k], -128, Max);
	}
}

static MVoid Even_Odd_Line_UpScaele_C1(MUInt8* CurFullLine, MUInt8* NexFullLine, MUInt8* PreFullLine, MUInt8* pHalfLine, MInt32 lFullWidth, MInt32 lHalfWidth)
{
	MInt32 Max = 255;
	MInt32 x = 0;
	MInt32 k = 0;

#ifdef USE_NEON //todo:neon需要兼容

	uint8x16_t src01_u8x16;
	uint8x16_t src02_u8x16;
	uint8x16x2_t pre_u8x16x2;
	uint8x16x2_t dst01_u8x16x2;
	uint8x16x2_t dst02_u8x16x2;
	//uint16x8_t sum01_u16x8, sum02_u16x8;
	for (x = 0; x < lHalfWidth - 16; x += 16, k += 32)
	{
		src01_u8x16 = vld1q_u8((MUInt8*)pHalfLine + x);
		src02_u8x16 = vld1q_u8((MUInt8*)pHalfLine + x + 1);
		//sum01_u16x8 = vaddl_u8(vget_low_u8(src01_u8x16), vget_low_u8(src02_u8x16));
		//sum02_u16x8 = vaddl_u8(vget_high_u8(src01_u8x16), vget_high_u8(src02_u8x16));
		//dst01_u8x16x2.val[0] = src01_u8x16;
		//dst01_u8x16x2.val[1] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));
		dst01_u8x16x2.val[0] = src01_u8x16;
		dst01_u8x16x2.val[1] = vrhaddq_u8(src01_u8x16, src02_u8x16);
		pre_u8x16x2 = vld2q_u8((MUInt8*)PreFullLine + k);
		vst2q_u8((MUInt8*)NexFullLine + k, dst01_u8x16x2);

		//sum01_u16x8 = vaddl_u8(vget_low_u8(pre_u8x16x2.val[0]), vget_low_u8(dst01_u8x16x2.val[0]));
		//sum02_u16x8 = vaddl_u8(vget_high_u8(pre_u8x16x2.val[0]), vget_high_u8(dst01_u8x16x2.val[0]));
		//dst02_u8x16x2.val[0] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));

		//sum01_u16x8 = vaddl_u8(vget_low_u8(pre_u8x16x2.val[1]), vget_low_u8(dst01_u8x16x2.val[1]));
		//sum02_u16x8 = vaddl_u8(vget_high_u8(pre_u8x16x2.val[1]), vget_high_u8(dst01_u8x16x2.val[1]));
		//dst02_u8x16x2.val[1] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));

		dst02_u8x16x2.val[0] = vrhaddq_u8(pre_u8x16x2.val[0], dst01_u8x16x2.val[0]);
		dst02_u8x16x2.val[1] = vrhaddq_u8(pre_u8x16x2.val[1], dst01_u8x16x2.val[1]);
		vst2q_u8((MUInt8*)CurFullLine + k, dst02_u8x16x2);
	}

#endif

	NexFullLine[k] = pHalfLine[x];
	CurFullLine[k] = (NexFullLine[k] + PreFullLine[k] + 1) >> 1;
	k++;
	x++;
	for (; x < lHalfWidth; x++, k += 2)//x = 1
	{
		NexFullLine[k] = (pHalfLine[x] + pHalfLine[x - 1] + 1) >> 1;
		NexFullLine[k + 1] = pHalfLine[x];

		CurFullLine[k] = (NexFullLine[k] + PreFullLine[k] + 1) >> 1;
		CurFullLine[k + 1] = (NexFullLine[k + 1] + PreFullLine[k + 1] + 1) >> 1;
	}
	for (; k < lFullWidth; k++)
	{
		NexFullLine[k] = NexFullLine[k - 1];
		CurFullLine[k] = CurFullLine[k - 1];
	}
}


static MInt32 Guass3x3Up2_u8(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 lTopLine, MInt32 lBotLine)
{
	MInt32 lret = MOK;
	MUInt8* pFullData = pDstImg->ppu8Plane[0];
	MInt32 lFullWidth = pDstImg->i32Width;
	MInt32 lFullHeight = pDstImg->i32Height;
	MInt32 lFullPitch = pDstImg->pi32Pitch[0];

	MUInt8* pHalfData = pSrcImg->ppu8Plane[0];
	MInt32 lHalfWidth = pSrcImg->i32Width;
	MInt32 lHalfHeight = pSrcImg->i32Height;
	MInt32 lHalfPitch = pSrcImg->pi32Pitch[0];

	MInt32 y, k;
	MUInt8* pPreFullImg;
	MUInt8* pCurFullImg = pFullData, * pNexFullImg = pFullData;
	MUInt8* tmppHalfImg = pHalfData;

	y = lTopLine;
	if (0 == lTopLine)
	{
		tmppHalfImg = pHalfData;
		Odd_Line_UpScaele_C1(pCurFullImg, tmppHalfImg, lFullWidth, lHalfWidth);
		k = 1;
		y = 1;
	}
	else
	{
		pCurFullImg = pFullData + lFullPitch * (y * 2 - 2);
		tmppHalfImg = pHalfData + (y - 1) * lHalfPitch;
		Odd_Line_UpScaele_C1(pCurFullImg, tmppHalfImg, lFullWidth, lHalfWidth);
		k = y * 2 - 1;
	}

	for (; y < lBotLine; y++, k += 2)//y = 1
	{
		pPreFullImg = pFullData + (k - 1) * lFullPitch;
		pCurFullImg = pPreFullImg + lFullPitch;
		pNexFullImg = pCurFullImg + lFullPitch;
		tmppHalfImg = pHalfData + y * lHalfPitch;
		Even_Odd_Line_UpScaele_C1(pCurFullImg, pNexFullImg, pPreFullImg, tmppHalfImg, lFullWidth, lHalfWidth);
	}

	if (lBotLine == lHalfHeight)
	{
		for (; k < lFullHeight; k++)
		{
			pPreFullImg = pFullData + (k - 1) * lFullPitch;
			pCurFullImg = pPreFullImg + lFullPitch;
			MMemCpy(pCurFullImg, pPreFullImg, lFullWidth);
		}
	}
	return lret;
}


struct IMG_SG_UPSCALE
{
	LPASVLOFFSCREEN pSrcImg;
	LPASVLOFFSCREEN pDstImg;
	MInt32 lTopLine;
	MInt32 lBotLine;
};

MInt32 Img_Guass3x3_Up2_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg)
{
	START_TIME;

	MInt32 lret = MOK;
	MInt32 lHeight = pSrcImg->i32Height;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
	{
		MInt32 lTaskNum = lHeight > 1024 ? 16 : 8;
		/// 设置回调函数
		auto func_lamda = [](MVoid* HParam) -> MVoid
		{
			auto SG_NLM_sturct = (IMG_SG_UPSCALE*)HParam;

			Guass3x3Up2_u8(SG_NLM_sturct->pSrcImg,
				SG_NLM_sturct->pDstImg,
				SG_NLM_sturct->lTopLine,
				SG_NLM_sturct->lBotLine);

		};
		MVoid(*func)(MVoid*) = func_lamda;



		/// 设置参数
		MInt32 lTaskHeight = lHeight / lTaskNum;
		lTaskHeight = (lTaskHeight >> 2) << 2;

		IMG_SG_UPSCALE pParam[16] = { MNull };
		for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].lTopLine = lTaskHeight * lnum;
			pParam[lnum].lBotLine = lTaskHeight * (lnum + 1);
		}
		pParam[0].lTopLine = 0;
		pParam[lTaskNum - 1].lBotLine = lHeight;


		for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParam[lnum].pSrcImg = pSrcImg;
			pParam[lnum].pDstImg = pDstImg;
		}

		/// 创建线程     
		MInt32 lTaskID[16] = { MNull };
		for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
		{
			lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
			if (lTaskID[lnum] < 0)
			{
				lret = MERR_BAD_STATE;
			}
		}

		for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
		}
	}

#else
	lret = Guass3x3Up2(pSrcImg, pDstImg, 0, lHeight);
#endif
	END_TIME;
	return lret;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END