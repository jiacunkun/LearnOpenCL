#include "single_image_enhancement_define.h"
#include "mobilecv.h"

#define MED_TASK_NUM 8

#define MINMAX(a,b)			\
{MInt32 t = (a)-(b); t = MAX(t, 0); (b) += t, (a) -= t; }


#define KERNEL_SIZE    (5)
#define SORTED_SIZE    (9)

#if defined(MCV_MULTI_THREAD)
typedef struct _tag_CROSS_MEDIAN_5X5 {
	MInt32          thread_ID;
	MByte           *pSrcImg;
	MByte           *pDstImg;
	MInt32          lSrcPitch;
	MInt32          lDstPitch;
	MInt32          lImgHeight;
	MInt32          lImgWidth;
	MInt32          lTopLine;
	MInt32          lBotLine;
	MByte			*pBufData;
	MInt32			cn;
}MedianFilter;
#endif

MVoid cross_median_5x5_rows_cn1(MByte *srcdata, MInt32 lsrc_step, MByte *dstdata, MInt32 ldst_step, MInt32 lWidth, MInt32 lHeight, MByte *tmprow, MInt32 startRow, MInt32 endRow)
{
	MInt32 i, j, m;
	MByte tmpMed;
	MByte *src, *src0, *src1, *src2, *src3, *src4, *tmpSrc2, *dstPtr;
	MByte p0, p1, p2, p3, p4, p5, p6, p7, p8;
#ifdef _ARM_NEON_
	uint8x16_t srcdata00, srcdata01, srcdata02, srcdata03, srcdata04, srcdata05, srcdata06, srcdata07, srcdata08;
	uint8x16_t tmpdata_8x16;
#endif

	if (0 == startRow)
	{
		src0 = srcdata + 2 * lsrc_step;
		src1 = srcdata + 1 * lsrc_step;
		tmpSrc2 = srcdata;
		src3 = srcdata + 1 * lsrc_step;
		src4 = srcdata + 2 * lsrc_step;
	}
	else
	{
		src0 = srcdata + (startRow - 2) * lsrc_step;
		src1 = srcdata + (startRow - 1) * lsrc_step;
		tmpSrc2 = srcdata + startRow * lsrc_step;
		src3 = srcdata + (startRow + 1) * lsrc_step;
		src4 = srcdata + (startRow + 2) * lsrc_step;
	}
	for (i = startRow; i < endRow; ++i)
	{
		src2 = tmprow + 2;
		dstPtr = dstdata + i*ldst_step;

		MMemCpy(src2, tmpSrc2, lWidth*sizeof(MByte));
		src2[-1] = src2[1];
		src2[-2] = src2[2];
		src2[lWidth] = src2[lWidth - 2];
		src2[lWidth + 1] = src2[lWidth - 3];

		j = 0;
#ifdef _ARM_NEON_
		for (; j < lWidth - 15; j += 16)
		{
			srcdata00 = vld1q_u8(src0 + j);
			srcdata01 = vld1q_u8(src1 + j);
			srcdata02 = vld1q_u8(src3 + j);
			srcdata03 = vld1q_u8(src4 + j);
			srcdata04 = vld1q_u8(src2 + j - 2);
			srcdata05 = vld1q_u8(src2 + j - 1);
			srcdata06 = vld1q_u8(src2 + j);
			srcdata07 = vld1q_u8(src2 + j + 1);
			srcdata08 = vld1q_u8(src2 + j + 2);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata05);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata05 = vaddq_u8(srcdata05, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata07, srcdata08);
			srcdata07 = vsubq_u8(srcdata07, tmpdata_8x16);
			srcdata08 = vaddq_u8(srcdata08, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);
			srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata03, srcdata04);
			srcdata03 = vsubq_u8(srcdata03, tmpdata_8x16);
			srcdata04 = vaddq_u8(srcdata04, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata06, srcdata07);
			srcdata06 = vsubq_u8(srcdata06, tmpdata_8x16);
			srcdata07 = vaddq_u8(srcdata07, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata05);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata05 = vaddq_u8(srcdata05, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata07, srcdata08);
			srcdata07 = vsubq_u8(srcdata07, tmpdata_8x16);
			srcdata08 = vaddq_u8(srcdata08, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata03);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);
			srcdata03 = vaddq_u8(srcdata03, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata05, srcdata08);
			srcdata05 = vsubq_u8(srcdata05, tmpdata_8x16);
			srcdata08 = vaddq_u8(srcdata08, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata07);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata07 = vaddq_u8(srcdata07, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata03, srcdata06);
			srcdata03 = vsubq_u8(srcdata03, tmpdata_8x16);
			srcdata06 = vaddq_u8(srcdata06, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata04);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			srcdata04 = vaddq_u8(srcdata04, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata02, srcdata05);
			srcdata02 = vsubq_u8(srcdata02, tmpdata_8x16);
			srcdata05 = vaddq_u8(srcdata05, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata07);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata07 = vaddq_u8(srcdata07, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata02);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata06, srcdata04);
			srcdata06 = vsubq_u8(srcdata06, tmpdata_8x16);
			srcdata04 = vaddq_u8(srcdata04, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata02);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);

			vst1q_u8(dstPtr + j, srcdata04);

		}
