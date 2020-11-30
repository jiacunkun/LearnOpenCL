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
#include "NEONvsSSE.h"

static MVoid  Rem_Line_Detail_Merger_C1(MByte* pFull_Line, MInt32* pBuf, MInt32 lFullWidth)
{
	MInt32 x;
	for (x = 0; x < lFullWidth; x++)
	{
		MInt32 lval = pFull_Line[x] + pBuf[x];
		pFull_Line[x] = TRIMBYTE(lval);
	}
	return;
}

static MVoid  Rem_Line_Detail_Merger_C2(MByte* pFull_Line, MInt32* pBuf, MInt32 lFullWidth)
{
	MInt32 x;
	for (x = 0; x < lFullWidth*2; x++)
	{
		MInt32 lval = pFull_Line[x] + pBuf[x];
		pFull_Line[x] = TRIMBYTE(lval);
	}
	return;
}

static MVoid  Odd_Line_Detail_Merger_C1(MByte* pFull_Line, MByte*  pHalf_Line, MInt32* pBuf, MInt32 lFullWidth, MInt32 lHalfWidth)
{
	MInt32 x = 0;
	MInt32 k = 0;
	MInt32 lval;

	pBuf[0] = (pHalf_Line[0] - 128) << 1;
	lval = pFull_Line[0] + pBuf[0];
	pFull_Line[0] = TRIMBYTE(lval);
	k = 1;
	for (x = 1; x < lHalfWidth; x++)
	{
		pBuf[k] = pHalf_Line[x] + pHalf_Line[x - 1] - 256;
		lval =  pFull_Line[k] + pBuf[k];
		pFull_Line[k] = TRIMBYTE(lval);
		k++;

		pBuf[k] = (pHalf_Line[x] - 128) << 1;;
		lval = pFull_Line[k] + pBuf[k];
		pFull_Line[k] = TRIMBYTE(lval);
		k++;
	}
	for (; k < lFullWidth; k++)
	{
		pBuf[k] = pBuf[k - 1];
		lval = pFull_Line[k] + pBuf[k];
		pFull_Line[k] = TRIMBYTE(lval);
	}
	return;
}


static MVoid  Odd_Line_Detail_Merger_C2(MByte* pFull_Line, MByte*  pHalf_LineU, MByte*  pHalf_LineV, MInt32* pBuf, MInt32 lFullWidth, MInt32 lHalfWidth)
{
	MInt32 x = 0;
	MInt32 k = 0;
	MInt32 lval0, lval1;

	pBuf[0] = (pHalf_LineU[0] - 128) << 1;
	pBuf[1] = (pHalf_LineV[0] - 128) << 1;
	lval0 = pFull_Line[0] + pBuf[0];
	lval1 = pFull_Line[1] + pBuf[1];
	pFull_Line[0] = TRIMBYTE(lval0);
	pFull_Line[1] = TRIMBYTE(lval1);
	k = 2;
	for (x = 1; x < lHalfWidth; x++)
	{
		pBuf[k]		= pHalf_LineU[x] + pHalf_LineU[x - 1] - 256;
		pBuf[k + 1] = pHalf_LineV[x] + pHalf_LineV[x - 1] - 256;
		lval0 = pFull_Line[k]   + pBuf[k];
		lval1 = pFull_Line[k+1] + pBuf[k+1];
		pFull_Line[k]   = TRIMBYTE(lval0);
		pFull_Line[k+1] = TRIMBYTE(lval1);
		k+=2;

		pBuf[k]   = (pHalf_LineU[x] - 128) << 1;
		pBuf[k+1] = (pHalf_LineV[x] - 128) << 1;
		lval0 = pFull_Line[k]     + pBuf[k];
		lval1 = pFull_Line[k + 1] + pBuf[k + 1];
		pFull_Line[k]     = TRIMBYTE(lval0);
		pFull_Line[k + 1] = TRIMBYTE(lval1);
		k+=2;
	}
	for (; k < lFullWidth * 2; k += 2)
	{
		pBuf[k]   = pBuf[k - 2];
		pBuf[k+1] = pBuf[k - 1];
		lval0 = pFull_Line[k]   + pBuf[k];
		lval1 = pFull_Line[k+1] + pBuf[k+1];
		pFull_Line[k]	= TRIMBYTE(lval0);
		pFull_Line[k+1] = TRIMBYTE(lval1);
	}
	return;
}


