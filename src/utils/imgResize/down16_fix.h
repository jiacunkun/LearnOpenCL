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
#ifndef ARCSOFTSFNR_DOWN16_FIX_H
#define ARCSOFTSFNR_DOWN16_FIX_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
#define TASK_NUM 16


/**
 * @brief 16bit图像下采样4倍
 * @param mcvParallelMonitor
 * @param srcLarge
 * @param dstSmall
 * @param widthLarge
 * @param heightLarge
 * @param pitchLarge
 * @param pitchSmall
 * @param cn
 * @return
 */
MInt32 down16(MHandle mcvParallelMonitor, MVoid* srcLarge, MVoid* dstSmall, MInt32 widthLarge, MInt32 heightLarge, MInt32 pitchLarge,
              MInt32 pitchSmall, MInt32 cn);


NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFTSFNR_DOWN16_FIX_H
