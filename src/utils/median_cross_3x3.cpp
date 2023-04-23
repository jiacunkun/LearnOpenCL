#include "single_image_enhancement_define.h"
#include "mobilecv.h"

#define PRE_TASK_NUM 4

#if defined(MCV_MULTI_THREAD)
	typedef struct _tag_BOX_MEDIAN_3X3 {
		MInt32          task_ID;
		MByte           *srcBuf;
		MByte           *dstBuf;
		MInt32          srcPitch;
		MInt32          dstPitch;
		MByte           *minArray;
		MByte           *medArray;
		MByte           *maxArray;
		MInt32          lImgHeight;
		MInt32          lImgWidth;
		MInt32          startRow;
		MInt32          endRow;
	}Box_Median_3x3;
#endif

#if defined(MCV_MULTI_THREAD)
	typedef struct _tag_CROSS_MEDIAN_3X3 {
		MInt32          task_ID;
		MByte           *srcBuf;
		MByte           *dstBuf;
		MInt32          srcPitch;
		MInt32          dstPitch;
		MInt32          lImgHeight;
		MInt32          lImgWidth;
		MInt32          startRow;
		MInt32          endRow;
		MInt32			cn;
		MInt32			interval;
		MInt32*			pHistDif;
	}Cross_Median_3x3;
#endif

#if defined(MCV_MULTI_THREAD)
typedef struct _tag_CROSS_MEDIAN_3X3_P010 {
		MInt32          task_ID;
		MUInt16         *srcBuf;
		MUInt16         *dstBuf;
		MInt32          srcPitch;
		MInt32          dstPitch;
		MInt32          lImgHeight;
		MInt32          lImgWidth;
		MInt32          startRow;
		MInt32          endRow;
		MInt32			cn;
	}Cross_Median_3x3_P010;
#endif

#define MINMAX(a,b)			\
{MInt32 t = (a)-(b); t = MAX(t, 0); (b) += t, (a) -= t; }

static MVoid NormalMedian_3x3_Rows(MByte *src, MInt32 src_step, MByte *dst, MInt32 dst_step, MInt32 lImgHeight, MInt32 lImgWidth,
	MByte *minArray, MByte *medArray, MByte *maxArray, MInt32 startRow, MInt32 endRow)
{
	MInt32 x, y, x0, x1, x2;
	MByte p0, p1, p2, p3, p4, p5, p6, p7, p8;

#ifdef	_ARM_NEON_
	uint8x16_t srcdata00, srcdata01, srcdata02;
	uint8x16_t maxminval, minmaxval, medmedval;
	uint8x16_t tmpdata_8x16;
#endif

	for (y = startRow; y < endRow; y++)
	{
		MByte* src0 = src + src_step*(y - 1);
		MByte* src1 = src0 + src_step;
		MByte* src2 = src1 + src_step;
		MByte* dstBuf = dst + y*dst_step;
		if (y == 0)
			src0 = src1;
		else if (y == lImgHeight - 1)
			src2 = src1;

		x = 0;
#ifdef _ARM_NEON_             //计算每一列的最小值，中值，最大值
		for (x = 0; x < lImgWidth - 15; x += 16)
		{
			srcdata00 = vld1q_u8(src0 + x);
			srcdata01 = vld1q_u8(src1 + x);
			srcdata02 = vld1q_u8(src2 + x);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata02);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);

			vst1q_u8(minArray + x, srcdata00);
			vst1q_u8(medArray + x, srcdata01);
			vst1q_u8(maxArray + x, srcdata02);
		}
