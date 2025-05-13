#include "merror.h"
#include "ammem.h"
#include "imagebase.h"
#include "mobilecv.h"

#include "up_down_scale_gaussian5x5.h"

#if defined(USE_NEON) || defined(__ARM_NEON__)
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif

#define NH_OMP_THREAD_NUM 16
#define TRIM_10B(x)  ((x) < 0)?(0) : ((x) > 1020 ? 1020 :(x))

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
//#define __ARM_NEON__

typedef struct _tag_GaussPyrDown4
{
	MInt32 taskID;
	MInt32 start;
	MInt32 end;
	MByte* srcdata;
	MByte* dstdata;
	MWord* rowBuf;
	MInt32 srcW;
	MInt32 srcH;
	MInt32 srcPitch;
	MInt32 dstW;
	MInt32 dstH;
	MInt32 dstPitch;
}GaussPyrDown4;

typedef struct __tag_gauPyrDown_PARA {
	MHandle hMemMgr;
	LPASVLOFFSCREEN pSrcImg;
	LPASVLOFFSCREEN pDstImg;
	LPASVLOFFSCREEN pRefImg;
	MUInt16* tmpBuf01;
	MUInt16* tmpBuf02;
	MInt32 lStart;
	MInt32 lEnd;
}GauPyrDownUpPara, * LP_GauPyrDownUpPara;


//{ 1, 4, 6, 4, 1 } 
static MVoid ver_line_process_gauss5x5_down2_u8(MUInt16* tmpRow, MByte* srcRow0, MByte* srcRow1, MByte* srcRow2, MByte* srcRow3, MByte* srcRow4, MInt32 lWidth)
{
	MInt32 x = 0;
#ifdef __ARM_NEON__
	uint8x16_t rowdata0, rowdata1, rowdata2, rowdata3, rowdata4;
	uint16x8_t reslowdata, reshighdata, tmpdata;
	const uint8x8_t vconstdata = vdup_n_u8(6);
#endif

	x = 0;
#ifdef __ARM_NEON__
	for (; x < lWidth - 15; x += 16)
	{
		rowdata0 = vld1q_u8(srcRow0 + x);
		rowdata1 = vld1q_u8(srcRow1 + x);
		rowdata2 = vld1q_u8(srcRow2 + x);
		rowdata3 = vld1q_u8(srcRow3 + x);
		rowdata4 = vld1q_u8(srcRow4 + x);

		reslowdata = vaddl_u8(vget_low_u8(rowdata0), vget_low_u8(rowdata4));
		tmpdata = vaddl_u8(vget_low_u8(rowdata1), vget_low_u8(rowdata3));
		tmpdata = vshlq_n_u16(tmpdata, 2);
		reslowdata = vaddq_u16(reslowdata, tmpdata);
		reslowdata = vmlal_u8(reslowdata, vget_low_u8(rowdata2), vconstdata);
		vst1q_u16(tmpRow + x, reslowdata);

		reshighdata = vaddl_u8(vget_high_u8(rowdata0), vget_high_u8(rowdata4));
		tmpdata = vaddl_u8(vget_high_u8(rowdata1), vget_high_u8(rowdata3));
		tmpdata = vshlq_n_u16(tmpdata, 2);
		reshighdata = vaddq_u16(reshighdata, tmpdata);
		reshighdata = vmlal_u8(reshighdata, vget_high_u8(rowdata2), vconstdata);
		vst1q_u16(tmpRow + x + 8, reshighdata);
	}
#endif //__ARM_NEON__
	for (; x < lWidth; x++)
	{
		tmpRow[x] = (srcRow0[x] + srcRow4[x]) + (srcRow1[x] + srcRow3[x]) * 4 + srcRow2[x] * 6;
	}
	return;
}

static MVoid hor_line_process_gauss5x5_down2_u8(MByte* pDstRow, MUInt16* tmpRow, MInt32 lSrcWidth, MInt32 lDstW)
{
	MInt32 x = 0;
	MInt32 k = 0;
	MInt32 lTrueDstWidth = MIN(lSrcWidth >> 1, lDstW);
#ifdef __ARM_NEON__
	//uint16x8_t srcdata00, srcdata01, srcdata02, srcdata03, srcdata04;
	uint16x8x2_t arrdata00, arrdata01, arrdata02, arrdata03, arrdata04;
	uint16x8_t tmpdata, tmpresdata;
	uint8x8_t resdata;
#endif //__ARM_NEON__

	x = 0, k = 0;
#ifdef __ARM_NEON__
	for (; x < lTrueDstWidth - 7; x += 8, k += 16)
	{
		arrdata00 = vld2q_u16(tmpRow + k - 2);
		arrdata01 = vld2q_u16(tmpRow + k - 1);
		arrdata02 = vld2q_u16(tmpRow + k);
		arrdata03 = vld2q_u16(tmpRow + k + 1);
		arrdata04 = vld2q_u16(tmpRow + k + 2);

		/*srcdata00 = arrdata00.val[0];
		srcdata01 = arrdata01.val[0];
		srcdata02 = arrdata02.val[0];
		srcdata03 = arrdata03.val[0];
		srcdata04 = arrdata04.val[0];*/

		tmpresdata = vaddq_u16(arrdata00.val[0], arrdata04.val[0]);
		tmpdata = vaddq_u16(arrdata01.val[0], arrdata03.val[0]);
		tmpresdata = vmlaq_n_u16(tmpresdata, tmpdata, 4);
		tmpresdata = vmlaq_n_u16(tmpresdata, arrdata02.val[0], 6);

		resdata = vrshrn_n_u16(tmpresdata, 8);
		vst1_u8(pDstRow + x, resdata);
	}
#endif //__ARM_NEON__

	for (; x < lTrueDstWidth; x++, k += 2)
	{
		MUInt32 lVal = (tmpRow[k - 2] + tmpRow[k + 2]) + (tmpRow[k - 1] + tmpRow[k + 1]) * 4 + tmpRow[k] * 6;
		pDstRow[x] = lVal + 128 >> 8;
	}

	for (; x < lDstW; ++x)
	{
		pDstRow[x] = pDstRow[x - 1];
	}
	return;
}



static MVoid gaussPyrdown_u8_range(MHandle hMemMgr, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MUInt16* tmpBuf, MInt32 lTopLine, MInt32 lBotLine)
{
	MInt32 y = 0;
	MInt32 lSrcWidth = 0, lSrcHeight = 0;
	MInt32 lSrcPitch = 0;
	MInt32 lDstW = 0, lDstH = 0;   //The true width and height of dstImg
	MInt32 lDstPitch = 0;
	MUInt16 *tmpRow = MNull;

	MByte  *pDstRow = MNull;
	MByte  *srcRow0 = MNull, *srcRow1 = MNull, *srcRow2 = MNull, *srcRow3 = MNull, *srcRow4 = MNull;

	//const MInt16 kernel[5] = { 1, 2, 3, 2, 1 }; //{ 1, 4, 6, 4, 1 } <---> { 0.5, 2, 3, 2, 0.5 }


	lSrcWidth = pSrcImg->i32Width;
	lSrcHeight = pSrcImg->i32Height;
	lSrcPitch = pSrcImg->pi32Pitch[0];
	lDstW = pDstImg->i32Width;
	lDstH = pDstImg->i32Height;
	lDstPitch = pDstImg->pi32Pitch[0];


	tmpRow = tmpBuf + 2;

	if (0 == lTopLine)
	{
		srcRow0 = pSrcImg->ppu8Plane[0];
		srcRow1 = pSrcImg->ppu8Plane[0];
		srcRow2 = pSrcImg->ppu8Plane[0];
		srcRow3 = pSrcImg->ppu8Plane[0] + lSrcPitch;
		srcRow4 = pSrcImg->ppu8Plane[0] + lSrcPitch * 2;
	}
	else
	{
	
		srcRow2 = pSrcImg->ppu8Plane[0] + lTopLine * 2 * lSrcPitch;
		srcRow0 = srcRow2 - lSrcPitch * 2;
		srcRow1 = srcRow2 - lSrcPitch;
		srcRow3 = srcRow2 + lSrcPitch;
		srcRow4 = srcRow2 + lSrcPitch * 2;
	}

	for (y = lTopLine; y < lBotLine; y++)
	{
		pDstRow = pDstImg->ppu8Plane[0] + y*lDstPitch;
		ver_line_process_gauss5x5_down2_u8(tmpRow, srcRow0, srcRow1, srcRow2, srcRow3, srcRow4, lSrcWidth);
		tmpRow[-2] = tmpRow[0];
		tmpRow[-1] = tmpRow[0];
		tmpRow[lSrcWidth] = tmpRow[lSrcWidth - 1];
		tmpRow[lSrcWidth + 1] = tmpRow[lSrcWidth - 1];
		hor_line_process_gauss5x5_down2_u8(pDstRow, tmpRow, lSrcWidth, lDstW);
		srcRow0 = srcRow2;
		srcRow1 = srcRow3;
		srcRow2 = srcRow4;
		if (y * 2 + 3 < lSrcHeight)
		{
			srcRow3 += 2 * lSrcPitch;
		}
		else
		{
			srcRow3 = pSrcImg->ppu8Plane[0] + (lSrcHeight - 1)*lSrcPitch;
		}

		if (y * 2 + 5 < lSrcHeight)
		{
			srcRow4 += 2 * lSrcPitch;
		}
		else
		{
			srcRow4 = pSrcImg->ppu8Plane[0] + (lSrcHeight - 1)*lSrcPitch;
		}
	}
	return;
}




