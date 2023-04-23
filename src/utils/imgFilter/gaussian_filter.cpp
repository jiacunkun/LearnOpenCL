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

#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "mthread.h"
#include "math.h"
#include "mobilecv.h"
//#include <NEON_2_SSE.h>
#include "imagebase.h"
#include "gaussian_filter.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
typedef struct _tag_GaussPyrDown
{
	MInt32 taskID;
	MInt32 start;
	MInt32 end;
	MByte *srcdata;
	MByte *dstdata;
	MWord *rowBuf;
	MInt32 srcW;
	MInt32 srcH;
	MInt32 srcPitch;
	MInt32 dstW;
	MInt32 dstH;
	MInt32 dstPitch;
}GaussPyrDownData;

/******************************************************************************************************************/

static MVoid LocalPyrDown2(MByte *srcPtr, MByte *dstPtr, MWord *rowBuf, MInt32 sw, MInt32 sh, MInt32 src_step,
	MInt32 dw, MInt32 dh, MInt32 dst_step, MInt32 start, MInt32 end)
{

	MWord* buf = rowBuf + 1;
	MInt32 k, x, y, sy0 = -(3 >> 1), sy = sy0, width0 = MIN(((sw - (3 >> 1) - 1) >> 1) + 1, dw);
	MByte* src00, *src01, *src02;

#ifdef __ARM_NEON__
	uint8x8_t vConst2_8x8;
	uint16x8_t vConst2_16x8;
	vConst2_8x8 = vdup_n_u8(2);
	vConst2_16x8 = vdupq_n_u16(2);
#endif

	for (y = start; y < end; y++)
	{
		MByte* tmpdst = (MByte*)(dstPtr + dst_step*y);
		sy = y * 2;
		src00 = (sy - 1 >= 0) ? srcPtr + src_step*(sy - 1) : srcPtr + src_step;
		src01 = srcPtr + src_step*sy;
		src02 = (sy + 1 <= sh - 1) ? srcPtr + src_step*(sy + 1) : srcPtr + src_step*(sh - 2);
		x = 0;
#ifdef __ARM_NEON__
		for (; x < sw - 15; x += 16)
		{
			uint8x16_t vSrc00_8x16, vSrc01_8x16, vSrc02_8x16;
			uint16x8_t vSum00, vSum01;

			vSrc00_8x16 = vld1q_u8(src00 + x);
			vSrc02_8x16 = vld1q_u8(src02 + x);
			vSrc01_8x16 = vld1q_u8(src01 + x);

			vSum00 = vaddl_u8(vget_low_u8(vSrc00_8x16), vget_low_u8(vSrc02_8x16));
			vSum01 = vaddl_u8(vget_high_u8(vSrc00_8x16), vget_high_u8(vSrc02_8x16));
			vSum00 = vmlal_u8(vSum00, vget_low_u8(vSrc01_8x16), vConst2_8x8);
			vSum01 = vmlal_u8(vSum01, vget_high_u8(vSrc01_8x16), vConst2_8x8);
			vst1q_u16(buf + x, vSum00);
			vst1q_u16(buf + x + 8, vSum01);
		}
		for (; x < sw - 7; x += 8)
		{
			uint8x8_t vSrc00_8x8, vSrc01_8x8, vSrc02_8x8;
			uint16x8_t vSum00;

			vSrc00_8x8 = vld1_u8(src00 + x);
			vSrc02_8x8 = vld1_u8(src02 + x);
			vSrc01_8x8 = vld1_u8(src01 + x);

			vSum00 = vaddl_u8(vSrc00_8x8, vSrc02_8x8);
			vSum00 = vmlal_u8(vSum00, vSrc01_8x8, vConst2_8x8);
			vst1q_u16(buf + x, vSum00);
		}
#endif
		for (; x < sw; x++)
		{
			buf[x] = src01[x] * 2 + (src00[x] + src02[x]);
		}

		buf[-1] = buf[1];
		buf[sw] = buf[sw - 2];
		x = 0;
		k = 0;
#ifdef __ARM_NEON__
		for (x = 0, k = 0; x < dw - 10; x += 8, k += 16)
		{
			uint16x8x2_t src00_16x8x2 = vld2q_u16(buf + k - 1);
			uint16x8x2_t src01_16x8x2 = vld2q_u16(buf + k + 1);
			uint16x8_t sum00_16x8;
			sum00_16x8 = vaddq_u16(src00_16x8x2.val[0], src01_16x8x2.val[0]);
			sum00_16x8 = vmlaq_u16(sum00_16x8, src00_16x8x2.val[1], vConst2_16x8);
			vst1_u8(tmpdst + x, vrshrn_n_u16(sum00_16x8, 4));
		}
#endif
		for (; x < dw; x++, k += 2)//x = 0, k = 0
		{
			tmpdst[x] = (buf[k] * 2 + (buf[k - 1] + buf[k + 1]) + 8) >> 4;
		}
	}
}