#endif
		for (; x < lImgWidth; ++x)
		{
			minArray[x] = src0[x]; medArray[x] = src1[x]; maxArray[x] = src2[x];
			MINMAX(minArray[x], medArray[x]); MINMAX(minArray[x], maxArray[x]); MINMAX(medArray[x], maxArray[x]);
		}

		{
			x0 = 0; x1 = 0; x2 = 1;
			//求准确中值
			p0 = minArray[x0]; p1 = minArray[x1]; p2 = minArray[x2];
			p3 = medArray[x0]; p4 = medArray[x1]; p5 = medArray[x2];
			p6 = maxArray[x0]; p7 = maxArray[x1]; p8 = maxArray[x2];
			MINMAX(p0, p2); MINMAX(p1, p2);      //求最小值组里的最大值
			MINMAX(p3, p4); MINMAX(p3, p5); MINMAX(p4, p5);    //求中值组里的中值
			MINMAX(p6, p7); MINMAX(p6, p8);      //求最大值组里的最小值
			MINMAX(p2, p4); MINMAX(p2, p6); MINMAX(p4, p6); //求中值

			dstBuf[x1] = (MByte)p4;
		}

		x = 1;
#ifdef _ARM_NEON_
		for (x = 1; x < lImgWidth - 15; x += 16)
		{
			//求准确中值
			//求最小值数组里的最大值
			srcdata00 = vld1q_u8(minArray + x - 1);
			srcdata01 = vld1q_u8(minArray + x);
			srcdata02 = vld1q_u8(minArray + x + 1);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);
			srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			minmaxval = vaddq_u8(srcdata02, tmpdata_8x16);

			//求中值数组里的中值
			srcdata00 = vld1q_u8(medArray + x - 1);
			srcdata01 = vld1q_u8(medArray + x);
			srcdata02 = vld1q_u8(medArray + x + 1);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata02);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			medmedval = vsubq_u8(srcdata01, tmpdata_8x16);

			//求最大值数组里的最小值
			srcdata00 = vld1q_u8(maxArray + x - 1);
			srcdata01 = vld1q_u8(maxArray + x);
			srcdata02 = vld1q_u8(maxArray + x + 1);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata02);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);
			maxminval = vsubq_u8(srcdata00, tmpdata_8x16);

			//计算中值
			tmpdata_8x16 = vqsubq_u8(minmaxval, medmedval);
			medmedval = vaddq_u8(medmedval, tmpdata_8x16);
			minmaxval = vsubq_u8(minmaxval, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(minmaxval, maxminval);
			maxminval = vaddq_u8(maxminval, tmpdata_8x16);
			minmaxval = vsubq_u8(minmaxval, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(medmedval, maxminval);
			maxminval = vaddq_u8(maxminval, tmpdata_8x16);
			medmedval = vsubq_u8(medmedval, tmpdata_8x16);

			vst1q_u8(dstBuf + x, medmedval);
		}
#endif

		for (; x < lImgWidth; x++)
		{
			if ((lImgWidth - 1) == x)
			{
				x0 = lImgWidth - 2; x1 = lImgWidth - 1; x2 = lImgWidth - 1;
			}
			else
			{
				x0 = x - 1; x1 = x; x2 = x + 1;
			}

			//求准确中值
			p0 = minArray[x0]; p1 = minArray[x1]; p2 = minArray[x2];
			p3 = medArray[x0]; p4 = medArray[x1]; p5 = medArray[x2];
			p6 = maxArray[x0]; p7 = maxArray[x1]; p8 = maxArray[x2];
			MINMAX(p0, p2); MINMAX(p1, p2);      //求最小值组里的最大值
			MINMAX(p3, p4); MINMAX(p3, p5); MINMAX(p4, p5);    //求中值组里的中值
			MINMAX(p6, p7); MINMAX(p6, p8);      //求最大值组里的最小值
			MINMAX(p2, p4); MINMAX(p2, p6); MINMAX(p4, p6);  //求中值

			dstBuf[x] = (MByte)p4;
		}
	}
}

#if defined(MCV_MULTI_THREAD)
static MVoid thread_NormalMedian_3x3_Rows(MVoid* pParam)
{
	Box_Median_3x3 *nm = (Box_Median_3x3*)pParam;

	NormalMedian_3x3_Rows(nm->srcBuf, nm->srcPitch, nm->dstBuf, nm->dstPitch,
		nm->lImgHeight, nm->lImgWidth, nm->minArray, nm->medArray, nm->maxArray, nm->startRow, nm->endRow);
}
#endif

