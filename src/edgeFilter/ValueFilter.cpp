#include "ValueFilter.h"
#include "SetLPASVLOFFSCREEN.h"
#include "DefineForDebug.h"
#include "boxfilter.h"
#include <math.h>

USING_NS_SINFLE_IMAGE_ENHANCEMENT

#if defined(DEBUG_OUTPUT)
static cv::Mat LPASVLOFFSCREEN2Mat(LPASVLOFFSCREEN img, int type)
{
    cv::Mat im(img->i32Height, img->i32Width, type, img->ppu8Plane[0], img->pi32Pitch[0]);
    return im;
}
#endif

typedef struct _tag_DENOISE_PARAM
{
    MInt32 chroma;
    MInt32 detail;
    MInt32 smooth;
    MInt32 step;
} DENOISE_PARAM;

typedef struct CNR_Y_Coef
{
    MInt16 smooth_step;
    MInt16 smooth_radius;
    MFloat detail_value;
    MFloat smooth_factor;
    MFloat diff_y_weight;
} CNR_Y_Coef_t;

MInt32 ValueFilterHor(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pTmpImage, LPASVLOFFSCREEN pDstImage, CNR_Y_Coef_t coef, MInt32 lRadius)
{
    MInt32 lRet = 0;
    START_TIME;

    MInt32 lWidth = pDstImage->i32Width;
    MInt32 lHeight = pDstImage->i32Height;
    MInt32 lSrcPitchY = pSrcImage->pi32Pitch[0];
    MInt32 lTmpPitchY = pTmpImage->pi32Pitch[0];
    MInt32 lDstPitchY = pDstImage->pi32Pitch[0];

    for (MInt32 y = 0; y < lHeight; y++)
    {
        auto *pTempY = pSrcImage->ppu8Plane[0] + y * lSrcPitchY;
        auto* pTempTmpY = pTmpImage->ppu8Plane[0] + y * lTmpPitchY;
        auto *pTempDstY = pDstImage->ppu8Plane[0] + y * lDstPitchY;

        MInt32 x = 0;

        for (; x < lWidth; x++)
        {
            auto nCurY = pTempY[x];

            MFloat fSumY = nCurY;
            MFloat fSumWeight = 1.0;

            MFloat fDiff_y_weight = coef.diff_y_weight;

            //对边缘处理，超过边缘的点直接舍弃，不拷贝扩充
            MInt16 nLeftRadius = -lRadius + MAX(0, lRadius - x);
            MInt16 nRightRadius = lRadius - MAX(0, lRadius - (lWidth - 1 - x));
            for (MInt16 i = nLeftRadius; i <= nRightRadius; i += 1)
            {
                if (i == 0)
                {
                    continue;
                }

                MInt16 nIndex = x + i;
                MInt32 nTmpIndex = x + i;
                MFloat diffY = (nCurY - pTempY[nIndex]);
#if 0
                diffY /= 255.0;
                MFloat fSmoothVal = i * i * coef.smooth_factor;

                MFloat fWeight = 1.0 + diffY * diffY * fDiff_y_weight * fSmoothVal;

                fWeight = fWeight * fWeight * fWeight;
                CLAMP(fWeight, 0.0, 1.0);
#else

                MFloat fWeight = exp(-diffY * diffY / 50);

#endif

                fSumWeight += fWeight;
                fSumY += 1.0 * pTempTmpY[nTmpIndex] * fWeight;
            }

            pTempDstY[x] = MIN(255, fSumY / fSumWeight + 0.5);
        }
    }
    END_TIME;
    return lRet;
}