#endif
		for (; j < lWidth; ++j)
		{
			p0 = src0[j];        p1 = src1[j];        p2 = src3[j];
			p3 = src4[j];        p4 = src2[j - 2];    p5 = src2[j - 1];
			p6 = src2[j];        p7 = src2[j + 1];    p8 = src2[j + 2];

			MINMAX(p1, p2);   MINMAX(p4, p5);   MINMAX(p7, p8);
			MINMAX(p0, p1);   MINMAX(p3, p4);   MINMAX(p6, p7);
			MINMAX(p1, p2);   MINMAX(p4, p5);   MINMAX(p7, p8);
			MINMAX(p0, p3);   MINMAX(p5, p8);   MINMAX(p4, p7);
			MINMAX(p3, p6);   MINMAX(p1, p4);   MINMAX(p2, p5);
			MINMAX(p4, p7);   MINMAX(p4, p2);   MINMAX(p6, p4);
			MINMAX(p4, p2);

			dstPtr[j] = p4;
		}
		src0 = src1;
		src1 = tmpSrc2;
		tmpSrc2 = src3;
		src3 = src4;
		src4 = (i + 2 >= lHeight - 1) ? src4 - lsrc_step : src4 + lsrc_step;
	}
}

static MVoid cross_median_5x5_rows(MByte *srcdata, MInt32 lsrc_step, MByte *dstdata, MInt32 ldst_step, MInt32 lWidth, MInt32 lHeight, MByte *tmprow, MInt32 cn, MInt32 startRow, MInt32 endRow)
{
    START_TIME;
	MInt32 i, j, m;
	MByte tmpMed;
	MByte *src, *src0, *src1, *src2, *src3, *src4, *dstPtr;
	MByte p0, p1, p2, p3, p4, p5, p6, p7, p8;
#ifdef _ARM_NEON_
	uint8x16_t srcdata00, srcdata01, srcdata02, srcdata03, srcdata04, srcdata05, srcdata06, srcdata07, srcdata08;
	uint8x16_t tmpdata_8x16;
#endif

	for (i = startRow; i < endRow; ++i)
	{
		src0 = (i - 2) < 0 ? srcdata : srcdata + (i - 2) * lsrc_step;
		src1 = (i - 1) < 0 ? srcdata : srcdata + (i - 1) * lsrc_step;

		src3 = (i + 1) >= lHeight ? srcdata + (lHeight - 1) * lsrc_step : srcdata + (i + 1) * lsrc_step;
		src4 = (i + 2) >= lHeight ? srcdata + (lHeight - 1) * lsrc_step : srcdata + (i + 2) * lsrc_step;
		src2 = tmprow + cn * 2;
		dstPtr = dstdata + i * ldst_step;

		MMemCpy(src2, srcdata + i * lsrc_step, lWidth * cn *sizeof(MByte));
		for (m = 0; m < cn; m++)
		{
			src2[-cn + m] = src2[m];
			src2[-cn * 2 + m] = src2[m];

			src2[lWidth * cn + m] = src2[(lWidth - 1) * cn + m];
			src2[(lWidth + 1) * cn + m] = src2[(lWidth - 1) * cn + m];

		}

		j = 0;
#ifdef _ARM_NEON_
		for (; j < lWidth * cn - 15; j += 16)
		{
			srcdata00 = vld1q_u8(src0 + j);
			srcdata01 = vld1q_u8(src1 + j);
			srcdata02 = vld1q_u8(src3 + j);
			srcdata03 = vld1q_u8(src4 + j);
			srcdata04 = vld1q_u8(src2 + j - 2 * cn);
			srcdata05 = vld1q_u8(src2 + j - 1* cn);
			srcdata06 = vld1q_u8(src2 + j);
			srcdata07 = vld1q_u8(src2 + j + 1* cn);
			srcdata08 = vld1q_u8(src2 + j + 2* cn);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata05);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata05 = vaddq_u8(srcdata05, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata07, srcdata08);
			srcdata07 = vsubq_u8(srcdata07, tmpdata_8x16);
			srcdata08 = vaddq_u8(srcdata08, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);
			srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata03, srcdata04);
			srcdata03 = vsubq_u8(srcdata03, tmpdata_8x16);
			srcdata04 = vaddq_u8(srcdata04, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata06, srcdata07);
			srcdata06 = vsubq_u8(srcdata06, tmpdata_8x16);
			srcdata07 = vaddq_u8(srcdata07, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata05);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata05 = vaddq_u8(srcdata05, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata07, srcdata08);
			srcdata07 = vsubq_u8(srcdata07, tmpdata_8x16);
			srcdata08 = vaddq_u8(srcdata08, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata03);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);
			srcdata03 = vaddq_u8(srcdata03, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata05, srcdata08);
			srcdata05 = vsubq_u8(srcdata05, tmpdata_8x16);
			srcdata08 = vaddq_u8(srcdata08, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata07);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata07 = vaddq_u8(srcdata07, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata03, srcdata06);
			srcdata03 = vsubq_u8(srcdata03, tmpdata_8x16);
			srcdata06 = vaddq_u8(srcdata06, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata04);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			srcdata04 = vaddq_u8(srcdata04, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata02, srcdata05);
			srcdata02 = vsubq_u8(srcdata02, tmpdata_8x16);
			srcdata05 = vaddq_u8(srcdata05, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata07);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata07 = vaddq_u8(srcdata07, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata02);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata06, srcdata04);
			srcdata06 = vsubq_u8(srcdata06, tmpdata_8x16);
			srcdata04 = vaddq_u8(srcdata04, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata04, srcdata02);
			srcdata04 = vsubq_u8(srcdata04, tmpdata_8x16);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);

			vst1q_u8(dstPtr + j, srcdata04);

		}
