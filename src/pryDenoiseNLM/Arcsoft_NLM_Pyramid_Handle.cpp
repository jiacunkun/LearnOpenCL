#include <ammem.h>
#include "Arcsoft_NLM_Pyramid.h"
#include "Arcsoft_NLM_Pyramid_Handle.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

Arcsoft_NLM_Pyramid_I16 *obj;

Arcsoft_NLM_Pyramid_Handle::Arcsoft_NLM_Pyramid_Handle(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 width,
                                                       MInt32 height, MInt32 pitch, MInt32 layer)
{
    LOGD("Arcsoft_NLM_Pyramid_Handle++");
    obj = new Arcsoft_NLM_Pyramid_I16(hMemMgr, mcvParallelMonitor, width, height, pitch, pitch, layer);
}

Arcsoft_NLM_Pyramid_Handle::~Arcsoft_NLM_Pyramid_Handle()
{
    delete obj;
}

MInt32 Arcsoft_NLM_Pyramid_Handle::run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat fNoiseVarY,
                                       MBool isFirstLayerDenoise, LPASVLOFFSCREEN pShade)
{
    return obj->run(pSrc,  pDst,  fNoiseVarY, isFirstLayerDenoise,  pShade);
}

MInt32 Arcsoft_NLM_Pyramid_Handle::run(MInt16 *pSrc, MInt16 *pDst, LPASVLOFFSCREEN pShade, MInt32 lWidth, MInt32 lHeight,
                                       MInt32 lPitch, MFloat fNoiseVarY, MBool isFirstLayerDenoise)
 {
     return obj->run(pSrc, pDst, pShade, lWidth, lHeight, lPitch, fNoiseVarY, isFirstLayerDenoise);
 }