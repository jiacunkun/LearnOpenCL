#include "Arcsoft_Anisotropic_Pyramid_Handle.h"
#include "single_image_enhancement_define.h"
#include "Arcsoft_SingleImageDenoise_Handle.h"
#include "Arcsoft_Anisotropic_Pyramid_Down2Up.h"
#include "Arcsoft_Anisotropic_Pyramid_Up2Down.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

MInt32 Arcsoft_Anisotropic_Pyramid_Handle(MHandle hMemMgr,
                                         MHandle mcvParallelMonitor,
                                         LPASVLOFFSCREEN pSrc,
                                         LPASVLOFFSCREEN pDst,
                                         MInt32 lLayer,
                                         MFloat* pEps, MInt32* pSharpenIntensity,
                                         LPASVLOFFSCREEN pShade)
{
    LOGD("Arcsoft_Anisotropic_Pyramid_Handle++");
#if CALCULATE_TIME
    BasicTimer time;
#endif
    MInt32 lret = 0;

    lret =  Arcsoft_Anisotropic_Pyramid_Down2Up(hMemMgr, mcvParallelMonitor, pDst->i32Width, pDst->i32Height, pDst->pi32Pitch[0], lLayer).
            run(pSrc, pDst, pEps, pSharpenIntensity, pShade);

#if CALCULATE_TIME
    LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
    LOGD("Arcsoft_Anisotropic_Pyramid_Handle--");
    return lret;
}


MInt32 Arcsoft_Anisotropic_Pyramid_Up2Down_Handle(MHandle hMemMgr,
    MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrc,
    LPASVLOFFSCREEN pDst,
    MInt32 lLayer,
    MFloat* pEps, MInt32* pSharpenIntensity,
    LPASVLOFFSCREEN pShade)
{
    LOGD("Arcsoft_Anisotropic_Pyramid_Handle++");
#if CALCULATE_TIME
    BasicTimer time;
#endif
    MInt32 lret = 0;

    lret = Arcsoft_Anisotropic_Pyramid_Up2Down<MUInt8>(hMemMgr, mcvParallelMonitor, pDst->i32Width, pDst->i32Height, pDst->pi32Pitch[0], lLayer).
        run(pSrc, pDst, pEps, pSharpenIntensity, pShade);

#if CALCULATE_TIME
    LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
    LOGD("Arcsoft_Anisotropic_Pyramid_Handle--");
    return lret;
}