static MVoid thread_GauPyrDown_U8(MVoid *para)
{
	LP_GauPyrDownUpPara pPara = (LP_GauPyrDownUpPara)para;
	if (MNull != pPara)
	{
		gaussPyrdown_u8_range(pPara->hMemMgr, pPara->pSrcImg, pPara->pDstImg, pPara->tmpBuf01, pPara->lStart, pPara->lEnd);
	}
	return;
}

MInt32 Img_Guass5x5_Down2_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg)
{
	MRESULT lret = MOK;
	MInt32  lSrcWidth = 0;
	MInt32  lDstH = 0;
	MUInt16 *tmpBuf[16] = { MNull };
	MInt32 lTaskNum = 1;
	MInt32 lnum;

	if (!pSrcImg || !pDstImg)
		return MERR_INVALID_PARAM;

	lSrcWidth = pSrcImg->i32Width;
	lDstH = pDstImg->i32Height;

	if (lDstH > 2048)
	{
		lTaskNum = 16;
	}
	else if (lDstH > 1024)
	{
		lTaskNum = 8;
	}
	else if (lDstH > 512)
	{
		lTaskNum = 4;
	}
	else if (lDstH > 128)
	{
		lTaskNum = 2;
	}
	else
	{
		lTaskNum = 1;
	}

	for (lnum = 0; lnum < MAX(lTaskNum, NH_OMP_THREAD_NUM); lnum++)
	{
		tmpBuf[lnum] = (MUInt16*)MMemAlloc(hMemMgr, (lSrcWidth + 4) * sizeof(MUInt16));
		if (tmpBuf[lnum] == MNull)
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}
	}

#ifdef MCV_MULTI_THREAD
	{
		MInt32 lTaskID[16] = { 0 };
		GauPyrDownUpPara  pParams[16] = { MNull };
		MInt32 lTaskH = 0;

		lTaskH = lDstH / lTaskNum;
		lTaskH = lTaskH + 2 >> 2 << 2;

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParams[lnum].lStart = lTaskH * lnum;
			pParams[lnum].lEnd = lTaskH * (lnum + 1);
		}
		pParams[lTaskNum - 1].lEnd = lDstH;

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			pParams[lnum].hMemMgr = hMemMgr;
			pParams[lnum].pSrcImg = pSrcImg;
			pParams[lnum].pDstImg = pDstImg;
			pParams[lnum].tmpBuf01 = tmpBuf[lnum];
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_GauPyrDown_U8, (MVoid*)&pParams[lnum]);
			if (lTaskID[lnum] < 0)
			{
				lret = MERR_BAD_STATE;
				goto exit;
			}
		}

		for (lnum = 0; lnum < lTaskNum; lnum++)
		{
			lret = mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
			if (MOK != lret)
			{
				lret = MERR_BAD_STATE;
				goto exit;
			}
		}
	}

#else
	{

		MInt32 taskHeight = lDstH / lTaskNum >> 2 << 2;

#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
		for (MInt32 lTask = 0; lTask < lTaskNum; lTask++)
		{
			MInt32 rowBegin = lTask * taskHeight;
			MInt32 rowEnd = (lTask == lTaskNum - 1) ? lDstH : (rowBegin + taskHeight);

			MInt32 threadnum = 0;
#ifdef NH_ENABLE_OPENMP
			threadnum = omp_get_thread_num();
#endif
			gaussPyrdown_u8_range(hMemMgr, pSrcImg, pDstImg, tmpBuf[threadnum], rowBegin, rowEnd);
		}
	}
#endif
	
exit:
	for (lnum = 0; lnum < 16; lnum++)
	{
		if (tmpBuf[lnum])
		{
			MMemFree(hMemMgr, tmpBuf[lnum]);
			tmpBuf[lnum] = MNull;
		}
	}
	return lret;
}


static MVoid ver_line_process_gauss_5x5_up2_u8(MUInt16* tmpRow01, MUInt16* tmpRow02, MByte* srcRow0, MByte* srcRow1, MByte* srcRow2, MInt32 lWidth)
{
	MInt32 x;

#ifdef __ARM_NEON__
	uint8x16_t srcdata0, srcdata1, srcdata2;
	uint16x8_t tmpdata0, tmpdata1;
	const uint8x8_t vconstdata = vdup_n_u8(6);
#endif //__ARM_NEON__

	x = 0;
#ifdef __ARM_NEON__
	for (; x < lWidth - 15; x += 16)
	{
		srcdata0 = vld1q_u8(srcRow0 + x);
		srcdata1 = vld1q_u8(srcRow1 + x);
		srcdata2 = vld1q_u8(srcRow2 + x);

		tmpdata0 = vaddl_u8(vget_low_u8(srcdata0), vget_low_u8(srcdata2));
		tmpdata0 = vmlal_u8(tmpdata0, vget_low_u8(srcdata1), vconstdata);
		tmpdata1 = vaddl_u8(vget_low_u8(srcdata1), vget_low_u8(srcdata2));
		vst1q_u16(tmpRow01 + x, tmpdata0);
		vst1q_u16(tmpRow02 + x, tmpdata1);

		tmpdata0 = vaddl_u8(vget_high_u8(srcdata0), vget_high_u8(srcdata2));
		tmpdata0 = vmlal_u8(tmpdata0, vget_high_u8(srcdata1), vconstdata);
		tmpdata1 = vaddl_u8(vget_high_u8(srcdata1), vget_high_u8(srcdata2));
		vst1q_u16(tmpRow01 + x + 8, tmpdata0);
		vst1q_u16(tmpRow02 + x + 8, tmpdata1);
	}
#endif //__ARM_NEON__
	for (; x < lWidth; x++)
	{
		tmpRow01[x] = srcRow0[x] + srcRow2[x] + 6 * srcRow1[x];
		tmpRow02[x] = srcRow1[x] + srcRow2[x];
	}
	return;
}

static MVoid hor_line_process_gauss5x5_up2_u8(MByte* pDstRow01, MByte* pDstRow02, MUInt16* tmpRow01, MUInt16* tmpRow02, MInt32 lSrcWidth, MInt32 lDstWidth)
{
	MInt32 x = 0;
	MInt32 k = 0;
	MInt32 lNorWidth = lDstWidth >> 1 << 1;
#ifdef __ARM_NEON__
	uint16x8_t tmpData0, tmpData1, tmpData2;
	uint16x8_t resData0, resData1;
	uint8x8x2_t dstData;
#endif //__ARM_NEON__

	x = 0;
	k = 0;
#ifdef __ARM_NEON__
	for (; x < lNorWidth - 15; x += 16, k += 8)
	{
		tmpData0 = vld1q_u16(tmpRow01 + k - 1);
		tmpData1 = vld1q_u16(tmpRow01 + k);
		tmpData2 = vld1q_u16(tmpRow01 + k + 1);

		resData0 = vaddq_u16(tmpData0, tmpData2);
		resData0 = vmlaq_n_u16(resData0, tmpData1, 6);
		dstData.val[0] = vrshrn_n_u16(resData0, 6);

		resData1 = vaddq_u16(tmpData1, tmpData2);
		dstData.val[1] = vrshrn_n_u16(resData1, 4);

		vst2_u8(pDstRow01 + x, dstData);

		tmpData0 = vld1q_u16(tmpRow02 + k - 1);
		tmpData1 = vld1q_u16(tmpRow02 + k);
		tmpData2 = vld1q_u16(tmpRow02 + k + 1);

		resData0 = vaddq_u16(tmpData0, tmpData2);
		resData0 = vmlaq_n_u16(resData0, tmpData1, 6);
		dstData.val[0] = vrshrn_n_u16(resData0, 4);

		resData1 = vaddq_u16(tmpData1, tmpData2);
		dstData.val[1] = vrshrn_n_u16(resData1, 2);

		vst2_u8(pDstRow02 + x, dstData);
	}
#endif //__ARM_NEON__

	for (; x < lNorWidth; x += 2, k++)
	{
		MInt32 lVal00, lVal01, lVal10, lVal11;

		lVal00 = (tmpRow01[k - 1] + tmpRow01[k + 1]) + 6 * tmpRow01[k];
		pDstRow01[x] = lVal00 + 32 >> 6;

		lVal01 = tmpRow01[k] + tmpRow01[k + 1];
		pDstRow01[x + 1] = lVal01 + 8 >> 4;

		lVal10 = (tmpRow02[k - 1] + tmpRow02[k + 1]) + 6 * tmpRow02[k];
		pDstRow02[x] = lVal10 + 8 >> 4;

		lVal11 = tmpRow02[k] + tmpRow02[k + 1];
		pDstRow02[x + 1] = lVal11 + 2 >> 2;
	}

	for (; x < lDstWidth; x++)
	{
		pDstRow01[x] = pDstRow01[x - 1];
		pDstRow02[x] = pDstRow02[x - 1];
	}
	return;
}