#ifdef MCV_MULTI_THREAD
static MVoid threadLocalPyrDown2(MVoid* pParam)
{
	GaussPyrDownData *gp = (GaussPyrDownData*)pParam;
	LocalPyrDown2(gp->srcdata, gp->dstdata, gp->rowBuf, gp->srcW, gp->srcH, gp->srcPitch, gp->dstW, gp->dstH, gp->dstPitch, gp->start, gp->end);
}
#endif

MInt32 GaussPyrDown2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte *dstdata, MInt32 srcW, MInt32 srcH,
	MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch)
{
	MInt32 i, lret = MOK;
	const MInt32 lTask_Num = 8;
	MWord *rowBuf[lTask_Num] = { MNull };
	MWord *tmpBuf = (MWord*)MMemAlloc(hMemMgr, (srcW + 3)*lTask_Num*sizeof(MWord));
	if (tmpBuf == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	for (i = 0; i < lTask_Num; i++)
	{
		rowBuf[i] = tmpBuf + i*(srcW + 3);
	}

#ifdef MCV_MULTI_THREAD
	{
		MFloat  scale = 1.0f / lTask_Num;
		MInt32 taskID[lTask_Num] = { 0 };
		GaussPyrDownData pParams[lTask_Num] = { 0 };

		for (i = 0; i < lTask_Num; ++i)
		{
			pParams[i].taskID = i;
			pParams[i].srcdata = srcdata;
			pParams[i].dstdata = dstdata;
			pParams[i].rowBuf = rowBuf[i];
			pParams[i].srcW = srcW;
			pParams[i].srcH = srcH;
			pParams[i].srcPitch = srcPitch;
			pParams[i].dstW = dstW;
			pParams[i].dstH = dstH;
			pParams[i].dstPitch = dstPitch;
			pParams[i].start = (MInt32)(dstH*scale*i);
			pParams[i].end = (MInt32)(dstH*scale*(i + 1));
		}
		pParams[lTask_Num - 1].end = dstH;

		for (i = 0; i < lTask_Num; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, threadLocalPyrDown2, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTask_Num; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
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

/*******************************************************************************************************************/

static MVoid LocalPyrDown2_C2(MByte *srcPtr, MByte *dstPtr, MWord *rowBuf, MInt32 sw, MInt32 sh, MInt32 src_step, MInt32 dw, MInt32 dh, MInt32 dst_step, MInt32 start, MInt32 end)
{

	MWord* buf = rowBuf + 2;
	MInt32 k, x, y, sy;
	MByte* src00, *src01, *src02;

#ifdef __ARM_NEON__
	uint8x8_t vConst2_8x8;
	uint16x8_t vConst2_16x8;
	uint8x8x2_t resdata;
	uint16x8x4_t src00_16x8x4;
	uint16x8x4_t src01_16x8x4;
	uint16x8_t sum00_16x8;
	vConst2_8x8 = vdup_n_u8(2);
	vConst2_16x8 = vdupq_n_u16(2);
#endif

	for (y = start; y < end; y++)
	{
		MByte* tmpdst = (MByte*)(dstPtr + dst_step*y);
		sy = y * 2;
		src00 = (sy - 1 >= 0) ? srcPtr + src_step*(sy - 1) : srcPtr + src_step;
		src01 = srcPtr + src_step*sy;
		src02 = (sy + 1 <= sh - 1) ? srcPtr + src_step*(sy + 1) : srcPtr + src_step*(sh - 2);
		x = 0;
#ifdef __ARM_NEON__
		for (; x < sw - 15; x += 16)
		{
			uint8x16_t vSrc00_8x16, vSrc01_8x16, vSrc02_8x16;
			uint16x8_t vSum00, vSum01;

			vSrc00_8x16 = vld1q_u8(src00 + x);
			vSrc02_8x16 = vld1q_u8(src02 + x);
			vSrc01_8x16 = vld1q_u8(src01 + x);

			vSum00 = vaddl_u8(vget_low_u8(vSrc00_8x16), vget_low_u8(vSrc02_8x16));
			vSum01 = vaddl_u8(vget_high_u8(vSrc00_8x16), vget_high_u8(vSrc02_8x16));
			vSum00 = vmlal_u8(vSum00, vget_low_u8(vSrc01_8x16), vConst2_8x8);
			vSum01 = vmlal_u8(vSum01, vget_high_u8(vSrc01_8x16), vConst2_8x8);
			vst1q_u16(buf + x, vSum00);
			vst1q_u16(buf + x + 8, vSum01);
		}
		for (; x < sw - 7; x += 8)
		{
			uint8x8_t vSrc00_8x8, vSrc01_8x8, vSrc02_8x8;
			uint16x8_t vSum00;

			vSrc00_8x8 = vld1_u8(src00 + x);
			vSrc02_8x8 = vld1_u8(src02 + x);
			vSrc01_8x8 = vld1_u8(src01 + x);

			vSum00 = vaddl_u8(vSrc00_8x8, vSrc02_8x8);
			vSum00 = vmlal_u8(vSum00, vSrc01_8x8, vConst2_8x8);
			vst1q_u16(buf + x, vSum00);
		}
#endif
		for (; x < sw; x++)
		{
			buf[x] = src01[x] * 2 + (src00[x] + src02[x]);
		}

		buf[-1] = buf[1];
		buf[-2] = buf[0];
		buf[sw] = buf[sw - 2];
		buf[sw + 1] = buf[sw - 1];
		x = 0;
		k = 0;
#ifdef __ARM_NEON__
		for (x = 0, k = 0; x < dw - 20; x += 16, k += 32)
		{
			src00_16x8x4 = vld4q_u16(buf + k - 2);
			src01_16x8x4 = vld4q_u16(buf + k + 2);
			sum00_16x8 = vaddq_u16(src00_16x8x4.val[0], src01_16x8x4.val[0]);
			sum00_16x8 = vmlaq_u16(sum00_16x8, src00_16x8x4.val[2], vConst2_16x8);
			resdata.val[0] = vrshrn_n_u16(sum00_16x8, 4);

			sum00_16x8 = vaddq_u16(src00_16x8x4.val[1], src01_16x8x4.val[1]);
			sum00_16x8 = vmlaq_u16(sum00_16x8, src00_16x8x4.val[3], vConst2_16x8);
			resdata.val[1] = vrshrn_n_u16(sum00_16x8, 4);
			vst2_u8(tmpdst + x, resdata);
		}
#endif
		for (; x < dw; x += 2, k += 4)//x = 0, k = 0
		{
			tmpdst[x] = (buf[k] * 2 + (buf[k - 2] + buf[k + 2]) + 8) >> 4;
			tmpdst[x + 1] = (buf[k + 1] * 2 + (buf[k - 1] + buf[k + 3]) + 8) >> 4;
		}
	}
}

#ifdef MCV_MULTI_THREAD
static MVoid threadLocalPyrDown2_C2(MVoid* pParam)
{
	GaussPyrDownData *gp = (GaussPyrDownData*)pParam;
	LocalPyrDown2_C2(gp->srcdata, gp->dstdata, gp->rowBuf, gp->srcW, gp->srcH, gp->srcPitch, gp->dstW, gp->dstH, gp->dstPitch, gp->start, gp->end);
}
#endif

MInt32 GaussPyrDown2_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte *dstdata, MInt32 srcW, MInt32 srcH,
	MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch)
{
	MInt32 i, lret = MOK;
	const MInt32 lTask_Num = 8;
	MWord *rowBuf[lTask_Num] = { MNull };
	MWord *tmpBuf = (MWord*)MMemAlloc(hMemMgr, (srcW + 6)*lTask_Num*sizeof(MWord));
	if (tmpBuf == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	for (i = 0; i < lTask_Num; i++)
	{
		rowBuf[i] = tmpBuf + i*(srcW + 6);
	}

#ifdef MCV_MULTI_THREAD
	{
		MFloat  scale = 1.0f / lTask_Num;
		MInt32 taskID[lTask_Num] = { 0 };
		GaussPyrDownData pParams[lTask_Num] = { 0 };

		for (i = 0; i < lTask_Num; ++i)
		{
			pParams[i].taskID = i;
			pParams[i].srcdata = srcdata;
			pParams[i].dstdata = dstdata;
			pParams[i].rowBuf = rowBuf[i];
			pParams[i].srcW = srcW;
			pParams[i].srcH = srcH;
			pParams[i].srcPitch = srcPitch;
			pParams[i].dstW = dstW;
			pParams[i].dstH = dstH;
			pParams[i].dstPitch = dstPitch;
			pParams[i].start = (MInt32)(dstH*scale*i);
			pParams[i].end = (MInt32)(dstH*scale*(i + 1));
		}
		pParams[lTask_Num - 1].end = dstH;

		for (i = 0; i < lTask_Num; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, threadLocalPyrDown2_C2, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTask_Num; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
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

/*******************************************************************************************************************/

static MVoid LocalPyrDown4(MByte *srcPtr, MByte *dstPtr, MWord *rowBuf, MInt32 sw, MInt32 sh, MInt32 src_step, 
	MInt32 dw, MInt32 dh, MInt32 dst_step, MInt32 start, MInt32 end)
{
	MWord* buf = rowBuf + 2;
	MInt32 k, x, y, sy0 = -(5 >> 1), sy = sy0;
	MByte *src00, *src01, *src02, *src03, *src04;

#ifdef __ARM_NEON__
	uint8x8_t vConst6_8x8;
	uint16x8_t vConst4, vConst6_16x8;
	vConst4 = vdupq_n_u16(4);
	vConst6_8x8 = vdup_n_u8(6);
	vConst6_16x8 = vdupq_n_u16(6);
#endif

	for (y = start; y < end; y++)
	{
		MByte *tmpdst = (MByte*)(dstPtr + dst_step*y);
		sy = 4 * y;
		if (0 == y)
		{
			src00 = srcPtr + 2 * src_step;
			src01 = srcPtr + src_step;
		}
		else
		{
			//如下写法在某几张测试图上访问第0行数据会crash，具体原因不详
			src00 = ((sy - 2) >= 0) ? (srcPtr + src_step*(sy - 2)) : (srcPtr + src_step*(2 - sy));
			src01 = ((sy - 1) >= 0) ? (srcPtr + src_step*(sy - 1)) : (srcPtr + src_step*(1 - sy));
		}
		src02 = srcPtr + src_step* sy;
		src03 = (sy + 1) <= (sh - 1) ? (srcPtr + src_step*(sy + 1)) : (srcPtr + src_step*(2 * sh - 2 - sy - 1));
		src04 = (sy + 2) <= (sh - 1) ? (srcPtr + src_step*(sy + 2)) : (srcPtr + src_step*(2 * sh - 2 - sy - 2));
		
		x = 0;
#ifdef __ARM_NEON__
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

#ifdef __ARM_NEON__
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
static MVoid threadLocalPyrDown4(MVoid* pParam)
{
	GaussPyrDownData *pyrdown4_D = (GaussPyrDownData*)pParam;
	LocalPyrDown4(pyrdown4_D->srcdata, pyrdown4_D->dstdata, pyrdown4_D->rowBuf, pyrdown4_D->srcW, pyrdown4_D->srcH, pyrdown4_D->srcPitch,
		pyrdown4_D->dstW, pyrdown4_D->dstH, pyrdown4_D->dstPitch, pyrdown4_D->start, pyrdown4_D->end);
}
#endif

MInt32 GaussPyrDown4(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte *dstdata, MInt32 srcW, MInt32 srcH,
	MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch)
{
	MInt32 i, lret = MOK;
	const MInt32 lTask_Num = 8;
	MWord *rowBuf[lTask_Num] = { MNull };
	MWord *tmpBuf = (MWord*)MMemAlloc(hMemMgr, (srcW + 5)*lTask_Num*sizeof(MWord));
	if (tmpBuf == MNull)
	{
		lret = MERR_NO_MEMORY;
		goto exit;
	}

	for (i = 0; i < lTask_Num; i++)
	{
		rowBuf[i] = tmpBuf + i*(srcW + 5);
	}

#ifdef MCV_MULTI_THREAD
	{
		MFloat  scale = 1.0f / lTask_Num;
		MInt32 taskID[lTask_Num] = { 0 };
		GaussPyrDownData pParams[lTask_Num] = { 0 };

		for (i = 0; i < lTask_Num; ++i)
		{
			pParams[i].taskID = i;
			pParams[i].srcdata = srcdata;
			pParams[i].dstdata = dstdata;
			pParams[i].rowBuf = rowBuf[i];
			pParams[i].srcW = srcW;
			pParams[i].srcH = srcH;
			pParams[i].srcPitch = srcPitch;
			pParams[i].dstW = dstW;
			pParams[i].dstH = dstH;
			pParams[i].dstPitch = dstPitch;
			pParams[i].start = (MInt32)(dstH*scale*i);
			pParams[i].end = (MInt32)(dstH*scale*(i + 1));
		}
		pParams[lTask_Num - 1].end = dstH;

		for (i = 0; i < lTask_Num; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, threadLocalPyrDown4, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTask_Num; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
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

#if defined(MCV_MULTI_THREAD)
typedef struct _tag_GAU_BLUR_3X3 {
	MInt32          task_ID;
	MUInt8* src;
	MUInt8* dst;
	MInt32          srcPitch;
	MInt32          dstPitch;
	MInt32          height;
	MInt32          width;
	MInt32          startRow;
	MInt32          endRow;
	MUInt16* pBuf;
	MInt32			cn;
}Gau_Blur_3x3;
#endif

MRESULT GaussianBlur3x3_Rows_cn(MUInt8* src, MInt32 width, MInt32 height, MInt32 srcPitch,
	MUInt8* dst, MInt32 dstPitch, MUInt16* pBuf, MInt32 cn, MInt32 startRow, MInt32 endRow)
{
	MRESULT res = MOK;
	MInt32 x, y, i;
	MUInt16* pBuf1, * pTmp;
	MUInt8* ps0, * ps1, * ps2;

	if (!src || !dst || width < 3 || height < 3)
		return MERR_INVALID_PARAM;

	pBuf1 = pBuf + cn;
	dst += startRow * dstPitch;

	for (y = startRow; y < endRow; y++, dst += dstPitch)
	{
		ps1 = src + y * srcPitch;
		ps0 = ps1 - srcPitch;
		ps2 = ps1 + srcPitch;

		// do vertical convolution
		if (y == 0)
		{
			ps0 = ps2;
		}
		else if (y == height - 1)
		{
			ps2 = ps0;
		}

		x = 0;
#ifdef __ARM_NEON__
		for (; x < width * cn - 8; x += 8)
		{
			uint8x8_t vA, vB, vC;
			uint16x8_t vB16;

			vA = vld1_u8(ps0 + x);
			vB = vld1_u8(ps1 + x);
			vC = vld1_u8(ps2 + x);

			vB16 = vaddq_u16(vshll_n_u8(vB, 1), vaddl_u8(vA, vC));

			vst1q_u16(pBuf1 + x, vB16);
		}
#endif

		for (; x < width * cn; x++)
		{
			MInt32 s0 = ((ps0[x] + ps2[x])) + (ps1[x] << 1);
			pBuf1[x] = s0;
		}
		//printf(::)
		// make border
		for (i = 0; i < cn; i++)
		{
			pBuf1[-cn + i] = pBuf1[cn + i];
			pBuf1[width * cn + i] = pBuf1[(width - 2) * cn + i];
		}

		// do horizontal convolution
		pTmp = pBuf1;
		x = 0;

#ifdef __ARM_NEON__
		for (x; x + cn < width * cn - 8; x += 8, pTmp += 8)
		{
			uint16x8_t vA, vB, vC;

			vA = vld1q_u16(pTmp - cn);
			vB = vld1q_u16(pTmp);
			vC = vld1q_u16(pTmp + cn);

			vA = vaddq_u16(vA, vC);
			vB = vaddq_u16(vshlq_n_u16(vB, 1), vA);

			vst1_u8(dst + x, vrshrn_n_u16(vB, 4));
		}
#endif

		for (; x < width * cn; x++, pTmp++)
		{
			MInt32 s0 = ((pTmp[-cn] + pTmp[cn])) + (pTmp[0] << 1);
			dst[x] = (s0 + 8) >> 4;
		}
	}


	return res;
}

#if defined(MCV_MULTI_THREAD)
static MVoid thread_GaussianBlur_3x3_Rows(MVoid* pParam)
{
	Gau_Blur_3x3* pGau = (Gau_Blur_3x3*)pParam;

	GaussianBlur3x3_Rows_cn(pGau->src, pGau->width, pGau->height, pGau->srcPitch,
		pGau->dst, pGau->dstPitch, pGau->pBuf, pGau->cn, pGau->startRow, pGau->endRow);
}
#endif


MRESULT GaussianBlur3x3(MHandle hMemMgr, MHandle mcvParallelMonitor, MUInt8* src, MInt32 width, MInt32 height, MInt32 srcPitch,
	MUInt8* dst, MInt32 dstPitch, MInt32 cn)
{


	MInt32 lTaskNum = PRE_TASK_NUM;
	MInt32 i = 0;

	MInt32 bufSize = (width + 2) * cn + 8;
	MUInt16* pBuf = (MUInt16*)MMemAlloc(hMemMgr, bufSize * sizeof(MUInt16) * PRE_TASK_NUM);

	if (!pBuf)
		return MERR_NO_MEMORY;
	if (!src || !dst || width < 3 || height < 3)
		return MERR_INVALID_PARAM;

#ifdef MCV_MULTI_THREAD
	{
		MInt32 lSize, i;
		MInt32 taskID[PRE_TASK_NUM] = { 0 };
		Gau_Blur_3x3 pParams[PRE_TASK_NUM] = { 0 };

		lSize = height / lTaskNum;
		lSize = lSize >> 1 << 1;

		pParams[0].startRow = 0;
		pParams[0].endRow = lSize;
		for (i = 1; i < lTaskNum; i++)
		{
			pParams[i].startRow = i * lSize;
			pParams[i].endRow = (i + 1) * lSize;
		}
		pParams[i - 1].endRow = height;

		for (i = 0; i < lTaskNum; i++)
		{
			pParams[i].task_ID = i;
			pParams[i].src = src;
			pParams[i].srcPitch = srcPitch;
			pParams[i].dst = dst;
			pParams[i].dstPitch = dstPitch;
			pParams[i].height = height;
			pParams[i].width = width;
			pParams[i].cn = cn;
			pParams[i].pBuf = pBuf + i * bufSize;
		}

		for (i = 0; i < lTaskNum; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, thread_GaussianBlur_3x3_Rows, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTaskNum; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}
	}
#else
	GaussianBlur3x3_Rows_cn(src, width, height, srcPitch, dst, dstPitch, pBuf, cn, 0, height);
#endif

	MMemFree(hMemMgr, pBuf);
	return MOK;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END
