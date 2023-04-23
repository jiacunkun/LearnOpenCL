//
// Created by jck7075 on 2020/4/13.
//
#include "convYUVToBGR.h"
#include "merror.h"
#include "mobilecv.h"
#include "imagebase.h"
#include "single_image_enhancement_define.h"

#if defined(USE_NEON) || defined(__ARM_NEON__)
#if defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
// 函数实现
    MVoid localConvYUVToBGR(MUInt8 *pY, MInt32 lStepY, MUInt8 *pUV, MInt32 lStepUV,
                            MUInt8 *pBGR, MInt32 lStepBGR, MInt32 lWidth, MInt32 lStart, MInt32 lEnd)
    {

        MUInt8 *pYRow = MNull, *pYNextRow = MNull, *pUVRow = MNull;
        MUInt8 *pBGRRow = MNull, *pBGRNextRow = MNull;
        MInt32 i = 0, j = 0;
        MInt32 cr = 0, cb = 0;
        MInt32 y00 = 0, y01 = 0;
        MInt32 y10 = 0, y11 = 0;
        MInt32 offset = 0;

#ifdef __ARM_NEON__
        uint8x8_t ydata_8x8;
	uint8x8x2_t uvdata_8x8x2;
	uint8x8x3_t resdata;
	int16x8_t tmpdata, udata, vdata, combdata;
	int32x4_t ylowdata_32x4, yhighdata_32x4, lowsumdata, highsumdata;
	int16x4_t crdata_16x4, cbdata_16x4, lowdata, highdata;
	int16x4x2_t zip_cr, zip_cb;
	int16x8_t combine_cr, combine_cb;
	int32x4_t Gcrlowdata, Gcrhighdata, Rcrlowdata, Rcrhighdata;
	int32x4_t Bcblowdata, Bcbhighdata, Gcblowdata, Gcbhighdata;
#endif

        for (i = lStart; i < lEnd; i += 1)
        {
            pYRow = pY + i * lStepY;
            pUVRow = pUV + i * lStepUV;
            pBGRRow = pBGR + i * lStepBGR;

            j = 0;
#ifdef __ARM_NEON__
            for (; j < lWidth - 8; j += 8)
		{
			offset = j * 3;

			ydata_8x8 = vld1_u8(pYRow + j);
			tmpdata = vreinterpretq_s16_u16(vmovl_u8(ydata_8x8));
			ylowdata_32x4 = vshll_n_s16(vget_low_s16(tmpdata), yuv_shift);
			yhighdata_32x4 = vshll_n_s16(vget_high_s16(tmpdata), yuv_shift);

			uvdata_8x8x2 = vld2_u8(pUVRow + 2*j);
			udata = vreinterpretq_s16_u16(vmovl_u8(uvdata_8x8x2.val[0]));
			vdata = vreinterpretq_s16_u16(vmovl_u8(uvdata_8x8x2.val[1]));
			combine_cr = vsubq_s16(udata, vdupq_n_s16(128));
			combine_cb = vsubq_s16(vdata, vdupq_n_s16(128));

			Bcblowdata = vmull_s16(vget_low_s16(combine_cb), vdup_n_s16(yuvBCb));
			Bcbhighdata = vmull_s16(vget_high_s16(combine_cb), vdup_n_s16(yuvBCb));
			Gcblowdata = vmull_s16(vget_low_s16(combine_cb), vdup_n_s16(yuvGCb));
			Gcbhighdata = vmull_s16(vget_high_s16(combine_cb), vdup_n_s16(yuvGCb));
			Gcrlowdata = vmull_s16(vget_low_s16(combine_cr), vdup_n_s16(yuvGCr));
			Gcrhighdata = vmull_s16(vget_high_s16(combine_cr), vdup_n_s16(yuvGCr));
			Rcrlowdata = vmull_s16(vget_low_s16(combine_cr), vdup_n_s16(yuvRCr));
			Rcrhighdata = vmull_s16(vget_high_s16(combine_cr), vdup_n_s16(yuvRCr));

			lowsumdata = vaddq_s32(ylowdata_32x4, Bcblowdata);
			lowdata = vrshrn_n_s32(lowsumdata, yuv_shift);
			highsumdata = vaddq_s32(yhighdata_32x4, Bcbhighdata);
			highdata = vrshrn_n_s32(highsumdata, yuv_shift);
			combdata = vcombine_s16(lowdata, highdata);
			resdata.val[0] = vqmovun_s16(combdata);

			lowsumdata = vaddq_s32(ylowdata_32x4, Gcblowdata);
			lowsumdata = vaddq_s32(lowsumdata, Gcrlowdata);
			lowdata = vrshrn_n_s32(lowsumdata, yuv_shift);
			highsumdata = vaddq_s32(yhighdata_32x4, Gcbhighdata);
			highsumdata = vaddq_s32(highsumdata, Gcrhighdata);
			highdata = vrshrn_n_s32(highsumdata, yuv_shift);
			combdata = vcombine_s16(lowdata, highdata);
			resdata.val[1] = vqmovun_s16(combdata);

			lowsumdata = vaddq_s32(ylowdata_32x4, Rcrlowdata);
			lowdata = vrshrn_n_s32(lowsumdata, yuv_shift);
			highsumdata = vaddq_s32(yhighdata_32x4, Rcrhighdata);
			highdata = vrshrn_n_s32(highsumdata, yuv_shift);
			combdata = vcombine_s16(lowdata, highdata);
			resdata.val[2] = vqmovun_s16(combdata);
			vst3_u8(pBGRRow + offset, resdata);
		}
#endif
            for (; j < lWidth; j += 1)
            {
                offset = j * 3;

                y00 = pYRow[j];
                cr = (MInt32)pUVRow[2*j] - 128;
                cb = (MInt32)pUVRow[2*j + 1] - 128;

                y00 = yuv_prescale(y00);

                pBGRRow[offset + 0] = ET_YUV_TO_B_2(y00, cb);
                pBGRRow[offset + 1] = ET_YUV_TO_G_2(y00, cb, cr);
                pBGRRow[offset + 2] = ET_YUV_TO_R_2(y00, cr);
            }
        }
    }

    MVoid thread_localConvYUVToBGR(MVoid *para)
    {
        ConvYUVToBGRPara *pPara = (ConvYUVToBGRPara *)para;
        localConvYUVToBGR(pPara->pY, pPara->lStepY, pPara->pUV, pPara->lStepUV, pPara->pBGR,
                          pPara->lStepBGR, pPara->lWidth, pPara->lStart, pPara->lEnd);
    }

    MRESULT convYUVToBGR(MHandle mcvParallelMonitor, MByte *pYdata, MByte *pUVdata, MInt32 lYPitch, MInt32 lUVPitch, LPASVLOFFSCREEN pDstBGR)
    {
        MRESULT lret = MOK;
        MInt32 lWidth = 0, lHeight = 0;
        MUInt8 *pY = MNull, *pUV = MNull;
        MUInt8 *pBGR = MNull;
        MInt32 lStepY = 0, lStepUV = 0;
        MInt32 lStepBGR = 0;

        MInt32 lBlockH = 0;
        const MInt32 taskNum = 8;
        ConvYUVToBGRPara para[taskNum] = { MNull };
        MInt32 taskId[taskNum] = { MNull };
        MInt32 k = 0;

        if (!pDstBGR || !pYdata || !pUVdata
            || pDstBGR->u32PixelArrayFormat != ASVL_PAF_RGB24_B8G8R8)
            return MERR_INVALID_PARAM;

        lWidth = pDstBGR->i32Width;
        lHeight = pDstBGR->i32Height;

        pY = pYdata;
        pUV = pUVdata;
        pBGR = pDstBGR->ppu8Plane[0];

        lStepY = lYPitch;
        lStepUV = lUVPitch;
        lStepBGR = pDstBGR->pi32Pitch[0];

        lBlockH = (lHeight / 8) >> 1 << 1;

        for (k = 0; k < taskNum; ++k)
        {
            para[k].pY = pY;
            para[k].lStepY = lStepY;
            para[k].pUV = pUV;
            para[k].lStepUV = lStepUV;
            para[k].pBGR = pBGR;
            para[k].lStepBGR = lStepBGR;
            para[k].lWidth = lWidth;
            para[k].lStart = k * lBlockH;
            para[k].lEnd = (k + 1) * lBlockH;
        }
        para[taskNum - 1].lEnd = lHeight;

        for (k = 0; k < taskNum; ++k)
        {
            taskId[k] = mcvAddTask(mcvParallelMonitor, thread_localConvYUVToBGR, (MVoid *)&para[k]);
        }

        for (k = 0; k < taskNum; ++k)
        {
            lret = mcvWaitTask(mcvParallelMonitor, taskId[k]);
            if (MOK != lret)
            {
                lret = MERR_BAD_STATE;
                return lret;
            }
        }

        return lret;
    }

