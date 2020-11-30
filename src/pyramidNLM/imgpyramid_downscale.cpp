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
#include "imgpyramid_block_nlm.h"
#include "imgpyramid_process.h"
#include "imagebase.h"
#include "merror.h"
#include "ammem.h"
#include <math.h>


static MVoid ver_smooth(MByte* tmpSrc00, MByte* tmpSrc01, MByte* tmpSrc02, MShort* pSmoothBuf, MInt32 lWidth)
{
	MInt32 x;
	for (x = 0; x < lWidth; x++)
	{
		pSmoothBuf[x] = tmpSrc00[x] + tmpSrc02[x] + (tmpSrc01[x] << 1);
		//pSmoothBuf[x] = tmpSrc00[x] + tmpSrc02[x] + (tmpSrc01[x] * 6);
	}
	return;
}

static MVoid hor_smooth(MShort* pSmoothBuf, MByte* tmpDst, MInt32 lWidth)
{
	MInt32 x = 0;
	for (x = 0; x < lWidth; x++)
	{
		MInt32 lval = (pSmoothBuf[x] << 1) + pSmoothBuf[x - 1] + pSmoothBuf[x + 1] + 8 >> 4;
		//MInt32 lval = (pSmoothBuf[x] * 6) + pSmoothBuf[x - 1] + pSmoothBuf[x + 1] + 32 >> 6;
		tmpDst[x] = lval;
	}
	return;
}


static MVoid hor_smooth_down2_C1(MShort* SmoothBuf, MByte* tmpDst, MInt32 lSrcWidth, MInt32 lDstWidth)
{
	MInt32 x = 0, k = 0;
	for (x = 0; x < lDstWidth; x++, k += 2)
	{
		MInt32 lval = (SmoothBuf[k] << 1) + SmoothBuf[k - 1] + SmoothBuf[k + 1] + 8 >> 4;
		//MInt32 lval = (SmoothBuf[k]*6) + SmoothBuf[k - 1] + SmoothBuf[k + 1] + 32 >> 6; 
		tmpDst[x] = lval;
	}
	return;
}

static MVoid hor_smooth_down2_C2(MShort* SmoothBuf, MByte* tmpDstC1, MByte* tmpDstC2, MInt32 lSrcWidth, MInt32 lDstWidth)
{
	MInt32 x = 0, k = 0;
	for (x = 0; x < lDstWidth; x++, k += 4)
	{
		MInt32 lval = (SmoothBuf[k] << 1) + SmoothBuf[k - 2] + SmoothBuf[k + 2] + 8 >> 4;
		//MInt32 lval = (SmoothBuf[k] * 6) + SmoothBuf[k - 2] + SmoothBuf[k + 2] + 32 >> 6;
		tmpDstC1[x] = lval;
		
		lval = (SmoothBuf[k+1] << 1) + SmoothBuf[k - 1] + SmoothBuf[k + 3] + 8 >> 4;
		//lval = (SmoothBuf[k + 1] * 6) + SmoothBuf[k - 1] + SmoothBuf[k + 3] + 32 >> 6;
		tmpDstC2[x] = lval;
	} 
	return;
}


static MInt32 Img_Guass3x3_Down2_C1_Range(MHandle hMemMgr, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
								   MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch,
								   MInt32 lTopLine, MInt32 lBotLine, MShort* SmoothBuf)
{
	MInt32 lret = MOK;
	MInt32 y;
	for (y = lTopLine; y < lBotLine; y++)
	{
		MByte* tmpSrc01 = pSrcImg + y * 2 * lSrcPitch;
		MByte* tmpSrc00 = (0 == y) ? tmpSrc01 : tmpSrc01 - lSrcPitch;
		MByte* tmpSrc02 = (y * 2 == lSrcHeight - 1) ? tmpSrc01 : tmpSrc01 + lSrcPitch;
		MByte* tmpDst = pDstImg + y * lDstPitch;

		ver_smooth(tmpSrc00, tmpSrc01, tmpSrc02, SmoothBuf + 1, lSrcWidth);
		SmoothBuf[0] = SmoothBuf[1];
		SmoothBuf[lSrcWidth + 1] = SmoothBuf[lSrcWidth];
		hor_smooth_down2_C1(SmoothBuf + 1, tmpDst, lSrcWidth, lDstWidth);
	}
exit:
	return lret;
}

