#include "Arcsoft_Sharpen_Pyramid.h"
#include "single_image_enhancement_define.h"
#include "Arcsoft_Sharpen_Pyramid_Handle.h"
#include "Sharpen_Pyramid_U8.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

MInt32  Arcsoft_Sharpen_Pyramid_Handle_U8_ForY(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pDstImg, MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers, MInt32 lMethod)
{
    START_TIME;
    MInt32 lRet = 0;

#if 1 // 重构锐化
    Sharpen_Pyramid_U8 obj;
    lRet = obj.init(hMemMgr, mcvParallelMonitor, pDstImg->i32Width, pDstImg->i32Height, lLayers);
    if (lRet != 0)
    {
        return lRet;
    }
    if (lMethod == 0)
    {
        lRet = obj.run_nlm(pDstImg, pDstImg, pIntensity, pRange, lFilterVal, lLayers);
    }
    else if (lMethod == 1)
    {
        lRet = obj.run(pDstImg, pDstImg, pIntensity, pRange, lFilterVal, lLayers);
    }
    obj.release();
#else
    Arcsoft_Sharpen_Pyramid<MUInt8> obj(hMemMgr, mcvParallelMonitor, pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], lLayers);
    lRet = obj.init();
    if (lRet != 0)
    {
        return lRet;
    }
    lRet = obj.run(pDstImg->ppu8Plane[0], pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], pIntensity, pRange, lFilterVal);
#endif

    END_TIME;
    return lRet;
}

MInt32 Arcsoft_Sharpen_Pyramid_Handle_I16_ForY(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pDstImg, MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers)
{
    MInt32 lret = 0;

    Arcsoft_Sharpen_Pyramid<MInt16> obj(hMemMgr, mcvParallelMonitor, pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], lLayers);
    lret = obj.init();
    if (lret != 0)
    {
        return lret;
    }
    lret = obj.run((MInt16*)pDstImg->ppu8Plane[0], pDstImg->i32Width, pDstImg->i32Height, pDstImg->pi32Pitch[0], pIntensity, pRange, lFilterVal);


    return lret;
}

MInt32 Arcsoft_Sharpen_Pyramid_Handle_U8(MHandle hMemMgr, MHandle mcvParallelMonitor,
                                         MUInt8 *pSrcDst, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                                         MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers)
{
    MInt32 lret = 0;

    Arcsoft_Sharpen_Pyramid<MUInt8> obj(hMemMgr, mcvParallelMonitor, lWidth, lHeight, lPitch, lLayers);
    lret = obj.init();
    if (lret != 0)
    {
        return lret;
    }
    lret = obj.run(pSrcDst, lWidth, lHeight, lPitch, pIntensity, pRange, lFilterVal);


    return lret;
}

MInt32 Arcsoft_Sharpen_Pyramid_Handle_I16(MHandle hMemMgr, MHandle mcvParallelMonitor,
                                          MInt16 *pSrcDst, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                                          MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers)
{
    MInt32 lret = 0;

    Arcsoft_Sharpen_Pyramid<MInt16> obj(hMemMgr, mcvParallelMonitor, lWidth, lHeight, lPitch, lLayers);
    lret = obj.init();
    if (lret != 0)
    {
        return lret;
    }
    lret = obj.run(pSrcDst, lWidth, lHeight, lPitch, pIntensity, pRange, lFilterVal);


    return lret;
}