MInt32 Box_Median_Filter_3x3(MHandle hHandle, MHandle mcvParallelMonitor, MByte *srcImg, MByte *dstImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch)
{
	MInt32  x, y, lret = MOK;
	MBool   flag = MFalse;
	MByte   *minArray = MNull, *medArray = MNull, *maxArray = MNull;

	if (dstImg == MNull)
	{
		dstImg = (MByte*)MMemAlloc(hHandle, lHeight*lPitch);
		if (dstImg == MNull)
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}
		flag = MTrue;
	}

	{
		const MInt32 lTaskNum = PRE_TASK_NUM;
		minArray = (MByte*)MMemAlloc(hHandle, lWidth*lTaskNum*sizeof(MByte));
		medArray = (MByte*)MMemAlloc(hHandle, lWidth*lTaskNum*sizeof(MByte));
		maxArray = (MByte*)MMemAlloc(hHandle, lWidth*lTaskNum*sizeof(MByte));
		if (minArray == MNull || medArray == MNull || maxArray == MNull)
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}
#ifdef MCV_MULTI_THREAD
		{
			MInt32 lSize, i;
			MInt32 taskID[PRE_TASK_NUM] = { 0 };
			Box_Median_3x3 pParams[PRE_TASK_NUM] = { 0 };

			lSize = lHeight / lTaskNum;
			lSize = lSize >> 1 << 1;

			pParams[0].startRow = 0;
			pParams[0].endRow = lSize;
			for (i = 1; i < lTaskNum; i++)
			{
				pParams[i].startRow = i*lSize;
				pParams[i].endRow = (i + 1)*lSize;
			}
			pParams[i - 1].endRow = lHeight;

			for (i = 0; i < lTaskNum; i++)
			{
				pParams[i].task_ID = i;
				pParams[i].srcBuf = srcImg;
				pParams[i].srcPitch = lPitch;
				pParams[i].dstBuf = dstImg;
				pParams[i].dstPitch = lPitch;
				pParams[i].lImgHeight = lHeight;
				pParams[i].lImgWidth = lWidth;
				pParams[i].minArray = minArray + i*lWidth;
				pParams[i].medArray = medArray + i*lWidth;
				pParams[i].maxArray = maxArray + i*lWidth;
			}

			for (i = 0; i < lTaskNum; i++)
			{
				taskID[i] = mcvAddTask(mcvParallelMonitor, thread_NormalMedian_3x3_Rows, (MVoid*)&pParams[i]);
			}
			for (i = 0; i < lTaskNum; i++)
			{
				mcvWaitTask(mcvParallelMonitor, taskID[i]);
			}
		}
#else
		NormalMedian_3x3_Rows(srcImg, lPitch, dstImg, lPitch, lHeight, lWidth, minArray, medArray, maxArray, 0, lHeight);
#endif
	}

	if (flag == MTrue)
	{
		MMemCpy(srcImg, dstImg, lHeight*lPitch);
	}

exit:
	if (MTrue == flag && dstImg)
	{
		MMemFree(hHandle, dstImg);
		dstImg = MNull;
	}
	if (minArray)
	{
		MMemFree(hHandle, minArray);
		minArray = MNull;
	}
	if (medArray)
	{
		MMemFree(hHandle, medArray);
		medArray = MNull;
	}
	if (maxArray)
	{
		MMemFree(hHandle, maxArray);
		maxArray = MNull;
	}
	return lret;
}

