/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary
and confidential information.

The information and code contained in this file is only for authorized ArcSoft
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy,
distribute, modify, or take any action in reliance on it.

If you have received this file in error, please immediately notify ArcSoft and
permanently delete the original and any copy of any file and any printout
thereof.
*******************************************************************************/

#ifndef ARCSOFTSFNR_REMOVE_BLOCK_NOISE_H
#define ARCSOFTSFNR_REMOVE_BLOCK_NOISE_H

#include "wavelet_LL.h"
#include "arcsoft_single_image_enhancement.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

/**
 * @brief 用来平滑大块的噪声，只对小波分解后的最低频区域做引导滤波降噪
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pGuidedImage          [in] 引导图像
 * @param pGuidedImgWlDecom     [in] 建议设置为空，转成内部处理；否则，小波分解后的引导图像，外部对引导图分解后传入
 * @param delight               [in, out] 原图像输入，处理完后输出
 * @param dwtLevels             [in] 小波层数
 * @param param                 [in]
 * @return
 */
    MLong RemoveBlockNoise_LLwavelet_optimize(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pGuidedImage,
                                              WAVELET_LPDATA_8U pGuidedImgWlDecom, LPASVLOFFSCREEN delight,
                                              MInt32 dwtLevels, BASE_SINGLE_IMAGE_PARAM param);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFTSFNR_REMOVE_BLOCK_NOISE_H