#define	yuv_shift		14
#define	yuv_fix(x)		(MInt32)((x) * (1 << (yuv_shift)) + 0.5f)
#define	yuv_descale(x)	(((x) + (1 << ((yuv_shift)-1))) >> (yuv_shift))
#define	yuv_prescale(x)	((x) << yuv_shift)

#define	yuvYr	yuv_fix(0.299f)
#define	yuvYg	yuv_fix(0.587f)
#define	yuvYb	yuv_fix(0.114f)
#define	yuvCr	yuv_fix(0.713f)
#define	yuvCb	yuv_fix(0.564f)

#define	yuvRCr	yuv_fix(1.403f)
#define	yuvGCr	(-yuv_fix(0.714f))
#define	yuvGCb	(-yuv_fix(0.344f))
#define	yuvBCb	yuv_fix(1.773f)

    MVoid BGRToYUV444Planar(MByte* pBGR, MInt32 nW, MInt32 nH, MInt32 nBGRStride,
                            MByte* pY, MInt32 lYStride, MByte* pU, MInt32 lUStride, MByte* pV, MInt32 lVStride)
    {
        for (MInt32 y = 0; y < nH; y++)
        {
            MByte* pbSrcX = pBGR + y * nBGRStride;
            MByte* pbDstY = pY + y * lYStride;
            MByte* pbDstU = pU + y * lUStride;
            MByte* pbDstV = pV + y * lVStride;

            for (MInt32 x = 0; x < nW; x++)
            {
                MLong r, g, b, y0, cb, cr;

                b = pbSrcX[0];
                g = pbSrcX[1];
                r = pbSrcX[2];
                y0 = yuv_descale(b * yuvYb + g * yuvYg + r * yuvYr);
                cb = yuv_descale((b - y0) * yuvCb) + 128;
                cr = yuv_descale((r - y0) * yuvCr) + 128;

                if (pY)
                {
                    pbDstY[0] = ET_CAST_8U(y0);
                }

                pbDstU[0] = ET_CAST_8U(cr);
                pbDstV[0] = ET_CAST_8U(cb);

                pbSrcX += 3;
                pbDstY++;
                pbDstU++;
                pbDstV++;
            }
        }
    }

    MVoid YUV444ToBGRPlanar(MByte* pY, MInt32 lYStride, MByte* pU, MInt32 lUStride, MByte* pV, MInt32 lVStride,
                            MByte* pBGR, MInt32 nW, MInt32 nH, MInt32 nBGRStride)
    {
        for (MInt32 y = 0; y < nH; y++)
        {
            MByte* pbSrcY = pY + y * lYStride;
            MByte* pbSrcU = pU + y * lUStride;
            MByte* pbSrcV = pV + y * lVStride;
            MByte* pbDstX = pBGR + y * nBGRStride;

            for (MInt32 x = 0; x < nW; x+=1)
            {
                MLong r, g, b, y0, cb, cr;

                y0 = pbSrcY[x];
                cb = pbSrcU[x];
                cr = pbSrcV[x];

				b = y0 + 1.403*(cr - 128) + 0.5;
				g = y0 - 0.344*(cb - 128) - 0.714*(cr - 128) + 0.5;
				r = y0 + 1.770*(cb - 128) + 0.5;

                pbDstX[0] = ET_CAST_8U(b);
                pbDstX[1] = ET_CAST_8U(g);
                pbDstX[2] = ET_CAST_8U(r);

                pbDstX += 3;
            }
        }
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END


