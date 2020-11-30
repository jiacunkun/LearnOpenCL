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
#include "imagebase.h"
#include "merror.h"
#include "ammem.h"
#include <math.h>
#include "NEONvsSSE.h"


MInt32 Local_ImgData(MHandle hMemMgr, LPImg_Plane_Data pImgData, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lCn)
{
	MInt32 lret = MOK;
	if (MNull == pImgData)
	{
		lret = MERR_INVALID_PARAM;
		goto exit;
	}
	MMemSet(pImgData, 0, sizeof(Img_Plane_Data));
	pImgData->lWidth = lWidth;
	pImgData->lHeight = lHeight;
	pImgData->lPitch = lPitch;
	pImgData->lCn = lCn;
	pImgData->pImage = (MByte*)MMemAlloc(hMemMgr, lHeight * lPitch * sizeof(MByte));
exit:
	return lret;
}


MVoid Free_ImgData(MHandle hMemMgr, LPImg_Plane_Data pImgData)
{
	if (pImgData->pImage)
	{
		MMemFree(hMemMgr, pImgData->pImage);
	}
	MMemSet(pImgData, 0, sizeof(Img_Plane_Data));
	return;
}

MVoid Free_Image_Pyramid_Mem(MHandle hMemMgr, LPImg_Plane_Data Y_Data, LPImg_Plane_Data UVData,	 MInt32 lLevel)
{
	MInt32 lL = 0;
	for (lL = 1; lL < lLevel; lL++)
	{
		if (Y_Data[lL].pImage)
		{
			Free_ImgData(hMemMgr, &Y_Data[lL]);
		}
		if (UVData)
		{
			if (UVData[2 * lL].pImage)
			{
				Free_ImgData(hMemMgr, &UVData[2 * lL]);
			}
			if (UVData[2 * lL + 1].pImage)
			{
				Free_ImgData(hMemMgr, &UVData[2 * lL + 1]);
			}
		}
	}
	return;
}


MInt32 Alloc_Image_Pyramid_Mem(MHandle hMemMgr, LPImg_Plane_Data Y_Data, LPImg_Plane_Data UVData,
	MInt32 lWidth, MInt32 lHeight, MInt32 lLevel, MBool  bY_Flag, MBool bUV_Flag)
{
	MInt32 lret = MOK;
	MInt32 lL = 0;
	if (bY_Flag)
	{
		MInt32 lTmpWidth = lWidth;
		MInt32 lTmpHeight = lHeight;
		MInt32 lTmpPitch = 0;
		for (lL = 1; lL < lLevel; lL++)
		{
			lTmpWidth = lTmpWidth + 1 >> 1;
			lTmpHeight = lTmpHeight + 1 >> 1;
			lTmpPitch = lTmpWidth + 3 >> 2 << 2;

			lret = Local_ImgData(hMemMgr, &Y_Data[lL], lTmpWidth, lTmpHeight, lTmpPitch, 1);
			if (MOK != lret)
			{
				goto exit;
			}
		}
	}
	if (bUV_Flag)
	{
		MInt32 lTmpUVWidth = lWidth >> 1;
		MInt32 lTmpUVHeight = lHeight >> 1;
		MInt32 lTmpUVPitch = 0;
		for (lL = 1; lL < lLevel; lL++)
		{
			lTmpUVWidth = lTmpUVWidth + 1 >> 1;
			lTmpUVHeight = lTmpUVHeight + 1 >> 1;
			lTmpUVPitch = lTmpUVWidth + 3 >> 2 << 2;
			if (bUV_Flag)
			{
				lret = Local_ImgData(hMemMgr, &UVData[lL * 2], lTmpUVWidth, lTmpUVHeight, lTmpUVPitch, 1);
				if (MOK != lret)
				{
					goto exit;
				}
				lret = Local_ImgData(hMemMgr, &UVData[lL * 2 + 1], lTmpUVWidth, lTmpUVHeight, lTmpUVPitch, 1);
				if (MOK != lret)
				{
					goto exit;
				}
			}
		}
	}
exit:
	return lret;
}



