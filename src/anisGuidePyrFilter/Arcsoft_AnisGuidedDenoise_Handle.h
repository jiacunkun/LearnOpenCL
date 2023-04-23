
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISGUIDEDDENOISE_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISGUIDEDDENOISE_HANDLE_H

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
 * @brief 降噪金字塔从大图到小图逐层降噪
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc      [in], 输入原图像
 * @param pGuided   [in], 输入的引导图像，可以和输入原图像一致
 * @param pDst      [in], 输出结果图
 * @param lLayer    [in], 金字塔降噪的层数
 * @param pEps      [in], 每层金字塔降噪的强度
 * @param pSharpenIntensity [in], 每层金字塔锐化强度
 * @param pShade    [in], 外部传入的降噪mask
 * @param lScale    [in], 最大层降噪缩小倍数
 * @return
 */
API_EXPORT MInt32 Arcsoft_AnisGuided_Pyramid_Handle(MHandle hMemMgr,
                                         MHandle mcvParallelMonitor,
                                         LPASVLOFFSCREEN pSrc,
                                         LPASVLOFFSCREEN pGuided,
                                         LPASVLOFFSCREEN pDst,
                                         MInt32 lLayer,
                                         MFloat* pEps, MInt32* pSharpenIntensity,
                                         LPASVLOFFSCREEN pShade, MInt32 lScale);

/**
 * @brief 降噪金字塔从小图到大图逐层降噪
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrc      [in], 输入原图像
 * @param pGuided   [in], 输入的引导图像，可以和输入原图像一致
 * @param pDst      [in], 输出结果图
 * @param lLayer    [in], 金字塔降噪的层数
 * @param pEps      [in], 每层金字塔降噪的强度
 * @param pSharpenIntensity [in], 每层金字塔锐化强度
 * @param pShade    [in], 外部传入的降噪mask
 * @param lScale    [in], 最大层降噪缩小倍数
 * @return
 */
API_EXPORT MInt32 Arcsoft_AnisGuided_Pyramid_Up2Down_Handle(MHandle hMemMgr,
                                                 MHandle mcvParallelMonitor,
                                                 LPASVLOFFSCREEN pSrc,
                                                 LPASVLOFFSCREEN pGuided,
                                                 LPASVLOFFSCREEN pDst,
                                                 MInt32 lLayer,
                                                 MFloat* pEps, MInt32* pSharpenIntensity,
                                                 LPASVLOFFSCREEN pShade, MInt32 lScale);
#ifdef __cplusplus
}
#endif

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISGUIDEDDENOISE_HANDLE_H