#endif
		for (; j < lWidth * cn; j++)
		{
			p0 = src0[j];        p1 = src1[j];        p2 = src3[j];
			p3 = src4[j];        p4 = src2[j - cn * 2];    p5 = src2[j - cn];
			p6 = src2[j];        p7 = src2[j + cn];    p8 = src2[j + cn * 2];

			MINMAX(p1, p2);   MINMAX(p4, p5);   MINMAX(p7, p8);
			MINMAX(p0, p1);   MINMAX(p3, p4);   MINMAX(p6, p7);
			MINMAX(p1, p2);   MINMAX(p4, p5);   MINMAX(p7, p8);
			MINMAX(p0, p3);   MINMAX(p5, p8);   MINMAX(p4, p7);
			MINMAX(p3, p6);   MINMAX(p1, p4);   MINMAX(p2, p5);
			MINMAX(p4, p7);   MINMAX(p4, p2);   MINMAX(p6, p4);
			MINMAX(p4, p2);

			dstPtr[j] = p4;
		}

	}
	END_TIME;
}



#ifdef MCV_MULTI_THREAD
static MVoid thread_cross_median_5x5_rows(MVoid* pParam)
{
	MedianFilter *Filter = (MedianFilter*)pParam;
	MInt32 lret = MOK;

	cross_median_5x5_rows(Filter->pSrcImg, Filter->lSrcPitch, Filter->pDstImg, Filter->lDstPitch,
		Filter->lImgWidth, Filter->lImgHeight, Filter->pBufData, Filter->cn, Filter->lTopLine, Filter->lBotLine);
	return;
}
#endif



