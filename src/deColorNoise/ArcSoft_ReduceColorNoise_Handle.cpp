#include <merror.h>
#include "ArcSoft_ReduceColorNoise_Handle.h"
#include "single_image_enhancement_define.h"
#include "ArcSoft_GuideFilter_For_DownSampleUV.h"
#include "Arcsoft_ReduceColorNoise.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

static const MLong noiseKernelSizeUV[11] = {0, 7, 9, 11, 13, 15, 19, 21, 25, 27, 29};

MInt32 ArcSoft_ReduceColorNoise_Guide_Process(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcDst, MInt32 lUVIntensity)
{
    START_TIME;
    MInt32 lRet = MOK;
    MInt32 kernelSizeUV = 5;
    LPASVLOFFSCREEN pImg = pSrcDst;
    if (lUVIntensity == (lUVIntensity >> 1 << 1))
    {
        MInt32 lval = lUVIntensity >> 1;
        lval = lval > 9 ? 9 : lval;
        kernelSizeUV = noiseKernelSizeUV[lval];
    } else
    {
        MInt32 lval = lUVIntensity >> 1;
        lval = lval > 9 ? 9 : lval;
        kernelSizeUV = ((noiseKernelSizeUV[lval] + noiseKernelSizeUV[lval + 1]) >> 1) + 1;
    }
    LOGD("kernelSizeUV = %d\n", kernelSizeUV);
    LPASVLOFFSCREEN pShadeMap = MNull;
    lRet = ArcSoft_GuideFilter_For_DownSampleUV(hMemMgr, mcvParallelMonitor, pImg, kernelSizeUV,
                                                lUVIntensity, pShadeMap, MFalse);
    END_TIME;
    return lRet;
}

MInt32 ArcSoft_ReduceColorNoise_BF_Process(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MInt32 lUVIntensity)
{
    START_TIME;
    MInt32 lRet = MOK;

#if 1

    Arcsoft_ReduceColorNoise obj(hMemMgr, mcvParallelMonitor);
    obj.init(pSrc->i32Width, pSrc->i32Height);
    obj.runNV21(pSrc, pDst, lUVIntensity);
    obj.release();

#else
    MInt32 kernelSizeUV = 5;
    if (lUVIntensity == (lUVIntensity >> 1 << 1))
    {
        MInt32 lval = lUVIntensity >> 1;
        lval = lval > 9 ? 9 : lval;
        kernelSizeUV = noiseKernelSizeUV[lval];
    }
    else
    {
        MInt32 lval = lUVIntensity >> 1;
        lval = lval > 9 ? 9 : lval;
        kernelSizeUV = ((noiseKernelSizeUV[lval] + noiseKernelSizeUV[lval + 1]) >> 1) + 1;
    }
    LOGD("kernelSizeUV = %d\n", kernelSizeUV);
    LPASVLOFFSCREEN pShadeMap = MNull;


    lRet = ArcSoft_GuideFilter_For_DownSampleUV(hMemMgr, mcvParallelMonitor, pSrc, pDst, kernelSizeUV,
        lUVIntensity, pShadeMap);
#endif

    END_TIME;
    return lRet;
}

MInt32 ArcSoft_ReduceColorNoise_BF_Process(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcDst, MInt32 lUVIntensity)
{
    START_TIME;
    MInt32 lRet = MOK;

#if 1

    Arcsoft_ReduceColorNoise obj(hMemMgr, mcvParallelMonitor);
    obj.init(pSrcDst->i32Width, pSrcDst->i32Height);
    obj.runNV21(pSrcDst, pSrcDst, lUVIntensity);
    obj.release();

#else
    MInt32 kernelSizeUV = 5;

    if (lUVIntensity == (lUVIntensity >> 1 << 1))
    {
        MInt32 lval = lUVIntensity >> 1;
        lval = lval > 9 ? 9 : lval;
        kernelSizeUV = noiseKernelSizeUV[lval];
    }
    else
    {
        MInt32 lval = lUVIntensity >> 1;
        lval = lval > 9 ? 9 : lval;
        kernelSizeUV = ((noiseKernelSizeUV[lval] + noiseKernelSizeUV[lval + 1]) >> 1) + 1;
    }
    LOGD("kernelSizeUV = %d\n", kernelSizeUV);
    LPASVLOFFSCREEN pShadeMap = MNull;


    lRet = ArcSoft_GuideFilter_For_DownSampleUV(hMemMgr, mcvParallelMonitor, pSrcDst, kernelSizeUV,
        lUVIntensity, pShadeMap, MTrue);
#endif

    END_TIME;
    return lRet;
}