#include "boxfilter.h"
#include "ammem.h"
#include "merror.h"
#include <cmath>
#include <stdio.h>
#include <mobilecv.h>
#include "img_interpolation.h"
#include "single_image_enhancement_define.h"
#include "DefineForDebug.h"

#ifdef USE_NEON
#define CV_NEON 1
#endif

#define DENOISE_TASK_NUM 8

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef TRIMBYTE
#define TRIMBYTE(x)	(MByte)((x)&(~255)?((-(x))>>31):(x))
#endif

#ifdef MCV_MULTI_THREAD

typedef struct _tag_IMG_BOX {
	MInt32			taskID;
	MRESULT			errCode;
	MInt32			lImgWidth;
	MInt32			lImgHeight;
	MByte			*pSrcImg;
	MInt32			lSrcPitch;
	MByte			*pDstImg;
	MInt32			lDstPitch;
	MInt32			lStartRow;
	MInt32			lEndRow;
	MInt32          lRadius;
	MInt32          *pBoxSumBuf;
} Img_Box, *LP_Img_Box;

#endif

typedef struct _tag_LOCAL_VAR_BRI
{
    MInt32         task_ID;
    MRESULT        errCode;
    MByte          *pSrc;
    MByte          *pVar;
    MInt32         lWidth;
    MInt32         lHeight;
    MInt32         lSrcPitch;
    MInt32         lDstPitch;
    MInt32         lRadius;
    MInt32         lRate;
    MInt32         invDivNum01;
    MInt32         invDivNum02;
    MInt32         *pBoxSumBuf;
    MInt32         *pBoxSquSumBuf;
    MInt32         *Var_Rem_Rate;
    MInt32         *Bri_Reduce_Rate;
    MInt32         startRow;
    MInt32         endRow;
}Local_Var_Bri;

#define BOX_SHIFT     (22)
#define SQU_BOX_SHIFT (30)

#define FAST_IMPLEMENT
#define HALF_BOX_SHIFT      (11)
#define HALF_SQU_BOX_SHIFT  (15)

static MVoid boxBlurProcessRow_add(MInt32* pSumLine, MByte* addSrc, MInt32 lImgWidth, MInt32 lRadius)
{
    MInt32   i;
    MDWord* DaddSrc = (MDWord*)addSrc;
    MDWord  nValue;
    MInt32   DWidth = lImgWidth>>2;
    MInt32   nSum = 0;

    nValue = addSrc[0];
    pSumLine[0] = 0;
    pSumLine++;
    for (i = 0; i < lRadius; i++)
    {
        nSum += nValue;
        (pSumLine++)[0] += nSum;
    }

    i = 0;
#if CV_NEON
    {
		int32x4_t curr_sum_32x4;
		uint8x8x4_t tmpaddsrc_8x8x4;
		int16x8_t tmpsum00, tmpsum01, tmpsum02, tmpsum03;
		int16x8x2_t Sum00_16x8x2, Sum01_16x8x2;
		int16x8x2_t res00_16x8x2, res01_16x8x2;
		curr_sum_32x4 = vdupq_n_s32(nSum);
		for (i = 0; i < lImgWidth - 31; i += 32)
		{
			int32x4_t pSum_32x4;
			tmpaddsrc_8x8x4 = vld4_u8((addSrc + i));

			tmpsum00 = vreinterpretq_s16_u16(vmovl_u8(tmpaddsrc_8x8x4.val[0]));
			tmpsum01 = vaddq_s16(tmpsum00, vreinterpretq_s16_u16(vmovl_u8(tmpaddsrc_8x8x4.val[1])));
			tmpsum02 = vaddq_s16(tmpsum01, vreinterpretq_s16_u16(vmovl_u8(tmpaddsrc_8x8x4.val[2])));
			tmpsum03 = vaddq_s16(tmpsum02, vreinterpretq_s16_u16(vmovl_u8(tmpaddsrc_8x8x4.val[3])));

			//交叉存取
			Sum00_16x8x2 = vzipq_s16(tmpsum00, tmpsum02);
			Sum01_16x8x2 = vzipq_s16(tmpsum01, tmpsum03);

			res00_16x8x2 = vzipq_s16(Sum00_16x8x2.val[0],Sum01_16x8x2.val[0]);
			res01_16x8x2 = vzipq_s16(Sum00_16x8x2.val[1],Sum01_16x8x2.val[1]);

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_low_s16(res00_16x8x2.val[0])));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_high_s16(res00_16x8x2.val[0])));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_low_s16(res00_16x8x2.val[1])));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_high_s16(res00_16x8x2.val[1])));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_low_s16(res01_16x8x2.val[0])));
			nSum = vgetq_lane_s32(curr_sum_32x4, 3);
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_n_s32(nSum);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_high_s16(res01_16x8x2.val[0])));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_low_s16(res01_16x8x2.val[1])));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_high_s16(res01_16x8x2.val[1])));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;
		}
		nSum = vgetq_lane_s32(curr_sum_32x4, 3);
	}
	i >>= 2;
#endif
    for (; i < DWidth; i++)
    {
        nValue = DaddSrc[i];
        nSum += (nValue << 24 >> 24);
        (pSumLine++)[0] += nSum;
        nSum += (nValue << 16 >> 24);
        (pSumLine++)[0] += nSum;
        nSum += (nValue << 8 >> 24);
        (pSumLine++)[0] += nSum;
        nSum += (nValue >> 24);
        (pSumLine++)[0] += nSum;
    }
    for (i = DWidth<<2; i < lImgWidth; i++)
    {
        nValue = addSrc[i];
        nSum +=  nValue;
        (pSumLine++)[0] += nSum;
    }
    nValue = addSrc[lImgWidth-1];
    for (i = 0; i < lRadius; i++)
    {
        nSum +=  nValue;
        (pSumLine++)[0] += nSum;
    }
    return;
}

static MVoid boxBlurProcessRow_squ_add(MInt32* pSumLine, MByte* addSrc, MInt32 lImgWidth, MInt32 lRadius)
{
    MInt32   i;
    MDWord* DaddSrc = (MDWord*)addSrc;
    MDWord  nValue;
    MInt32   DWidth = lImgWidth >> 2;
    MInt32   nSum = 0;

    nValue = addSrc[0];
    pSumLine[0] = 0;
    pSumLine++;
    for (i = 0; i < lRadius; i++)
    {
        nSum += nValue * nValue;
        (pSumLine++)[0] += nSum;
    }
    for (i = 0; i < DWidth; i++)
    {
        MInt32 lval;
        nValue = DaddSrc[i];
        lval = (nValue << 24 >> 24);
        nSum += lval * lval;
        (pSumLine++)[0] += nSum;

        lval = (nValue << 16 >> 24);
        nSum += lval * lval;
        (pSumLine++)[0] += nSum;

        lval = (nValue << 8 >> 24);
        nSum += lval * lval;
        (pSumLine++)[0] += nSum;

        lval = (nValue >> 24);
        nSum += lval * lval;
        (pSumLine++)[0] += nSum;
    }
    for (i = DWidth << 2; i < lImgWidth; i++)
    {
        nValue = addSrc[i];
        nSum += nValue * nValue;
        (pSumLine++)[0] += nSum;
    }
    nValue = addSrc[lImgWidth - 1];
    for (i = 0; i < lRadius; i++)
    {
        nSum += nValue * nValue;
        (pSumLine++)[0] += nSum;
    }
    return;
}