MInt32 Box_Median_Filter_3x3_ST(MByte *srcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MByte *dstImg)
{
	MInt32  x, y, lret = MOK;
	MByte   *minArray = MNull, *medArray = MNull, *maxArray = MNull;

	{
		minArray = (MByte*)MMemAlloc(MNull, lWidth*sizeof(MByte));
		medArray = (MByte*)MMemAlloc(MNull, lWidth*sizeof(MByte));
		maxArray = (MByte*)MMemAlloc(MNull, lWidth*sizeof(MByte));
		if (minArray == MNull || medArray == MNull || maxArray == MNull)
		{
			lret = MERR_NO_MEMORY;
			goto exit;
		}

		NormalMedian_3x3_Rows(srcImg, lPitch, dstImg, lPitch, lHeight, lWidth, minArray, medArray, maxArray, 0, lHeight);
	}

	MMemCpy(srcImg, dstImg, lHeight*lPitch);

exit:
	if (minArray)
	{
		MMemFree(MNull, minArray);
		minArray = MNull;
	}
	if (medArray)
	{
		MMemFree(MNull, medArray);
		medArray = MNull;
	}
	if (maxArray)
	{
		MMemFree(MNull, maxArray);
		maxArray = MNull;
	}
	return lret;
}


/**********************************************************************************************************/


static MVoid CrossMedian_3x3_Rows(MByte *src, MInt32 src_step, MByte *dst, MInt32 dst_step, MInt32 lImgHeight, MInt32 lImgWidth,
	MInt32 startRow, MInt32 endRow, MInt32 cn)
{
	START_TIME;
	MInt32 x, y, x1, x2;
	MByte p0, p1, p2, p3, p4;

#ifdef _ARM_NEON_
	uint8x16_t srcdata00, srcdata01, srcdata02, srcdata03, srcdata04;
	uint8x16_t tmpdata_8x16;
#endif

	for (y = startRow; y < endRow; y++)
	{
		MByte* src0 = (y == 0 ? src + src_step*(1 - y) : src + src_step*(y - 1));
		MByte* src1 = src + y*src_step;
		MByte* src2 = (y == (lImgHeight - 1) ? src + src_step*(lImgHeight - 2) : src + src_step*(y + 1));
		MByte* dstBuf = dst + y*dst_step;

		//x = 0
		for (x = 0; x < cn; x++)
		{
			p0 = src0[x];    p1 = src1[x];     p2 = src2[x];
			p3 = src1[x + cn];    p4 = src1[x + cn];
			MINMAX(p0, p1); MINMAX(p3, p4); MINMAX(p0, p3);
			MINMAX(p1, p4); MINMAX(p1, p2); MINMAX(p2, p3);
			MINMAX(p1, p2);
			dstBuf[x] = p2;
		}

		x = cn;
#ifdef _ARM_NEON_
		for (; x < (lImgWidth - 1) * cn - 15; x += 16)
		{
			srcdata00 = vld1q_u8(src0 + x);
			srcdata01 = vld1q_u8(src1 + x);
			srcdata02 = vld1q_u8(src2 + x);
			srcdata03 = vld1q_u8(src1 + x - cn);
			srcdata04 = vld1q_u8(src1 + x + cn);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata01);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);
			srcdata01 = vaddq_u8(srcdata01, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata03, srcdata04);
			srcdata03 = vsubq_u8(srcdata03, tmpdata_8x16);
			srcdata04 = vaddq_u8(srcdata04, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata00, srcdata03);
			srcdata00 = vsubq_u8(srcdata00, tmpdata_8x16);
			srcdata03 = vaddq_u8(srcdata03, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata04);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			srcdata04 = vaddq_u8(srcdata04, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata02, srcdata03);
			srcdata02 = vsubq_u8(srcdata02, tmpdata_8x16);
			srcdata03 = vaddq_u8(srcdata03, tmpdata_8x16);

			tmpdata_8x16 = vqsubq_u8(srcdata01, srcdata02);
			srcdata01 = vsubq_u8(srcdata01, tmpdata_8x16);
			srcdata02 = vaddq_u8(srcdata02, tmpdata_8x16);

			vst1q_u8(dstBuf + x, srcdata02);
		}
