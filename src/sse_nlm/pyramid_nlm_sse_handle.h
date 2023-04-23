#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_PYRAMID_NLM_SSE_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_PYRAMID_NLM_SSE_HANDLE_H

#include <asvloffscreen.h>
#include "Arcsoft_SingleImageDenoise_Handle.h"

MInt32 arcsoft_pyramid_nlm_sse_process(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN  pSrcImg, LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pShade, MFloat fNoiseVarY);

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_PYRAMID_NLM_SSE_HANDLE_H