static MVoid hor_line_process_gauss5x5_up2_sub_u8(MByte* pDstRow01, MByte* pDstRow02, MByte* pRefRow01, MByte* pRefRow02, 
	MUInt16* tmpRow01, MUInt16* tmpRow02, MInt32 lSrcWidth, MInt32 lDstWidth)
{
	MInt32 x = 0;
	MInt32 k = 0;
	MInt32 lNorWidth = lDstWidth >> 1 << 1;
#ifdef __ARM_NEON__
	uint16x8_t tmpData0, tmpData1, tmpData2;
	uint16x8_t resData0, resData1;
	uint8x8x2_t dstData;
	uint8x8_t const_128 = vdup_n_u8(128);
#endif //__ARM_NEON__

	x = 0;
	k = 0;
#ifdef __ARM_NEON__
	for (; x < lNorWidth - 15; x += 16, k += 8)
	{
		uint8x8x2_t refData_u8x8x2;
		uint16x8_t refData0_u16, refData1_u16;
		refData_u8x8x2 = vld2_u8(pRefRow01 + x);
		refData0_u16 = vaddl_u8(refData_u8x8x2.val[0], const_128);
		refData1_u16 = vaddl_u8(refData_u8x8x2.val[1], const_128);
		tmpData0 = vld1q_u16(tmpRow01 + k - 1);
		tmpData1 = vld1q_u16(tmpRow01 + k);
		tmpData2 = vld1q_u16(tmpRow01 + k + 1);

		resData0 = vaddq_u16(tmpData0, tmpData2);
		resData0 = vmlaq_n_u16(resData0, tmpData1, 6);
		resData0 = vrshrq_n_u16(resData0, 6);
		resData0 = vqsubq_u16(refData0_u16, resData0);
		dstData.val[0] = vqmovn_u16(resData0);

		resData1 = vaddq_u16(tmpData1, tmpData2);
		resData1 = vrshrq_n_u16(resData1, 4);
		resData1 = vqsubq_u16(refData1_u16, resData1);
		dstData.val[1] = vqmovn_u16(resData1);

		vst2_u8(pDstRow01 + x, dstData);

		refData_u8x8x2 = vld2_u8(pRefRow02 + x);
		refData0_u16 = vaddl_u8(refData_u8x8x2.val[0], const_128);
		refData1_u16 = vaddl_u8(refData_u8x8x2.val[1], const_128);
		tmpData0 = vld1q_u16(tmpRow02 + k - 1);
		tmpData1 = vld1q_u16(tmpRow02 + k);
		tmpData2 = vld1q_u16(tmpRow02 + k + 1);

		resData0 = vaddq_u16(tmpData0, tmpData2);
		resData0 = vmlaq_n_u16(resData0, tmpData1, 6);
		resData0 = vrshrq_n_u16(resData0, 4);
		resData0 = vqsubq_u16(refData0_u16, resData0);
		dstData.val[0] = vqmovn_u16(resData0);

		resData1 = vaddq_u16(tmpData1, tmpData2);
		resData1 = vrshrq_n_u16(resData1, 2);
		resData1 = vqsubq_u16(refData1_u16, resData1);
		dstData.val[1] = vqmovn_u16(resData1);

		vst2_u8(pDstRow02 + x, dstData);
	}
#endif //__ARM_NEON__



	for (; x < lNorWidth; x += 2, k++)
	{
		MInt32 lVal00, lVal01, lVal10, lVal11;
		MInt32 lValRes;


		lVal00 = (tmpRow01[k - 1] + tmpRow01[k + 1]) + 6 * tmpRow01[k];
		lValRes = lVal00 + 32 >> 6;
		lValRes = pRefRow01[x] - lValRes + 128;

		pDstRow01[x] = TRIMBYTE(lValRes);

		lVal01 = tmpRow01[k] + tmpRow01[k + 1];
		lValRes = lVal01 + 8 >> 4;
		lValRes = pRefRow01[x + 1] - lValRes + 128;

		pDstRow01[x + 1] = TRIMBYTE(lValRes);

		lVal10 = (tmpRow02[k - 1] + tmpRow02[k + 1]) + 6 * tmpRow02[k];
		lValRes = lVal10 + 8 >> 4;
		lValRes = pRefRow02[x] - lValRes + 128;

		pDstRow02[x] = TRIMBYTE(lValRes);

		lVal11 = tmpRow02[k] + tmpRow02[k + 1];
		lValRes = lVal11 + 2 >> 2;
		lValRes = pRefRow02[x + 1] - lValRes + 128;

		pDstRow02[x + 1] = TRIMBYTE(lValRes);

	}

	for (; x < lDstWidth; x++)
	{
		pDstRow01[x] = 128;
		pDstRow02[x] = 128;
	}
	return;
}

MRESULT Img_Guass5x5_Up2_u8(MHandle hMemMgr, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg)
{
	MInt32 y = 0;
	MInt32 lSrcWidth = 0, lSrcHeight = 0;
	MInt32 lSrcPitch = 0;
	MInt32 lDstWidth = 0, lDstHeight = 0;
	MInt32 lDstPitch = 0;

	lSrcWidth = pSrcImg->i32Width;
	lSrcHeight = pSrcImg->i32Height;
	lSrcPitch = pSrcImg->pi32Pitch[0];

	lDstWidth = pDstImg->i32Width;
	lDstHeight = pDstImg->i32Height;
	lDstPitch = pDstImg->pi32Pitch[0];

	MUInt16* tmpBuf1[NH_OMP_THREAD_NUM] = { 0 }, * tmpBuf2[NH_OMP_THREAD_NUM] = { 0 };
	MInt32 bufSize = lSrcWidth + 4;
	tmpBuf1[0] = (MUInt16*)MMemAlloc(hMemMgr, bufSize * sizeof(MUInt16) * NH_OMP_THREAD_NUM);
	tmpBuf2[0] = (MUInt16*)MMemAlloc(hMemMgr, bufSize * sizeof(MUInt16) * NH_OMP_THREAD_NUM);
	if (!tmpBuf1[0] || !tmpBuf2[0])
	{
		return MERR_NO_MEMORY;
	}

	for (MInt32 i = 1; i < NH_OMP_THREAD_NUM; i++)
	{
		tmpBuf1[i] = tmpBuf1[0] + i * bufSize;
		tmpBuf2[i] = tmpBuf2[0] + i * bufSize;
	}

#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic, 64)
#endif
	for (y = 0; y < (lDstHeight >> 1 << 1); y += 2)
	{
		MInt32 threadnum = 0;
#ifdef NH_ENABLE_OPENMP
		threadnum = omp_get_thread_num();
#endif

		MUInt16* tmpRow1 = MNull, * tmpRow2 = MNull;
		tmpRow1 = tmpBuf1[threadnum] + 1;
		tmpRow2 = tmpBuf2[threadnum] + 1;

		MInt32 k = y >> 1;
		MByte *srcRow0 = (k - 1 < 0) ? pSrcImg->ppu8Plane[0] : pSrcImg->ppu8Plane[0] + (k - 1)*lSrcPitch;
		MByte *srcRow1 = (k >= lSrcHeight) ? pSrcImg->ppu8Plane[0] + (lSrcHeight - 1)*lSrcPitch : pSrcImg->ppu8Plane[0] + k*lSrcPitch;
		MByte *srcRow2 = (k + 1 >= lSrcHeight) ? pSrcImg->ppu8Plane[0] + (lSrcHeight - 1)*lSrcPitch : pSrcImg->ppu8Plane[0] + (k + 1)*lSrcPitch;

		MByte *pDstRow1 = pDstImg->ppu8Plane[0] + y * lDstPitch;
		MByte *pDstRow2 = pDstRow1 + lDstPitch;

		ver_line_process_gauss_5x5_up2_u8(tmpRow1, tmpRow2, srcRow0, srcRow1, srcRow2, lSrcWidth);

		tmpRow1[-1] = tmpRow1[0];
		tmpRow2[-1] = tmpRow2[0];
		tmpRow1[lSrcWidth] = tmpRow1[lSrcWidth - 1];
		tmpRow2[lSrcWidth] = tmpRow2[lSrcWidth - 1];

		hor_line_process_gauss5x5_up2_u8(pDstRow1, pDstRow2, tmpRow1, tmpRow2, lSrcWidth, lDstWidth);
	}


	for (y = (lDstHeight >> 1 << 1); y < lDstHeight; y++)
	{
		MByte *pDstCur = pDstImg->ppu8Plane[0] + y * lDstPitch;
		MByte *pDstPre = pDstCur - lDstPitch;
		MMemCpy(pDstCur, pDstPre, lDstWidth*sizeof(MByte));
	}
	
	if (tmpBuf1[0])
	{
		MMemFree(hMemMgr, tmpBuf1[0]);
	}

	if (tmpBuf2[0])
	{
		MMemFree(hMemMgr, tmpBuf2[0]);
	}
	return MOK;
}

