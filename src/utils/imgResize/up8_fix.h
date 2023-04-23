//
// Created by jck7075 on 2020/4/22.
//

#ifndef ARCSOFTSFNR_UP8_FIX_H
#define ARCSOFTSFNR_UP8_FIX_H
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
#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
#define TASK_NUM 16

/**
 * @brief 16bit图像上采样4倍到8bit
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param srcSmall
 * @param dstSmall
 * @param dstLarge
 * @param widthLarge
 * @param heightLarge
 * @param pitchSmall
 * @param pitchLarge
 * @param cn
 * @return
 */
MInt32 up8(MHandle hMemMgr, MHandle mcvParallelMonitor, MVoid* srcSmall, MVoid* dstSmall, MVoid* dstLarge, MInt32 widthLarge, MInt32 heightLarge,
           MInt32 pitchSmall, MInt32 pitchLarge, MInt32 cn);


NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFTSFNR_UP8_FIX_H