static MVoid Even_Odd_Line_Detail_Merger_C1(MByte* pFull_Line00, MByte* pFull_Line01, MByte*  pHalf_Line, MInt32* pPreBuf, MInt32*  pCurBuf, MInt32 lFullWidth, MInt32 lHalfWidth)
{

	MInt32 x = 0;
	MInt32 k = 0;
	MInt32 lVal;
	MInt32 lVal00, lVal01;

	pCurBuf[0] = pHalf_Line[0] - 128 << 1;
	lVal = pCurBuf[0] + pPreBuf[0] + 1 >> 1;
	lVal00 = pFull_Line00[0] + lVal;
	lVal01 = pFull_Line01[0] + pCurBuf[0];
	pFull_Line00[0] = TRIMBYTE(lVal00);
	pFull_Line01[0] = TRIMBYTE(lVal01);

	k = 1;
	for (x = 1; x < lHalfWidth; x++)
	{
		pCurBuf[k] = pHalf_Line[x] + pHalf_Line[x - 1] - 256;
		lVal = (pCurBuf[k] + pPreBuf[k] + 1) >> 1;	
		lVal00 = pFull_Line00[k] + lVal;
		lVal01 = pFull_Line01[k] + pCurBuf[k];
		pFull_Line00[k] = TRIMBYTE(lVal00);
		pFull_Line01[k] = TRIMBYTE(lVal01);	
		k++;

		pCurBuf[k] = (pHalf_Line[x] - 128) <<1;
		lVal = (pCurBuf[k] + pPreBuf[k] + 1) >> 1;
		lVal00 = pFull_Line00[k] + lVal;
		lVal01 = pFull_Line01[k] + pCurBuf[k];
		pFull_Line00[k] = TRIMBYTE(lVal00);	
		pFull_Line01[k] = TRIMBYTE(lVal01);	
		k++;
	}
	for (; k < lFullWidth; k++)
	{
		pCurBuf[k] = pCurBuf[k - 1];
		lVal = pCurBuf[k] + pPreBuf[k] + 1 >> 1;
		lVal00 = pFull_Line00[k] + lVal;
		lVal01 = pFull_Line01[k] + pCurBuf[k];	
		pFull_Line00[k] = TRIMBYTE(lVal00);
		pFull_Line01[k] = TRIMBYTE(lVal01);	
	}
	return;
}