static MVoid boxBlurProcessRow_squ_add_sub(MInt32* pSumLine, MByte* addSrc, MByte* subSrc, MInt32 lImgWidth, MInt32 lRadius)
{
    MInt32   i;
    MDWord* DaddSrc = (MDWord*)addSrc;
    MDWord* DsubSrc = (MDWord*)subSrc;
    MDWord  nAValue, nSValue;
    MInt32   lVal;
    MInt32   DWidth = lImgWidth >> 2;
    MInt32   nSum = 0;

    //lVal = addSrc[0] - subSrc[0];
    lVal = addSrc[0] * addSrc[0] - subSrc[0] * subSrc[0];
    pSumLine[0] = 0;
    pSumLine++;
    for (i = 0; i < lRadius; i++)
    {
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }
    i = 0;
#if CV_NEON_    //设备上性能更慢
    {
		int32x4_t curr_sum_32x4;
		uint8x8x4_t tmpaddsrc_8x8x4;
		uint8x8x4_t tmpsubsrc_8x8x4;
		uint16x8x4_t muladdsrc_16x8x4, mulsubsrc_16x8x4;
		int32x4_t diffhigh00_32x4, difflow00_32x4;
		int32x4_t diffhigh01_32x4, difflow01_32x4;
		int32x4_t diffhigh02_32x4, difflow02_32x4;
		int32x4_t diffhigh03_32x4, difflow03_32x4;
		int32x4x2_t sumlow_32x4x2, sumhigh_32x4x2;
		int32x4x2_t lowdata_32x4x2, highdata_32x4x2;
		curr_sum_32x4 = vdupq_n_s32(nSum);
		for (i = 0; i < lImgWidth - 31; i += 32)
		{
			int32x4_t pSum_32x4;
			tmpaddsrc_8x8x4 = vld4_u8((addSrc + i));
			tmpsubsrc_8x8x4 = vld4_u8((subSrc + i));

			muladdsrc_16x8x4.val[0] = vmull_u8(tmpaddsrc_8x8x4.val[0], tmpaddsrc_8x8x4.val[0]);
			muladdsrc_16x8x4.val[1] = vmull_u8(tmpaddsrc_8x8x4.val[1], tmpaddsrc_8x8x4.val[1]);
			muladdsrc_16x8x4.val[2] = vmull_u8(tmpaddsrc_8x8x4.val[2], tmpaddsrc_8x8x4.val[2]);
			muladdsrc_16x8x4.val[3] = vmull_u8(tmpaddsrc_8x8x4.val[3], tmpaddsrc_8x8x4.val[3]);

			mulsubsrc_16x8x4.val[0] = vmull_u8(tmpsubsrc_8x8x4.val[0], tmpsubsrc_8x8x4.val[0]);
			mulsubsrc_16x8x4.val[1] = vmull_u8(tmpsubsrc_8x8x4.val[1], tmpsubsrc_8x8x4.val[1]);
			mulsubsrc_16x8x4.val[2] = vmull_u8(tmpsubsrc_8x8x4.val[2], tmpsubsrc_8x8x4.val[2]);
			mulsubsrc_16x8x4.val[3] = vmull_u8(tmpsubsrc_8x8x4.val[3], tmpsubsrc_8x8x4.val[3]);

			difflow00_32x4 = vsubq_s32(vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(muladdsrc_16x8x4.val[0]))), vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(mulsubsrc_16x8x4.val[0]))));
			diffhigh00_32x4 = vsubq_s32(vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(muladdsrc_16x8x4.val[0]))), vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(mulsubsrc_16x8x4.val[0]))));
			difflow01_32x4 = vsubq_s32(vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(muladdsrc_16x8x4.val[1]))), vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(mulsubsrc_16x8x4.val[1]))));
			diffhigh01_32x4 = vsubq_s32(vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(muladdsrc_16x8x4.val[1]))), vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(mulsubsrc_16x8x4.val[1]))));
			difflow02_32x4 = vsubq_s32(vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(muladdsrc_16x8x4.val[2]))), vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(mulsubsrc_16x8x4.val[2]))));
			diffhigh02_32x4 = vsubq_s32(vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(muladdsrc_16x8x4.val[2]))), vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(mulsubsrc_16x8x4.val[2]))));
			difflow03_32x4 = vsubq_s32(vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(muladdsrc_16x8x4.val[3]))), vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(mulsubsrc_16x8x4.val[3]))));
			diffhigh03_32x4 = vsubq_s32(vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(muladdsrc_16x8x4.val[3]))), vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(mulsubsrc_16x8x4.val[3]))));

			difflow01_32x4 = vaddq_s32(difflow01_32x4, difflow00_32x4);
			diffhigh01_32x4 = vaddq_s32(diffhigh01_32x4, diffhigh00_32x4);
			difflow02_32x4 = vaddq_s32(difflow02_32x4, difflow01_32x4);
			diffhigh02_32x4 = vaddq_s32(diffhigh02_32x4, diffhigh01_32x4);
			difflow03_32x4 = vaddq_s32(difflow03_32x4, difflow02_32x4);
			diffhigh03_32x4 = vaddq_s32(diffhigh03_32x4, diffhigh02_32x4);

			//交叉存取
			sumlow_32x4x2 = vzipq_s32(difflow00_32x4, difflow02_32x4);
			sumhigh_32x4x2 = vzipq_s32(difflow01_32x4, difflow03_32x4);
			lowdata_32x4x2 = vzipq_s32(sumlow_32x4x2.val[0], sumhigh_32x4x2.val[0]);
			highdata_32x4x2 = vzipq_s32(sumlow_32x4x2.val[1], sumhigh_32x4x2.val[1]);

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, lowdata_32x4x2.val[0]);
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, lowdata_32x4x2.val[1]);
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, highdata_32x4x2.val[0]);
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, highdata_32x4x2.val[1]);
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			//交叉存取
			sumlow_32x4x2 = vzipq_s32(diffhigh00_32x4, diffhigh02_32x4);
			sumhigh_32x4x2 = vzipq_s32(diffhigh01_32x4, diffhigh03_32x4);
			lowdata_32x4x2 = vzipq_s32(sumlow_32x4x2.val[0], sumhigh_32x4x2.val[0]);
			highdata_32x4x2 = vzipq_s32(sumlow_32x4x2.val[1], sumhigh_32x4x2.val[1]);

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, lowdata_32x4x2.val[0]);
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, lowdata_32x4x2.val[1]);
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, highdata_32x4x2.val[0]);
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, highdata_32x4x2.val[1]);
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;
		}
		nSum = vgetq_lane_s32(curr_sum_32x4, 3);
	}
	i >>= 2;

