#pragma once

#include <asvloffscreen.h>
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

#define MAX_PYRAMID_LAYER 4

class nlm_pyramid
{
public:
    nlm_pyramid();
    ~nlm_pyramid();

    MInt32 init(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPryLayer);
    MVoid release();
    MInt32 run(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MFloat* pValue, MInt32 lRadius);
    MInt32 run_mask(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pMask, MFloat* pValue, MInt32 lRadius);

private:
    MVoid FillExpandPixels(LPASVLOFFSCREEN pSrcDst, MInt32 lExpandSize);
    MVoid ImgAddDiff_sub128_u8(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDiffImg, LPASVLOFFSCREEN pDstImg);
    MVoid ImgSubImg_add128_u8(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pSubImg, LPASVLOFFSCREEN pDstImg);

private:
    MHandle m_hMemMgr;
    MHandle m_mcvParallelMonitor;
    MInt32 m_lPryLayer;
    MInt32 m_lExpandSize;
    MInt32 m_lWidth;
    MInt32 m_lHeight;

    ASVLOFFSCREEN m_pSrcPyrGau[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pSrcPyrGauInner[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pDstPyrGau[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pDstPyrGauInner[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pTmpPyrGau[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pTmpPyrGauInner[MAX_PYRAMID_LAYER];
    ASVLOFFSCREEN m_pMaskGau[MAX_PYRAMID_LAYER];
};

NS_SINFLE_IMAGE_ENHANCEMENT_END