#endif
		for (; x < (lImgWidth - 1) * cn; x++)
		{
			p0 = src0[x];    p1 = src1[x];     p2 = src2[x];
			p3 = src1[x - cn];  p4 = src1[x + cn];

			MINMAX(p0, p1); MINMAX(p3, p4); MINMAX(p0, p3);
			MINMAX(p1, p4); MINMAX(p1, p2); MINMAX(p2, p3);
			MINMAX(p1, p2);

			dstBuf[x] = p2;
		}

		for (; x < lImgWidth * cn; x++)
		{
			p0 = src0[x];    p1 = src1[x];     p2 = src2[x];
			p3 = src1[x - cn];    p4 = src1[x - cn];
			MINMAX(p0, p1); MINMAX(p3, p4); MINMAX(p0, p3);
			MINMAX(p1, p4); MINMAX(p1, p2); MINMAX(p2, p3);
			MINMAX(p1, p2);
			dstBuf[x] = p2;
		}
	}

	END_TIME;
}



#if defined(MCV_MULTI_THREAD)
static MVoid thread_CrossMedian_3x3_Rows(MVoid* pParam)
{
	Cross_Median_3x3 *nm = (Cross_Median_3x3*)pParam;

	CrossMedian_3x3_Rows(nm->srcBuf, nm->srcPitch, nm->dstBuf, nm->dstPitch,
		nm->lImgHeight, nm->lImgWidth, nm->startRow, nm->endRow, nm->cn);
}
#endif

//五个数求中值比较7次
MInt32 Cross_Median_Filter_3x3(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcImg, MInt32 lPitchSrc, MByte *dstImg, MInt32 lPitchDst, MInt32 lWidth, MInt32 lHeight, MInt32 cn)
{
	START_TIME;
	MInt32  x, y, lret = MOK;

	{
		const MInt32 lTaskNum = PRE_TASK_NUM;
#ifdef MCV_MULTI_THREAD
		if (mcvParallelMonitor != MNull)
		{
			MInt32 lSize, i;
			MInt32 taskID[PRE_TASK_NUM] = { 0 };
			Cross_Median_3x3 pParams[PRE_TASK_NUM] = { 0 };

			lSize = lHeight / lTaskNum;
			lSize = lSize >> 1 << 1;

			pParams[0].startRow = 0;
			pParams[0].endRow = lSize;
			for (i = 1; i < lTaskNum; i++)
			{
				pParams[i].startRow = i * lSize;
				pParams[i].endRow = (i + 1) * lSize;
			}
			pParams[i - 1].endRow = lHeight;

			for (i = 0; i < lTaskNum; i++)
			{
				pParams[i].task_ID = i;
				pParams[i].srcBuf = srcImg;
				pParams[i].srcPitch = lPitchSrc;
				pParams[i].dstBuf = dstImg;
				pParams[i].dstPitch = lPitchDst;
				pParams[i].lImgHeight = lHeight;
				pParams[i].lImgWidth = lWidth;
				pParams[i].cn = cn;
			}

			for (i = 0; i < lTaskNum; i++)
			{
				taskID[i] = mcvAddTask(mcvParallelMonitor, thread_CrossMedian_3x3_Rows, (MVoid*)&pParams[i]);
			}
			for (i = 0; i < lTaskNum; i++)
			{
				mcvWaitTask(mcvParallelMonitor, taskID[i]);
			}
		}
		else
		{
			CrossMedian_3x3_Rows(srcImg, lPitchSrc, dstImg, lPitchDst, lHeight, lWidth, 0, lHeight, cn);
		}
#else
		CrossMedian_3x3_Rows(srcImg, lPitchSrc, dstImg, lPitchDst, lHeight, lWidth, 0, lHeight, cn);
#endif
	}

exit:
	END_TIME;
	return lret;
}