#endif
    for (; i < DWidth; i++)
    {
        MInt32 lval01, lval02;
        nAValue = DaddSrc[i];
        nSValue = DsubSrc[i];
        lval01 = (nAValue << 24 >> 24);
        lval02 = (nSValue << 24 >> 24);
        lVal = lval01 * lval01 - lval02 * lval02;
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        lval01 = (nAValue << 16 >> 24);
        lval02 = (nSValue << 16 >> 24);
        lVal = lval01 * lval01 - lval02 * lval02;
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        lval01 = (nAValue << 8 >> 24);
        lval02 = (nSValue << 8 >> 24);
        lVal = lval01 * lval01 - lval02 * lval02;
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        lval01 = (nAValue >> 24);
        lval02 = (nSValue >> 24);
        lVal = lval01 * lval01 - lval02 * lval02;
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }

    for (i = DWidth << 2; i < lImgWidth; i++)
    {
        lVal = addSrc[i] * addSrc[i] - subSrc[i] * subSrc[i];
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }

    for (i = 0; i < lRadius; i++)
    {
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }
    return;
}

static MVoid boxBlurProcessRow_add_sub(MInt32* pSumLine, MByte* addSrc, MByte* subSrc, MInt32 lImgWidth, MInt32 lRadius)
{
    MInt32   i = 0;
    MDWord* DaddSrc = (MDWord*)addSrc;
    MDWord* DsubSrc = (MDWord*)subSrc;
    MDWord  nAValue, nSValue;
    MInt32   lVal;
    MInt32   DWidth = lImgWidth>>2;
    MInt32   nSum = 0;

    lVal = addSrc[0] - subSrc[0];
    pSumLine[0] = 0;
    pSumLine++;
    for (i = 0; i < lRadius; i++)
    {
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }
    i = 0;
#if CV_NEON
    //{
	//	uint8x16_t addsrc8x16, subsrc8x16;
	//	uint8x8_t   addsrc8x8_01, addsrc8x8_02,
	//		       subsrc8x8_01, subsrc8x8_02;
	//	int16x8_t dif_16x8_01, dif_16x8_02;
	//	MInt16 dif01[8], dif02[8];
	//	for (i = 0; i < lImgWidth - 15; i+=16)
	//	{
	//		addsrc8x16 = vld1q_u8(addSrc + i);
	//		subsrc8x16 = vld1q_u8(subSrc + i);
	//		addsrc8x8_01 = vget_low_u8(addsrc8x16);
	//		subsrc8x8_01 = vget_low_u8(subsrc8x16);
	//		addsrc8x8_02 = vget_high_u8(addsrc8x16);
	//		subsrc8x8_02 = vget_high_u8(subsrc8x16);
	//		dif_16x8_01 = vreinterpretq_s16_u16(vsubl_u8(addsrc8x8_01, subsrc8x8_01));
	//		dif_16x8_02 = vreinterpretq_s16_u16(vsubl_u8(addsrc8x8_02, subsrc8x8_02));
	//		vst1q_s16(dif01, dif_16x8_01);
	//		vst1q_s16(dif02, dif_16x8_02);
	//
	//		nSum += dif01[0];			(pSumLine++)[0] += nSum;
	//		nSum += dif01[1];			(pSumLine++)[0] += nSum;
	//		nSum += dif01[2];			(pSumLine++)[0] += nSum;
	//		nSum += dif01[3];			(pSumLine++)[0] += nSum;
	//		nSum += dif01[4];			(pSumLine++)[0] += nSum;
	//		nSum += dif01[5];			(pSumLine++)[0] += nSum;
	//		nSum += dif01[6];			(pSumLine++)[0] += nSum;
	//		nSum += dif01[7];			(pSumLine++)[0] += nSum;
	//		nSum += dif02[0];			(pSumLine++)[0] += nSum;
	//		nSum += dif02[1];			(pSumLine++)[0] += nSum;
	//		nSum += dif02[2];			(pSumLine++)[0] += nSum;
	//		nSum += dif02[3];			(pSumLine++)[0] += nSum;
	//		nSum += dif02[4];			(pSumLine++)[0] += nSum;
	//		nSum += dif02[5];			(pSumLine++)[0] += nSum;
	//		nSum += dif02[6];			(pSumLine++)[0] += nSum;
	//		nSum += dif02[7];			(pSumLine++)[0] += nSum;
	//	}
	//}
	//i = i >> 2;

	{
		MInt16 TempDif01[8] = { 0 }, TempDif02[8];
		int32x4_t curr_sum_32x4;
		uint8x8x4_t tmpaddsrc_8x8x4;
		uint8x8x4_t tmpsubsrc_8x8x4;
		int16x8_t sug_dif00, sug_dif01, sug_dif02, sug_dif03;
		int16x8x2_t tmpSum00_16x8x2, tmpSum01_16x8x2;
		int32x4x2_t tmpSum00_32x4x2, tmpSum01_32x4x2;
		curr_sum_32x4 = vdupq_n_s32(nSum);
		for (i = 0; i <lImgWidth - 31; i += 32)
		{
			int32x4_t pSum_32x4;
			tmpaddsrc_8x8x4 = vld4_u8((addSrc + i));
			tmpsubsrc_8x8x4 = vld4_u8((subSrc + i));
			sug_dif00 = vreinterpretq_s16_u16(vsubl_u8(tmpaddsrc_8x8x4.val[0], tmpsubsrc_8x8x4.val[0]));
			sug_dif01 = vreinterpretq_s16_u16(vsubl_u8(tmpaddsrc_8x8x4.val[1], tmpsubsrc_8x8x4.val[1]));
			sug_dif02 = vreinterpretq_s16_u16(vsubl_u8(tmpaddsrc_8x8x4.val[2], tmpsubsrc_8x8x4.val[2]));
			sug_dif03 = vreinterpretq_s16_u16(vsubl_u8(tmpaddsrc_8x8x4.val[3], tmpsubsrc_8x8x4.val[3]));

			sug_dif01 = vaddq_s16(sug_dif00, sug_dif01);
			sug_dif02 = vaddq_s16(sug_dif01, sug_dif02);
			sug_dif03 = vaddq_s16(sug_dif02, sug_dif03);

			//交叉存取
			tmpSum00_16x8x2 = vzipq_s16(sug_dif00, sug_dif01);
			tmpSum01_16x8x2 = vzipq_s16(sug_dif02, sug_dif03);

			tmpSum00_32x4x2 = vzipq_s32(vreinterpretq_s32_s16(tmpSum00_16x8x2.val[0]), vreinterpretq_s32_s16(tmpSum01_16x8x2.val[0]));
			tmpSum01_32x4x2 = vzipq_s32(vreinterpretq_s32_s16(tmpSum00_16x8x2.val[1]), vreinterpretq_s32_s16(tmpSum01_16x8x2.val[1]));

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_low_s16(vreinterpretq_s16_s32(tmpSum00_32x4x2.val[0]))));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_high_s16(vreinterpretq_s16_s32(tmpSum00_32x4x2.val[0]))));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_low_s16(vreinterpretq_s16_s32(tmpSum00_32x4x2.val[1]))));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_high_s16(vreinterpretq_s16_s32(tmpSum00_32x4x2.val[1]))));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_low_s16(vreinterpretq_s16_s32(tmpSum01_32x4x2.val[0]))));
			nSum = vgetq_lane_s32(curr_sum_32x4, 3);
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_n_s32(nSum);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_high_s16(vreinterpretq_s16_s32(tmpSum01_32x4x2.val[0]))));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_low_s16(vreinterpretq_s16_s32(tmpSum01_32x4x2.val[1]))));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;

			curr_sum_32x4 = vdupq_lane_s32(vget_high_s32(curr_sum_32x4), 1);
			pSum_32x4 = vld1q_s32(pSumLine);
			curr_sum_32x4 = vaddq_s32(curr_sum_32x4, vmovl_s16(vget_high_s16(vreinterpretq_s16_s32(tmpSum01_32x4x2.val[1]))));
			pSum_32x4 = vaddq_s32(pSum_32x4, curr_sum_32x4);
			vst1q_s32(pSumLine, pSum_32x4);			pSumLine += 4;
		}
		nSum = vgetq_lane_s32(curr_sum_32x4, 3);
	}
	i >>= 2;

