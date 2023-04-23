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
#include "remove_block_noise.h"
#include <math.h>
#include <time.h>

#include "img_interpolation.h"
#include "defcompilesetting.h"
#include "adlbase.h"
#include "imagebase.h"
#include "wavelet_LL.h"
#include "BoxFilterNS.h"
#include "ArcsoftLog.h"
#include "DefineForDebug.h"

#define _INT_A_SHIFT_ (15)

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN


    static MInt32 find_min_val(MHandle hMemMgr, MInt32 *cov_Ip, MInt32 pixels)
    {
        MInt32 lret = 15;
        MInt32 thr_num = pixels * 15 * 0.01;
        MInt32 *Vhist = MNull;
        MInt32 max_num = 0;
        MInt32 lnum;

        for (lnum = 0; lnum < pixels; lnum++)
        {
            MInt32 lval = (MInt32) ((MFloat) (ABS(cov_Ip[lnum])) * 0.25 + 0.5);
            max_num = MAX(lval, max_num);
        }
        Vhist = (MInt32 *) MMemAlloc(hMemMgr, (max_num + 1) * sizeof(MInt32));
        if (MNull == Vhist)
        {
            return MERR_NO_MEMORY;
        }
        MMemSet(Vhist, 0, (max_num + 1) * sizeof(MInt32));
        for (lnum = 0; lnum < pixels; lnum++)
        {
            MInt32 lval = (MInt32) ((MFloat) (ABS(cov_Ip[lnum])) * 0.25 + 0.5);
            Vhist[lval]++;
        }

        for (lnum = 0; lnum < max_num; lnum++)
        {
            if (thr_num < Vhist[lnum])
            {
                break;
            }
            thr_num -= Vhist[lnum];
        }
        return lnum * 4;
    }

/**
 *
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param I [in] 引导图像
 * @param p_data [in,out] 原图输入，结果输出
 * @param lWidth
 * @param lHeight
 * @param r
 * @param eps
 * @return
 */
    MLong fastGuidedFilter_32I(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *I, MInt32 *p_data, MInt32 lWidth,
                               MInt32 lHeight, MInt32 r, MFloat eps)
    {
        MLong res = MOK;
        MInt64 tmpVal = 0;
        MInt32 s = 1; //在下采样尺度为s时计算a,b矩阵,默认为1，再大可能出问题
        MInt32 sHeight = lHeight >> s, sWidth = lWidth >> s;
        MInt32 i, pixels = sHeight * sWidth, upPxls = lHeight * lWidth;
        const MInt32 shiftInteger = 1 << _INT_A_SHIFT_;

        MInt32 *I_data = (MInt32 *) MMemAlloc(hMemMgr, upPxls * sizeof(MInt32));
        MInt32 *up_mean_a = (MInt32 *) MMemAlloc(hMemMgr, upPxls * sizeof(MInt32));
        MInt32 *up_mean_b = (MInt32 *) MMemAlloc(hMemMgr, upPxls * sizeof(MInt32));
        MInt32 *subImgMem = (MInt32 *) MMemAlloc(hMemMgr, 8 * pixels * sizeof(MInt32));
        MInt32 *subI = MNull, *minI = MNull, *mean_I = MNull, *mean_minI = MNull, *minII = MNull, *mean_minI_mul = MNull, *mean_minII = MNull, *var_I = MNull;
        MInt32 *subp = MNull, *minp = MNull, *mean_p = MNull, *mean_minp = MNull, *minIp = MNull, *mean_minp_mul = MNull, *mean_minIp = MNull, *cov_Ip = MNull;
        MInt32 *a = MNull, *b = MNull, *mean_a = MNull, *mean_b = MNull;
        if (I_data == MNull || up_mean_a == MNull || up_mean_b == MNull || subImgMem == MNull)
        {
            res = MERR_NO_MEMORY;
            goto exit;
        }
        i = 0;
#if CV_NEON
        {
            int16x8_t tmpdata_16x8;
            int16x4_t lowdata_16x4, highdata_16x4;
            int32x4_t resdata_32x4;

            for (i = 0; i < upPxls - 7; i += 8)
            {
                tmpdata_16x8 = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(I + i)));
                lowdata_16x4 = vget_low_s16(tmpdata_16x8);
                highdata_16x4 = vget_high_s16(tmpdata_16x8);

                resdata_32x4 = vshll_n_s16(lowdata_16x4, 12);
                vst1q_s32(I_data + i, resdata_32x4);

                resdata_32x4 = vshll_n_s16(highdata_16x4, 12);
                vst1q_s32(I_data + i + 4, resdata_32x4);
            }
        }