static MVoid CrossMedian_3x3_CountDif_Rows(MByte* src, MInt32 src_step, MInt32 lImgHeight, MInt32 lImgWidth,
	MInt32 cn, MInt32 interval, MInt32 *pHistDif, MInt32 startRow, MInt32 endRow)
{
	MInt32 x, y, x1, x2;
	MByte p0, p1, p2, p3, p4;


	for (y = startRow; y < endRow; y += interval)
	{
		MByte* src0 = (y == 0 ? src + src_step * (1 - y) : src + src_step * (y - 1));
		MByte* src1 = src + y * src_step;
		MByte* src2 = (y == (lImgHeight - 1) ? src + src_step * (lImgHeight - 2) : src + src_step * (y + 1));

		x = cn;

		for (; x < (lImgWidth - 1) * cn; x += interval)
		{
			p0 = src0[x];    p1 = src1[x];     p2 = src2[x];
			p3 = src1[x - cn];  p4 = src1[x + cn];
			MInt32 tmpSrc = p1;

			MINMAX(p0, p1); MINMAX(p3, p4); MINMAX(p0, p3);
			MINMAX(p1, p4); MINMAX(p1, p2); MINMAX(p2, p3);
			MINMAX(p1, p2);

			MInt32 dif = ABS(p2 - tmpSrc);
			pHistDif[dif]++;
		}

	}
}



#if defined(MCV_MULTI_THREAD)
static MVoid thread_CrossMedian_3x3_CountDif(MVoid* pParam)
{
	Cross_Median_3x3* nm = (Cross_Median_3x3*)pParam;

	CrossMedian_3x3_CountDif_Rows(nm->srcBuf, nm->srcPitch, nm->lImgHeight, nm->lImgWidth, 
		nm->cn, nm->interval, nm->pHistDif, nm->startRow, nm->endRow);
}
#endif

//Cross中值前后的差的数量存在pHistDif里
MInt32 Cross_Median_Filter_3x3_CountDif(MHandle mcvParallelMonitor, MByte* srcImg, MInt32 lPitchSrc,
	MInt32 lWidth, MInt32 lHeight, MInt32 cn, MInt32 * pHistDif, MInt32 *pixNum)
{
	MInt32  x, y, lret = MOK;
	MInt32 interval = 8;
	MMemSet(pHistDif, 0, 256 * sizeof(MInt32));
	*pixNum = 0;
	{
		const MInt32 lTaskNum = PRE_TASK_NUM;
#ifdef MCV_MULTI_THREAD
		{
			MInt32 lSize, i;
			MInt32 taskID[PRE_TASK_NUM] = { 0 };
			Cross_Median_3x3 pParams[PRE_TASK_NUM] = { 0 };
			MInt32 bufHistDif[PRE_TASK_NUM * 256] = { 0 };

			lSize = lHeight / lTaskNum;
			lSize = lSize / interval * interval;

			pParams[0].startRow = 0;
			pParams[0].endRow = lSize;
			for (i = 1; i < lTaskNum; i++)
			{
				pParams[i].startRow = i * lSize;
				pParams[i].endRow = (i + 1) * lSize;
			}
			pParams[i - 1].endRow = lHeight;

			for (i = 0; i < lTaskNum; i++)
			{
				pParams[i].task_ID = i;
				pParams[i].srcBuf = srcImg;
				pParams[i].srcPitch = lPitchSrc;
				pParams[i].lImgHeight = lHeight;
				pParams[i].lImgWidth = lWidth;
				pParams[i].cn = cn;
				pParams[i].interval = interval;
				pParams[i].pHistDif = bufHistDif + i * 256;
			}

			for (i = 0; i < lTaskNum; i++)
			{
				taskID[i] = mcvAddTask(mcvParallelMonitor, thread_CrossMedian_3x3_CountDif, (MVoid*)& pParams[i]);
			}
			for (i = 0; i < lTaskNum; i++)
			{
				mcvWaitTask(mcvParallelMonitor, taskID[i]);
			}

							
			for (i = 0; i < PRE_TASK_NUM; i++)
			{
				for (MInt32 x = 0; x < 256; x++)
				{
					pHistDif[x] += bufHistDif[i * 256 + x];
					*pixNum += bufHistDif[i * 256 + x];
				}
			}
			
		}
#else
		CrossMedian_3x3_CountDif_Rows(srcImg, lPitchSrc, lHeight, lWidth, 0, cn, interval, pHistDif, lHeight);
#endif
	}

exit:
	return lret;
}