// sizeSrc = (sizeDst >> 1)
MRESULT Img_Guass5x5_Up2AndSub_u8(MHandle hMemMgr, LPASVLOFFSCREEN pSmallSrcImg, LPASVLOFFSCREEN pLargeRefImg, LPASVLOFFSCREEN pLargeDstImg)
{
	MInt32 y = 0;
	MInt32 lSrcWidth = 0, lSrcHeight = 0;
	MInt32 lSrcPitch = 0;
	MInt32 lDstWidth = 0, lDstHeight = 0;
	MInt32 lDstPitch = 0;
	MInt32 lRefPitch = pLargeRefImg->pi32Pitch[0];
	MUInt16* tmpBuf1[NH_OMP_THREAD_NUM] = { 0 }, * tmpBuf2[NH_OMP_THREAD_NUM] = { 0 };

	lSrcWidth = pSmallSrcImg->i32Width;
	lSrcHeight = pSmallSrcImg->i32Height;
	lSrcPitch = pSmallSrcImg->pi32Pitch[0];

	lDstWidth = pLargeDstImg->i32Width;
	lDstHeight = pLargeDstImg->i32Height;
	lDstPitch = pLargeDstImg->pi32Pitch[0];

	MInt32 bufSize = lSrcWidth + 4;
	tmpBuf1[0] = (MUInt16*)MMemAlloc(hMemMgr, bufSize * sizeof(MUInt16) * NH_OMP_THREAD_NUM);
	tmpBuf2[0] = (MUInt16*)MMemAlloc(hMemMgr, bufSize * sizeof(MUInt16) * NH_OMP_THREAD_NUM);
	if (!tmpBuf1[0] || !tmpBuf2[0])
	{
		return MERR_NO_MEMORY;
	}

	for (MInt32 i = 1; i < NH_OMP_THREAD_NUM; i++)
	{
		tmpBuf1[i] = tmpBuf1[0] + i * bufSize;
		tmpBuf2[i] = tmpBuf2[0] + i * bufSize;
	}


#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic, 64)
#endif
	for (y = 0; y < (lDstHeight >> 1 << 1); y += 2)
	{
		MInt32 threadnum = 0;
#ifdef NH_ENABLE_OPENMP
		threadnum = omp_get_thread_num();
#endif

		MUInt16* tmpRow1 = MNull, * tmpRow2 = MNull;
		tmpRow1 = tmpBuf1[threadnum] + 1;
		tmpRow2 = tmpBuf2[threadnum] + 1;

		MInt32 k = y >> 1;
		MByte* srcRow0 = (k - 1 < 0) ? pSmallSrcImg->ppu8Plane[0] : pSmallSrcImg->ppu8Plane[0] + (k - 1) * lSrcPitch;
		MByte* srcRow1 = (k >= lSrcHeight) ? pSmallSrcImg->ppu8Plane[0] + (lSrcHeight - 1) * lSrcPitch : pSmallSrcImg->ppu8Plane[0] + k * lSrcPitch;
		MByte* srcRow2 = (k + 1 >= lSrcHeight) ? pSmallSrcImg->ppu8Plane[0] + (lSrcHeight - 1) * lSrcPitch : pSmallSrcImg->ppu8Plane[0] + (k + 1) * lSrcPitch;

		MByte* pDstRow1 = pLargeDstImg->ppu8Plane[0] + y * lDstPitch;
		MByte* pDstRow2 = pDstRow1 + lDstPitch;

		MByte* pRefRow1 = pLargeRefImg->ppu8Plane[0] + y * lRefPitch;
		MByte* pRefRow2 = pRefRow1 + lRefPitch;


		ver_line_process_gauss_5x5_up2_u8(tmpRow1, tmpRow2, srcRow0, srcRow1, srcRow2, lSrcWidth);

		tmpRow1[-1] = tmpRow1[0];
		tmpRow2[-1] = tmpRow2[0];
		tmpRow1[lSrcWidth] = tmpRow1[lSrcWidth - 1];
		tmpRow2[lSrcWidth] = tmpRow2[lSrcWidth - 1];

		hor_line_process_gauss5x5_up2_sub_u8(pDstRow1, pDstRow2, pRefRow1, pRefRow2, tmpRow1, tmpRow2, lSrcWidth, lDstWidth);
	}


	for (y = (lDstHeight >> 1 << 1); y < lDstHeight; y++)
	{
		MMemSet(pLargeDstImg->ppu8Plane[0] + y * lDstPitch, 128, lDstWidth);
	}
	

	if (tmpBuf1[0])
	{
		MMemFree(hMemMgr, tmpBuf1[0]);
	}

	if (tmpBuf2[0])
	{
		MMemFree(hMemMgr, tmpBuf2[0]);
	}
	return MOK;
}




static MVoid Img_Guass5x5_Down4_Stripe(MByte* srcPtr, MByte* dstPtr, MWord* rowBuf, MInt32 sw, MInt32 sh, MInt32 src_step,
	MInt32 dw, MInt32 dh, MInt32 dst_step, MInt32 start, MInt32 end)
{
	MWord* buf = rowBuf + 2;
	MInt32 k, x, y, sy0 = -(5 >> 1), sy = sy0;
	MByte* src00, * src01, * src02, * src03, * src04;

#ifdef _ARM_NEON_
	uint8x8_t vConst6_8x8;
	uint16x8_t vConst4, vConst6_16x8;
	vConst4 = vdupq_n_u16(4);
	vConst6_8x8 = vdup_n_u8(6);
	vConst6_16x8 = vdupq_n_u16(6);
#endif

	for (y = start; y < end; y++)
	{
		MByte* tmpdst = (MByte*)(dstPtr + dst_step * y);
		sy = 4 * y;
		if (0 == y)
		{
			src00 = srcPtr + 2 * src_step;
			src01 = srcPtr + src_step;
		}
		else
		{
			//如下写法在某几张测试图上访问第0行数据会crash，具体原因不详
			src00 = ((sy - 2) >= 0) ? (srcPtr + src_step * (sy - 2)) : (srcPtr + src_step * (2 - sy));
			src01 = ((sy - 1) >= 0) ? (srcPtr + src_step * (sy - 1)) : (srcPtr + src_step * (1 - sy));
		}
		src02 = srcPtr + src_step * sy;
		src03 = (sy + 1) <= (sh - 1) ? (srcPtr + src_step * (sy + 1)) : (srcPtr + src_step * (2 * sh - 2 - sy - 1));
		src04 = (sy + 2) <= (sh - 1) ? (srcPtr + src_step * (sy + 2)) : (srcPtr + src_step * (2 * sh - 2 - sy - 2));

		x = 0;
#ifdef _ARM_NEON_
		for (; x < sw - 15; x += 16)
		{
			uint8x16_t s00_8x16, s01_8x16, s02_8x16, s03_8x16, s04_8x16;
			uint16x8_t sum00_16x8, sum01_16x8;
			uint16x8_t tmpsum00, tmpsum01;
			s00_8x16 = vld1q_u8(src00 + x);
			s01_8x16 = vld1q_u8(src01 + x);
			s02_8x16 = vld1q_u8(src02 + x);
			s03_8x16 = vld1q_u8(src03 + x);
			s04_8x16 = vld1q_u8(src04 + x);
			tmpsum00 = vaddl_u8(vget_low_u8(s01_8x16), vget_low_u8(s03_8x16));
			tmpsum01 = vaddl_u8(vget_high_u8(s01_8x16), vget_high_u8(s03_8x16));
			sum00_16x8 = vmull_u8(vget_low_u8(s02_8x16), vConst6_8x8);
			sum01_16x8 = vmull_u8(vget_high_u8(s02_8x16), vConst6_8x8);
			sum00_16x8 = vmlaq_u16(sum00_16x8, tmpsum00, vConst4);
			sum01_16x8 = vmlaq_u16(sum01_16x8, tmpsum01, vConst4);
			tmpsum00 = vaddl_u8(vget_low_u8(s00_8x16), vget_low_u8(s04_8x16));
			tmpsum01 = vaddl_u8(vget_high_u8(s00_8x16), vget_high_u8(s04_8x16));
			sum00_16x8 = vaddq_u16(sum00_16x8, tmpsum00);
			sum01_16x8 = vaddq_u16(sum01_16x8, tmpsum01);
			vst1q_u16(buf + x, sum00_16x8);
			vst1q_u16(buf + x + 8, sum01_16x8);
		}
		for (; x < sw - 7; x += 8)
		{
			uint8x8_t s00_8x8, s01_8x8, s02_8x8, s03_8x8, s04_8x8;
			uint16x8_t sum00_16x8;
			uint16x8_t tmpsum00, tmpsum01;
			s00_8x8 = vld1_u8(src00 + x);
			s01_8x8 = vld1_u8(src01 + x);
			s02_8x8 = vld1_u8(src02 + x);
			s03_8x8 = vld1_u8(src03 + x);
			s04_8x8 = vld1_u8(src04 + x);
			tmpsum00 = vaddl_u8(s01_8x8, s03_8x8);
			sum00_16x8 = vmull_u8(s02_8x8, vConst6_8x8);
			tmpsum01 = vaddl_u8(s00_8x8, s04_8x8);
			sum00_16x8 = vmlaq_u16(sum00_16x8, tmpsum00, vConst4);
			sum00_16x8 = vaddq_u16(sum00_16x8, tmpsum01);
			vst1q_u16(buf + x, sum00_16x8);
		}
#endif
		for (; x < sw; x++)
		{
			buf[x] = src02[x] * 6 + (src01[x] + src03[x]) * 4 + src00[x] + src04[x];
		}

		// fill the ring buffer (horizontal convolution and decimation)
		buf[-1] = buf[1];
		buf[-2] = buf[2];
		buf[sw] = buf[sw - 2];
		buf[sw + 1] = buf[sw - 3];
		x = 0; k = 0;

#ifdef _ARM_NEON_
		for (; x < dw - 7; x += 8, k += 32)
		{
			uint16x8x4_t bufsrc00_16x8x4 = vld4q_u16(buf + k - 2);
			uint16x8x4_t bufsrc01_16x8x4 = vld4q_u16(buf + k + 2);
			uint16x8_t tmp_sum16x8 = vaddq_u16(bufsrc00_16x8x4.val[0], bufsrc01_16x8x4.val[0]);
			uint16x8_t sum00_16x8 = vaddq_u16(bufsrc00_16x8x4.val[1], bufsrc00_16x8x4.val[3]);
			tmp_sum16x8 = vmlaq_u16(tmp_sum16x8, bufsrc00_16x8x4.val[2], vConst6_16x8);
			sum00_16x8 = vmlaq_u16(tmp_sum16x8, sum00_16x8, vConst4);
			vst1_u8(tmpdst + x, vrshrn_n_u16(sum00_16x8, 8));
		}
#endif
		for (; x < dw; x++, k += 4)
		{
			tmpdst[x] = (buf[k] * 6 + (buf[k - 1] + buf[k + 1]) * 4 + buf[k - 2] + buf[k + 2] + 128) >> 8;
		}

	}
}