static MVoid Even_Odd_Line_Detail_Merger_C2(MByte* pFull_Line00, MByte* pFull_Line01, MByte* pHalf_LineU, MByte* pHalf_LineV,
											MInt32* pPreBuf, MInt32*  pCurBuf, MInt32 lFullWidth, MInt32 lHalfWidth)
{
	MInt32 x = 0;
	MInt32 k = 0;
	MInt32 lVal0, lVal1;
	MInt32 lVal00, lVal01;
	MInt32 lVal10, lVal11;

	pCurBuf[0] = pHalf_LineU[0] - 128 << 1;
	pCurBuf[1] = pHalf_LineV[0] - 128 << 1;
	lVal0 = pCurBuf[0] + pPreBuf[0] + 1 >> 1;
	lVal1 = pCurBuf[1] + pPreBuf[1] + 1 >> 1;

	lVal00 = pFull_Line00[0] + lVal0;
	pFull_Line00[0] = TRIMBYTE(lVal00);
	lVal01 = pFull_Line01[0] + pCurBuf[0];
	pFull_Line01[0] = TRIMBYTE(lVal01);

	lVal10 = pFull_Line00[1] + lVal1;
	pFull_Line00[1] = TRIMBYTE(lVal10);
	lVal11 = pFull_Line01[1] + pCurBuf[1];
	pFull_Line01[1] = TRIMBYTE(lVal11);


	k = 2;
	for (x = 1; x < lHalfWidth; x++)
	{
		pCurBuf[k]	   = pHalf_LineU[x] + pHalf_LineU[x - 1] - 256;
		pCurBuf[k + 1] = pHalf_LineV[x] + pHalf_LineV[x - 1] - 256;
		lVal0 = (pCurBuf[k]		+ pPreBuf[k] + 1) >> 1;
		lVal1 = (pCurBuf[k + 1] + pPreBuf[k + 1] + 1) >> 1;	
		lVal00 = pFull_Line00[k] + lVal0;
		lVal01 = pFull_Line01[k] + pCurBuf[k];
		pFull_Line00[k] = TRIMBYTE(lVal00);	
		pFull_Line01[k] = TRIMBYTE(lVal01);
		lVal10 = pFull_Line00[k + 1] + lVal1;
		lVal11 = pFull_Line01[k + 1] + pCurBuf[k + 1];
		pFull_Line00[k + 1] = TRIMBYTE(lVal10);
		pFull_Line01[k + 1] = TRIMBYTE(lVal11);
	
		k += 2;

		pCurBuf[k]	   = (pHalf_LineU[x] - 128) << 1;
		pCurBuf[k + 1] = (pHalf_LineV[x] - 128) << 1;
		lVal0 = (pCurBuf[k]		+ pPreBuf[k] + 1) >> 1;
		lVal1 = (pCurBuf[k + 1] + pPreBuf[k + 1] + 1) >> 1;	
		lVal00 = pFull_Line00[k] + lVal0;
		lVal01 = pFull_Line01[k] + pCurBuf[k];	
		pFull_Line00[k] = TRIMBYTE(lVal00);
		pFull_Line01[k] = TRIMBYTE(lVal01);	
		lVal10 = pFull_Line00[k + 1] + lVal1;	
		lVal11 = pFull_Line01[k + 1] + pCurBuf[k + 1];	
		pFull_Line00[k + 1] = TRIMBYTE(lVal10);
		pFull_Line01[k + 1] = TRIMBYTE(lVal11);

		k += 2;
	}
	for (; k < lFullWidth*2; k+=2)
	{
		pCurBuf[k]	   = pCurBuf[k - 2];
		pCurBuf[k + 1] = pCurBuf[k - 1];
		lVal0 = (pCurBuf[k]		+ pPreBuf[k] + 1) >> 1;
		lVal1 = (pCurBuf[k + 1] + pPreBuf[k + 1] + 1) >> 1;			
		lVal00 = pFull_Line00[k] + lVal0;
		lVal01 = pFull_Line01[k] + pCurBuf[k];
		pFull_Line00[k] = TRIMBYTE(lVal00);
		pFull_Line01[k] = TRIMBYTE(lVal01);
		lVal10 = pFull_Line00[k + 1] + lVal1;
		lVal11 = pFull_Line01[k + 1] + pCurBuf[k + 1];		
		pFull_Line00[k + 1] = TRIMBYTE(lVal10);
		pFull_Line01[k + 1] = TRIMBYTE(lVal11);
	
	}
	return;
}

