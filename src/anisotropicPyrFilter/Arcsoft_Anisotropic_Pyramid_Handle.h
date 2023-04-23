#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISOTROPIC_PYRAMID_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISOTROPIC_PYRAMID_HANDLE_H

#ifdef ENABLE_DLL
#define API_EXPORT	__declspec(dllexport)
#else
#define API_EXPORT
#endif

#include "asvloffscreen.h"

#ifdef __cplusplus
extern "C" {
#endif

API_EXPORT MInt32 Arcsoft_Anisotropic_Pyramid_Up2Down_Handle(MHandle hMemMgr,
                                         MHandle mcvParallelMonitor,
                                         LPASVLOFFSCREEN pSrc,
                                         LPASVLOFFSCREEN pDst,
                                         MInt32 lLayer,
                                         MFloat* pEps, MInt32* pSharpenIntensity,
                                         LPASVLOFFSCREEN pShade);

API_EXPORT MInt32 Arcsoft_Anisotropic_Pyramid_Handle(MHandle hMemMgr,
                                         MHandle mcvParallelMonitor,
                                         LPASVLOFFSCREEN pSrc,
                                         LPASVLOFFSCREEN pDst,
                                         MInt32 lLayer,
                                         MFloat* pEps, MInt32* pSharpenIntensity,
                                         LPASVLOFFSCREEN pShade);
#ifdef __cplusplus
}
#endif

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISOTROPIC_PYRAMID_HANDLE_H