#endif
        for (; i < upPxls; ++i)
        {
            I_data[i] = (MInt32) (I[i]) << 12;
        }

        subI = subImgMem;
        minI = subImgMem + pixels;
        mean_I = subImgMem + 2 * pixels;
        mean_minI = subImgMem + 3 * pixels;
        minII = minI;
        mean_minI_mul = mean_minI;
        mean_minII = subI;
        var_I = mean_minII;
        subp = subImgMem + 4 * pixels;
        minp = subImgMem + 5 * pixels;
        mean_p = subImgMem + 6 * pixels;
        mean_minp = subImgMem + 7 * pixels;
        minIp = minp;
        mean_minp_mul = mean_minp;
        mean_minIp = subp;
        cov_Ip = mean_minIp;

        a = minI;
        b = minp;
        mean_a = mean_I;
        mean_b = mean_p;

        res = fastBilinearInter_32I(mcvParallelMonitor, I_data, subI, lHeight, lWidth, 0, s);
        if (MOK != res)
            goto exit;
        //mean_I = boxfilter(I, r)
        res = Box_Filter_NS32I(hMemMgr, mcvParallelMonitor, subI, sWidth, sHeight, sWidth, mean_I, sWidth, r >> 1);
        if (MOK != res)
            goto exit;

        i = 0;
#if CV_NEON
        {
            int32x4_t tmpdata_32x4;
            int32x4_t resdata_32x4;

            for (i = 0; i < pixels - 3; i += 4)
            {
                tmpdata_32x4 = vld1q_s32(subI + i);
                resdata_32x4 = vshrq_n_s32(tmpdata_32x4, 10);
                vst1q_s32(minI + i, resdata_32x4);
            }
        }
#endif
        for (; i < pixels; ++i)
        {
            minI[i] = (subI[i] >> 10);
        }

        res = Box_Filter_NS32I(hMemMgr, mcvParallelMonitor, minI, sWidth, sHeight, sWidth, mean_minI, sWidth, r >> 1);
        if (res != MOK)
            goto exit;

        res = fastBilinearInter_32I(mcvParallelMonitor, p_data, subp, lHeight, lWidth, 0, s);
        if (MOK != res)
            goto exit;
        //mean_p = boxfilter(p, r)
        res = Box_Filter_NS32I(hMemMgr, mcvParallelMonitor, subp, sWidth, sHeight, sWidth, mean_p, sWidth, r >> 1);
        if (MOK != res)
            goto exit;

        i = 0;
#if CV_NEON
        {
            int32x4_t tmpdata_32x4;
            int32x4_t resdata_32x4;

            for (i = 0; i < pixels - 3; i += 4)
            {
                tmpdata_32x4 = vld1q_s32(subp + i);
                resdata_32x4 = vshrq_n_s32(tmpdata_32x4, 10);
                vst1q_s32(minp + i, resdata_32x4);
            }
        }
#endif

        for (; i < pixels; ++i)
        {
            minp[i] = (subp[i] >> 10);
        }
        res = Box_Filter_NS32I(hMemMgr, mcvParallelMonitor, minp, sWidth, sHeight, sWidth, mean_minp, sWidth, r >> 1);
        if (MOK != res)
            goto exit;

        i = 0;
#if CV_NEON
        {
            int32x4_t tmpdata00_32x4, tmpdata01_32x4;
            int32x4_t resdata_32x4;

            for (i = 0; i < pixels - 3; i += 4)
            {
                tmpdata00_32x4 = vld1q_s32(minI + i);
                tmpdata01_32x4 = vld1q_s32(minp + i);
                resdata_32x4 = vmulq_s32(tmpdata00_32x4, tmpdata01_32x4);
                vst1q_s32(minIp + i, resdata_32x4);
            }
        }
#endif

        for (; i < pixels; ++i)
        {
            minIp[i] = minI[i] * minp[i];
        }

        i = 0;
#if CV_NEON
        {
            int32x4_t tmpdata_32x4, resdata_32x4;

            for (i = 0; i < pixels - 3; i += 4)
            {
                tmpdata_32x4 = vld1q_s32(minI + i);
                resdata_32x4 = vmulq_s32(tmpdata_32x4, tmpdata_32x4);
                vst1q_s32(minII + i, resdata_32x4);
            }
        }
#endif
        for (; i < pixels; ++i)
        {
            minII[i] = minI[i] * minI[i];
        }

        i = 0;
