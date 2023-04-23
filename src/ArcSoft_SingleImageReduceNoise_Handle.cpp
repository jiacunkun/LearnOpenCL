#include "ArcSoft_SingleImageReduceNoise_Handle.h"
#include "single_image_enhancement_define.h"
#include "Arcsoft_SingleImageDenoise_Handle.h"
#include <mobilecv.h>
#include <merror.h>
#include <ArcSoft_ReduceColorNoise_Handle.h>
#include "Arcsoft_Pyramid_Handle.h"

struct SINGLE_IMAGE_RN_PARAM
{
    MHandle hMemMgr;
    MHandle mcvParallelMonitor;
    MInt32 lYMethod;
    MInt32 lLayer;
    MInt32 lScale;
    MInt32 pYIntensity[4];
    MInt32 lUVMethod;
    MInt32 lUVIntensity;
};


MInt32 BASE_ASSIRN_Init(MHandle hMemMgr, MHandle* pHandle)
{
    START_TIME;
    MInt32 lRet = 0;

    SINGLE_IMAGE_RN_PARAM *ptr = MNull;
    ptr = (SINGLE_IMAGE_RN_PARAM *) MMemAlloc(hMemMgr, sizeof(SINGLE_IMAGE_RN_PARAM));
    MMemSet(ptr, 0, sizeof(SINGLE_IMAGE_RN_PARAM));
    ptr->hMemMgr = hMemMgr;

    MHandle mcvParallelMonitor = MNull;
    mcvParallelMonitor = mcvParallelInit(hMemMgr, 16);
    if (mcvParallelMonitor == 0)
    {
        LOGD("Failed to start parallel engine!!\n");
        return MERR_BAD_STATE;
    }
    ptr->mcvParallelMonitor = mcvParallelMonitor;

    *pHandle = ptr;

    END_TIME;
    return lRet;
}

MInt32 BASE_ASSIRN_Uninit(MHandle* pHandle)
{
    START_TIME;
    MInt32 lRet = 0;

    auto* ptr = (SINGLE_IMAGE_RN_PARAM*)*pHandle;

    MHandle hMemMgr = ptr->hMemMgr;
    MHandle mcvParallelMonitor = ptr->mcvParallelMonitor;
    if (mcvParallelMonitor)
    {
        if (mcvParallelUninit(mcvParallelMonitor) < 0)
        {
            return MERR_BAD_STATE;
        }
    }
    SAFE_FREE_ARRAY(hMemMgr, ptr);
    *pHandle = MNull;

    END_TIME;
    return lRet;
}

MVoid BASE_ASSIRN_SetYMethod(MHandle pHandle, MInt32 lMethod)
{
    auto* ptr = (SINGLE_IMAGE_RN_PARAM*)pHandle;
    ptr->lYMethod = lMethod;
}

MInt32 BASE_ASSIRN_GetYMethod(MHandle pHandle)
{
    auto* ptr = (SINGLE_IMAGE_RN_PARAM*)pHandle;
    return ptr->lUVMethod;
}

MVoid BASE_ASSIRN_SetUVMethod(MHandle pHandle, MInt32 lMethod)
{
    auto* ptr = (SINGLE_IMAGE_RN_PARAM*)pHandle;
    ptr->lUVMethod = lMethod;
}

MInt32 BASE_ASSIRN_GetUVMethod(MHandle pHandle)
{
    auto* ptr = (SINGLE_IMAGE_RN_PARAM*)pHandle;
    return ptr->lUVMethod;
}

MVoid BASE_ASSIRN_SetYIntensity(MHandle pHandle, MInt32 *pIntensity, MInt32 lLayer)
{
    auto* ptr = (SINGLE_IMAGE_RN_PARAM*)pHandle;
    ptr->lLayer = lLayer;
    for (MInt32 i = 0; i < lLayer; i++)
    {
        ptr->pYIntensity[i] = pIntensity[i];
    }
}

MVoid BASE_ASSIRN_GetYIntensity(MHandle pHandle, MInt32* pIntensity, MInt32 lLayer)
{
    auto* ptr = (SINGLE_IMAGE_RN_PARAM*)pHandle;
    for (MInt32 i = 0; i < lLayer; i++)
    {
        pIntensity[i] = ptr->pYIntensity[i];
    }
}

