#include "single_image_enhancement_define.h"
#include "arcsoft_edge_filter.h"
#include "nlm_pyramid.h"
#include "arcsoft_edge_filter_handle.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

MInt32 arcsoft_edge_filter_process(MHandle hMemMgr,
    MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrc,
    LPASVLOFFSCREEN pGuided,
    LPASVLOFFSCREEN pDst,
    MInt32 lLayer,
    MFloat* pEps,
    MInt32 lScale,
    MInt32 lMethod)
{
    START_TIME;
    MInt32 lRet = 0;

    MInt32 lWidth = pDst->i32Width;
    MInt32 lHeight = pDst->i32Height;
    MInt32 lStride = pDst->pi32Pitch[0];

    arcsoft_edge_filter obj;
    lRet = obj.init(hMemMgr, mcvParallelMonitor, lWidth, lHeight, lStride, lLayer);
    if (lMethod == 0)
    {
        if (pSrc == pGuided)
        {
            lRet = obj.run(pSrc, pDst, pEps, lScale);
        }
        else
        {
            lRet = obj.run(pSrc, pGuided, pDst, pEps, lScale);
        }
    }
    else if (lMethod == 1)
    {
        lRet = obj.runWithNLM(pSrc, pGuided, pDst, pEps, lScale);
    }
    else if (lMethod == 2)
    {
        lRet = obj.run2(pSrc, pGuided, pDst, pEps, lScale);
    }
    else if (lMethod == 3)
    {
        lRet = obj.runB(pSrc, pDst, pEps, lScale);
    }
    else if (lMethod == 4)
    {
        lRet = obj.runValue(pSrc, pDst, pEps, lScale);
    }
    obj.release();

    END_TIME;
    return lRet;
}

MInt32 arcsoft_edge_filter_nlm_process(MHandle hMemMgr,
                                       MHandle mcvParallelMonitor,
                                       LPASVLOFFSCREEN pSrc,
                                       LPASVLOFFSCREEN pDst,
                                       MInt32 lLayer,
                                       MInt32 lRadius,
                                       MFloat* pEps)
{
    START_TIME;
    MInt32 lRet = 0;

    MInt32 lWidth = pDst->i32Width;
    MInt32 lHeight = pDst->i32Height;
    MInt32 lStride = pDst->pi32Pitch[0];

    nlm_pyramid obj;
    lRet = obj.init(hMemMgr, mcvParallelMonitor, lWidth, lHeight, lLayer);

    lRet = obj.run(pSrc, pDst, pEps, lRadius);

    obj.release();

    END_TIME;
    return lRet;
}

MInt32 arcsoft_edge_filter_nlm_mask_process(MHandle hMemMgr,
    MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrc,
    LPASVLOFFSCREEN pDst,
    LPASVLOFFSCREEN pMask,
    MInt32 lLayer,
    MInt32 lRadius,
    MFloat* pEps)
{
    START_TIME;
    MInt32 lRet = 0;

    MInt32 lWidth = pDst->i32Width;
    MInt32 lHeight = pDst->i32Height;
    MInt32 lStride = pDst->pi32Pitch[0];

    nlm_pyramid obj;
    lRet = obj.init(hMemMgr, mcvParallelMonitor, lWidth, lHeight, lLayer);

    if (pMask == MNull)
    {
        lRet = obj.run(pSrc, pDst, pEps, lRadius);
    }
    else
    {
        lRet = obj.run_mask(pSrc, pDst, pMask, pEps, lRadius);
    }

    obj.release();

    END_TIME;
    return lRet;
}