#if CV_NEON
        {
            int32x4_t tmpdata00_32x4, tmpdata01_32x4;
            int32x4_t resdata_32x4;

            for (i = 0; i < pixels - 3; i += 4)
            {
                tmpdata00_32x4 = vld1q_s32(mean_minI + i);
                tmpdata01_32x4 = vld1q_s32(mean_minp + i);
                resdata_32x4 = vmulq_s32(tmpdata00_32x4, tmpdata01_32x4);
                vst1q_s32(mean_minp_mul + i, resdata_32x4);
            }
        }
#endif
        for (; i < pixels; ++i)
        {
            mean_minp_mul[i] = mean_minI[i] * mean_minp[i];
        }

        i = 0;
#if CV_NEON
        {
            int32x4_t tmpdata_32x4, resdata_32x4;

            for (i = 0; i < pixels - 3; i += 4)
            {
                tmpdata_32x4 = vld1q_s32(mean_minI + i);
                resdata_32x4 = vmulq_s32(tmpdata_32x4, tmpdata_32x4);
                vst1q_s32(mean_minI_mul + i, resdata_32x4);
            }
        }
#endif
        for (; i < pixels; ++i)
        {
            mean_minI_mul[i] = mean_minI[i] * mean_minI[i];
        }

        //mean_II = boxfilter(I.*I, r)
        res = Box_Filter_NS32I(hMemMgr, mcvParallelMonitor, minII, sWidth, sHeight, sWidth, mean_minII, sWidth, r >> 1);
        if (MOK != res)
            goto exit;
        //mean_Ip = boxfilter(I.*p, r)
        res = Box_Filter_NS32I(hMemMgr, mcvParallelMonitor, minIp, sWidth, sHeight, sWidth, mean_minIp, sWidth, r >> 1);
        if (MOK != res)
            goto exit;

        i = 0;
#if CV_NEON
        {
            int32x4_t tmpdata00_32x4, tmpdata01_32x4;
            int32x4_t resdata_32x4;

            for (i = 0; i < pixels - 3; i += 4)
            {
                tmpdata00_32x4 = vld1q_s32(mean_minII + i);
                tmpdata01_32x4 = vld1q_s32(mean_minI_mul + i);
                resdata_32x4 = vsubq_s32(tmpdata00_32x4, tmpdata01_32x4);
                vst1q_s32(var_I + i, resdata_32x4);
            }
        }
#endif
        //var_I = mean_II - mean_I .* mean_I;
        for (; i < pixels; ++i)
        {
            var_I[i] = (mean_minII[i] - mean_minI_mul[i]);
        }

        i = 0;
#if CV_NEON
        {
            int32x4_t tmpdata00_32x4, tmpdata01_32x4;
            int32x4_t resdata_32x4;

            for (i = 0; i < pixels - 3; i += 4)
            {
                tmpdata00_32x4 = vld1q_s32(mean_minIp + i);
                tmpdata01_32x4 = vld1q_s32(mean_minp_mul + i);
                resdata_32x4 = vsubq_s32(tmpdata00_32x4, tmpdata01_32x4);
                vst1q_s32(cov_Ip + i, resdata_32x4);
            }
        }
#endif
        //cov_Ip = mean_Ip - mean_I .* mean_p; % this is the covariance of (I, p) in each local patch.
        for (; i < pixels; ++i)
        {
            cov_Ip[i] = (mean_minIp[i] - mean_minp_mul[i]);
        }

        //自适应一个eps值
        //取值为var_I的最小的%15的值得2倍
        if (eps < 0)
        {
            MInt32 leps = 0;
            leps = find_min_val(hMemMgr, cov_Ip, pixels);
            if (MERR_NO_MEMORY == leps)
            {
                goto exit;
            }
            eps = leps * 2;
        }

        //a = cov_Ip ./ (var_I + eps);
        for (i = 0; i < pixels; ++i)
        {
            MFloat lval = var_I[i] + eps;
            if (lval * 10 < cov_Ip[i])
            {
                lval = cov_Ip[i] * 0.1;
            }
            a[i] = shiftInteger * (cov_Ip[i] / lval);
        }
        //b = mean_p - a .* mean_I;
        for (i = 0; i < pixels; ++i)
        {
            tmpVal = (MInt64) (mean_I[i]) * (MInt64) (a[i]);
            b[i] = mean_p[i] - (MInt32) (tmpVal >> _INT_A_SHIFT_);
        }

        //mean_a = boxfilter(a, r)
        res = Box_Filter_NS32I(hMemMgr, mcvParallelMonitor, a, sWidth, sHeight, sWidth, mean_a, sWidth, r >> 1);
        if (MOK != res)
            goto exit;
        //mean_b = boxfilter(b, r)
        res = Box_Filter_NS32I(hMemMgr, mcvParallelMonitor, b, sWidth, sHeight, sWidth, mean_b, sWidth, r >> 1);
        if (MOK != res)
            goto exit;

        //upSample(mean_a)    upSample(mean_b)
        res = fastBilinearInter_32I(mcvParallelMonitor, mean_a, up_mean_a, lHeight, lWidth, s, 0);
        if (MOK != res)
            goto exit;
        res = fastBilinearInter_32I(mcvParallelMonitor, mean_b, up_mean_b, lHeight, lWidth, s, 0);
        if (MOK != res)
            goto exit;