MVoid BASE_ASSIRN_SetUVIntensity(MHandle pHandle, MInt32 lIntensity)
{
    auto* ptr = (SINGLE_IMAGE_RN_PARAM*)pHandle;
    ptr->lUVIntensity = lIntensity;
}

MInt32 BASE_ASSIRN_GetUVIntensity(MHandle pHandle)
{
    auto* ptr = (SINGLE_IMAGE_RN_PARAM*)pHandle;
    return ptr->lUVIntensity;
}

MInt32 BASE_ASSIRN_Process(MHandle pHandle, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg)
{
    START_TIME;
    MInt32 lRet = 0;

    auto* ptr = (SINGLE_IMAGE_RN_PARAM*)pHandle;
    MInt32 lYMethod = ptr->lYMethod;
    MHandle hMemMgr = ptr->hMemMgr;
    MHandle mcvParallelMonitor = ptr->mcvParallelMonitor;
    MInt32 lLayer = ptr->lLayer;
    MInt32 lVal = ptr->pYIntensity[0];
    MInt32 lScale = 2;

    switch (lYMethod)
    {
        case 0:
        {
            lLayer = MIN(lLayer, 2);
            lRet = Arcsoft_RemoveBlockNoise_U8_Handle( hMemMgr,  mcvParallelMonitor, pSrcImg,  pDstImg,  lVal,  lLayer);
            break;
        }
        case 1:
        {
            lVal *= 5;
            MBool bIsFrist = MTrue;
            lRet = Arcsoft_NLM_Pyramid_C_Handle_U8_For_Y(hMemMgr, mcvParallelMonitor, pSrcImg, pDstImg, MNull, lVal, bIsFrist, lLayer);
            break;
        }
        case 2:
        {
            MFloat pEps[4] = {0};
            MInt32 pSharp[4] = {0};
            for (MInt32 i = 0; i < lLayer; i++)
            {
                pEps[i] = ptr->pYIntensity[i];
            }
            //pEps[0] = 0;
            lRet = Arcsoft_AnisGuided_Pyramid_Up2Down_Handle(hMemMgr, mcvParallelMonitor, pSrcImg, pSrcImg, pDstImg, lLayer, pEps, pSharp, MNull, lScale);
            break;
        }
        case 3:
        {
            MFloat pEps[4] = { 0 };
            MInt32 pSharp[4] = { 0 };
            for (MInt32 i = 0; i < lLayer; i++)
            {
                pEps[i] = ptr->pYIntensity[i];
            }
            MHandle pDenoiseHandle = MNull;
            MInt32 lWidth = pDstImg->i32Width;
            MInt32 lHeight = pDstImg->i32Height;
            MInt32 lPitch = pDstImg->pi32Pitch[0];
            BASE_PYRAMID_IMAGE_PARAM Param;
            {
                Param.bDoFirstLayer = ptr->pYIntensity[0] > 0;
                Param.lIntensity = ptr->pYIntensity[0];
                Param.lLayer = lLayer;
                Param.lMethod = Pyramid_Anis;
            }
            Arcsoft_Pyramid_Handle_U8_Init(&pDenoiseHandle, hMemMgr, mcvParallelMonitor, lWidth, lHeight, lPitch, lLayer);
            Arcsoft_Pyramid_Handle_U8_Process(pDenoiseHandle, pSrcImg, pDstImg, MNull, &Param);
            Arcsoft_Pyramid_Handle_U8_Uninit(&pDenoiseHandle);
            break;
        }
        case 4:
        {
            MFloat pEps[4] = { 0 };
            MInt32 pSharp[4] = { 0 };
            for (MInt32 i = 0; i < lLayer; i++)
            {
                pEps[i] = ptr->pYIntensity[i];
            }
            lRet = Arcsoft_Anisotropic_Pyramid_Handle(hMemMgr, mcvParallelMonitor, pSrcImg, pDstImg, lLayer, pEps, pSharp, MNull);
            break;
        }
        default:
            break;
    }
    END_TIME;

    MInt32 lUVVal = ptr->lUVIntensity;
    if (lUVVal > 0)
    {
        lRet = ArcSoft_ReduceColorNoise_BF_Process(hMemMgr, mcvParallelMonitor, pSrcImg, pDstImg, lUVVal);
    }
    END_TIME;

    return lRet;
}

