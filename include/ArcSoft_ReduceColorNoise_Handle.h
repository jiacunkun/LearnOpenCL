#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_REDUCECOLORNOISE_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_REDUCECOLORNOISE_HANDLE_H

#ifdef ENABLE_DLL
#define API_EXPORT	__declspec(dllexport)
#else
#define API_EXPORT
#endif

#include "asvloffscreen.h"

/**
 * @brief Guide降彩噪接口
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrcDst               [in,out] 输入为待降噪的图像，输出为处理结果，即输出直接覆盖原来数据，共用一块内存
 * @param lUVIntensity          [in] 降噪强度，范围在0-20
 * @return
 */
API_EXPORT MInt32 ArcSoft_ReduceColorNoise_Guide_Process(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcDst, MInt32 lUVIntensity);

/**
 * @brief 双边降彩噪接口
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrcDst               [in,out] 输入为待降噪的图像，输出为处理结果， 支持pSrc和pDst相同
 * @param lUVIntensity          [in] 降噪强度，范围在0-100
 * @return
 */
API_EXPORT MInt32 ArcSoft_ReduceColorNoise_BF_Process(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MInt32 lUVIntensity);

API_EXPORT MInt32 ArcSoft_ReduceColorNoise_BF_Process(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcDst, MInt32 lUVIntensity);

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_REDUCECOLORNOISE_HANDLE_H