#ifdef  MCV_MULTI_THREAD
static MVoid thread_Img_Guass5x5_Down4(MVoid* pParam)
{
	GaussPyrDown4* pyrdown4_D = (GaussPyrDown4*)pParam;
	Img_Guass5x5_Down4_Stripe(pyrdown4_D->srcdata, pyrdown4_D->dstdata, pyrdown4_D->rowBuf, pyrdown4_D->srcW, pyrdown4_D->srcH, pyrdown4_D->srcPitch,
		pyrdown4_D->dstW, pyrdown4_D->dstH, pyrdown4_D->dstPitch, pyrdown4_D->start, pyrdown4_D->end);
}
#endif


MInt32 Img_Guass5x5_Down4(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch)
{
	MInt32 i, lret = MOK;
	const MInt32 lTask_Num = 8;
	MWord* rowBuf[lTask_Num] = { MNull };
	MWord* tmpBuf = (MWord*)MMemAlloc(hMemMgr, (lSrcWidth + 5) * lTask_Num * sizeof(MWord));
	if (tmpBuf == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	for (i = 0; i < lTask_Num; i++)
	{
		rowBuf[i] = tmpBuf + i * (lSrcWidth + 5);
	}

#ifdef MCV_MULTI_THREAD
	{
		MFloat  scale = 1.0f / lTask_Num;
		MInt32 taskID[lTask_Num] = { 0 };
		GaussPyrDown4 pParams[lTask_Num] = { 0 };

		for (i = 0; i < lTask_Num; ++i)
		{
			pParams[i].taskID = i;
			pParams[i].srcdata = pSrcImg;
			pParams[i].dstdata = pDstImg;
			pParams[i].rowBuf = rowBuf[i];
			pParams[i].srcW = lSrcWidth;
			pParams[i].srcH = lSrcHeight;
			pParams[i].srcPitch = lSrcPitch;
			pParams[i].dstW = lDstWidth;
			pParams[i].dstH = lDstHeight;
			pParams[i].dstPitch = lDstPitch;
			pParams[i].start = (MInt32)(lDstHeight * scale * i);
			pParams[i].end = (MInt32)(lDstHeight * scale * (i + 1));
		}
		pParams[lTask_Num - 1].end = lDstHeight;

		for (i = 0; i < lTask_Num; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, thread_Img_Guass5x5_Down4, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTask_Num; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}
	}
#else
	{
		MInt32 lTaskHeight = lDstHeight / lTask_Num >> 1 << 1;

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

			Img_Guass5x5_Down4_Stripe(pSrcImg, pDstImg, rowBuf[curThread], lSrcWidth, lSrcHeight, lSrcPitch,
				lDstWidth, lDstHeight, lDstPitch, rowBegin, rowEnd);
		}
	}
#endif

exit:
	if (tmpBuf)
	{
		MMemFree(hMemMgr, tmpBuf);
		tmpBuf = MNull;
	}
	return lret;
}



static MVoid Img_Guass5x5_Down2_u16_Stripe(MUInt16* tmpRow, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg,
	MInt32 lStart, MInt32 lEnd)
{
	MInt32 i = 0, j = 0;
	MInt32 lSrcWidth = 0, lSrcHeight = 0;
	MInt32 lSrcStep = 0;
	MInt32 lDstWidth = 0, lDstHeight = 0; // The width and height is just half of srcImg
	MInt32 lDstW = 0, lDstH = 0;   //The true width and height of dstImg
	MInt32 lDstStep = 0;
	MUInt16* pSrcRow = MNull, * pDstRow = MNull;
	MUInt16* srcRow0 = MNull, * srcRow1 = MNull, * srcRow2 = MNull, * srcRow3 = MNull, * srcRow4 = MNull;
	const MInt32 lEnd_last = lEnd;
	MUInt16* pSrc0 = (MUInt16*)pSrcImg->ppu8Plane[0];
	MUInt16* pDst0 = (MUInt16*)pDstImg->ppu8Plane[0];


	lSrcWidth = pSrcImg->i32Width;
	lSrcHeight = pSrcImg->i32Height;
	lSrcStep = pSrcImg->pi32Pitch[0] >> 1;
	lDstW = lDstWidth = pDstImg->i32Width;
	lDstH = lDstHeight = pDstImg->i32Height;
	lDstStep = pDstImg->pi32Pitch[0] >> 1;

	if ((lSrcWidth >> 1) < lDstWidth)
	{
		lDstWidth = lSrcWidth >> 1;
	}

	if ((lSrcHeight >> 1) < lDstHeight)
	{
		if (lEnd == lDstHeight)
			lEnd = lSrcHeight >> 1;

		lDstHeight = lSrcHeight >> 1;
	}

	tmpRow += 2;

	for (i = lStart; i < lEnd; i++)
	{
		pDstRow = pDst0 + i * lDstStep;
		srcRow0 = (i == 0) ? pSrc0 + lSrcStep : pSrc0 + (2 * i - 2) * lSrcStep;
		srcRow1 = (i == 0) ? pSrc0 : pSrc0 + (2 * i - 1) * lSrcStep;
		srcRow2 = pSrc0 + 2 * i * lSrcStep;
		srcRow3 = (i == lDstHeight - 1) ? pSrc0 + (2 * i - 1) * lSrcStep : pSrc0 + (2 * i + 1) * lSrcStep;
		srcRow4 = (i == lDstHeight - 1) ? pSrc0 + 2 * i * lSrcStep : pSrc0 + (2 * i + 2) * lSrcStep;

		j = 0;

		for (; j < lSrcWidth; j++)
		{
			tmpRow[j] = ((srcRow0[j] + srcRow4[j]) + ((srcRow3[j] + srcRow1[j]) * 4) + srcRow2[j] * 6) >> 4;
		}
		tmpRow[-2] = tmpRow[1];
		tmpRow[-1] = tmpRow[0];
		tmpRow[lSrcWidth] = tmpRow[lSrcWidth - 1];
		tmpRow[lSrcWidth + 1] = tmpRow[lSrcWidth - 2];

		j = 0;

		for (; j < lDstWidth; j++)
		{
			pDstRow[j] = (tmpRow[2 * j - 2] + tmpRow[2 * j + 2] ) +
				((tmpRow[2 * j - 1] + tmpRow[2 * j + 1]) * 4) + tmpRow[2 * j] * 6 >> 4;
		}

		if (lDstWidth < lDstW)
		{
			for (j = lDstWidth; j < lDstW; ++j)
			{
				pDstRow[j] = pDstRow[lDstWidth - 1];
			}
		}
	}

	if (lEnd < lEnd_last && lDstHeight < lDstH)
	{
		for (i = lDstHeight; i < lDstH; ++i)
		{
			MMemCpy(pDst0 + i * lDstStep, pDst0 + (lDstHeight - 1) * lDstStep, sizeof(MInt16) * lDstStep);
		}
	}

	tmpRow -= 2;
	return;
}


static MVoid thread_Img_Guass5x5_Down2_u16(MVoid* para)
{
	GauPyrDownUpPara* pPara = (GauPyrDownUpPara*)para;

	if (MNull != para)
	{
		Img_Guass5x5_Down2_u16_Stripe(pPara->tmpBuf01, pPara->pSrcImg, pPara->pDstImg, pPara->lStart, pPara->lEnd);
	}
	return;
}

MRESULT Img_Guass5x5_Down2_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg,
	LPASVLOFFSCREEN pDstImg)
{
	MRESULT lret = MOK;
	MInt32 lDstHeight = 0, lSrcWidth = 0;
	MInt32 lBlockH = 0;
	MInt32 taskId[8] = { MNull };
	GauPyrDownUpPara para[8] = { MNull };
	MUInt16* pTmpBuf[8] = { MNull };
	MInt32 k = 0, lTaskNum = 1;

	if (!pSrcImg || !pDstImg)
		return MERR_INVALID_PARAM;

	lDstHeight = pDstImg->i32Height;
	lSrcWidth = pSrcImg->i32Width;

	lTaskNum = (lDstHeight > 1024) ? 8 : 4;
	lBlockH = (lDstHeight / lTaskNum) >> 1 << 1;

	pTmpBuf[0] = (MUInt16*)MMemAlloc(hMemMgr, lTaskNum * (lSrcWidth + 4) * sizeof(MUInt16));
	if (MNull == pTmpBuf[0])
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	for (k = 1; k < lTaskNum; ++k)
	{
		pTmpBuf[k] = pTmpBuf[k - 1] + (lSrcWidth + 4);
	}


#ifdef MCV_MULTI_THREAD
	for (k = 0; k < lTaskNum; ++k)
	{
		para[k].tmpBuf01 = (MUInt16*)pTmpBuf[k];
		para[k].pSrcImg = pSrcImg;
		para[k].pDstImg = pDstImg;
		para[k].lStart = k * lBlockH;
		para[k].lEnd = (k + 1) * lBlockH;
	}
	para[lTaskNum - 1].lEnd = lDstHeight;

	for (k = 0; k < lTaskNum; ++k)
	{
		taskId[k] = mcvAddTask(mcvParallelMonitor, thread_Img_Guass5x5_Down2_u16, (MVoid*)&para[k]);
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		lret = mcvWaitTask(mcvParallelMonitor, taskId[k]);
		if (MOK != lret)
		{
			lret = MERR_BAD_STATE;
			goto exit;
		}
	}
#else
	{
#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
 		for (MInt32 lTask = 0; lTask < lTaskNum; lTask++)
		{
			MInt32 rowBegin = lTask * lBlockH;
			MInt32 rowEnd = (lTask == lTaskNum - 1) ? lDstHeight : (rowBegin + lBlockH);
			MInt32 curThread = 0;
#ifdef NH_ENABLE_OPENMP
			curThread = omp_get_thread_num();
#endif	

			Img_Guass5x5_Down2_u16_Stripe(pTmpBuf[curThread], pSrcImg, pDstImg, rowBegin, rowEnd);
		}
	}
#endif

exit:
	if (MNull != pTmpBuf[0])
	{
		MMemFree(hMemMgr, pTmpBuf[0]);
		pTmpBuf[0] = MNull;
	}
	return lret;
}