MInt32 Img_expand_C1(MHandle hMemMgr, LPImg_Plane_Data pFullImg, LPImg_Plane_Data pHalfImg)
{
	MInt32 lret = MOK;
	MByte* pSrcDstData = pFullImg->pImage;
	MInt32 lFullWidth  = pFullImg->lWidth;
	MInt32 lFullHeight = pFullImg->lHeight;
	MInt32 lFullPitch = pFullImg->lPitch;
	MByte* pHalfData  = pHalfImg->pImage;
	MInt32 lHalfWidth = pHalfImg->lWidth;
	MInt32 lHalfHeight = pHalfImg->lHeight;
	MInt32 lHalfPitch = pHalfImg->lPitch;
	MInt32 *pBufData = MNull;
	MInt32 *pBuf[2] = { MNull };

	MInt32 y, k;

	MByte* tmpFullImg00 = pSrcDstData, *tmpFullImg01 = pSrcDstData;
	MByte* tmppHalfImg = pHalfData;

	pBufData = (MInt32*)MMemAlloc(hMemMgr, lFullWidth * 2 * sizeof(MInt32));
	if (MNull == pBufData)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	pBuf[0] = pBufData;
	pBuf[1] = pBufData + lFullWidth;

	Odd_Line_Detail_Merger_C1(tmpFullImg00, pHalfData, pBuf[0], lFullWidth, lHalfWidth);
	k = 1;

	for (y = 1; y < lHalfHeight; y++, k += 2)
	{
		MInt32* tmpBuf;
		tmpFullImg00 = pSrcDstData + k * lFullPitch;
		tmpFullImg01 = tmpFullImg00 + lFullPitch;
		tmppHalfImg = pHalfData + y * lHalfPitch;
		Even_Odd_Line_Detail_Merger_C1(tmpFullImg00, tmpFullImg01, tmppHalfImg, pBuf[0], pBuf[1], lFullWidth, lHalfWidth);
		tmpBuf = pBuf[0];
		pBuf[0] = pBuf[1];
		pBuf[1] = tmpBuf;
	}
	for (; k < lFullHeight; k++)
	{
		tmpFullImg00 = pSrcDstData + k * lFullPitch;
		Rem_Line_Detail_Merger_C1(tmpFullImg00, pBuf[0], lFullWidth);
	}
exit:
	if (pBufData)
	{
		MMemFree(hMemMgr, pBufData);
		pBufData = MNull;
	}
	return lret;
}




MInt32 Img_expand_C2(MHandle hMemMgr, LPImg_Plane_Data pFullImgUV, LPImg_Plane_Data pHalfImgU, LPImg_Plane_Data pHalfImgV)
{
	MInt32 lret = MOK;
	MByte* pSrcDstData = pFullImgUV->pImage;
	MInt32 lFullWidth  = pFullImgUV->lWidth;
	MInt32 lFullHeight = pFullImgUV->lHeight;
	MInt32 lFullPitch = pFullImgUV->lPitch;
	MByte* pHalfDataU = pHalfImgU->pImage;
	MByte* pHalfDataV = pHalfImgV->pImage;
	MInt32 lHalfWidth = pHalfImgU->lWidth;
	MInt32 lHalfHeight = pHalfImgU->lHeight;
	MInt32 lHalfPitch = pHalfImgU->lPitch;
	MInt32 *pBufData = MNull;
	MInt32 *pBuf[2] = { MNull };


	MInt32 y, k;
	MByte* tmpFullImg00 = pSrcDstData, *tmpFullImg01 = pSrcDstData;
	MByte* tmppHalfImgU = pHalfDataU, *tmppHalfImgV = pHalfDataV;
		
	pBufData = (MInt32*)MMemAlloc(hMemMgr, lFullWidth * 2 * 2 * sizeof(MInt32));
	if (MNull == pBufData)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	pBuf[0] = pBufData;
	pBuf[1] = pBufData + lFullWidth * 2;

	Odd_Line_Detail_Merger_C2(tmpFullImg00, pHalfDataU, pHalfDataV, pBuf[0], lFullWidth, lHalfWidth);
	k = 1;

	for (y = 1; y < lHalfHeight; y++, k += 2)
	{
		MInt32* tmpBuf;
		tmpFullImg00 = pSrcDstData + k * lFullPitch;
		tmpFullImg01 = tmpFullImg00 + lFullPitch;
		tmppHalfImgU = pHalfDataU + y * lHalfPitch;
		tmppHalfImgV = pHalfDataV + y * lHalfPitch;
		Even_Odd_Line_Detail_Merger_C2(tmpFullImg00, tmpFullImg01, tmppHalfImgU, tmppHalfImgV, pBuf[0], pBuf[1], lFullWidth, lHalfWidth);
		tmpBuf = pBuf[0];
		pBuf[0] = pBuf[1];
		pBuf[1] = tmpBuf;
	}
	for (; k < lFullHeight; k++)
	{
		tmpFullImg00 = pSrcDstData + k * lFullPitch;
		Rem_Line_Detail_Merger_C2(tmpFullImg00, pBuf[0], lFullWidth);
	}
exit:
	if (pBufData)
	{
		MMemFree(hMemMgr, pBufData);
		pBufData = MNull;
	}
	return lret;
}