MInt32 ValueFilterVer(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pTmpImage, LPASVLOFFSCREEN pDstImage, CNR_Y_Coef_t coef, MInt32 lRadius)
{
    MInt32 lRet = 0;
    START_TIME;


    MInt32 lWidth = pDstImage->i32Width;
    MInt32 lHeight = pDstImage->i32Height;
    MInt32 lSrcPitchY = pSrcImage->pi32Pitch[0];
    MInt32 lTmpPitchY = pTmpImage->pi32Pitch[0];
    MInt32 lDstPitchY = pDstImage->pi32Pitch[0];

    for (MInt32 y = 0; y < lHeight; y++)
    {
        auto *pTempY = pSrcImage->ppu8Plane[0] + y * lSrcPitchY;
        auto* pTempTmpY = pTmpImage->ppu8Plane[0] + y * lTmpPitchY;
        auto *pTempDstY = pDstImage->ppu8Plane[0] + y * lDstPitchY;

        MInt16 nLeftRadius = -lRadius + MAX(0, lRadius - y);
        MInt16 nRightRadius = lRadius - MAX(0, lRadius - (lHeight - 1 - y));

        MInt32 x = 0;
        for (; x < lWidth; x++)
        {
            auto nCurY = pTempY[x];

            MFloat fSumY = nCurY;
            MFloat fSumWeight = 1.0;

            MFloat fDiff_y_weight = coef.diff_y_weight;

            for (MInt16 i = nLeftRadius; i <= nRightRadius; i += 1)
            {
                if (i == 0)
                {
                    continue;
                }

                MInt32 nIndex = x + i * lSrcPitchY;
                MInt32 nTmpIndex = x + i * lTmpPitchY;
                MFloat diffY = (nCurY - pTempY[nIndex]);
#if 0
                diffY /= 255.0;
                MFloat fSmoothVal = i * i * coef.smooth_factor;

                MFloat fWeight = 1.0 + diffY * diffY * fDiff_y_weight * fSmoothVal;

                fWeight = fWeight * fWeight * fWeight;
                CLAMP(fWeight, 0.0, 1.0);
#else

                MFloat fWeight = exp(-diffY * diffY / 50);

#endif
                fSumWeight += fWeight;
                fSumY += 1.0 * pTempTmpY[nTmpIndex] * fWeight;
            }

            pTempDstY[x] = MIN(255, fSumY / fSumWeight + 0.5);
        }
    }

    END_TIME;
    return lRet;
}

MInt32 ValueFilterRect(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pTmpImage, LPASVLOFFSCREEN pDstImg, CNR_Y_Coef_t coef, MInt32 lRadius)
{
    MInt32 lRet = 0;
    START_TIME;

    MInt32 lWidth = pDstImg->i32Width;
    MInt32 lHeight = pDstImg->i32Height;
    MInt32 lSrcPitch = pSrcImg->pi32Pitch[0];
    MInt32 lTmpPitchY = pTmpImage->pi32Pitch[0];
    MInt32 lDstPitch = pDstImg->pi32Pitch[0];

    MFloat fDiff_y_weight = coef.diff_y_weight;

    for (MInt32 y = 0; y < lHeight; y++)
    {
        auto *pTmpSrc = pSrcImg->ppu8Plane[0] + y * lSrcPitch;
        auto* pTempTmpY = pTmpImage->ppu8Plane[0] + y * lTmpPitchY;
        auto *pTmpDst = pDstImg->ppu8Plane[0] + y * lDstPitch;

        MInt16 nUpRadius = -lRadius + MAX(0, lRadius - y);
        MInt16 nDownRadius = lRadius - MAX(0, lRadius - (lHeight - 1 - y));

        for (MInt32 x = 0; x < lWidth; x++)
        {
            auto nCurY = pTmpSrc[x];

            MFloat fSumY = nCurY * coef.detail_value;
            MFloat fSumWeight = coef.detail_value;

            MInt16 nLeftRadius = -lRadius + MAX(0, lRadius - x);
            MInt16 nRightRadius = lRadius - MAX(0, lRadius - (lWidth - 1 - x));

            for (MInt32 j = nUpRadius; j <= nDownRadius; j++)
            {
                for (MInt32 i = nLeftRadius; i <= nRightRadius; i++)
                {
                    MInt32 nIndex = x + j * lSrcPitch + i;
                    MInt32 nTmpIndex = x + j * lTmpPitchY + i;
                    MFloat diffY = (nCurY - pTmpSrc[nIndex]);
                    diffY /= 255.0;

                    //MFloat fSmoothVal = i * i * coef.smooth_factor;

                    MFloat fWeight = 1.0 + diffY * diffY * fDiff_y_weight;

                    fWeight = fWeight * fWeight * fWeight;
                    CLAMP(fWeight, 0.0, 1.0);

                    fSumWeight += fWeight;
                    fSumY += 1.0 * pTempTmpY[nTmpIndex] * fWeight;
                }
            }

            pTmpDst[x] = MIN(255, fSumY / fSumWeight + 0.5);
        }
    }


    END_TIME;
    return lRet;
}

