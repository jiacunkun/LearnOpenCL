#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NV21_DOWNSCALE4_TO_I444_RGB_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NV21_DOWNSCALE4_TO_I444_RGB_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

/**
 * @brief NV21 下采样4倍后转I444，并转为rgb图像
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrcImg 【输入，NV21图像】
 * @param YUV444 【输出，I444图像】
 * @param guideRGB 【输出，rgb图像】
 * @return
 */
MInt32 Arcsoft_NV21_DownScale4_To_I444_RGB(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg,
                                           ASVLOFFSCREEN& YUV444, ASVLOFFSCREEN& guideRGB);

/**
 * @brief NV21 下采样4倍后转I444
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrcImg 【输入，NV21图像】
 * @param YUV444 【输出，I444图像】
 * @return
 */
MInt32 Arcsoft_NV21_DownScale4_To_I444(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, ASVLOFFSCREEN& YUV444);

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NV21_DOWNSCALE4_TO_I444_RGB_H
