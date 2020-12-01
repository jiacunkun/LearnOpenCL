#ifndef NLM_OCL_PYRAMIDNLM_OCL_HANDLER_H
#define NLM_OCL_PYRAMIDNLM_OCL_HANDLER_H

#include <CL/cl.h>
#include "asvloffscreen.h"

#ifdef __cplusplus
extern "C" {
#endif

    MInt32 PyramidNLM_OCL_Handle(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat fNoiseVarY, MFloat fNoiseVarUV);

#ifdef __cplusplus
}
#endif

#endif //NLM_OCL_PYRAMIDNLM_OCL_HANDLER_H