static MVoid Odd_Line_UpScaele_C1(MByte* FullLine, MByte* pHalfLine, MInt32 lFullWidth, MInt32 lHalfWidth)
{
	MInt32 x = 0;
	MInt32 k = 0;
	FullLine[0] = pHalfLine[0];
	k = 1;
	for (x = 1; x < lHalfWidth; x++, k+=2)
	{
		FullLine[k] = pHalfLine[x] + pHalfLine[x-1] + 1>>1;
		FullLine[k + 1] = pHalfLine[x];
	}
	for (; k < lFullWidth; k++)
	{
		FullLine[k] = FullLine[k - 1];
	}
	return;
}

static MVoid Even_Odd_Line_UpScaele_C1(MByte* CurFullLine, MByte* NexFullLine, MByte* PreFullLine, MByte* pHalfLine, MInt32 lFullWidth, MInt32 lHalfWidth)
{
	MInt32 x = 0;
	MInt32 k = 0;

#ifdef __ARM_NEON__
	uint8x16_t src01_u8x16;
	uint8x16_t src02_u8x16;
	uint8x16x2_t pre_u8x16x2;
	uint8x16x2_t dst01_u8x16x2;
	uint8x16x2_t dst02_u8x16x2;
	//uint16x8_t sum01_u16x8, sum02_u16x8;
	for (x = 0; x < lHalfWidth - 16; x += 16, k += 32)
	{
		src01_u8x16 = vld1q_u8(pHalfLine + x);
		src02_u8x16 = vld1q_u8(pHalfLine + x + 1);
		//sum01_u16x8 = vaddl_u8(vget_low_u8(src01_u8x16), vget_low_u8(src02_u8x16));
		//sum02_u16x8 = vaddl_u8(vget_high_u8(src01_u8x16), vget_high_u8(src02_u8x16));
		//dst01_u8x16x2.val[0] = src01_u8x16;
		//dst01_u8x16x2.val[1] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));
		dst01_u8x16x2.val[0] = src01_u8x16;
		dst01_u8x16x2.val[1] = vrhaddq_u8(src01_u8x16, src02_u8x16);
		pre_u8x16x2 = vld2q_u8(PreFullLine + k);
		vst2q_u8(NexFullLine + k, dst01_u8x16x2);

		//sum01_u16x8 = vaddl_u8(vget_low_u8(pre_u8x16x2.val[0]), vget_low_u8(dst01_u8x16x2.val[0]));
		//sum02_u16x8 = vaddl_u8(vget_high_u8(pre_u8x16x2.val[0]), vget_high_u8(dst01_u8x16x2.val[0]));
		//dst02_u8x16x2.val[0] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));

		//sum01_u16x8 = vaddl_u8(vget_low_u8(pre_u8x16x2.val[1]), vget_low_u8(dst01_u8x16x2.val[1]));
		//sum02_u16x8 = vaddl_u8(vget_high_u8(pre_u8x16x2.val[1]), vget_high_u8(dst01_u8x16x2.val[1]));
		//dst02_u8x16x2.val[1] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));

		dst02_u8x16x2.val[0] = vrhaddq_u8(pre_u8x16x2.val[0], dst01_u8x16x2.val[0]);
		dst02_u8x16x2.val[1] = vrhaddq_u8(pre_u8x16x2.val[1], dst01_u8x16x2.val[1]);
		vst2q_u8(CurFullLine + k, dst02_u8x16x2);
	}
#endif

	NexFullLine[k] = pHalfLine[x];
	CurFullLine[k] = NexFullLine[k] + PreFullLine[k] + 1 >> 1;
	k++;
	x++;
	for (; x < lHalfWidth; x++, k += 2)//x = 1
	{
		NexFullLine[k] = pHalfLine[x] + pHalfLine[x - 1] + 1 >> 1;
		NexFullLine[k + 1] = pHalfLine[x];

		CurFullLine[k] = NexFullLine[k] + PreFullLine[k] + 1 >> 1;
		CurFullLine[k + 1] = NexFullLine[k + 1] + PreFullLine[k + 1] + 1 >> 1;
	}
	for (; k < lFullWidth; k++)
	{
		NexFullLine[k] = NexFullLine[k - 1];
		CurFullLine[k] = CurFullLine[k - 1];
	}
	return;
}