#endif // CV_NEON

    for (; i < DWidth; i++)//i = 0
    {
        nAValue = DaddSrc[i];
        nSValue = DsubSrc[i];
        lVal = (nAValue << 24 >> 24) - (nSValue << 24 >> 24);
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        lVal =  (nAValue << 16 >> 24) - (nSValue << 16 >> 24);
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        lVal =  (nAValue << 8 >> 24) - (nSValue << 8 >> 24);
        nSum += lVal;
        (pSumLine++)[0] += nSum;

        lVal =  (nAValue >> 24) - (nSValue >> 24);
        nSum += lVal;
        (pSumLine++)[0] += nSum;
    }

    for (i = DWidth<<2; i < lImgWidth; i++)
    {
        lVal = addSrc[i] - subSrc[i];
        nSum +=  lVal;
        (pSumLine++)[0] += nSum;
    }
    lVal = addSrc[lImgWidth - 1] - subSrc[lImgWidth - 1];
    for (i = 0; i < lRadius; i++)
    {
        nSum +=  lVal;
        (pSumLine++)[0] += nSum;
    }
    return;
}

static MVoid BoxFilterRow_Var(MByte* pDst, MInt32* pBoxSumBuf, MInt32 *pBoxSquSumBuf, MInt32 lImgWidth, MInt32 lRadius, MInt32 invDivNum)
{
    MInt32 x;
    MInt32 lbVal = 0;
    MInt32 lboxSize = lRadius * 2 + 1;
    MInt32 lSurBox = lboxSize * lboxSize;

    for (x = 0; x < lImgWidth; x++)
    {
        MInt64 lMean = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];;
        MInt64 lSquMean = pBoxSquSumBuf[lboxSize] - pBoxSquSumBuf[0];

        //lMean = lMean * invDivNum >> 22;
        //lSquMean = lSquMean * invDivNum >> 22;
        //lbVal = lSquMean - lMean * lMean;

        lSquMean = lSquMean * lSurBox;
        lMean = lMean * lMean;
        lSquMean -= lMean;
        lbVal = lSquMean * invDivNum >> SQU_BOX_SHIFT;
        //lbVal >>= 2;
        if (lbVal > 255)
        {
            lbVal = 255;
        }
        pDst[x] = TRIMBYTE(lbVal);
        pBoxSumBuf++;
        pBoxSquSumBuf++;
    }
}

static MVoid BoxFilterRow_Var_Bright_Reduce(MByte* pDst, MInt32* pBoxSumBuf, MInt32 *pBoxSquSumBuf, MInt32 lImgWidth,
                                            MInt32 lRadius, MInt32 invDivNum01, MInt32 invDivNum02, MInt32* Var_Rem_Rate, MInt32* Bri_Reduce_Rate)
{
    MInt32 x;
    MInt32 lbVal = 0;
    MInt32 lboxSize = lRadius * 2 + 1;
    MInt32 lSurBox = lboxSize * lboxSize;

    for (x = 0; x < lImgWidth; x++)
    {
        MInt64 lMean = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
        MInt32 lMean02 = lMean;
        MInt64 lSquMean = pBoxSquSumBuf[lboxSize] - pBoxSquSumBuf[0];

        //lMean = lMean * invDivNum >> 22;
        //lSquMean = lSquMean * invDivNum >> 22;
        //lbVal = lSquMean - lMean * lMean;

        lSquMean = lSquMean * lSurBox;
        lMean = lMean * lMean;
        lSquMean -= lMean;
        lbVal = lSquMean * invDivNum01 >> SQU_BOX_SHIFT;
        //lbVal >>= 2;
        if (lbVal > 255)
        {
            lbVal = 255;
        }

        lMean02 = lMean02 * invDivNum02 >> BOX_SHIFT;
        lbVal = ((1024 - Var_Rem_Rate[lbVal]) * Bri_Reduce_Rate[lMean02])>>10;
        lbVal = 1024 - lbVal;
        lbVal = lbVal >> 2;
        pDst[x] = TRIMBYTE(lbVal);
        pBoxSumBuf++;
        pBoxSquSumBuf++;
    }
}