static MInt32 Img_Guass3x3_Down2_C2_Range(MHandle hMemMgr, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
								   MByte* pDstImgC1, MByte* pDstImgC2, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch,
								   MInt32 lTopLine, MInt32 lBotLine, MShort* SmoothBuf)
{
	MInt32 lret = MOK;
	MInt32 y;
	for (y = lTopLine; y < lBotLine; y++)
	{
		MByte* tmpSrc01 = pSrcImg + y * 2 * lSrcPitch;
		MByte* tmpSrc00 = (0 == y) ? tmpSrc01 : tmpSrc01 - lSrcPitch;
		MByte* tmpSrc02 = (y * 2 == lSrcHeight - 1) ? tmpSrc01 : tmpSrc01 + lSrcPitch;
		MByte* tmpDstC1 = pDstImgC1 + y * lDstPitch;
		MByte* tmpDstC2 = pDstImgC2 + y * lDstPitch;

		ver_smooth(tmpSrc00, tmpSrc01, tmpSrc02, SmoothBuf + 2, lSrcWidth * 2);
		SmoothBuf[0] = SmoothBuf[2];
		SmoothBuf[1] = SmoothBuf[3];
		SmoothBuf[lSrcWidth * 2 + 2] = SmoothBuf[lSrcWidth * 2];
		SmoothBuf[lSrcWidth * 2 + 3] = SmoothBuf[lSrcWidth * 2 + 1];
		hor_smooth_down2_C2(SmoothBuf + 2, tmpDstC1, tmpDstC2, lSrcWidth, lDstWidth);
	}
	return lret;
}


typedef struct _tag_IMG_DOWN_SCALE_ST{
	MHandle hMemMgr;
	MByte* pSrcImg;
	MInt32 lSrcWidth;
	MInt32 lSrcHeight;
	MInt32 lSrcPitch;
	MByte* pDstImg01; 
	MByte* pDstImg02;
	MInt32 lDstWidth;
	MInt32 lDstHeight;
	MInt32 lDstPitch;
	MInt32  topline;
	MInt32  botline;
	MShort* SmoothBuf;

	MInt32  lTaskHeight;
	MInt32* pNext_Task;
	MInt32  lTotal_TaskNum;
	MInt32  lret;
	MHandle* phEventCritical;
	MInt32   thread_ID;
} Img_Down_Scale_St, *LpImg_Down_Scale_St;



static MVoid thread_Img_GuassDownScale_C1(MVoid* pParam)
{
	LpImg_Down_Scale_St Img_Down2_sturct = (LpImg_Down_Scale_St)pParam;
	MHandle hMemMgr = Img_Down2_sturct->hMemMgr;
	MByte* pSrcImg = Img_Down2_sturct->pSrcImg;
	MInt32 lSrcWidth = Img_Down2_sturct->lSrcWidth;
	MInt32 lSrcHeight = Img_Down2_sturct->lSrcHeight;
	MInt32 lSrcPitch = Img_Down2_sturct->lSrcPitch;
	MByte* pDstImg = Img_Down2_sturct->pDstImg01;
	MInt32 lDstWidth = Img_Down2_sturct->lDstWidth;
	MInt32 lDstHeight = Img_Down2_sturct->lDstHeight;
	MInt32 lDstPitch = Img_Down2_sturct->lDstPitch;
	MShort* SmoothBuf = Img_Down2_sturct->SmoothBuf;
	MInt32 lret = MOK;

	lret = Img_Guass3x3_Down2_C1_Range(hMemMgr, pSrcImg, lSrcWidth, lSrcHeight, lSrcPitch,
		pDstImg, lDstWidth, lDstHeight, lDstPitch, Img_Down2_sturct->topline, Img_Down2_sturct->botline, SmoothBuf);
	Img_Down2_sturct->lret = lret;
}