static MInt32 Img_UpScale2_C1_Range(MHandle hMemMgr, LPImg_Plane_Data pHalfImg, LPImg_Plane_Data pFullImg, MInt32 lTopLine, MInt32 lBotLine)
{
	MInt32 lret = MOK;
	MByte* pFullData = pFullImg->pImage;
	MInt32 lFullWidth = pFullImg->lWidth;
	MInt32 lFullHeight = pFullImg->lHeight;
	MInt32 lFullPitch = pFullImg->lPitch;
	MByte* pHalfData = pHalfImg->pImage;
	MInt32 lHalfWidth = pHalfImg->lWidth;
	MInt32 lHalfHeight = pHalfImg->lHeight;
	MInt32 lHalfPitch = pHalfImg->lPitch;

	MInt32 y, k;
	MByte* pPreFullImg;
	MByte* pCurFullImg = pFullData, *pNexFullImg = pFullData;
	MByte* tmppHalfImg = pHalfData;

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
		tmppHalfImg = pHalfData + (y - 1)*lHalfPitch;
		Odd_Line_UpScaele_C1(pCurFullImg, tmppHalfImg, lFullWidth, lHalfWidth);
		k = y * 2 - 1;
	}

	for (; y < lBotLine; y++, k += 2)//y = 1
	{
		pPreFullImg = pFullData  + (k - 1) * lFullPitch;
		pCurFullImg = pPreFullImg + lFullPitch;
		pNexFullImg = pCurFullImg + lFullPitch;
		tmppHalfImg = pHalfData + y * lHalfPitch;
		Even_Odd_Line_UpScaele_C1(pCurFullImg, pNexFullImg, pPreFullImg, tmppHalfImg,  lFullWidth, lHalfWidth);
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
exit:
	return lret;
}


typedef struct _tag_IMG_UP_SCALE_ST{
	MHandle hMemMgr;
	LPImg_Plane_Data pHalfImg;
	LPImg_Plane_Data pFullImg;
	MInt32  topline;
	MInt32  botline;

	MInt32  lTaskHeight;
	MInt32* pNext_Task;
	MInt32  lTotal_TaskNum;
	MInt32  lret;
	MHandle* phEventCritical;
	MInt32   thread_ID;
} Img_Up_Scale_St, *LpImg_Up_Scale_St;


static MVoid thread_Img_UpScale_C1(MVoid* pParam)
{
	LpImg_Up_Scale_St Img_Up2_sturct = (LpImg_Up_Scale_St)pParam;
	MHandle hMemMgr = Img_Up2_sturct->hMemMgr;
	LPImg_Plane_Data pHalfImg = Img_Up2_sturct->pHalfImg;
	LPImg_Plane_Data pFullImg = Img_Up2_sturct->pFullImg;
	MInt32 lHalfHeight = Img_Up2_sturct->pHalfImg->lHeight;
	MInt32 lret = MOK;
	
	lret = Img_UpScale2_C1_Range(hMemMgr, pHalfImg, pFullImg, Img_Up2_sturct->topline, Img_Up2_sturct->botline);
	Img_Up2_sturct->lret = lret;		
}