#ifdef _WIN32_DEBUG_
        {
            img_save(p_data, lWidth, lHeight, 12, "before_fastres");
        }
#endif

        //q = mean_a .* I + mean_b;
        for (i = 0; i < upPxls; ++i)
        {
            tmpVal = (MInt64) (up_mean_a[i]) * (MInt64) (I_data[i]);
            p_data[i] = (MInt32) (tmpVal >> _INT_A_SHIFT_) + up_mean_b[i];
        }

#ifdef _WIN32_DEBUG_
        {
            img_save(p_data, lWidth, lHeight, 12, "fast_res");
        }
#endif

        exit:
        {
#if defined(_ARM_TIME_)
            MLong lTime;
            START_PROFILE();
#endif
            if (I_data)
            {
                MMemFree(hMemMgr, I_data);
                I_data = MNull;
            }
            if (up_mean_a)
            {
                MMemFree(hMemMgr, up_mean_a);
                up_mean_a = MNull;
            }
            if (up_mean_b)
            {
                MMemFree(hMemMgr, up_mean_b);
                up_mean_b = MNull;
            }
            if (subImgMem)
            {
                MMemFree(hMemMgr, subImgMem);
                subImgMem = MNull;
            }
#if defined(_ARM_TIME_)
            END_PROFILE(lTime);
            PrintfB(6, "VNS", "aveDoEnhancement FastGuidedFilter MMemFree() consume time = %d", lTime);
#endif
        }
        return res;
    }