MInt32 Img_Guass3x3_Down2_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
							 MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch)
{
	MInt32 lret = MOK;
	MByte* pDstLine;
	MInt32 lTk_Num = lDstHeight >= 1024 ? 16 : 8;
	Img_Down_Scale_St pParams[16] = { MNull };
	MInt32 lnum;
	MShort* SmoothBuf[16] = { MNull };
	
	for (lnum = 0; lnum < lTk_Num; lnum++)
	{
		SmoothBuf[lnum] = (MShort*)MMemAlloc(hMemMgr, (lSrcWidth + 2)*sizeof(MShort));
		if (MNull == SmoothBuf[lnum])
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}
	}
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
	{
		MInt32 taskID[16] = { 0 };
		MInt32 lTaskHeight = lDstHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight * lnum;
			pParams[lnum].botline = lTaskHeight * (lnum + 1);
		}
		pParams[lTk_Num - 1].botline = lDstHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].thread_ID = lnum;
			pParams[lnum].hMemMgr = hMemMgr;
			pParams[lnum].pSrcImg = pSrcImg;
			pParams[lnum].lSrcWidth = lSrcWidth;
			pParams[lnum].lSrcHeight = lSrcHeight;
			pParams[lnum].lSrcPitch = lSrcPitch;
			pParams[lnum].pDstImg01 = pDstImg;
			pParams[lnum].lDstWidth = lDstWidth;
			pParams[lnum].lDstHeight = lDstHeight;
			pParams[lnum].lDstPitch = lDstPitch;
			pParams[lnum].SmoothBuf = SmoothBuf[lnum];

			pParams[lnum].lTaskHeight = lTaskHeight;
			pParams[lnum].lTotal_TaskNum = lTk_Num;
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_GuassDownScale_C1, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}

#else
    lret =	Img_Guass3x3_Down2_C1_Range(hMemMgr, pSrcImg,  lSrcWidth,  lSrcHeight,  lSrcPitch,
										pDstImg,  lDstWidth,  lDstHeight,  lDstPitch,
										0, lSrcHeight, SmoothBuf);
#endif
	
exit:
	for (lnum = 0; lnum < lTk_Num; lnum++)
	{
		if (SmoothBuf[lnum])
		{
			MMemFree(hMemMgr, SmoothBuf[lnum]);
			SmoothBuf[lnum] = MNull;
		}
	}
	return lret;
}



static MVoid thread_Img_GuassDownScale_C2(MVoid* pParam)
{
	LpImg_Down_Scale_St Img_Down2_sturct = (LpImg_Down_Scale_St)pParam;
	MHandle hMemMgr = Img_Down2_sturct->hMemMgr;
	MByte* pSrcImg = Img_Down2_sturct->pSrcImg;
	MInt32 lSrcWidth = Img_Down2_sturct->lSrcWidth;
	MInt32 lSrcHeight = Img_Down2_sturct->lSrcHeight;
	MInt32 lSrcPitch = Img_Down2_sturct->lSrcPitch;
	MByte* pDstImgC1 = Img_Down2_sturct->pDstImg01;
	MByte* pDstImgC2 = Img_Down2_sturct->pDstImg02;
	MInt32 lDstWidth = Img_Down2_sturct->lDstWidth;
	MInt32 lDstHeight = Img_Down2_sturct->lDstHeight;
	MInt32 lDstPitch = Img_Down2_sturct->lDstPitch;
	MShort* SmoothBuf = Img_Down2_sturct->SmoothBuf;
	MInt32 lret = MOK;
	
	lret = Img_Guass3x3_Down2_C2_Range(hMemMgr, pSrcImg, lSrcWidth, lSrcHeight, lSrcPitch,
		pDstImgC1, pDstImgC2, lDstWidth, lDstHeight, lDstPitch, Img_Down2_sturct->topline, Img_Down2_sturct->botline, SmoothBuf);
	Img_Down2_sturct->lret = lret;	
}

MInt32 Img_Guass3x3_Down2_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImgC1, MByte* pDstImgC2, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch)
{
	MInt32 lret = MOK;
	MByte* pDstLine;
	MInt32 lTk_Num = lDstHeight >= 1024 ? 16 : 8;
	Img_Down_Scale_St pParams[16] = { MNull };
	MInt32 lnum;
	MShort* SmoothBuf[16] = { MNull };

	for (lnum = 0; lnum < lTk_Num; lnum++)
	{
		SmoothBuf[lnum] = (MShort*)MMemAlloc(hMemMgr, (lSrcWidth + 2) * 2 * sizeof(MShort)); 
		if (MNull == SmoothBuf[lnum])
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}
	}
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
	{
		MInt32 taskID[16] = { 0 };
		MInt32 lTaskHeight = lDstHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight * lnum;
			pParams[lnum].botline = lTaskHeight * (lnum + 1);
		}
		pParams[lTk_Num - 1].botline = lDstHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].thread_ID = lnum;
			pParams[lnum].hMemMgr = hMemMgr;
			pParams[lnum].pSrcImg = pSrcImg;
			pParams[lnum].lSrcWidth = lSrcWidth;
			pParams[lnum].lSrcHeight = lSrcHeight;
			pParams[lnum].lSrcPitch = lSrcPitch;
			pParams[lnum].pDstImg01 = pDstImgC1;
			pParams[lnum].pDstImg02 = pDstImgC2;
			pParams[lnum].lDstWidth = lDstWidth;
			pParams[lnum].lDstHeight = lDstHeight;
			pParams[lnum].lDstPitch = lDstPitch;
			pParams[lnum].SmoothBuf = SmoothBuf[lnum];

			pParams[lnum].lTaskHeight = lTaskHeight;
			pParams[lnum].lTotal_TaskNum = lTk_Num;
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_GuassDownScale_C2, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}