static MVoid CrossMedian_3x3_Rows_P010(MUInt16 *src, MInt32 src_step, MUInt16 *dst, MInt32 dst_step, MInt32 lImgHeight, MInt32 lImgWidth,
	MInt32 startRow, MInt32 endRow, MInt32 cn)
{
	MInt32 x, y, x1, x2;
	MUInt16 p0, p1, p2, p3, p4;

#ifdef _ARM_NEON_
	uint16x8_t srcdata00, srcdata01, srcdata02, srcdata03, srcdata04;
	uint16x8_t tmpdata_16x8;
#endif

	for (y = startRow; y < endRow; y++)
	{
		MUInt16* src0 = (y == 0 ? src + src_step*(1 - y) : src + src_step*(y - 1));
		MUInt16* src1 = src + y * src_step;
		MUInt16* src2 = (y == (lImgHeight - 1) ? src + src_step*(lImgHeight - 2) : src + src_step*(y + 1));
		MUInt16* dstBuf = dst + y * dst_step;

		//x = 0
		for (x = 0; x < cn; x++)
		{
			p0 = src0[x];    p1 = src1[x];     p2 = src2[x];
			p3 = src1[x + cn];    p4 = src1[x + cn];
			MINMAX(p0, p1); MINMAX(p3, p4); MINMAX(p0, p3);
			MINMAX(p1, p4); MINMAX(p1, p2); MINMAX(p2, p3);
			MINMAX(p1, p2);
			dstBuf[x] = p2;
		}

		x = cn;

#ifdef _ARM_NEON_
		for (; x < (lImgWidth - 1) * cn - 7; x += 8)
		{
			srcdata00 = vld1q_u16(src0 + x);
			srcdata01 = vld1q_u16(src1 + x);
			srcdata02 = vld1q_u16(src2 + x);
			srcdata03 = vld1q_u16(src1 + x - cn);
			srcdata04 = vld1q_u16(src1 + x + cn);

			tmpdata_16x8 = vqsubq_u16(srcdata00, srcdata01);
			srcdata00 = vsubq_u16(srcdata00, tmpdata_16x8);
			srcdata01 = vaddq_u16(srcdata01, tmpdata_16x8);

			tmpdata_16x8 = vqsubq_u16(srcdata03, srcdata04);
			srcdata03 = vsubq_u16(srcdata03, tmpdata_16x8);
			srcdata04 = vaddq_u16(srcdata04, tmpdata_16x8);

			tmpdata_16x8 = vqsubq_u16(srcdata00, srcdata03);
			srcdata00 = vsubq_u16(srcdata00, tmpdata_16x8);
			srcdata03 = vaddq_u16(srcdata03, tmpdata_16x8);

			tmpdata_16x8 = vqsubq_u16(srcdata01, srcdata04);
			srcdata01 = vsubq_u16(srcdata01, tmpdata_16x8);
			srcdata04 = vaddq_u16(srcdata04, tmpdata_16x8);

			tmpdata_16x8 = vqsubq_u16(srcdata01, srcdata02);
			srcdata01 = vsubq_u16(srcdata01, tmpdata_16x8);
			srcdata02 = vaddq_u16(srcdata02, tmpdata_16x8);

			tmpdata_16x8 = vqsubq_u16(srcdata02, srcdata03);
			srcdata02 = vsubq_u16(srcdata02, tmpdata_16x8);
			srcdata03 = vaddq_u16(srcdata03, tmpdata_16x8);

			tmpdata_16x8 = vqsubq_u16(srcdata01, srcdata02);
			srcdata01 = vsubq_u16(srcdata01, tmpdata_16x8);
			srcdata02 = vaddq_u16(srcdata02, tmpdata_16x8);

			vst1q_u16(dstBuf + x, srcdata02);
		}
#endif

		for (; x < (lImgWidth - 1) * cn; x++)
		{
			p0 = src0[x];    p1 = src1[x];     p2 = src2[x];
			p3 = src1[x - cn];  p4 = src1[x + cn];

			MINMAX(p0, p1); MINMAX(p3, p4); MINMAX(p0, p3);
			MINMAX(p1, p4); MINMAX(p1, p2); MINMAX(p2, p3);
			MINMAX(p1, p2);

			dstBuf[x] = p2;
		}

		for (; x < lImgWidth * cn; x++)
		{
			p0 = src0[x];    p1 = src1[x];     p2 = src2[x];
			p3 = src1[x - cn];    p4 = src1[x - cn];
			MINMAX(p0, p1); MINMAX(p3, p4); MINMAX(p0, p3);
			MINMAX(p1, p4); MINMAX(p1, p2); MINMAX(p2, p3);
			MINMAX(p1, p2);
			dstBuf[x] = p2;
		}
	}
}

