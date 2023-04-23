#include "Arcsoft_NLM_Pyramid.h"
#include "single_image_enhancement_define.h"
#include "Arcsoft_NLM_Pyramid_C_Handle.h"
#include "Arcsoft_SingleImageDenoise_Handle.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

MInt32 Arcsoft_NLM_Pyramid_C_Handle_U8_For_Y(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, LPASVLOFFSCREEN pShade,
                                             MInt32 lIntensity, MBool isFirstLayerDenoise, MInt32 layer)
{
    MInt32 lret = 0;

    MFloat fNoiseVarY = 0;

#if 0
    MFloat fMNoiseVarY[11] = {0, 2, 3, 3, 5, 10, 15, 20, 30, 50, 70};
    //const MFloat fMNoiseVarY[11] = { 0, 5, 8, 10, 15, 20, 25, 30, 50, 70, 100 };

    if (lIntensity > 20)
    {
        lIntensity = 20;
    }

    if (lIntensity == ((lIntensity >> 1) << 1))
    {
        MInt32 lval = lIntensity >> 1;
        fNoiseVarY = fMNoiseVarY[lval];
    }
    else
    {
        MInt32 lval = lIntensity >> 1;
        fNoiseVarY = (fMNoiseVarY[lval] + fMNoiseVarY[lval + 1]) * 0.5f;
    }
#else
    fNoiseVarY = lIntensity;
#endif

    Arcsoft_NLM_Pyramid<MUInt8> obj(hMemMgr, mcvParallelMonitor, pDst->i32Width, pDst->i32Height, pSrc->pi32Pitch[0], pDst->pi32Pitch[0], layer);
    lret = obj.run(pSrc,  pDst,  fNoiseVarY, isFirstLayerDenoise,  pShade);

    return lret;
}

MInt32 Arcsoft_NLM_Pyramid_U8_For_Y(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, LPASVLOFFSCREEN pShade,
    MFloat* pIntensity, MInt32 layer)
{
    START_TIME;

    MInt32 lret = 0;

    Arcsoft_NLM_Pyramid<MUInt8> obj(hMemMgr, mcvParallelMonitor, pDst->i32Width, pDst->i32Height, pSrc->pi32Pitch[0], pDst->pi32Pitch[0], layer);
    lret = obj.run(pSrc, pDst, pIntensity, pShade);

    END_TIME;

    return lret;
}

MInt32 Arcsoft_NLM_Pyramid_C_Handle_U8(MHandle hMemMgr, MHandle mcvParallelMonitor, MUInt8 *pSrc, MUInt8 *pDst, LPASVLOFFSCREEN pShade,
                                       MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lIntensity, MBool isFirstLayerDenoise, MInt32 layer)
{
    MInt32 lret = 0;

    MFloat fNoiseVarY = 0;

#if 0
    MFloat fMNoiseVarY[11] = {0, 2, 3, 3, 5, 10, 15, 20, 30, 50, 70};
    //const MFloat fMNoiseVarY[11] = { 0, 5, 8, 10, 15, 20, 25, 30, 50, 70, 100 };

    if (lIntensity > 20)
    {
        lIntensity = 20;
    }

    if (lIntensity == ((lIntensity >> 1) << 1))
    {
        MInt32 lval = lIntensity >> 1;
        fNoiseVarY = fMNoiseVarY[lval];
    }
    else
    {
        MInt32 lval = lIntensity >> 1;
        fNoiseVarY = (fMNoiseVarY[lval] + fMNoiseVarY[lval + 1]) * 0.5f;
    }
#else
    fNoiseVarY = lIntensity;
#endif

    Arcsoft_NLM_Pyramid<MUInt8> obj(hMemMgr, mcvParallelMonitor, lWidth, lHeight, lPitch, lPitch, layer);
    lret = obj.run(pSrc, pDst, pShade, lWidth, lHeight, lPitch, fNoiseVarY, isFirstLayerDenoise);

    return lret;
}

MInt32 Arcsoft_NLM_Pyramid_U8(MHandle hMemMgr, MHandle mcvParallelMonitor, MUInt8* pSrc, MUInt8* pDst, LPASVLOFFSCREEN pShade,
    MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MFloat* pIntensity, MInt32 layer)
{
    START_TIME;

    MInt32 lret = 0;

    Arcsoft_NLM_Pyramid<MUInt8> obj(hMemMgr, mcvParallelMonitor, lWidth, lHeight, lPitch, lPitch, layer);
    lret = obj.run(pSrc, pDst, pShade, lWidth, lHeight, lPitch, pIntensity);

    END_TIME;

    return lret;
}

MInt32 Arcsoft_NLM_Pyramid_C_Handle_I16_For_Y(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, LPASVLOFFSCREEN pShade,
                                              MInt32 lIntensity, MBool isFirstLayerDenoise, MInt32 layer)
{
    MInt32 lret = 0;

    MFloat fNoiseVarY = 0;

#if 0
    MFloat fMNoiseVarY[11] = {0, 2, 3, 3, 5, 10, 15, 20, 30, 50, 70};
    //const MFloat fMNoiseVarY[11] = { 0, 5, 8, 10, 15, 20, 25, 30, 50, 70, 100 };

    if (lIntensity > 20)
    {
        lIntensity = 20;
    }

    if (lIntensity == ((lIntensity >> 1) << 1))
    {
        MInt32 lval = lIntensity >> 1;
        fNoiseVarY = fMNoiseVarY[lval];
    }
    else
    {
        MInt32 lval = lIntensity >> 1;
        fNoiseVarY = (fMNoiseVarY[lval] + fMNoiseVarY[lval + 1]) * 0.5f;
    }
#else
    fNoiseVarY = lIntensity;
#endif

    Arcsoft_NLM_Pyramid_I16 obj(hMemMgr, mcvParallelMonitor, pDst->i32Width, pDst->i32Height, pSrc->pi32Pitch[0], pDst->pi32Pitch[0], layer);
    lret = obj.run(pSrc, pDst, fNoiseVarY, isFirstLayerDenoise, pShade);

    return lret;
}

MInt32 Arcsoft_NLM_Pyramid_C_Handle_I16(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt16 *pSrc, MInt16 *pDst, LPASVLOFFSCREEN pShade,
                                        MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lIntensity, MBool isFirstLayerDenoise, MInt32 layer)
{
    MInt32 lret = 0;

    MFloat fNoiseVarY = 0;

#if 0
    MFloat fMNoiseVarY[11] = {0, 2, 3, 3, 5, 10, 15, 20, 30, 50, 70};
    //const MFloat fMNoiseVarY[11] = { 0, 5, 8, 10, 15, 20, 25, 30, 50, 70, 100 };

    if (lIntensity > 20)
    {
        lIntensity = 20;
    }

    if (lIntensity == ((lIntensity >> 1) << 1))
    {
        MInt32 lval = lIntensity >> 1;
        fNoiseVarY = fMNoiseVarY[lval];
    }
    else
    {
        MInt32 lval = lIntensity >> 1;
        fNoiseVarY = (fMNoiseVarY[lval] + fMNoiseVarY[lval + 1]) * 0.5f;
    }
#else
    fNoiseVarY = lIntensity;
#endif

    Arcsoft_NLM_Pyramid_I16 obj(hMemMgr, mcvParallelMonitor, lWidth, lHeight, lPitch, lPitch, layer);
    lret = obj.run(pSrc, pDst, pShade, lWidth, lHeight, lPitch, fNoiseVarY, isFirstLayerDenoise);

    return lret;
}