MInt32 Img_UpScale2_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImg_Plane_Data pHalfImg, LPImg_Plane_Data pFullImg)
{
	MInt32 lret = MOK;
	MInt32 lHalfHeight = pHalfImg->lHeight;
	MByte* pDstLine;
	Img_Up_Scale_St pParams[16] = { MNull };
	MInt32 lnum;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
	{
		MInt32 lTk_Num = lHalfHeight >= 1024 ? 16 : 8;
		MInt32 taskID[16] = { 0 };
		MInt32 lTaskHeight = lHalfHeight / lTk_Num;
		lTaskHeight = lTaskHeight >> 2 << 2;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].topline = lTaskHeight * lnum;
			pParams[lnum].botline = lTaskHeight * (lnum + 1);
		}
		pParams[lTk_Num - 1].botline = lHalfHeight;

		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			pParams[lnum].thread_ID = lnum;
			pParams[lnum].hMemMgr = hMemMgr;
			pParams[lnum].pHalfImg = pHalfImg;
			pParams[lnum].pFullImg = pFullImg;

			pParams[lnum].lTaskHeight = lTaskHeight;
			pParams[lnum].lTotal_TaskNum = lTk_Num;
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			taskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_Img_UpScale_C1, (MVoid*)&pParams[lnum]);
		}
		for (lnum = 0; lnum < lTk_Num; lnum++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[lnum]);
		}
	}

#else
	lret = Img_UpScale2_C1_Range(hMemMgr,  pHalfImg,  pFullImg,  0,  lHalfHeight);
#endif

exit:
	return lret;
}


MInt32 Img_UpScale2_Haar_C1(MHandle hMemMgr, LPImg_Plane_Data pHalfImg, LPImg_Plane_Data pFullImg)
{
	MInt32 lret = MOK;
	MByte* pFullData = pFullImg->pImage;
	MInt32 lFullWidth = pFullImg->lWidth;
	MInt32 lFullHeight = pFullImg->lHeight;
	MInt32 lFullPitch = pFullImg->lPitch;
	MByte* pHalfData = pHalfImg->pImage;
	MInt32 lHalfWidth = pHalfImg->lWidth;
	MInt32 lHalfHeight = pHalfImg->lHeight;
	MInt32 lHalfPitch = pHalfImg->lPitch;

	MInt32 y, x, k;
	MByte* pPreFullImg;
	MByte* pCurFullImg = pFullData, *pNexFullImg = pFullData;
	MByte* tmppHalfImg = pHalfData;

	for (y = 0; y < lHalfHeight-1; y++)
	{
		MByte* cur_ful_data00 = pFullData + y * 2 * lFullPitch;
		MByte* cur_ful_data01 = cur_ful_data00 + lFullPitch;
		MByte* cur_half_data = pHalfData + y * lHalfPitch;
		k = 0;
		for (x = 0; x < lHalfWidth-1; x++, k+=2)
		{
			cur_ful_data00[k] = cur_ful_data00[k + 1] = cur_half_data[x];
			cur_ful_data01[k] = cur_ful_data01[k + 1] = cur_half_data[x];
		}
		for (; k < lFullWidth; k++)
		{
			cur_ful_data00[k] = cur_half_data[lHalfWidth - 1];
			cur_ful_data01[k] = cur_half_data[lHalfWidth - 1];
		}
	}
	y = 2 * (lHalfHeight - 2);
	for (; y < lFullHeight; y++)
	{
		MByte* cur_ful_data = pFullData  +  y * lFullPitch;
		MByte* cur_half_data = pHalfData + (lHalfHeight-1) * lHalfPitch;
		k = 0;
		for (x = 0; x < lHalfWidth - 1; x++, k += 2)
		{
			cur_ful_data[k] = cur_ful_data[k + 1] = cur_half_data[x];
		}
		for (; k < lFullWidth; k++)
		{
			cur_ful_data[k] = cur_half_data[lHalfWidth - 1];
		}
	}
exit:
	return lret;
}