static void ver_line_process_gauss_5x5_up2_u16(MInt32 lSrcWidth, MUInt16* srcRow0, MUInt16* srcRow1, MUInt16* srcRow2, MUInt16* tmpRow1, MUInt16* tmpRow2)
{
	MInt32 j = 0;

	for (; j < lSrcWidth; j++)
	{
		tmpRow1[j] = (((srcRow0[j] + srcRow2[j]) ) + 6 * srcRow1[j]) >> 3;
		tmpRow2[j] = (srcRow1[j] + srcRow2[j]) >> 1;
	}

	tmpRow1[-1] = tmpRow1[1];
	tmpRow2[-1] = tmpRow2[1];
	tmpRow1[lSrcWidth] = tmpRow1[lSrcWidth - 2];
	tmpRow2[lSrcWidth] = tmpRow2[lSrcWidth - 2];
}

static void hor_line_process_gauss5x5_up2_sub_u16(MUInt16* pDstRow1, MUInt16* pDstRow2, MUInt16*pRefRow1, MUInt16*pRefRow2,
	MUInt16* tmpRow1, MUInt16* tmpRow2, MInt32 lSrcWidth)
{
	MInt32 j = 0;

	for (; j < lSrcWidth; j++)
	{
		MInt32 lval;

		lval = ((tmpRow1[j - 1] + tmpRow1[j + 1]) + 6 * tmpRow1[j]) >> 3;
		pDstRow1[2 * j] = TRIM_10B(pRefRow1[2 * j] - lval + 512);

		lval = (tmpRow1[j] + tmpRow1[j + 1]) >> 1;
		pDstRow1[2 * j + 1] = TRIM_10B(pRefRow1[2 * j + 1] - lval + 512);

		lval = ((tmpRow2[j - 1] + tmpRow2[j + 1]) + 6 * tmpRow2[j]) >> 3;
		pDstRow2[2 * j] = TRIM_10B(pRefRow2[2 * j] - lval + 512);

		lval = (tmpRow2[j] + tmpRow2[j + 1]) >> 1;
		pDstRow2[2 * j + 1] = TRIM_10B(pRefRow2[2 * j + 1] - lval + 512);
	}

}

static void hor_line_process_gauss5x5_up2_add_u16(MUInt16* pDstRow1, MUInt16* pDstRow2, MUInt16* pRefRow1, MUInt16* pRefRow2,
	MUInt16* tmpRow1, MUInt16* tmpRow2, MInt32 lSrcWidth)
{
	MInt32 j = 0;

	for (; j < lSrcWidth; j++)
	{
		MInt32 lval;

		lval = ((tmpRow1[j - 1] + tmpRow1[j + 1]) + 6 * tmpRow1[j]) >> 3;
		pDstRow1[2 * j] = TRIM_10B(pRefRow1[2 * j] + lval - 512);

		lval = (tmpRow1[j] + tmpRow1[j + 1]) >> 1;
		pDstRow1[2 * j + 1] = TRIM_10B(pRefRow1[2 * j + 1] + lval - 512);

		lval = ((tmpRow2[j - 1] + tmpRow2[j + 1]) + 6 * tmpRow2[j]) >> 3;
		pDstRow2[2 * j] = TRIM_10B(pRefRow2[2 * j] + lval - 512);

		lval = (tmpRow2[j] + tmpRow2[j + 1]) >> 1;
		pDstRow2[2 * j + 1] = TRIM_10B(pRefRow2[2 * j + 1] + lval - 512);
	}

}

static void hor_line_process_gauss5x5_up2_u16(MInt32 lSrcWidth, MUInt16* tmpRow1, MUInt16* pDstRow1, MUInt16* tmpRow2, MUInt16* pDstRow2)
{
	MInt32 j = 0;

	for (; j < lSrcWidth; j++)
	{
		pDstRow1[2 * j] = ((tmpRow1[j - 1] + tmpRow1[j + 1]) + 6 * tmpRow1[j]) >> 3;
		pDstRow1[2 * j + 1] = (tmpRow1[j] + tmpRow1[j + 1]) >> 1;

		pDstRow2[2 * j] = ((tmpRow2[j - 1] + tmpRow2[j + 1]) + 6* tmpRow2[j]) >> 3;
		pDstRow2[2 * j + 1] = (tmpRow2[j] + tmpRow2[j + 1]) >> 1;
	}
}

static MRESULT Img_Guass5x5_Up2_u16_Stripe(MUInt16* tmpRow1, MUInt16* tmpRow2, LPASVLOFFSCREEN pSrcImg,
	LPASVLOFFSCREEN pDstImg, MInt32 lStart, MInt32 lEnd)
{
	MRESULT lret = MOK;
	MInt32 i = 0, j = 0;
	MInt32 lSrcWidth = 0, lSrcHeight = 0;
	MInt32 lDstWidth = 0, lDstHeight = 0;
	MInt32 lSrcStep = 0;
	MInt32 lDstStep = 0;
	MUInt16* pSrcRow = MNull, * pDstRow1 = MNull, * pDstRow2 = MNull;
	MUInt16* pSrc0, * pDst0;
	MInt32 lDstWidthCrop = 0;
	//const MInt16 kernel[5] = { 1, 2, 3, 2, 1 }; //{ 1, 4, 6, 4, 1 } <---> { 0.5, 2, 3, 2, 0.5 }

	lSrcWidth = pSrcImg->i32Width;
	lSrcHeight = pSrcImg->i32Height;
	lSrcStep = pSrcImg->pi32Pitch[0] >> 1;
	pSrc0 = (MUInt16*)pSrcImg->ppu8Plane[0];

	lDstWidth = pDstImg->i32Width;
	lDstHeight = pDstImg->i32Height;
	lDstStep = pDstImg->pi32Pitch[0] >> 1;
	pDst0 = (MUInt16*)pDstImg->ppu8Plane[0];

	if ((lSrcWidth << 1) > lDstWidth)
		lSrcWidth = lDstWidth >> 1;

	if ((lSrcHeight << 1) > lDstHeight)
	{
		if (lEnd == lSrcHeight)
		{
			lEnd = lDstHeight >> 1;
		}
		lSrcHeight = lDstHeight >> 1;
	}
	lDstWidthCrop = lDstWidth >> 1 << 1;

	tmpRow1++;
	tmpRow2++;

	for (i = lStart; i < lEnd; i++)
	{
		MUInt16* srcRow0 = i - 1 < 0 ? pSrc0 + lSrcStep : pSrc0 + (i - 1) * lSrcStep;
		MUInt16* srcRow1 = pSrc0 + i * lSrcStep;
		MUInt16* srcRow2 = i + 1 >= lSrcHeight ? pSrc0 + (lSrcHeight - 1) * lSrcStep : pSrc0 + (i + 1) * lSrcStep;

		MUInt16* pDstRow1 = pDst0 + 2 * i * lDstStep;
		MUInt16* pDstRow2 = pDstRow1 + lDstStep;

		ver_line_process_gauss_5x5_up2_u16(lSrcWidth, srcRow0, srcRow1, srcRow2, tmpRow1, tmpRow2);

		hor_line_process_gauss5x5_up2_u16(lSrcWidth, tmpRow1, pDstRow1, tmpRow2, pDstRow2);

		for (MInt32 x = lDstWidthCrop; x < lDstWidth; x++)
		{
			pDstRow1[x] = pDstRow1[x - 1];
			pDstRow2[x] = pDstRow2[x - 1];
		}
	}

	return lret;
}


static MVoid thread_Img_Guass5x5_Up2_u16(MVoid* para)
{
	if (MNull != para)
	{
		GauPyrDownUpPara* pPara = (GauPyrDownUpPara*)para;
		Img_Guass5x5_Up2_u16_Stripe(pPara->tmpBuf01, pPara->tmpBuf02, pPara->pSrcImg, pPara->pDstImg,
			pPara->lStart, pPara->lEnd);
	}

	return;
}