MRESULT Cross_Median_Filter_5x5(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
								   MInt32 width, MInt32 height, MInt32 cn)
{
	START_TIME;

	MRESULT lret = MOK;
	MByte* tmpRowBuf[MED_TASK_NUM] = { 0 };
	MInt32 i;
	for (i = 0; i < MED_TASK_NUM; i++)
	{
		tmpRowBuf[i] = (MByte*)MMemAlloc(hMemMgr, (width + 4)*cn*sizeof(MByte));
		if (MNull == tmpRowBuf[i])
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}
	}

#ifdef MCV_MULTI_THREAD0
	if (mcvParallelMonitor != MNull)
	{
		const MInt32 lTaskNum = MED_TASK_NUM;
		MInt32 lTaskSize = 0;
		MInt32 taskID[MED_TASK_NUM] = { 0 };
		MedianFilter pParams[MED_TASK_NUM] = { 0 };

		lTaskSize = height / lTaskNum;
		lTaskSize = lTaskSize >> 2 << 2;

		for (i = 0; i < lTaskNum; i++)
		{
			pParams[i].lTopLine = lTaskSize*i;
			pParams[i].lBotLine = lTaskSize*(i + 1);
		}
		pParams[i - 1].lBotLine = height;

		for (i = 0; i < lTaskNum; i++)
		{
			pParams[i].thread_ID = i;
			pParams[i].lImgWidth = width;
			pParams[i].lImgHeight = height;
			pParams[i].lSrcPitch = src_step;
			pParams[i].pSrcImg = src;
			pParams[i].lDstPitch = dst_step;
			pParams[i].pDstImg = dst;
			pParams[i].pBufData = tmpRowBuf[i];
			pParams[i].cn = cn;
		}

		for (i = 0; i < lTaskNum; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, thread_cross_median_5x5_rows, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTaskNum; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}
	}
	else
    {
        cross_median_5x5_rows(src,src_step, dst, dst_step, width, height, tmpRowBuf[0], cn, 0, height);
    }
#else
	cross_median_5x5_rows(src,src_step, dst, dst_step, width, height, tmpRowBuf[0], cn, 0, height);
#endif
exit:
	for (i = 0; i < MED_TASK_NUM; i++)
	{
		if (tmpRowBuf[i])
		{
			MMemFree(hMemMgr, tmpRowBuf[i]);
			tmpRowBuf[i] = MNull;
		}
	}
	END_TIME;
	return lret;
}


/************************************************************************/
/*  CROSS_MEDIAN_5X5_P010                                               */
/************************************************************************/

#if defined(MCV_MULTI_THREAD)
typedef struct _tag_CROSS_MEDIAN_5X5_P010 {
	MInt32          thread_ID;
	MUInt16         *pSrcImg;
	MUInt16         *pDstImg;
	MInt32          lSrcPitch;
	MInt32          lDstPitch;
	MInt32          lImgHeight;
	MInt32          lImgWidth;
	MInt32          lTopLine;
	MInt32          lBotLine;
	MUInt16			*pBufData;
	MInt32			cn;
}MedianFilter_P010;
#endif