#else
	lret = Img_Guass3x3_Down2_C2_Range(hMemMgr, pSrcImg, lSrcWidth, lSrcHeight, lSrcPitch,
			pDstImgC1, pDstImgC2, lDstWidth, lDstHeight, lDstPitch,
			0, lSrcHeight, SmoothBuf);
#endif


exit:
	for (lnum = 0; lnum < lTk_Num; lnum++)
	{
		if (SmoothBuf[lnum])
		{
			MMemFree(hMemMgr, SmoothBuf[lnum]);
			SmoothBuf[lnum] = MNull;
		}
	}
	return lret;
}


MInt32 Img_Haar_Down2_C1(MHandle hMemMgr, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
						 MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch)
{
	MInt32 lret = MOK;
	MInt32 y, x;
	for (y = 0; y < lDstHeight; y++)
	{
		MByte* tmpSrc00 = pSrcImg + y * 2 * lSrcPitch;
		MByte* tmpSrc01 = (y * 2 == lSrcHeight - 1) ? tmpSrc00 : tmpSrc00 + lSrcPitch;
		MByte* tmpDst = pDstImg + y * lDstPitch;
		MInt32 k = 0;
		for (x = 0; x < lDstWidth-1; x++, k+=2)
		{
			tmpDst[x] = tmpSrc00[k] + tmpSrc00[k + 1] + tmpSrc01[k] + tmpSrc01[k + 1] + 2 >> 2;
		}
		if (lSrcWidth == lSrcWidth>>1<<1)
		{
			tmpDst[x] = tmpSrc00[k] + tmpSrc00[k + 1] + tmpSrc01[k] + tmpSrc01[k + 1] + 2 >> 2;
		}
		else
		{
			tmpDst[x] = tmpSrc00[k] + tmpSrc01[k] + 1 >> 1;
		}
	}

exit:

	return MOK;
}

MInt32 Img_Haar_Down2_C2(MHandle hMemMgr, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
						 MByte* pDstImgC1, MByte* pDstImgC2, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch)
{
	MInt32 lret = MOK;
	MInt32 y, x;
	for (y = 0; y < lDstHeight; y++)
	{
		MByte* tmpSrc00 = pSrcImg + y * 2 * lSrcPitch;
		MByte* tmpSrc01 = (y * 2 == lSrcHeight - 1) ? tmpSrc00 : tmpSrc00 + lSrcPitch;
		MByte* tmpDstC1 = pDstImgC1 + y * lDstPitch;
		MByte* tmpDstC2 = pDstImgC2 + y * lDstPitch;
		MInt32 k = 0;
		for (x = 0; x < lDstWidth - 1; x++, k += 4)
		{
			tmpDstC1[x] = tmpSrc00[k]	+ tmpSrc00[k + 2] + tmpSrc01[k]   + tmpSrc01[k + 2] + 2 >> 2;
			tmpDstC2[x] = tmpSrc00[k+1] + tmpSrc00[k + 3] + tmpSrc01[k+1] + tmpSrc01[k + 3] + 2 >> 2;
		}
		if (lSrcWidth == lSrcWidth >> 1 << 1)
		{
			tmpDstC1[x] = tmpSrc00[k] + tmpSrc00[k + 2] + tmpSrc01[k] + tmpSrc01[k + 2] + 2 >> 2;
			tmpDstC2[x] = tmpSrc00[k + 1] + tmpSrc00[k + 3] + tmpSrc01[k + 1] + tmpSrc01[k + 3] + 2 >> 2;
		}
		else
		{
			tmpDstC1[x] = tmpSrc00[k] + tmpSrc01[k] + 1 >> 1;
			tmpDstC2[x] = tmpSrc00[k + 1] + tmpSrc01[k + 1] + 1 >> 1;
		}
	}

exit:
	return MOK;
}