MRESULT Img_Guass5x5_Up2_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg)
{
	MRESULT lret = MOK;
	MInt32 k = 0;
	GauPyrDownUpPara para[8] = { MNull };
	MInt32 taskId[8] = { MNull };
	MInt32 lBlockH = 0;
	MInt32 lSrcWidth = 0, lSrcHeight = 0;
	MInt32 lDstWidth = 0, lDstHeight = 0;
	MUInt16* tmpBuf[16] = { MNull };
	MInt32 lSize = 0;
	MInt32 lTaskNum = 1;

	if (!pSrcImg || !pDstImg)
		return MERR_INVALID_PARAM;

	lSrcHeight = pSrcImg->i32Height;
	lSrcWidth = pSrcImg->i32Width;
	lDstHeight = pDstImg->i32Height;
	lDstWidth = pDstImg->i32Width;

	if ((lSrcWidth << 1) > lDstWidth)
		lSrcWidth = lDstWidth >> 1;

	if ((lSrcHeight << 1) > lDstHeight)
		lSrcHeight = lDstHeight >> 1;


	lTaskNum = (lDstHeight > 1024) ? 8 : 4;
	lBlockH = (lSrcHeight / lTaskNum) >> 1 << 1;

	lSize = (lSrcWidth + 2);

	tmpBuf[0] = (MUInt16*)MMemAlloc(hMemMgr, lTaskNum * lSize * 2 * sizeof(MUInt16));
	if (MNull == tmpBuf[0])
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	for (k = 1; k < (lTaskNum << 1); ++k)
	{
		tmpBuf[k] = tmpBuf[k - 1] + lSize;
	}



#ifdef MCV_MULTI_THREAD
	for (k = 0; k < lTaskNum; ++k)
	{
		para[k].tmpBuf01 = (MUInt16 *)tmpBuf[k << 1];
		para[k].tmpBuf02 = (MUInt16 *)tmpBuf[(k << 1) + 1];
		para[k].pSrcImg = pSrcImg;
		para[k].pDstImg = pDstImg;
		para[k].lStart = k * lBlockH;
		para[k].lEnd = (k + 1) * lBlockH;
	}
	para[lTaskNum - 1].lEnd = lSrcHeight;

	for (k = 0; k < lTaskNum; ++k)
	{
		taskId[k] = mcvAddTask(mcvParallelMonitor, thread_Img_Guass5x5_Up2_u16, (MVoid*)&para[k]);
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		lret = mcvWaitTask(mcvParallelMonitor, taskId[k]);
		if (MOK != lret)
		{
			lret = MERR_BAD_STATE;
			goto exit;
		}
	}
#else

#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
	for (MInt32 lTask = 0; lTask < lTaskNum; lTask++)
	{
		MInt32 rowBegin = lTask * lBlockH;
		MInt32 rowEnd = (lTask == lTaskNum - 1) ? lSrcHeight : (rowBegin + lBlockH);
		MInt32 curThread = 0;
#ifdef NH_ENABLE_OPENMP
		curThread = omp_get_thread_num();
#endif	

		Img_Guass5x5_Up2_u16_Stripe(tmpBuf[curThread * 2], tmpBuf[curThread * 2 + 1], pSrcImg, pDstImg, rowBegin, rowEnd);
	}


#endif

	// last line

	for (MInt32 y = (lDstHeight >> 1 << 1); y < lDstHeight; y++)
	{
		MUInt16* pDataCur = (MUInt16*)(pDstImg->ppu8Plane[0] + y * pDstImg->pi32Pitch[0]);
		MUInt16* pDataPre = pDataCur - (pDstImg->pi32Pitch[0] >> 1);
		
		MMemCpy(pDataCur, pDataPre, lDstWidth * sizeof(MUInt16));
	
	}


exit:
	if (MNull != tmpBuf[0])
	{
		MMemFree(hMemMgr, tmpBuf[0]);
		tmpBuf[0] = MNull;
	}
	return lret;
}

static MRESULT Img_Guass5x5_Up2_sub_u16_Stripe(MUInt16* tmpRow1, MUInt16* tmpRow2, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pRefImg,
	LPASVLOFFSCREEN pDstImg, MInt32 lStart, MInt32 lEnd)
{
	MRESULT lret = MOK;
	MInt32 i = 0, j = 0;
	MInt32 lSrcWidth = 0, lSrcHeight = 0;
	MInt32 lDstWidth = 0, lDstHeight = 0;
	MInt32 lSrcStep = 0, lRefStep = 0, lDstStep = 0;
	MUInt16* pSrcRow = MNull, * pDstRow1 = MNull, * pDstRow2 = MNull;
	MUInt16* pSrc0, * pRef0, * pDst0;
	MInt32 lDstWidthCrop;
	//const MInt16 kernel[5] = { 1, 2, 3, 2, 1 }; //{ 1, 4, 6, 4, 1 } <---> { 0.5, 2, 3, 2, 0.5 }

	lSrcWidth = pSrcImg->i32Width;
	lSrcHeight = pSrcImg->i32Height;
	lSrcStep = pSrcImg->pi32Pitch[0] >> 1;
	pSrc0 = (MUInt16*)pSrcImg->ppu8Plane[0];

	lDstWidth = pDstImg->i32Width;
	lDstHeight = pDstImg->i32Height;
	lDstStep = pDstImg->pi32Pitch[0] >> 1;
	pDst0 = (MUInt16*)pDstImg->ppu8Plane[0];

	pRef0 = (MUInt16*)pRefImg->ppu8Plane[0];
	lRefStep = pRefImg->pi32Pitch[0] >> 1;

	if ((lSrcWidth << 1) > lDstWidth)
		lSrcWidth = lDstWidth >> 1;

	if ((lSrcHeight << 1) > lDstHeight)
	{
		if (lEnd == lSrcHeight)
		{
			lEnd = lDstHeight >> 1;
		}
		lSrcHeight = lDstHeight >> 1;
	}
	lDstWidthCrop = lDstWidth >> 1 << 1;
	tmpRow1++;
	tmpRow2++;

	for (i = lStart; i < lEnd; i++)
	{
		MUInt16* srcRow0 = i - 1 < 0 ? pSrc0 + lSrcStep : pSrc0 + (i - 1) * lSrcStep;
		MUInt16* srcRow1 = pSrc0 + i * lSrcStep;
		MUInt16* srcRow2 = i + 1 >= lSrcHeight ? pSrc0 + (lSrcHeight - 1) * lSrcStep : pSrc0 + (i + 1) * lSrcStep;

		MUInt16* pDstRow1 = pDst0 + 2 * i * lDstStep;
		MUInt16* pDstRow2 = pDstRow1 + lDstStep;

		MUInt16* pRefRow1 = pRef0 + 2 * i * lRefStep;
		MUInt16* pRefRow2 = pRefRow1 + lRefStep;

		ver_line_process_gauss_5x5_up2_u16(lSrcWidth, srcRow0, srcRow1, srcRow2, tmpRow1, tmpRow2);

		hor_line_process_gauss5x5_up2_sub_u16(pDstRow1, pDstRow2, pRefRow1, pRefRow2, tmpRow1, tmpRow2, lSrcWidth);

		for (MInt32 x = lDstWidthCrop; x < lDstWidth; x++)
		{
			pDstRow1[x] = pDstRow1[x - 1];
			pDstRow2[x] = pDstRow2[x - 1];
		}
	}

	return lret;
}


static MVoid thread_Img_Guass5x5_Up2_sub_u16(MVoid* para)
{
	if (MNull != para)
	{
		GauPyrDownUpPara* pPara = (GauPyrDownUpPara*)para;
		Img_Guass5x5_Up2_sub_u16_Stripe(pPara->tmpBuf01, pPara->tmpBuf02, pPara->pSrcImg,
			pPara->pRefImg, pPara->pDstImg, pPara->lStart, pPara->lEnd);
	}

	return;
}

// dst = ref - src_up2
MRESULT Img_Guass5x5_Up2_sub_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pRefImg, LPASVLOFFSCREEN pDstImg)
{
	MRESULT lret = MOK;
	MInt32 k = 0;
	GauPyrDownUpPara para[8] = { MNull };
	MInt32 taskId[8] = { MNull };
	MInt32 lBlockH = 0;
	MInt32 lSrcWidth = 0, lSrcHeight = 0;
	MInt32 lDstWidth = 0, lDstHeight = 0;
	MUInt16* tmpBuf[16] = { MNull };
	MInt32 lSize = 0;
	MInt32 lTaskNum = 1;

	if (!pSrcImg || !pDstImg)
		return MERR_INVALID_PARAM;

	lSrcHeight = pSrcImg->i32Height;
	lSrcWidth = pSrcImg->i32Width;
	lDstHeight = pDstImg->i32Height;
	lDstWidth = pDstImg->i32Width;

	if ((lSrcWidth << 1) > lDstWidth)
		lSrcWidth = lDstWidth >> 1;

	if ((lSrcHeight << 1) > lDstHeight)
		lSrcHeight = lDstHeight >> 1;


	lTaskNum = (lDstHeight > 1024) ? 8 : 4;
	lBlockH = (lSrcHeight / lTaskNum) >> 1 << 1;

	lSize = (lSrcWidth + 2);

	tmpBuf[0] = (MUInt16*)MMemAlloc(hMemMgr, lTaskNum * lSize * 2 * sizeof(MUInt16));
	if (MNull == tmpBuf[0])
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	for (k = 1; k < (lTaskNum << 1); ++k)
	{
		tmpBuf[k] = tmpBuf[k - 1] + lSize;
	}



#ifdef MCV_MULTI_THREAD
	for (k = 0; k < lTaskNum; ++k)
	{
		para[k].tmpBuf01 = (MUInt16*)tmpBuf[k << 1];
		para[k].tmpBuf02 = (MUInt16*)tmpBuf[(k << 1) + 1];
		para[k].pSrcImg = pSrcImg;
		para[k].pRefImg = pRefImg;
		para[k].pDstImg = pDstImg;
		para[k].lStart = k * lBlockH;
		para[k].lEnd = (k + 1) * lBlockH;
	}
	para[lTaskNum - 1].lEnd = lSrcHeight;

	for (k = 0; k < lTaskNum; ++k)
	{
		taskId[k] = mcvAddTask(mcvParallelMonitor, thread_Img_Guass5x5_Up2_sub_u16, (MVoid*)&para[k]);
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		lret = mcvWaitTask(mcvParallelMonitor, taskId[k]);
		if (MOK != lret)
		{
			lret = MERR_BAD_STATE;
			goto exit;
		}
	}
#else

#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
	for (MInt32 lTask = 0; lTask < lTaskNum; lTask++)
	{
		MInt32 rowBegin = lTask * lBlockH;
		MInt32 rowEnd = (lTask == lTaskNum - 1) ? lSrcHeight : (rowBegin + lBlockH);
		MInt32 curThread = 0;
#ifdef NH_ENABLE_OPENMP
		curThread = omp_get_thread_num();
#endif	

		Img_Guass5x5_Up2_sub_u16_Stripe(tmpBuf[curThread * 2], tmpBuf[curThread * 2 + 1], pSrcImg, pRefImg, pDstImg, rowBegin, rowEnd);
	}

#endif

	for (MInt32 y = (lDstHeight >> 1 << 1); y < lDstHeight; y++)
	{
		MUInt16* pDataCur = (MUInt16*)(pDstImg->ppu8Plane[0] + y * pDstImg->pi32Pitch[0]);
		MUInt16* pDataPre = pDataCur - (pDstImg->pi32Pitch[0] >> 1);

		MMemCpy(pDataCur, pDataPre, lDstWidth * sizeof(MUInt16));

	}

exit:
	if (MNull != tmpBuf[0])
	{
		MMemFree(hMemMgr, tmpBuf[0]);
		tmpBuf[0] = MNull;
	}
	return lret;
}



