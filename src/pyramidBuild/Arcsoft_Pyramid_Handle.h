#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_PYRAMID_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_PYRAMID_HANDLE_H

#ifdef ENABLE_DLL
#define API_EXPORT	__declspec(dllexport)
#else
#define API_EXPORT
#endif

#include "asvloffscreen.h"

enum PyramidMethodAPI
{
    Pyramid_NLM       = 0, // NLM降噪方法
    Pyramid_AnisGuide = 1, // 引导各向异性降噪方法
	Pyramid_Anis	  = 2
};

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tag_BASE_PYRAMID_IMAGE_PARAM
{
	MInt32 lMethod;
	MInt32 lLayer;
	MInt32 lIntensity;
	MInt32 lScale;
	MBool bDoFirstLayer;

} BASE_PYRAMID_IMAGE_PARAM, *LPBASE_PYRAMID_IMAGE_PARAM;

API_EXPORT MInt32 Arcsoft_Pyramid_Handle_U8_Init(MHandle *pHandle, MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lLayer);

API_EXPORT MInt32 Arcsoft_Pyramid_Handle_U8_Process(MHandle pHandle, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, LPASVLOFFSCREEN pShade, LPBASE_PYRAMID_IMAGE_PARAM pParam);

API_EXPORT MInt32 Arcsoft_Pyramid_Handle_U8_Uninit(MHandle *pHandle);


#ifdef __cplusplus
}
#endif

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_PYRAMID_HANDLE_H
