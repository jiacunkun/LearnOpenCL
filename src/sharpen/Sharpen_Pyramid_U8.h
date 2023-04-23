#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_SHARPEN_PYRAMID_U8_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_SHARPEN_PYRAMID_U8_H

#include "single_image_enhancement_define.h"
#include <asvloffscreen.h>

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

#define MAX_PYRAMID_LAYER 4

class Sharpen_Pyramid_U8
{
public:
    Sharpen_Pyramid_U8();
    ~Sharpen_Pyramid_U8();

    MInt32 init(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPryLayer);
    MVoid release();

    MInt32 run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers);
    MInt32 run_nlm(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers);
    MInt32 run_nlm2(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers);

private:
    MInt32 GetDetail(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDetail);
    MInt32 FilterDetail(MInt32 lFilterVal, MInt32 index);
    MInt32 GetMask(MHandle hMemMgr, MHandle mcvParallelMonitor,
        LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDetailMask, MInt32 lMaskRange);
    MVoid FillExpandPixels(LPASVLOFFSCREEN pSrcDst, MInt32 lExpandSize);
    MVoid AddDetail_U8(MHandle hMemMgr, MHandle mcvParallelMonitor,
                        LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDetailImg, LPASVLOFFSCREEN pDst,
                        MInt32 intensity, MInt32 lRange);

private:
    MHandle m_hMemMgr;
    MHandle m_mcvParallelMonitor;
    MInt32 m_lPryLayer;
    MInt32 m_lExpandSize;
    MInt32 m_lWidth;
    MInt32 m_lHeight;

    ASVLOFFSCREEN m_pSrcPyrGau[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pSrcPyrGauInner[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pDetailPyrGau[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pDetailPyrGauInner[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pMeanA[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pMeanB[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pMeanAInner[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pMeanBInner[MAX_PYRAMID_LAYER];
};

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_SHARPEN_PYRAMID_U8_H
