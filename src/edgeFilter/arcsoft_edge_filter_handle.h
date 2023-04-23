#pragma once

#ifdef ENABLE_DLL
#define API_EXPORT	__declspec(dllexport)
#else
#define API_EXPORT
#endif

#include "asvloffscreen.h"

#ifdef __cplusplus
extern "C" {
#endif

    API_EXPORT MInt32 arcsoft_edge_filter_process(MHandle hMemMgr,
        MHandle mcvParallelMonitor,
        LPASVLOFFSCREEN pSrc,
        LPASVLOFFSCREEN pGuided,
        LPASVLOFFSCREEN pDst,
        MInt32 lLayer,
        MFloat* pEps,
        MInt32 lScale,
        MInt32 lMethod);

    API_EXPORT MInt32 arcsoft_edge_filter_nlm_process(MHandle hMemMgr,
        MHandle mcvParallelMonitor,
        LPASVLOFFSCREEN pSrc,
        LPASVLOFFSCREEN pDst,
        MInt32 lLayer,
        MInt32 lRadius,
        MFloat* pEps);

    API_EXPORT MInt32 arcsoft_edge_filter_nlm_mask_process(MHandle hMemMgr,
        MHandle mcvParallelMonitor,
        LPASVLOFFSCREEN pSrc,
        LPASVLOFFSCREEN pDst,
        LPASVLOFFSCREEN pMask,
        MInt32 lLayer,
        MInt32 lRadius,
        MFloat* pEps);

#ifdef __cplusplus
}
#endif