static MVoid cross_median_5x5_rows_P010(MUInt16 *srcdata, MInt32 lsrc_step, MUInt16 *dstdata, MInt32 ldst_step, MInt32 lWidth, MInt32 lHeight, MUInt16 *tmprow, MInt32 cn, MInt32 startRow, MInt32 endRow)
{
	MInt32 i, j, m;
	MUInt16 *src, *src0, *src1, *src2, *src3, *src4, *dstPtr;
	MUInt16 p0, p1, p2, p3, p4, p5, p6, p7, p8;

	for (i = startRow; i < endRow; ++i)
	{
		src0 = (i - 2) < 0 ? srcdata : srcdata + (i - 2) * lsrc_step;
		src1 = (i - 1) < 0 ? srcdata : srcdata + (i - 1) * lsrc_step;

		src3 = (i + 1) >= lHeight ? srcdata + (lHeight - 1) * lsrc_step : srcdata + (i + 1) * lsrc_step;
		src4 = (i + 2) >= lHeight ? srcdata + (lHeight - 1) * lsrc_step : srcdata + (i + 2) * lsrc_step;
		src2 = tmprow + cn * 2;
		dstPtr = dstdata + i * ldst_step;

		MMemCpy(src2, srcdata + i * lsrc_step, lWidth * cn * sizeof(MUInt16));
		for (m = 0; m < cn; m++)
		{
			src2[-cn + m] = src2[m];
			src2[-cn * 2 + m] = src2[m];

			src2[lWidth * cn + m] = src2[(lWidth - 1) * cn + m];
			src2[(lWidth + 1) * cn + m] = src2[(lWidth - 1) * cn + m];

		}

		j = 0;
#ifdef _ARM_NEON_
		for (; j < lWidth * cn - 7; j += 8)
		{
			uint16x8_t srcdata00, srcdata01, srcdata02, srcdata03, srcdata04, srcdata05, srcdata06, srcdata07, srcdata08;
			uint16x8_t tmpdata_u16x8;

			srcdata00 = vld1q_u16(src0 + j);
			srcdata01 = vld1q_u16(src1 + j);
			srcdata02 = vld1q_u16(src3 + j);
			srcdata03 = vld1q_u16(src4 + j);
			srcdata04 = vld1q_u16(src2 + j - 2 * cn);
			srcdata05 = vld1q_u16(src2 + j - cn);
			srcdata06 = vld1q_u16(src2 + j);
			srcdata07 = vld1q_u16(src2 + j + cn);
			srcdata08 = vld1q_u16(src2 + j + 2 * cn);

			tmpdata_u16x8 = vqsubq_u16(srcdata01, srcdata02);
			srcdata01 = vsubq_u16(srcdata01, tmpdata_u16x8);
			srcdata02 = vaddq_u16(srcdata02, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata04, srcdata05);
			srcdata04 = vsubq_u16(srcdata04, tmpdata_u16x8);
			srcdata05 = vaddq_u16(srcdata05, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata07, srcdata08);
			srcdata07 = vsubq_u16(srcdata07, tmpdata_u16x8);
			srcdata08 = vaddq_u16(srcdata08, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata00, srcdata01);
			srcdata00 = vsubq_u16(srcdata00, tmpdata_u16x8);
			srcdata01 = vaddq_u16(srcdata01, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata03, srcdata04);
			srcdata03 = vsubq_u16(srcdata03, tmpdata_u16x8);
			srcdata04 = vaddq_u16(srcdata04, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata06, srcdata07);
			srcdata06 = vsubq_u16(srcdata06, tmpdata_u16x8);
			srcdata07 = vaddq_u16(srcdata07, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata01, srcdata02);
			srcdata01 = vsubq_u16(srcdata01, tmpdata_u16x8);
			srcdata02 = vaddq_u16(srcdata02, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata04, srcdata05);
			srcdata04 = vsubq_u16(srcdata04, tmpdata_u16x8);
			srcdata05 = vaddq_u16(srcdata05, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata07, srcdata08);
			srcdata07 = vsubq_u16(srcdata07, tmpdata_u16x8);
			srcdata08 = vaddq_u16(srcdata08, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata00, srcdata03);
			srcdata00 = vsubq_u16(srcdata00, tmpdata_u16x8);
			srcdata03 = vaddq_u16(srcdata03, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata05, srcdata08);
			srcdata05 = vsubq_u16(srcdata05, tmpdata_u16x8);
			srcdata08 = vaddq_u16(srcdata08, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata04, srcdata07);
			srcdata04 = vsubq_u16(srcdata04, tmpdata_u16x8);
			srcdata07 = vaddq_u16(srcdata07, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata03, srcdata06);
			srcdata03 = vsubq_u16(srcdata03, tmpdata_u16x8);
			srcdata06 = vaddq_u16(srcdata06, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata01, srcdata04);
			srcdata01 = vsubq_u16(srcdata01, tmpdata_u16x8);
			srcdata04 = vaddq_u16(srcdata04, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata02, srcdata05);
			srcdata02 = vsubq_u16(srcdata02, tmpdata_u16x8);
			srcdata05 = vaddq_u16(srcdata05, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata04, srcdata07);
			srcdata04 = vsubq_u16(srcdata04, tmpdata_u16x8);
			srcdata07 = vaddq_u16(srcdata07, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata04, srcdata02);
			srcdata04 = vsubq_u16(srcdata04, tmpdata_u16x8);
			srcdata02 = vaddq_u16(srcdata02, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata06, srcdata04);
			srcdata06 = vsubq_u16(srcdata06, tmpdata_u16x8);
			srcdata04 = vaddq_u16(srcdata04, tmpdata_u16x8);

			tmpdata_u16x8 = vqsubq_u16(srcdata04, srcdata02);
			srcdata04 = vsubq_u16(srcdata04, tmpdata_u16x8);
			srcdata02 = vaddq_u16(srcdata02, tmpdata_u16x8);

			vst1q_u16(dstPtr + j, srcdata04);

		}
#endif
		for (j = 0; j < lWidth * cn; j++)
		{
			p0 = src0[j];        p1 = src1[j];        p2 = src3[j];
			p3 = src4[j];        p4 = src2[j - cn * 2];    p5 = src2[j - cn];
			p6 = src2[j];        p7 = src2[j + cn];    p8 = src2[j + cn * 2];

			MINMAX(p1, p2);   MINMAX(p4, p5);   MINMAX(p7, p8);
			MINMAX(p0, p1);   MINMAX(p3, p4);   MINMAX(p6, p7);
			MINMAX(p1, p2);   MINMAX(p4, p5);   MINMAX(p7, p8);
			MINMAX(p0, p3);   MINMAX(p5, p8);   MINMAX(p4, p7);
			MINMAX(p3, p6);   MINMAX(p1, p4);   MINMAX(p2, p5);
			MINMAX(p4, p7);   MINMAX(p4, p2);   MINMAX(p6, p4);
			MINMAX(p4, p2);

			dstPtr[j] = p4;
		}

	}
}