static MVoid GetCNRChromaCoef(DENOISE_PARAM& param, CNR_Y_Coef_t& coef)
{
    MFloat smooth_value = 0;
    // 计算颜色平滑参数
    if (param.smooth > 50)
    {
        smooth_value = 0.24 * (param.smooth - 50) + 12.0;
    }
    else
    {
        smooth_value = 0.12 * param.smooth + 6.0;
    }
    coef.smooth_step = param.step;
    //smooth_value *= coef.smooth_step;
    coef.smooth_factor = 1.0f / (smooth_value * smooth_value);
    // 基于颜色平滑参数得到的平滑半径
    coef.smooth_radius = (smooth_value / coef.smooth_step) * coef.smooth_step;

    //颜色参数计算
    MFloat chroma_val = MAX(0.0, MIN(1.0, param.chroma * 0.01));
    chroma_val = chroma_val * 0.027;// *1.2;

    coef.diff_y_weight = -1.0 / (chroma_val * chroma_val);

    //颜色细节参数计算
    MFloat detail_vale = MAX(0.0, MIN(1.0, param.detail * 0.01));
    coef.detail_value = detail_vale * detail_vale;
}

MInt32 ValueFilter(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, MFloat fValue)
{
    MInt32 lRet = 0;
    START_TIME;

    ASVLOFFSCREEN tmp, tmp2;
    MInt32 lWidth = pDstImage->i32Width;
    MInt32 lHeight = pDstImage->i32Height;
    lRet = AllocLPASVLOFFSCREEN(hMemMgr, &tmp, lWidth, lHeight, lWidth, ASVL_PAF_GRAY);
    lRet = AllocLPASVLOFFSCREEN(hMemMgr, &tmp2, lWidth, lHeight, lWidth, ASVL_PAF_GRAY);
    CHECK_ERROR(lRet);

    MInt32 lRadius = 3;
    DENOISE_PARAM param;
    param.chroma = fValue;
    param.smooth = fValue * fValue / 100;
    param.detail = 50 - param.smooth / 2;
    CNR_Y_Coef_t coef;
    GetCNRChromaCoef(param, coef);
    coef.diff_y_weight = -5000.0 / fValue;
    coef.detail_value = 10.0;
#if 0
    ValueFilterHor(pSrcImage, pSrcImage, &tmp, coef, lRadius);
    ValueFilterVer(pSrcImage, &tmp, pDstImage, coef, lRadius);
#else
    //ValueFilterVer(pSrcImage, pSrcImage, &tmp, coef, lRadius);
    //ValueFilterHor(pSrcImage, &tmp, pDstImage, coef, lRadius);
#endif
    //Box_Filter_C1(hMemMgr, mcvParallelMonitor, pSrcImage, &tmp2, 1);
    ValueFilterRect(pSrcImage, pSrcImage, &tmp, coef, lRadius);
    CopyY(pDstImage, &tmp);

#if defined(DEBUG_OUTPUT0)
    cv::Mat dst = LPASVLOFFSCREEN2Mat(&tmp, CV_8UC1);
    cv::Mat src = LPASVLOFFSCREEN2Mat(pSrcImage, CV_8UC1);

    cv::bilateralFilter(src, dst, 9, 5, 5);
    CopyY(pDstImage, &tmp);
#endif

    FreeLPASVLOFFSCREEN(hMemMgr, &tmp);
    FreeLPASVLOFFSCREEN(hMemMgr, &tmp2);

    END_TIME;
    return lRet;
}