static MVoid Img_Sub_C1_Range(LPImg_Plane_Data pSrcDstImg, LPImg_Plane_Data pSubImg, MInt32 lTopLine, MInt32 lBotLine)
{
	MByte* pSrcDstData = pSrcDstImg->pImage;
	MByte* pSubData = pSubImg->pImage;
	MInt32 lWidth = pSrcDstImg->lWidth;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lPitch = pSrcDstImg->lPitch;
	MInt32 x, y;

#ifdef CV_NEON
	uint8x16_t srcdata, subdata;
	uint16x8_t tmpdata;
	uint8x8_t  resdata;
#endif
	for (y = lTopLine; y < lBotLine; y++)
	{
		MByte* tmpSrcDst = pSrcDstData + y * lPitch;
		MByte* tmpSub = pSubData + y * lPitch;

		x = 0;
#ifdef CV_NEON
		for (; x < lWidth - 15; x += 16)
		{
			srcdata = vld1q_u8(tmpSrcDst + x);
			subdata = vld1q_u8(tmpSub + x);

			tmpdata = vaddl_u8(vget_low_u8(srcdata), vdup_n_u8(128));
			tmpdata = vsubw_u8(tmpdata, vget_low_u8(subdata));
			resdata = vmovn_u16(tmpdata);
			vst1_u8(tmpSrcDst + x, resdata);

			tmpdata = vaddl_u8(vget_high_u8(srcdata), vdup_n_u8(128));
			tmpdata = vsubw_u8(tmpdata, vget_high_u8(subdata));
			resdata = vmovn_u16(tmpdata);
			vst1_u8(tmpSrcDst + x + 8, resdata);
		}
		for (; x < lWidth; x++)
		{
			MInt32 lVal = tmpSrcDst[x] - tmpSub[x] + 128;
			tmpSrcDst[x] = TRIMBYTE(lVal);
		}
#else
		for (x = 0; x < lWidth; x++)
		{
			MInt32 lVal = tmpSrcDst[x] - tmpSub[x] + 128;
			tmpSrcDst[x] = TRIMBYTE(lVal);
		}
#endif
	}
	return;
}



static MVoid Img_Add_C1_Range(LPImg_Plane_Data pSrcDstImg, LPImg_Plane_Data pAddImg, MInt32 lTopLine, MInt32 lBotLine)
{
	MByte* pSrcDstData = pSrcDstImg->pImage;
	MByte* pAddData = pAddImg->pImage;
	MInt32 lWidth = pSrcDstImg->lWidth;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lPitch = pSrcDstImg->lPitch;
	MInt32 x, y;
	for (y = lTopLine; y < lBotLine; y++)
	{
		MByte* tmpSrcDst = pSrcDstData + y * lPitch;
		MByte* tmpAdd = pAddData + y * lPitch;
		for (x = 0; x < lWidth; x++)
		{
			MInt32 lVal = tmpSrcDst[x] + tmpAdd[x] - 128;
			tmpSrcDst[x] = TRIMBYTE(lVal);
		}
	}
	return;
}



static MVoid Img_Add_C2_Range(LPImg_Plane_Data pSrcDstImg, LPImg_Plane_Data pAddImg01, LPImg_Plane_Data pAddImg02, MInt32 lTopLine, MInt32 lBotLine)
{
	MByte* pSrcDstData = pSrcDstImg->pImage;
	MByte* pAddData01 = pAddImg01->pImage;
	MByte* pAddData02 = pAddImg02->pImage;
	MInt32 lWidth = pSrcDstImg->lWidth;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lPitchSrc = pSrcDstImg->lPitch;
	MInt32 lPitchAdd = pAddImg01->lPitch;
	MInt32 x, y;
#ifdef CV_NEON
	uint8x8x2_t srcdata, resdata;
	uint8x8_t  adddata00, adddata01;
	uint16x8_t tmpdata;
	uint16x8_t vconst_128 = vdupq_n_u16(128);
#endif
	for (y = lTopLine; y < lBotLine; y++)
	{
		MByte* tmpSrcDst = pSrcDstData + y * lPitchSrc;
		MByte* tmpAdd01 = pAddData01 + y * lPitchAdd;
		MByte* tmpAdd02 = pAddData02 + y * lPitchAdd;
		MInt32 k;

		x = 0;
		k = 0;
#ifdef CV_NEON
		for (; x < lWidth - 7; x += 8, k += 16)
		{
			adddata00 = vld1_u8(tmpAdd01 + x);
			adddata01 = vld1_u8(tmpAdd02 + x);
			srcdata = vld2_u8(tmpSrcDst + k);
			tmpdata = vaddl_u8(srcdata.val[0], adddata00);
			tmpdata = vqsubq_u16(tmpdata, vconst_128);
			resdata.val[0] = vqmovn_u16(tmpdata);
			tmpdata = vaddl_u8(srcdata.val[1], adddata01);
			tmpdata = vqsubq_u16(tmpdata, vconst_128);
			resdata.val[1] = vqmovn_u16(tmpdata);
			vst2_u8(tmpSrcDst + k, resdata);
		}
		for (; x < lWidth; x++, k += 2)
		{
			MInt32 lVal01 = tmpSrcDst[k] + tmpAdd01[x] - 128;
			MInt32 lVal02 = tmpSrcDst[k + 1] + tmpAdd02[x] - 128;
			tmpSrcDst[k] = TRIMBYTE(lVal01);
			tmpSrcDst[k+1] = TRIMBYTE(lVal02);
		}
#else
		for (x = 0, k = 0; x < lWidth; x++, k += 2)
		{
			MInt32 lVal01 = tmpSrcDst[k]   + tmpAdd01[x] - 128;
			MInt32 lVal02 = tmpSrcDst[k+1] + tmpAdd02[x] - 128;
			tmpSrcDst[k] = TRIMBYTE(lVal01);
			tmpSrcDst[k+1] = TRIMBYTE(lVal02);
		}
#endif
	}
	return;
}