#if defined(MCV_MULTI_THREAD)
MVoid thread_CrossMedian_3x3_Rows_P010(MVoid* pParam)
{
	Cross_Median_3x3_P010 *nm = (Cross_Median_3x3_P010 *)pParam;

	CrossMedian_3x3_Rows_P010(nm->srcBuf, nm->srcPitch, nm->dstBuf, nm->dstPitch,
		nm->lImgHeight, nm->lImgWidth, nm->startRow, nm->endRow, nm->cn);
}
#endif


//五个数求中值比较7次
MInt32 Cross_Median_Filter_3x3_P010(MHandle mcvParallelMonitor, MByte *srcImg, MInt32 lPitchSrc, MByte *dstImg, MInt32 lPitchDst, MInt32 lWidth, MInt32 lHeight, MInt32 cn)
{
	MInt32  x, y, lret = MOK;

	{
		const MInt32 lTaskNum = PRE_TASK_NUM;
#ifdef MCV_MULTI_THREAD
		{
			MInt32 lSize, i;
			MInt32 taskID[PRE_TASK_NUM] = { 0 };
			Cross_Median_3x3_P010 pParams[PRE_TASK_NUM] = { 0 };

			lSize = lHeight / lTaskNum;
			lSize = lSize >> 1 << 1;

			pParams[0].startRow = 0;
			pParams[0].endRow = lSize;
			for (i = 1; i < lTaskNum; i++)
			{
				pParams[i].startRow = i*lSize;
				pParams[i].endRow = (i + 1)*lSize;
			}
			pParams[i - 1].endRow = lHeight;

			for (i = 0; i < lTaskNum; i++)
			{
				pParams[i].task_ID = i;
				pParams[i].srcBuf = (MUInt16 *)srcImg;
				pParams[i].srcPitch = lPitchSrc >> 1;
				pParams[i].dstBuf = (MUInt16 *)dstImg;
				pParams[i].dstPitch = lPitchDst >> 1;
				pParams[i].lImgHeight = lHeight;
				pParams[i].lImgWidth = lWidth;
				pParams[i].cn = cn;
			}

			for (i = 0; i < lTaskNum; i++)
			{
				taskID[i] = mcvAddTask(mcvParallelMonitor, thread_CrossMedian_3x3_Rows_P010, (MVoid*)&pParams[i]);
			}
			for (i = 0; i < lTaskNum; i++)
			{
				mcvWaitTask(mcvParallelMonitor, taskID[i]);
			}
		}
#else
		CrossMedian_3x3_Rows_P010((MUInt16 *)srcImg, lPitchSrc >> 1, (MUInt16 *)dstImg, lPitchDst >> 1, lHeight, lWidth, 0, lHeight, cn);
#endif
	}

exit:
	return lret;
}