static MRESULT Img_Guass5x5_Up2_add_u16_Stripe(MUInt16* tmpRow1, MUInt16* tmpRow2, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pRefImg,
	LPASVLOFFSCREEN pDstImg, MInt32 lStart, MInt32 lEnd)
{
	MRESULT lret = MOK;
	MInt32 i = 0, j = 0;
	MInt32 lSrcWidth = 0, lSrcHeight = 0;
	MInt32 lDstWidth = 0, lDstHeight = 0;
	MInt32 lSrcStep = 0, lRefStep = 0, lDstStep = 0;
	MUInt16* pSrcRow = MNull, * pDstRow1 = MNull, * pDstRow2 = MNull;
	MUInt16* pSrc0, * pRef0, * pDst0;
	MInt32 lDstWidthCrop;
	//const MInt16 kernel[5] = { 1, 2, 3, 2, 1 }; //{ 1, 4, 6, 4, 1 } <---> { 0.5, 2, 3, 2, 0.5 }

	lSrcWidth = pSrcImg->i32Width;
	lSrcHeight = pSrcImg->i32Height;
	lSrcStep = pSrcImg->pi32Pitch[0] >> 1;
	pSrc0 = (MUInt16*)pSrcImg->ppu8Plane[0];

	lDstWidth = pDstImg->i32Width;
	lDstHeight = pDstImg->i32Height;
	lDstStep = pDstImg->pi32Pitch[0] >> 1;
	pDst0 = (MUInt16*)pDstImg->ppu8Plane[0];

	pRef0 = (MUInt16*)pRefImg->ppu8Plane[0];
	lRefStep = pRefImg->pi32Pitch[0] >> 1;

	if ((lSrcWidth << 1) > lDstWidth)
		lSrcWidth = lDstWidth >> 1;

	if ((lSrcHeight << 1) > lDstHeight)
	{
		if (lEnd == lSrcHeight)
		{
			lEnd = lDstHeight >> 1;
		}
		lSrcHeight = lDstHeight >> 1;
	}
	lDstWidthCrop = lDstWidth >> 1 << 1;
	tmpRow1++;
	tmpRow2++;

	for (i = lStart; i < lEnd; i++)
	{
		MUInt16* srcRow0 = i - 1 < 0 ? pSrc0 + lSrcStep : pSrc0 + (i - 1) * lSrcStep;
		MUInt16* srcRow1 = pSrc0 + i * lSrcStep;
		MUInt16* srcRow2 = i + 1 >= lSrcHeight ? pSrc0 + (lSrcHeight - 1) * lSrcStep : pSrc0 + (i + 1) * lSrcStep;

		MUInt16* pDstRow1 = pDst0 + 2 * i * lDstStep;
		MUInt16* pDstRow2 = pDstRow1 + lDstStep;

		MUInt16* pRefRow1 = pRef0 + 2 * i * lRefStep;
		MUInt16* pRefRow2 = pRefRow1 + lRefStep;

		ver_line_process_gauss_5x5_up2_u16(lSrcWidth, srcRow0, srcRow1, srcRow2, tmpRow1, tmpRow2);

		hor_line_process_gauss5x5_up2_add_u16(pDstRow1, pDstRow2, pRefRow1, pRefRow2, tmpRow1, tmpRow2, lSrcWidth);

		for (MInt32 x = lDstWidthCrop; x < lDstWidth; x++)
		{
			pDstRow1[x] = pDstRow1[x - 1];
			pDstRow2[x] = pDstRow2[x - 1];
		}
	}

	return lret;
}


static MVoid thread_Img_Guass5x5_Up2_add_u16(MVoid* para)
{
	if (MNull != para)
	{
		GauPyrDownUpPara* pPara = (GauPyrDownUpPara*)para;
		Img_Guass5x5_Up2_add_u16_Stripe(pPara->tmpBuf01, pPara->tmpBuf02, pPara->pSrcImg,
			pPara->pRefImg, pPara->pDstImg, pPara->lStart, pPara->lEnd);
	}

	return;
}

// dst = ref - src_up2
MRESULT Img_Guass5x5_Up2_add_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pRefImg, LPASVLOFFSCREEN pDstImg)
{
	MRESULT lret = MOK;
	MInt32 k = 0;
	GauPyrDownUpPara para[8] = { MNull };
	MInt32 taskId[8] = { MNull };
	MInt32 lBlockH = 0;
	MInt32 lSrcWidth = 0, lSrcHeight = 0;
	MInt32 lDstWidth = 0, lDstHeight = 0;
	MUInt16* tmpBuf[16] = { MNull };
	MInt32 lSize = 0;
	MInt32 lTaskNum = 1;

	if (!pSrcImg || !pDstImg)
		return MERR_INVALID_PARAM;

	lSrcHeight = pSrcImg->i32Height;
	lSrcWidth = pSrcImg->i32Width;
	lDstHeight = pDstImg->i32Height;
	lDstWidth = pDstImg->i32Width;

	if ((lSrcWidth << 1) > lDstWidth)
		lSrcWidth = lDstWidth >> 1;

	if ((lSrcHeight << 1) > lDstHeight)
		lSrcHeight = lDstHeight >> 1;


	lTaskNum = (lDstHeight > 1024) ? 8 : 4;
	lBlockH = (lSrcHeight / lTaskNum) >> 1 << 1;

	lSize = (lSrcWidth + 2);

	tmpBuf[0] = (MUInt16*)MMemAlloc(hMemMgr, lTaskNum * lSize * 2 * sizeof(MUInt16));
	if (MNull == tmpBuf[0])
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}
	for (k = 1; k < (lTaskNum << 1); ++k)
	{
		tmpBuf[k] = tmpBuf[k - 1] + lSize;
	}



#ifdef MCV_MULTI_THREAD
	for (k = 0; k < lTaskNum; ++k)
	{
		para[k].tmpBuf01 = (MUInt16*)tmpBuf[k << 1];
		para[k].tmpBuf02 = (MUInt16*)tmpBuf[(k << 1) + 1];
		para[k].pSrcImg = pSrcImg;
		para[k].pRefImg = pRefImg;
		para[k].pDstImg = pDstImg;
		para[k].lStart = k * lBlockH;
		para[k].lEnd = (k + 1) * lBlockH;
	}
	para[lTaskNum - 1].lEnd = lSrcHeight;

	for (k = 0; k < lTaskNum; ++k)
	{
		taskId[k] = mcvAddTask(mcvParallelMonitor, thread_Img_Guass5x5_Up2_add_u16, (MVoid*)&para[k]);
	}

	for (k = 0; k < lTaskNum; ++k)
	{
		lret = mcvWaitTask(mcvParallelMonitor, taskId[k]);
		if (MOK != lret)
		{
			lret = MERR_BAD_STATE;
			goto exit;
		}
	}
#else

#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
	for (MInt32 lTask = 0; lTask < lTaskNum; lTask++)
	{
		MInt32 rowBegin = lTask * lBlockH;
		MInt32 rowEnd = (lTask == lTaskNum - 1) ? lSrcHeight : (rowBegin + lBlockH);
		MInt32 curThread = 0;
#ifdef NH_ENABLE_OPENMP
		curThread = omp_get_thread_num();
#endif	

		Img_Guass5x5_Up2_add_u16_Stripe(tmpBuf[curThread * 2], tmpBuf[curThread * 2 + 1], pSrcImg, pRefImg, pDstImg, rowBegin, rowEnd);
	}


#endif

	for (MInt32 y = (lDstHeight >> 1 << 1); y < lDstHeight; y++)
	{
		MUInt16* pDataCur = (MUInt16*)(pDstImg->ppu8Plane[0] + y * pDstImg->pi32Pitch[0]);
		MUInt16* pDataPre = pDataCur - (pDstImg->pi32Pitch[0] >> 1);

		MMemCpy(pDataCur, pDataPre, lDstWidth * sizeof(MUInt16));

	}
exit:
	if (MNull != tmpBuf[0])
	{
		MMemFree(hMemMgr, tmpBuf[0]);
		tmpBuf[0] = MNull;
	}
	return lret;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END