typedef struct _tag_IMG_ADD_SUB_ST{
	LPImg_Plane_Data pSrcDstImg;
	LPImg_Plane_Data pSubImg;
	LPImg_Plane_Data pAddImg01;
	LPImg_Plane_Data pAddImg02;
	MInt32  topline;
	MInt32  botline;

	MInt32  lTaskHeight;
	MInt32* pNext_Task;
	MInt32  lTotal_TaskNum;
	MInt32  lret;
	MInt32  pcbnum;
	MHandle pg_asp_sem;
	MHandle* phEventCritical;
	MInt32   thread_ID;
} Img_Add_Sub_St, *LpImg_Add_Sub_St;


static MVoid thread_Img_Add_C1(MVoid* pParam)
{
	LpImg_Add_Sub_St Img_Sub_sturct = (LpImg_Add_Sub_St)pParam;
	LPImg_Plane_Data pSrcDstImg = Img_Sub_sturct->pSrcDstImg;
	LPImg_Plane_Data pAddImg = Img_Sub_sturct->pAddImg01;
	MInt32 lTopLine = Img_Sub_sturct->topline;
	MInt32 lBotLine = Img_Sub_sturct->botline;
	MInt32 lret = MOK;

	Img_Add_C1_Range(pSrcDstImg, pAddImg, lTopLine, lBotLine);
}


static MVoid thread_Img_Add_C2(MVoid* pParam)
{
	LpImg_Add_Sub_St Img_Sub_sturct = (LpImg_Add_Sub_St)pParam;
	LPImg_Plane_Data pSrcDstImg = Img_Sub_sturct->pSrcDstImg;
	LPImg_Plane_Data pAddImg01 = Img_Sub_sturct->pAddImg01;
	LPImg_Plane_Data pAddImg02 = Img_Sub_sturct->pAddImg02;
	MInt32 lTopLine = Img_Sub_sturct->topline;
	MInt32 lBotLine = Img_Sub_sturct->botline;
	MInt32 lret = MOK;

	Img_Add_C2_Range(pSrcDstImg, pAddImg01, pAddImg02, lTopLine, lBotLine);
}


static MVoid thread_Img_Sub_C1(MVoid* pParam)
{
	LpImg_Add_Sub_St Img_Sub_sturct = (LpImg_Add_Sub_St)pParam;
	LPImg_Plane_Data pSrcDstImg = Img_Sub_sturct->pSrcDstImg;
	LPImg_Plane_Data pSubImg = Img_Sub_sturct->pSubImg;
	MInt32 lTopLine = Img_Sub_sturct->topline;
	MInt32 lBotLine = Img_Sub_sturct->botline;
	MInt32 lret = MOK;

	Img_Sub_C1_Range(pSrcDstImg, pSubImg, lTopLine, lBotLine);
	Img_Sub_sturct->lret = MOK;
}

MVoid Img_Sub_C1(MHandle mcvParallelMonitor, LPImg_Plane_Data  pSrcDstImg, LPImg_Plane_Data pSubImg)
{
	MInt32 lret = MOK;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lnum;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
	{
		const MInt32 lTk_Num = 8;
		MInt32 taskID[lTk_Num] = { 0 };
		Img_Add_Sub_St pParams[lTk_Num] = { MNull };
		MInt32 lTaskHeight = lHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight * lnum;
			pParams[lnum].botline = lTaskHeight * (lnum + 1);
		}
		pParams[lTk_Num - 1].botline = lHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].thread_ID = lnum;
			pParams[lnum].pSrcDstImg = pSrcDstImg;
			pParams[lnum].pSubImg = pSubImg;
		}

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_Sub_C1, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}
#else
	Img_Sub_C1_Range(pSrcDstImg,  pSubImg,  lTopLine, lBotLine);
#endif
exit:
	return;
}