/**
 *
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pGuidedImage [in 引导图像]
 * @param pGuidedImgWlDecom
 * @param delight [in,out  输入和输出图像]
 * @param dwtLevels
 * @param param
 * @return
 */
    MLong RemoveBlockNoise_LLwavelet_optimize(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pGuidedImage,
                                              WAVELET_LPDATA_8U pGuidedImgWlDecom, LPASVLOFFSCREEN delight,
                                              MInt32 dwtLevels, BASE_SINGLE_IMAGE_PARAM param)
    {
        LOGD("RemoveBlockNoise_LLwavelet_optimize++");
        MLong res = MOK;
        MInt32 i, w, h, x, y, idx = 0, lWidth = delight->i32Width, lHeight = delight->i32Height;
        MInt32 area, pixelNums;
        MFloat eps = (param.lYReprocIntensity < 0) ? 2 * 7 : ((param.lYReprocIntensity > 20) ? 20 * 7 :
                                                              param.lYReprocIntensity * 7);
        MInt32 *detailGuided = MNull, *wlDelight = MNull;
        MByte *wlSrc = MNull;
        MByte *delight_data = delight->ppu8Plane[0];
        MInt32 delight_step = delight->pi32Pitch[0];
        bool bSrcWlDecom = false;

        WAVELET_LPDATA delightWlBuf = (WAVELET_LPDATA) MMemAlloc(hMemMgr, dwtLevels * sizeof(WAVELET_DATA));
        for (i = 0; i < dwtLevels; ++i)
        {
            delightWlBuf[i].h = lHeight >> (i + 1);
            delightWlBuf[i].w = lWidth >> (i + 1);
            delightWlBuf[i].LL_data = (MInt32 *) MMemAlloc(hMemMgr, delightWlBuf[i].h * delightWlBuf[i].w * sizeof(MInt32));
            if (delightWlBuf[i].LL_data == MNull)
            {
                res = MERR_NO_MEMORY;
                goto exit;
            }
        }
        res = dwt_LLNS(hMemMgr, mcvParallelMonitor, delight, delightWlBuf, dwtLevels);
        if (res != MOK)
            goto exit;


        if (MNull == pGuidedImgWlDecom)
        {
            if (MNull == pGuidedImage)
            {
                res = MERR_INVALID_PARAM;
                goto exit;
            }
            pGuidedImgWlDecom = (WAVELET_LPDATA_8U) MMemAlloc(hMemMgr, dwtLevels * sizeof(WAVELET_DATA_8U));;
            bSrcWlDecom = true;
            for (i = 0; i < dwtLevels; ++i)
            {
                pGuidedImgWlDecom[i].h = lHeight >> (i + 1);
                pGuidedImgWlDecom[i].w = lWidth >> (i + 1);
                pGuidedImgWlDecom[i].LL_data = (MByte *) MMemAlloc(hMemMgr,
                                                                   pGuidedImgWlDecom[i].h * pGuidedImgWlDecom[i].w *
                                                                   sizeof(MByte));
                if (pGuidedImgWlDecom[i].LL_data == MNull)
                {
                    res = MERR_NO_MEMORY;
                    goto exit;
                }
            }
            //only do low-frequency wavelet decomposition
            res = rough_dwt_LLNS(hMemMgr, mcvParallelMonitor, pGuidedImage, pGuidedImgWlDecom, dwtLevels);
        }

        w = lWidth >> dwtLevels;
        h = lHeight >> dwtLevels;
        pixelNums = w * h;

        detailGuided = (MInt32 *) MMemAlloc(hMemMgr, w * h * sizeof(MInt32));
        if (detailGuided == MNull)
        {
            res = MERR_NO_MEMORY;
            goto exit;
        }

        // 在最小层做引导滤波
        wlSrc = pGuidedImgWlDecom[dwtLevels - 1].LL_data;
        wlDelight = delightWlBuf[dwtLevels - 1].LL_data;
        MMemCpy(detailGuided, wlDelight, pixelNums * sizeof(MInt32));

        {
            res = fastGuidedFilter_32I(hMemMgr, mcvParallelMonitor, wlSrc, wlDelight, w, h, 9, eps);
            if (res != MOK)
                goto exit;
        }

        //低频部分做导向图前后的差
        i = 0;
#if CV_NEON
        {
            int32x4_t tmpdata00_32x4, tmpdata01_32x4;
            int32x4_t resdata_32x4;

            for (i = 0; i < pixelNums - 3; i += 4)
            {
                tmpdata00_32x4 = vld1q_s32(detailGuided + i);
                tmpdata01_32x4 = vld1q_s32(wlDelight + i);
                resdata_32x4 = vsubq_s32(tmpdata00_32x4, tmpdata01_32x4);
                vst1q_s32(wlDelight + i, resdata_32x4);
            }
        }
#endif
        for (; i < pixelNums; ++i)
        {
            wlDelight[i] = detailGuided[i] - wlDelight[i];
        }

        {
#if defined(_ARM_TIME_)
            MLong lTime;
            START_PROFILE();
#endif
            res = idwt_LLNS(hMemMgr, mcvParallelMonitor, delightWlBuf, lHeight, lWidth, dwtLevels, delight);

            if (res != MOK)
                goto exit;
#if defined(_ARM_TIME_)
            END_PROFILE(lTime);
            PrintfB(6, "VNS", "aveDoEnhancement idwt_LL_RECON consume time = %d\r\n", lTime);
#endif
        }

        exit:
        {
#if defined(_ARM_TIME_)
            MLong lTime;
            START_PROFILE();
#endif
            for (i = 0; i < dwtLevels; ++i)
            {
                if (delightWlBuf[i].LL_data)
                {
                    MMemFree(hMemMgr, delightWlBuf[i].LL_data);
                    delightWlBuf[i].LL_data = MNull;
                }
            }

            if (detailGuided)
            {
                MMemFree(hMemMgr, detailGuided);
                detailGuided = MNull;
            }
            if (delightWlBuf)
            {
                MMemFree(hMemMgr, delightWlBuf);
                delightWlBuf = MNull;
            }
            if (bSrcWlDecom)
            {
                for (i = 0; i < dwtLevels; ++i)
                {
                    MMemFree(hMemMgr, pGuidedImgWlDecom[i].LL_data);
                    pGuidedImgWlDecom[i].LL_data = MNull;
                }
                MMemFree(hMemMgr, pGuidedImgWlDecom);
                pGuidedImgWlDecom = MNull;
            }
#if defined(_ARM_TIME_)
            END_PROFILE(lTime);
            PrintfB(6, "VNS", "aveDoEnhancement MMeMFree() consume time = %d\r\n", lTime);
#endif
        }
        LOGD("RemoveBlockNoise_LLwavelet_optimize--");
        return res;
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END

