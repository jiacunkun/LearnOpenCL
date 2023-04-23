#include "Arcsoft_Pyramid_Handle.h"
#include "Arcsoft_Pyramid.h"
#include "ImageInfo.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

static const char gVersionString[] = "Arcsoft Video Deblur OCL version is 0.0.0!\n";

MInt32 Arcsoft_Pyramid_Handle_U8_Init(MHandle *pHandle, MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lLayer)
{
    LOGI(gVersionString);
    START_TIME;
    MInt32 lRet = 0;
    auto* ptr = new Arcsoft_Pyramid<MUInt8>(hMemMgr, mcvParallelMonitor, lWidth, lHeight, lPitch, lLayer, 8);
    lRet = ptr->init();
    *pHandle = ptr;

    END_TIME;
    return lRet;
}

MInt32 Arcsoft_Pyramid_Handle_U8_Process(MHandle pHandle, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, LPASVLOFFSCREEN pShade, LPBASE_PYRAMID_IMAGE_PARAM pParam)
{
    START_TIME;
    MInt32 lRet = 0;

    if (pSrc == MNull || pDst == MNull)
    {
        LOGD("pSrc == MNull || pDst == MNull!");
        return -1;
    }

    MInt32 lWidth = pSrc->i32Width;
    MInt32 lHeight = pSrc->i32Height;
    MInt32 lSrcPitchY = pSrc->pi32Pitch[0];

    ImageInfo<MUInt8> srcImage(pSrc);
    ImageInfo<MUInt8> dstImage(pDst);
    ImageInfo<MUInt8> shadeImage(pShade);
    ImageParam param[4];

    if (pParam->lMethod == Pyramid_NLM)
    {
        MFloat fPow[] = { 1.0, 0.5, 0.25, 0.125, 0.0625 };

        for (int i = pParam->lLayer - 1; i >= 0; i--)
        {
            param[i].fIntensity = pParam->lIntensity * fPow[i];
            param[i].fIntensity = MAX(1.0f, param[i].fIntensity);
        }
        param[0].fIntensity = pParam->bDoFirstLayer ? param[0].fIntensity : 0.0;
    }
    else if (pParam->lMethod == Pyramid_AnisGuide)
    {
        param[0].fIntensity = pParam->bDoFirstLayer ? pParam->lIntensity : 0;
        param[1].fIntensity = pParam->lIntensity / 2;
        param[2].fIntensity = pParam->lIntensity / 2;
        param[3].fIntensity = pParam->lIntensity / 2;
    }
    else if (pParam->lMethod == Pyramid_Anis)
    {
        param[0].fIntensity = pParam->bDoFirstLayer ? pParam->lIntensity : 0;
        param[1].fIntensity = pParam->lIntensity / 2;
        param[2].fIntensity = pParam->lIntensity / 2;
        param[3].fIntensity = pParam->lIntensity / 2;
    }
    else if (pParam->lMethod == 3)
    {
        param[0].fIntensity = pParam->bDoFirstLayer ? pParam->lIntensity : 0;
        param[1].fIntensity = pParam->lIntensity / 2;
        param[2].fIntensity = pParam->lIntensity / 2;
        param[3].fIntensity = pParam->lIntensity / 2;
    }

    auto *ptr = (Arcsoft_Pyramid<MUInt8>*)pHandle;
    ptr->run(&srcImage, &dstImage, &shadeImage, param, pParam->lMethod);

    END_TIME;
    return lRet;
}

MInt32 Arcsoft_Pyramid_Handle_U8_Uninit(MHandle *pHandle)
{
    LOGD("Arcsoft_Pyramid_Handle_U8_Uninit++");
    START_TIME;
    auto *ptr = (Arcsoft_Pyramid<MUInt8>*)*pHandle;
    SAFE_DELETE(ptr);
    *pHandle = MNull;
    END_TIME;
    LOGD("Arcsoft_Pyramid_Handle_U8_Uninit--");
    return 0;
}