MVoid Img_Add_C1(MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcDstImg, LPImg_Plane_Data pAddImg)
{
	MInt32 lret = MOK;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lnum;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
	{
		const MInt32 lTk_Num = 8;
		MInt32 taskID[lTk_Num] = { 0 };
		Img_Add_Sub_St pParams[lTk_Num] = { MNull };
		MInt32 lTaskHeight = lHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight * lnum;
			pParams[lnum].botline = lTaskHeight * (lnum + 1);
		}
		pParams[lTk_Num - 1].botline = lHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].thread_ID = lnum;
			pParams[lnum].pSrcDstImg = pSrcDstImg;
			pParams[lnum].pAddImg01 = pAddImg;
		}

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_Add_C1, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}

#else
	Img_Add_C1_Range(pSrcDstImg, pAddImg, lTopLine, lBotLine);
#endif
exit:
	return;
}


MVoid Img_Add_C2(MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcDstImg, LPImg_Plane_Data pAddImg01, LPImg_Plane_Data pAddImg02)
{
	MInt32 lret = MOK;
	MInt32 lHeight = pSrcDstImg->lHeight;
	MInt32 lnum;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
	{
		const MInt32 lTk_Num = 8;
		MInt32 taskID[lTk_Num] = { 0 };
		Img_Add_Sub_St pParams[lTk_Num] = { MNull };
		MInt32 lTaskHeight = lHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight * lnum;
			pParams[lnum].botline = lTaskHeight * (lnum + 1);
		}
		pParams[lTk_Num - 1].botline = lHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].thread_ID = lnum;
			pParams[lnum].pSrcDstImg = pSrcDstImg;
			pParams[lnum].pAddImg01 = pAddImg01;
			pParams[lnum].pAddImg02 = pAddImg02;
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_Add_C2, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}
#else
	Img_Add_C2_Range(pSrcDstImg, pAddImg01, pAddImg02, lTopLine, lBotLine);
#endif
exit:
	return;
}




MVoid Img_Add_C2_To_C1(MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pAddDstImg01, LPImg_Plane_Data pAddDstImg02)
{
	MInt32 lret = MOK;	
	MInt32 lWidth = pSrcImg->lWidth;
	MInt32 lHeight = pSrcImg->lHeight;
	MByte* pSrcData = pSrcImg->pImage;
	MInt32 lSrcPitch = pSrcImg->lPitch;
	MByte* pAddDstData01 = pAddDstImg01->pImage;
	MByte* pAddDstData02 = pAddDstImg02->pImage;
	MInt32 lDstPitch = pAddDstImg01->lPitch;

	MInt32 x, y;
	for (y = 0; y < lHeight; y++)
	{
		MByte* tmpSrc = pSrcData + y * lSrcPitch;
		MByte* tmpAddDst01 = pAddDstData01 + y * lDstPitch;
		MByte* tmpAddDst02 = pAddDstData02 + y * lDstPitch;
		MInt32 k = 0;
		for (x = 0; x < lWidth; x++, k += 2)
		{
			MInt32 lVal01 = tmpSrc[k] + tmpAddDst01[x] - 128;
			MInt32 lVal02 = tmpSrc[k + 1] + tmpAddDst02[x] - 128;
			tmpAddDst01[x] = TRIMBYTE(lVal01);
			tmpAddDst02[x] = TRIMBYTE(lVal02);
		}
	}
exit:
	return;
}





MVoid Img_Meger_C1_To_C2(MHandle mcvParallelMonitor, LPImg_Plane_Data pDstImgC2, LPImg_Plane_Data pSrcImg01, LPImg_Plane_Data pSrcImg02)
{
	MInt32 lret = MOK;
	MInt32 lWidth = pDstImgC2->lWidth;
	MInt32 lHeight = pDstImgC2->lHeight;
	MByte* pDstDataC2 = pDstImgC2->pImage;
	MInt32 lDstPitch = pDstImgC2->lPitch;
	MByte* pSrcData01 = pSrcImg01->pImage;
	MByte* pSrcData02 = pSrcImg02->pImage;
	MInt32 lSrcPitch = pSrcImg01->lPitch;

	MInt32 x, y;
	for (y = 0; y < lHeight; y++)
	{
		MByte* tmpDstC2 = pDstDataC2 + y * lDstPitch;
		MByte* tmpSrc01 = pSrcData01 + y * lSrcPitch;
		MByte* tmpSrc02 = pSrcData02 + y * lSrcPitch;
		MInt32 k = 0;
		for (x = 0; x < lWidth; x++, k += 2)
		{		
			tmpDstC2[k] = tmpSrc01[x];
			tmpDstC2[k + 1] = tmpSrc02[x];
		}
	}
exit:
	return;
}