MInt32 Img_Local_Var_C1(MHandle hMemMgr, MByte *pSrc, MByte* VarImg, MInt32 lImgWidth, MInt32 lImgHeight,
                        MInt32 lSrcPitch, MInt32 lDstPitch, MInt32 lRadius)
{
    MInt32 lret = MOK;
    MInt32 line;
    MByte *tmpaddSrc = pSrc;
    MByte *tmpsubSrc = pSrc;
    MByte *tmpSrc = pSrc;
    MByte *tmpDst = VarImg;
    MInt32 lboxSize = lRadius * 2 + 1;
    MInt32 invDivNum01 = (1 << SQU_BOX_SHIFT) / (lboxSize*lboxSize*lboxSize*lboxSize);
    MInt32 invDivNum02 = (1 << BOX_SHIFT) / (lboxSize*lboxSize);

    MInt32 *pBoxSumBuf = MNull, *pBoxSquSumBuf = MNull;

    //for (line = 0; line < lImgHeight; line++)
    //{
    //	MByte lval = line % 256;
    //	MMemSet(pSrc + line * lSrcPitch, lval, lSrcPitch);
    //}


    pBoxSumBuf = (MInt32*)MMemAlloc(hMemMgr, (lImgWidth + lRadius * 2 + 1 + 10) * sizeof(MInt32));
    pBoxSquSumBuf = (MInt32*)MMemAlloc(hMemMgr, (lImgWidth + lRadius * 2 + 1 + 10) * sizeof(MInt32));
    if (MNull == pBoxSumBuf ||
        MNull == pBoxSquSumBuf)
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    MMemSet(pBoxSumBuf,    0, (lImgWidth + lRadius * 2 + 1 + 10) * sizeof(MInt32));
    MMemSet(pBoxSquSumBuf, 0, (lImgWidth + lRadius * 2 + 1 + 10) * sizeof(MInt32));

    tmpaddSrc = tmpSrc;

    for (line = -lRadius; line < 0; line++)
    {
        boxBlurProcessRow_add(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
        boxBlurProcessRow_squ_add(pBoxSquSumBuf, tmpaddSrc, lImgWidth, lRadius);
    }
    for (; line < lRadius; line++)
    {
        boxBlurProcessRow_add(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
        boxBlurProcessRow_squ_add(pBoxSquSumBuf, tmpaddSrc, lImgWidth, lRadius);
        tmpaddSrc += lSrcPitch;
    }
    {
        boxBlurProcessRow_add(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
        boxBlurProcessRow_squ_add(pBoxSquSumBuf, tmpaddSrc, lImgWidth, lRadius);
        BoxFilterRow_Var(tmpDst, pBoxSumBuf, pBoxSquSumBuf, lImgWidth, lRadius, invDivNum01);
        tmpaddSrc += lSrcPitch;
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
        line++;
    }
    for (; line < lImgHeight; line++)
    {
        boxBlurProcessRow_add_sub(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
        boxBlurProcessRow_squ_add_sub(pBoxSquSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
        BoxFilterRow_Var(tmpDst, pBoxSumBuf, pBoxSquSumBuf, lImgWidth, lRadius, invDivNum01);
        tmpaddSrc += lSrcPitch;
        if (line >= lboxSize)
        {
            tmpsubSrc += lSrcPitch;
        }
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
    }
    tmpaddSrc -= lSrcPitch;
    for (; line < lImgHeight + lRadius; line++)
    {
        boxBlurProcessRow_add_sub(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
        boxBlurProcessRow_squ_add_sub(pBoxSquSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
        BoxFilterRow_Var(tmpDst, pBoxSumBuf, pBoxSquSumBuf, lImgWidth, lRadius, invDivNum01);
        tmpsubSrc += lSrcPitch;
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
    }
#ifdef _OUTPUT_LOG_
    {
		//save the ref pyramid image
		char szName[256];
		sprintf(szName, "%s/ref_img_var.bmp", OUTPUT_PATH);
		SaveToBMP(szName, VarImg, lImgWidth, lImgHeight, lDstPitch, 8);

}
#endif
    exit:
    if (pBoxSumBuf)
    {
        MMemFree(hMemMgr, pBoxSumBuf);
        pBoxSumBuf = MNull;
    }
    if (pBoxSquSumBuf)
    {
        MMemFree(hMemMgr, pBoxSquSumBuf);
        pBoxSquSumBuf = MNull;
    }
    return lret;
}

MVoid Compute_Var_Rem_Rate(MInt32* Var_Rem_Rate, MInt32 lRate, MInt32 lThreshTop, MInt32 lThreshBot)
{
    MInt32 k;
    MFloat lThreshVal = 1.0f / ((lThreshTop - lThreshBot) * (lThreshTop - lThreshBot));
    for (k = 0; k < lThreshBot; k++)
    {
        Var_Rem_Rate[k] = 0;
    }
    for (; k < lThreshTop; k++)
    {
        MInt32 val = MInt32(lThreshVal*(1024 - (k - lThreshBot) * (k - lThreshBot) * 1024));
        Var_Rem_Rate[k] = 1024 - (val * lRate >> 10);

    }
    for (; k < 256; k++)
    {
        Var_Rem_Rate[k] = 1024;
    }
    return;
}

MVoid Compute_Bri_Rem_Rate(MInt32* Bri_Reduce_Rate, MInt32 lThresh_Top, MInt32 lThresh_Bot)
{
    MInt32 k;
    MFloat lThreshVal = 1.0f / (lThresh_Top - lThresh_Bot);
    for (k = 0; k < lThresh_Bot; k++)
    {
        Bri_Reduce_Rate[k] = 1024;
    }
    for (; k < lThresh_Top; k++)
    {
        Bri_Reduce_Rate[k] = MInt32(lThreshVal * (lThresh_Top - k) * 1024);
    }
    for (; k < 256; k++)
    {
        Bri_Reduce_Rate[k] = 0;
    }

}

static MVoid Calc_Var_Bright_Reduce(MByte *pSrc, MByte *VarImg, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcPitch, MInt32 lDstPitch,
                                    MInt32 lRadius, MInt32 lRate, MInt32 invDivNum01, MInt32 invDivNum02, MInt32 *Var_Rem_Rate, MInt32 *Bri_Reduce_Rate,
                                    MInt32 *pBoxSumBuf, MInt32 *pBoxSquSumBuf, MInt32 startRow, MInt32 endRow)
{
    MInt32 lPreLine = MIN(lRadius, startRow),
            lNexLine = MIN(lRadius, lHeight - endRow);
    MInt32 lTop = startRow - lPreLine;
    MInt32 lBot = endRow + lNexLine;
    MByte *tmpaddSrc = pSrc + lTop * lSrcPitch;
    MByte *tmpsubSrc = pSrc + lTop * lSrcPitch;
    MByte *tmpSrc = pSrc + startRow * lSrcPitch;
    MByte *tmpDst = VarImg + startRow * lDstPitch;
    MInt32 line = startRow - lRadius;

    for (; line < lTop; line++)
    {
        boxBlurProcessRow_add(pBoxSumBuf, tmpaddSrc, lWidth, lRadius);
        boxBlurProcessRow_squ_add(pBoxSquSumBuf, tmpaddSrc, lWidth, lRadius);
    }

    for (; line < (startRow + lRadius); line++)
    {
        boxBlurProcessRow_add(pBoxSumBuf, tmpaddSrc, lWidth, lRadius);
        boxBlurProcessRow_squ_add(pBoxSquSumBuf, tmpaddSrc, lWidth, lRadius);
        tmpaddSrc += lSrcPitch;
    }
    {
        boxBlurProcessRow_add(pBoxSumBuf, tmpaddSrc, lWidth, lRadius);
        boxBlurProcessRow_squ_add(pBoxSquSumBuf, tmpaddSrc, lWidth, lRadius);
        BoxFilterRow_Var_Bright_Reduce(tmpDst, pBoxSumBuf, pBoxSquSumBuf, lWidth, lRadius, invDivNum01, invDivNum02, Var_Rem_Rate, Bri_Reduce_Rate);
        tmpaddSrc += lSrcPitch;
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
        line++;
    }
    for (; line < lBot; line++)
    {
        boxBlurProcessRow_add_sub(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lWidth, lRadius);
        boxBlurProcessRow_squ_add_sub(pBoxSquSumBuf, tmpaddSrc, tmpsubSrc, lWidth, lRadius);
        BoxFilterRow_Var_Bright_Reduce(tmpDst, pBoxSumBuf, pBoxSquSumBuf, lWidth, lRadius, invDivNum01, invDivNum02, Var_Rem_Rate, Bri_Reduce_Rate);
        tmpaddSrc += lSrcPitch;
        if (line >= (lTop + 2*lRadius + 1))
        {
            tmpsubSrc += lSrcPitch;
        }
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
    }
    tmpaddSrc -= lSrcPitch;
    for (; line < (endRow + lRadius); line++)
    {
        boxBlurProcessRow_add_sub(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lWidth, lRadius);
        boxBlurProcessRow_squ_add_sub(pBoxSquSumBuf, tmpaddSrc, tmpsubSrc, lWidth, lRadius);
        BoxFilterRow_Var_Bright_Reduce(tmpDst, pBoxSumBuf, pBoxSquSumBuf, lWidth, lRadius, invDivNum01, invDivNum02, Var_Rem_Rate, Bri_Reduce_Rate);
        tmpsubSrc += lSrcPitch;
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
    }
}

#ifdef MCV_MULTI_THREAD
MVoid thread_calc_var_bright(MVoid* pParam)
{
	Local_Var_Bri *vb = (Local_Var_Bri*)pParam;
	MInt32 lret = MOK;

	Calc_Var_Bright_Reduce(vb->pSrc, vb->pVar, vb->lWidth, vb->lHeight, vb->lSrcPitch, vb->lDstPitch, vb->lRadius,
		vb->lRate, vb->invDivNum01, vb->invDivNum02, vb->Var_Rem_Rate, vb->Bri_Reduce_Rate, vb->pBoxSumBuf,
		vb->pBoxSquSumBuf, vb->startRow, vb->endRow);
}
#endif


MInt32 Img_Local_Var_Bright_Reduce(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *pSrc, MByte* VarImg, MInt32 lImgWidth, MInt32 lImgHeight,
                                   MInt32 lSrcPitch, MInt32 lDstPitch, MInt32 lRadius, MInt32 lRate)
{
    MInt32 lret = MOK;
    MInt32 k, i, lboxSize, invDivNum01, invDivNum02;
    MByte *tmpSrc = MNull, *tmpVar = MNull;
    MInt32 *pBoxSumBuf[DENOISE_TASK_NUM] = { MNull }, *pBoxSquSumBuf[DENOISE_TASK_NUM] = { MNull };
    MInt32 Var_Rem_Rate[256] = { 0 };
    MInt32 Bri_Reduce_Rate[256] = { 0 };
    MByte* BufImgData = MNull;
    MInt32 dScale = 2, padsize = (1 << dScale) - 1;
    MInt32 smImgHeight, smImgWidth, smSrcPitch, smDstPitch;

    //在小图上计算
    smImgHeight = lImgHeight >> dScale;
    smImgWidth = lImgWidth >> dScale;
    smSrcPitch = smImgWidth*(lSrcPitch / lImgWidth);
    smDstPitch = smImgWidth*(lDstPitch / lImgWidth);
    lRadius = lRadius >> dScale;
    tmpSrc = (MByte*)MMemAlloc(hMemMgr, (smImgHeight - 1)*smSrcPitch + smImgWidth*sizeof(MByte));
    tmpVar = (MByte*)MMemAlloc(hMemMgr, (smImgHeight - 1)*smDstPitch + smImgWidth*sizeof(MByte));
    if (tmpSrc == MNull || tmpVar == MNull)
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    lret = fastBilinearInter_8U(mcvParallelMonitor, pSrc, lImgHeight, lImgWidth, lSrcPitch, tmpSrc,
                                smImgHeight, smImgWidth, smSrcPitch, 0, dScale);
    if(lret != MOK)
        goto exit;

    lboxSize = lRadius * 2 + 1;
    invDivNum01 = (1 << SQU_BOX_SHIFT) / (lboxSize*lboxSize*lboxSize*lboxSize);
    invDivNum02 = (1 << BOX_SHIFT) / (lboxSize*lboxSize);
    BufImgData = (MByte*)MMemAlloc(hMemMgr, (smImgHeight - 1)*smDstPitch + smImgWidth*sizeof(MByte));
    if (MNull == BufImgData)
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    Compute_Var_Rem_Rate(Var_Rem_Rate, lRate, 24, 8);

    Compute_Bri_Rem_Rate(Bri_Reduce_Rate, 48, 24);

    //for (line = 0; line < lImgHeight; line++)
    //{
    //	MByte lval = line % 256;
    //	MMemSet(pSrc + line * lSrcPitch, lval, lSrcPitch);
    //}

    pBoxSumBuf[0] = (MInt32*)MMemAlloc(hMemMgr, DENOISE_TASK_NUM*(smImgWidth + lRadius * 2 + 1 + 10) * sizeof(MInt32));
    pBoxSquSumBuf[0] = (MInt32*)MMemAlloc(hMemMgr, DENOISE_TASK_NUM*(smImgWidth + lRadius * 2 + 1 + 10) * sizeof(MInt32));
    if (MNull == pBoxSumBuf[0] ||
        MNull == pBoxSquSumBuf[0])
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    MMemSet(pBoxSumBuf[0], 0, DENOISE_TASK_NUM*(smImgWidth + lRadius * 2 + 1 + 10) * sizeof(MInt32));
    MMemSet(pBoxSquSumBuf[0], 0, DENOISE_TASK_NUM*(smImgWidth + lRadius * 2 + 1 + 10) * sizeof(MInt32));

    for (k = 1; k < DENOISE_TASK_NUM; ++k)
    {
        pBoxSumBuf[k] = pBoxSumBuf[0] + k*(smImgWidth + lRadius * 2 + 1 + 10);
        pBoxSquSumBuf[k] = pBoxSquSumBuf[0] + k*(smImgWidth + lRadius * 2 + 1 + 10);
    }

#ifdef MCV_MULTI_THREAD
    {
		MInt32 lSize;
		const MInt32 lTask_Num = DENOISE_TASK_NUM;
		MInt32 taskID[lTask_Num] = { 0 };
		Local_Var_Bri pParams[lTask_Num] = { 0 };

		lSize = smImgHeight / lTask_Num;
		lSize = lSize >> 1 << 1;

		pParams[0].startRow = 0;
		pParams[0].endRow = lSize;
		for (i = 1; i < lTask_Num; i++)
		{
			pParams[i].startRow = pParams[i - 1].endRow;
			pParams[i].endRow = pParams[i].startRow + lSize;
		}
		pParams[lTask_Num - 1].endRow = smImgHeight;

		for (i = 0; i < lTask_Num; ++i)
		{
			pParams[i].task_ID = i;
			pParams[i].pSrc = tmpSrc;
			pParams[i].pVar = tmpVar;
			pParams[i].lWidth = smImgWidth;
			pParams[i].lHeight = smImgHeight;
			pParams[i].lSrcPitch = smSrcPitch;
			pParams[i].lDstPitch = smDstPitch;
			pParams[i].lRadius = lRadius;
			pParams[i].lRate = lRate;
			pParams[i].invDivNum01 = invDivNum01;
			pParams[i].invDivNum02 = invDivNum02;
			pParams[i].Var_Rem_Rate = Var_Rem_Rate;
			pParams[i].Bri_Reduce_Rate = Bri_Reduce_Rate;
			pParams[i].pBoxSumBuf = pBoxSumBuf[i];
			pParams[i].pBoxSquSumBuf = pBoxSquSumBuf[i];
		}
		for (i = 0; i < lTask_Num; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, thread_calc_var_bright, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTask_Num; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}
	}
#else
    Calc_Var_Bright_Reduce(tmpSrc, tmpVar, lImgWidth, lImgHeight, lSrcPitch, lDstPitch, lRadius, lRate,
                           invDivNum01, invDivNum02, Var_Rem_Rate, Bri_Reduce_Rate, pBoxSumBuf[0], pBoxSquSumBuf[0], 0, lImgHeight);
#endif

    MMemCpy(BufImgData, tmpVar, (smImgHeight - 1)*smDstPitch + smImgWidth*sizeof(MByte));
    Box_Filter_C1(hMemMgr, mcvParallelMonitor, BufImgData, smImgWidth, smImgHeight, smDstPitch, tmpVar, smDstPitch, 5, MNull);
#if defined(_ARM_TIME_)
    MLong lTime;
	START_PROFILE();
#endif

    //双线性上采样
    lret = fastBilinearInter_8U(mcvParallelMonitor, tmpVar, smImgHeight, smImgWidth, smDstPitch, VarImg,
                                lImgHeight, lImgWidth, lDstPitch, dScale, 0);
    if(lret != MOK)
        goto exit;
#if defined(_ARM_TIME_)
    END_PROFILE(lTime);
	PrintfB(6, "NightShot", "LocalVar upSample time: %ldms\r\n", lTime);
#endif

    exit:
    if (BufImgData)
    {
        MMemFree(hMemMgr, BufImgData);
        BufImgData = MNull;
    }
    if (pBoxSumBuf[0])
    {
        MMemFree(hMemMgr, pBoxSumBuf[0]);
        pBoxSumBuf[0] = MNull;
    }
    if (pBoxSquSumBuf[0])
    {
        MMemFree(hMemMgr, pBoxSquSumBuf[0]);
        pBoxSquSumBuf[0] = MNull;
    }
    if (tmpSrc)
    {
        MMemFree(hMemMgr, tmpSrc);
        tmpSrc = MNull;
    }
    if (tmpVar)
    {
        MMemFree(hMemMgr, tmpVar);
        tmpVar = MNull;
    }
    return lret;
}

static MVoid BoxFilterRow(MByte* pDst, MInt32* pBoxSumBuf, MInt32 lImgWidth, MInt32 lRadius, MInt32 invDivNum)
{
    MInt32 x;
    MInt32 lbVal   = 0;
    MInt32 lboxSize = lRadius*2+1;

    x = 0;
#if CV_NEON
    {
		int32x4_t sumdata00_32x4, sumdata01_32x4;
		int32x4_t tmpdata00_32x4, tmpdata01_32x4;
		int32x4_t const_invDiv;
		int32x4_t resdata00_32x4, resdata01_32x4;
		int16x8_t resdata_16x8;
		int16x8_t const_255, const_0;
		const_invDiv = vdupq_n_s32(invDivNum);
		const_255 = vdupq_n_s16(255);
		const_0 = vdupq_n_s16(0);

		for (x = 0; x < lImgWidth - 7; x += 8)
		{
			sumdata00_32x4 = vld1q_s32(pBoxSumBuf + x + lboxSize);
			sumdata01_32x4 = vld1q_s32(pBoxSumBuf + x);
			tmpdata00_32x4 = vsubq_s32(sumdata00_32x4, sumdata01_32x4);
			tmpdata01_32x4 = vmulq_s32(tmpdata00_32x4, const_invDiv);
			resdata00_32x4 = vshrq_n_s32(tmpdata01_32x4, 22);

			sumdata00_32x4 = vld1q_s32(pBoxSumBuf + x + lboxSize + 4);
			sumdata01_32x4 = vld1q_s32(pBoxSumBuf + x + 4);
			tmpdata00_32x4 = vsubq_s32(sumdata00_32x4, sumdata01_32x4);
			tmpdata01_32x4 = vmulq_s32(tmpdata00_32x4, const_invDiv);
			resdata01_32x4 = vshrq_n_s32(tmpdata01_32x4, 22);

			resdata_16x8 = vcombine_s16(vmovn_s32(resdata00_32x4), vmovn_s32(resdata01_32x4));
			resdata_16x8 = vmaxq_s16(resdata_16x8, const_0);
			resdata_16x8 = vminq_s16(resdata_16x8, const_255);

			vst1_u8(pDst + x, vqmovun_s16(resdata_16x8));
		}
	}
#endif
    for (; x < lImgWidth; x++)
    {
        lbVal = pBoxSumBuf[lboxSize] - pBoxSumBuf[0];
        lbVal = lbVal * invDivNum >> 22;
        pDst[x] = TRIMBYTE(lbVal);
        pBoxSumBuf++;
    }
}



static MVoid Image_Box_Stripe(MByte* pSrcImg, MInt32 lImgWidth, MInt32 lImgHeight, MInt32 lSrcPitch,
                              MByte* pDstImg, MInt32 lDstPitch, MInt32 lStartRow, MInt32 lEndRow,
                              MInt32 lRadius, MInt32 *pBoxSumBuf)
{
    MInt32 lPreLine=MIN(lRadius,lStartRow),
            lNexLine=MIN(lRadius,lImgHeight-lEndRow);
    MInt32 lTop = lStartRow - lPreLine;
    MInt32 lBot = lEndRow + lNexLine;
    MByte *tmpaddSrc = pSrcImg + lTop * lSrcPitch;
    MByte *tmpsubSrc = pSrcImg + lTop * lSrcPitch;
    MByte *tmpSrc = pSrcImg + lStartRow * lSrcPitch;
    MByte *tmpDst = pDstImg + lStartRow * lDstPitch;
    MInt32 invDivNum = ( 1 << 22) / ((lRadius*2+1)*(lRadius*2+1));

    MInt32 line = lStartRow- lRadius;

    //MInt32 lboxSize = lRadius*2+1;

    MMemSet(pBoxSumBuf, 0, (lImgWidth + lRadius*2 + 1 + 100) * sizeof(MInt32));

    for (; line < lTop; line++)
    {
        boxBlurProcessRow_add(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
    }
    for (; line < lStartRow + lRadius; line++)
    {
        boxBlurProcessRow_add(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
        tmpaddSrc += lSrcPitch;
    }
    {
        boxBlurProcessRow_add(pBoxSumBuf, tmpaddSrc, lImgWidth, lRadius);
        BoxFilterRow(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
        tmpaddSrc += lSrcPitch;
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
        line++;
    }
    for (; line < lBot; line++)
    {
        boxBlurProcessRow_add_sub(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
        BoxFilterRow(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
        tmpaddSrc += lSrcPitch;
        if (line >= lTop + lRadius * 2 + 1)
        {
            tmpsubSrc += lSrcPitch;
        }
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
    }
    tmpaddSrc -= lSrcPitch;
    for (; line < lEndRow + lRadius; line++)
    {
        boxBlurProcessRow_add_sub(pBoxSumBuf, tmpaddSrc, tmpsubSrc, lImgWidth, lRadius);
        BoxFilterRow(tmpDst, pBoxSumBuf, lImgWidth, lRadius, invDivNum);
        tmpsubSrc += lSrcPitch;
        tmpSrc += lSrcPitch;
        tmpDst += lDstPitch;
    }
}

#ifdef MCV_MULTI_THREAD
MVoid thread_image_box(MVoid* pParam)
{
	Img_Box *Filter = (Img_Box*)pParam;
	MInt32 lret = MOK;

	Image_Box_Stripe(Filter->pSrcImg, Filter->lImgWidth, Filter->lImgHeight,
		Filter->lSrcPitch, Filter->pDstImg, Filter->lDstPitch, Filter->lStartRow, Filter->lEndRow,
		Filter->lRadius, Filter->pBoxSumBuf);

	Filter->errCode = lret;
}
#endif


MRESULT Box_Filter_C1(MHandle hMemMgr, MHandle mcvParallelMonitor,
    MByte* pSrcBuf, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcLineBytes,
    MByte* pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius, MInt32* boxRowBuf)
{
    START_TIME;
    MRESULT lret = MOK;
    MInt32 i;
    MInt32 *box_sum_buf[DENOISE_TASK_NUM] = {MNull};

    if (boxRowBuf == MNull)
    {
        box_sum_buf[0] = (MInt32*)MMemAlloc(hMemMgr, (lWidth + lRadius * 2 + 1 + 100) * DENOISE_TASK_NUM * sizeof(MInt32));
    }
    else
    {
        box_sum_buf[0] = boxRowBuf;
    }

    if (MNull == box_sum_buf[0])
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }
    for (i = 1; i < DENOISE_TASK_NUM; i++)
    {
        box_sum_buf[i] = box_sum_buf[0] + (lWidth + lRadius * 2 + 1 + 100) * i;
    }
    

#ifdef MCV_MULTI_THREAD
    if (mcvParallelMonitor)
	{
		MInt32 lSize;
		const MInt32 lTask_Num = DENOISE_TASK_NUM;
		MInt32 taskID[lTask_Num] = { 0 };
		Img_Box pParams[lTask_Num] = { 0 };

		lSize = lHeight / lTask_Num;
		lSize = lSize >> 1 << 1;

		pParams[0].lStartRow = 0;
		pParams[0].lEndRow = lSize;
		for(i = 1; i < lTask_Num; i++)
		{
			pParams[i].lStartRow = pParams[i-1].lEndRow;
			pParams[i].lEndRow = pParams[i].lStartRow + lSize;
		}
		pParams[lTask_Num-1].lEndRow = lHeight;

		for(i = 0; i < lTask_Num; i++)
		{
			pParams[i].taskID = i;
			pParams[i].lImgWidth  = lWidth;
			pParams[i].lImgHeight = lHeight;
			pParams[i].pSrcImg    = pSrcBuf;
			pParams[i].lSrcPitch  = lSrcLineBytes;
			pParams[i].pDstImg    = pDstBuf;
			pParams[i].lDstPitch  = lDstLineBytes;
			pParams[i].lRadius    = lRadius;
			pParams[i].pBoxSumBuf = box_sum_buf[i];
		}

		for (i = 0; i < lTask_Num; i++)
		{
			taskID[i] = mcvAddTask(mcvParallelMonitor, thread_image_box, (MVoid*)&pParams[i]);
		}
		for (i = 0; i < lTask_Num; i++)
		{
			mcvWaitTask(mcvParallelMonitor, taskID[i]);
		}
	}
	else
#endif
    {
        Image_Box_Stripe(pSrcBuf, lWidth, lHeight,
                         lSrcLineBytes, pDstBuf, lDstLineBytes, 0, lHeight,lRadius, box_sum_buf[0]);
    }

    exit:
    if (box_sum_buf[0] && boxRowBuf == MNull)
    {
        MMemFree(hMemMgr, box_sum_buf[0]);
        box_sum_buf[0] = MNull;
    }

    END_TIME;
    return lret;
}

MInt32  Box_Filter_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 lRadius, MInt32* boxRowBuf)
{
    MInt32 lRet = 0;

    lRet = Box_Filter_C1(hMemMgr, mcvParallelMonitor,
        pSrcImg->ppu8Plane[0], pSrcImg->i32Width, pSrcImg->i32Height, pSrcImg->pi32Pitch[0],
        pDstImg->ppu8Plane[0], pDstImg->pi32Pitch[0], lRadius, boxRowBuf);

    return lRet;
}

MRESULT Box_Filter_MinImg(MHandle hMemMgr, MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
                          MInt32 lSrcLineBytes, MByte *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius)
{
    MRESULT lret = MOK;
    MInt32 i;
    MInt32 *box_sum_buf = MNull;
    box_sum_buf = (MInt32*)MMemAlloc(hMemMgr, (lWidth + lRadius * 2 + 1 + 100) * sizeof(MInt32));
    if (MNull == box_sum_buf)
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    Image_Box_Stripe(pSrcBuf, lWidth, lHeight,
                     lSrcLineBytes, pDstBuf, lDstLineBytes, 0, lHeight, lRadius, box_sum_buf);

    exit:
    if (box_sum_buf)
    {
        MMemFree(hMemMgr, box_sum_buf);
        box_sum_buf = MNull;
    }
    return lret;
}

MVoid Box_Filter_RowBuf(MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
                        MInt32 lSrcLineBytes, MByte *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius, MInt32 *boxRowBuf)
{
    MMemSet(boxRowBuf, 0, (lWidth + lRadius * 2 + 1 + 100) * sizeof(MInt32));
    Image_Box_Stripe(pSrcBuf, lWidth, lHeight,
                     lSrcLineBytes, pDstBuf, lDstLineBytes, 0, lHeight, lRadius, boxRowBuf);
}

MVoid Box_Filter_RowBuf_C2(MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcLineBytes,
                           MByte *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius, MInt32 *boxRowBuf)
{
    MInt32 i, j, lPitch = lSrcLineBytes >> 1;
    MByte *srcU = (MByte*)MMemAlloc(MNull, lHeight*lPitch);
    MByte *srcV = (MByte*)MMemAlloc(MNull, lHeight*lPitch);
    MByte *resU = (MByte*)MMemAlloc(MNull, lHeight*lPitch);
    MByte *resV = (MByte*)MMemAlloc(MNull, lHeight*lPitch);
    if (srcU == MNull || srcV == MNull || resU == MNull || resV == MNull)
        goto exit;

    for (i = 0; i < lHeight; i++)
    {
        MByte *uPtr = srcU + i*lPitch;
        MByte *vPtr = srcV + i*lPitch;
        MByte *srcPtr = pSrcBuf + i*lSrcLineBytes;
        for (j = 0; j < (lWidth >> 1); j++)
        {
            uPtr[j] = srcPtr[2 * j];
            vPtr[j] = srcPtr[2 * j + 1];
        }
    }

    MMemSet(boxRowBuf, 0, ((lWidth >> 1) + lRadius * 2 + 1 + 100) * sizeof(MInt32));
    Image_Box_Stripe(srcU, lWidth >> 1, lHeight, lPitch, resU, lPitch, 0, lHeight, lRadius, boxRowBuf);

    MMemSet(boxRowBuf, 0, ((lWidth >> 1) + lRadius * 2 + 1 + 100) * sizeof(MInt32));
    Image_Box_Stripe(srcV, lWidth >> 1, lHeight, lPitch, resV, lPitch, 0, lHeight, lRadius, boxRowBuf);

    for (i = 0; i < lHeight; i++)
    {
        MByte *uPtr = resU + i*lPitch;
        MByte *vPtr = resV + i*lPitch;
        MByte *dstPtr = pDstBuf + i*lDstLineBytes;
        for (j = 0; j < (lWidth >> 1); j++)
        {
            dstPtr[2 * j] = uPtr[j];
            dstPtr[2 * j + 1] = vPtr[j];
        }
    }
    exit:
    if (srcU)
        MMemFree(MNull, srcU);
    if (srcV)
        MMemFree(MNull, srcV);
    if (resU)
        MMemFree(MNull, resU);
    if (resV)
        MMemFree(MNull, resV);
}

NS_SINFLE_IMAGE_ENHANCEMENT_END