#ifdef MCV_MULTI_THREAD
static MVoid thread_cross_median_5x5_rows_P010(MVoid* pParam)
{
	MedianFilter_P010 *Filter = (MedianFilter_P010 *)pParam;
	MInt32 lret = MOK;

	cross_median_5x5_rows_P010(Filter->pSrcImg, Filter->lSrcPitch, Filter->pDstImg, Filter->lDstPitch,
		Filter->lImgWidth, Filter->lImgHeight, Filter->pBufData, Filter->cn, Filter->lTopLine, Filter->lBotLine);
	return;
}
#endif



MRESULT Cross_Median_Filter_5x5_P010(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
	MInt32 width, MInt32 height, MInt32 cn)
{
	MRESULT lret = MOK;
	MUInt16* tmpRowBuf[MED_TASK_NUM] = { 0 };
	MInt32 i;
	for (i = 0; i < MED_TASK_NUM; i++)
	{
		tmpRowBuf[i] = (MUInt16*)MMemAlloc(hMemMgr, (width + 4) * cn * sizeof(MUInt16));
		if (MNull == tmpRowBuf[i])
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}
	}

#ifdef MCV_MULTI_THREAD
	{
		const MInt32 lTaskNum = MED_TASK_NUM;
		MInt32 lTaskSize = 0;
		MInt32 taskID[MED_TASK_NUM] = { 0 };
		MedianFilter_P010 pParams[MED_TASK_NUM] = { 0 };

		lTaskSize = height / lTaskNum;
		lTaskSize = lTaskSize >> 2 << 2;

		for (i = 0; i < lTaskNum; i++)
		{
			pParams[i].lTopLine = lTaskSize*i;
			pParams[i].lBotLine = lTaskSize*(i + 1);
		}
		pParams[lTaskNum - 1].lBotLine = height;

		for (i = 0; i < lTaskNum; i++)
		{
			pParams[i].thread_ID = i;
			pParams[i].lImgWidth = width;
			pParams[i].lImgHeight = height;
			pParams[i].lSrcPitch = src_step >> 1;
			pParams[i].pSrcImg = (MUInt16 *)src;
			pParams[i].lDstPitch = dst_step >> 1;
			pParams[i].pDstImg = (MUInt16 *)dst;
			pParams[i].pBufData = tmpRowBuf[i];
			pParams[i].cn = cn;
		}

		for (i = 0; i < lTaskNum; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, thread_cross_median_5x5_rows_P010, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTaskNum; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}
	}
#else
	cross_median_5x5_rows_P010((MUInt16 *)src, src_step >> 1, (MUInt16 *)dst, dst_step >> 1, width, height, tmpRowBuf[0], cn, 0, height);
#endif
exit:
	for (i = 0; i < MED_TASK_NUM; i++)
	{
		if (tmpRowBuf[i])
		{
			MMemFree(hMemMgr, tmpRowBuf[i]);
			tmpRowBuf[i] = MNull;
		}
	}
	return lret;
}