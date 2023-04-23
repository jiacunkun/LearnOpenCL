#include "single_image_enhancement_define.h"
#include "Arcsoft_AnisGuidedDenoise_Handle.h"
#include "Arcsoft_SingleImageDenoise_Handle.h"
#include "Arcsoft_AnisGuided_Pyramid_Down2Up.h"
#include "Arcsoft_AnisGuided_Pyramid_Up2Down.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

MInt32 Arcsoft_AnisGuided_Pyramid_Handle(MHandle hMemMgr,
                                        MHandle mcvParallelMonitor,
                                        LPASVLOFFSCREEN pSrc,
                                         LPASVLOFFSCREEN pGuided,
                                        LPASVLOFFSCREEN pDst,
                                         MInt32 lLayer,
                                         MFloat* pEps, MInt32* pSharpenIntensity,
                                         LPASVLOFFSCREEN pShade, MInt32 lScale)
{
    START_TIME;
    MInt32 lret = 0;

    lret =  Arcsoft_AnisGuided_Pyramid_Down2Up(hMemMgr, mcvParallelMonitor, pDst->i32Width, pDst->i32Height, pDst->pi32Pitch[0], lLayer).
            run(pSrc, pGuided, pDst, pEps, pSharpenIntensity, pShade, lScale);

    END_TIME;
    return lret;
}


MInt32 Arcsoft_AnisGuided_Pyramid_Up2Down_Handle(MHandle hMemMgr,
                                                 MHandle mcvParallelMonitor,
                                                 LPASVLOFFSCREEN pSrc,
                                                 LPASVLOFFSCREEN pGuided,
                                                 LPASVLOFFSCREEN pDst,
                                                 MInt32 lLayer,
                                                 MFloat* pEps, MInt32* pSharpenIntensity,
                                                 LPASVLOFFSCREEN pShade, MInt32 lScale)
{
    START_TIME;
    MInt32 lret = 0;

    lret =  Arcsoft_AnisGuided_Pyramid_Up2Down<MUInt8, MUInt8>(hMemMgr, mcvParallelMonitor, pDst->i32Width, pDst->i32Height, pDst->pi32Pitch[0], lLayer)
            .run(pSrc, pGuided, pDst, pEps, pSharpenIntensity, pShade, lScale);

    END_TIME;
    return lret;
}