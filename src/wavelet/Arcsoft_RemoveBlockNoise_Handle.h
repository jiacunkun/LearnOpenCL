
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_REMOVEBLOCKNOISE_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_REMOVEBLOCKNOISE_HANDLE_H

#ifdef ENABLE_DLL
#define API_EXPORT	__declspec(dllexport)
#else
#define API_EXPORT
#endif

#include "asvloffscreen.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 去除图像块噪
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc              【输入， 8bit NV21数据】
 * @param pDst              【输出， 8bit NV21数据】
 * @param feps              【输入， 降噪强度，范围 0 - 20】
 * @param lLayer            【输入， 分解层数，内部已经固定为2，修改无效】
 * @return
 */
API_EXPORT MInt32 Arcsoft_RemoveBlockNoise_U8_Handle(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat feps, MInt32 lLayer);


#ifdef __cplusplus
}
#endif

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_REMOVEBLOCKNOISE_HANDLE_H
