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
#ifndef ARCSOFTSFNR_ANIS_FILTERING_PROCESS8_H
#define ARCSOFTSFNR_ANIS_FILTERING_PROCESS8_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
#define TASK_NUM_ANIS_PYR_FILTER 16

/**
* @brief 各向异性滤波，金字塔第一层层使用
* @param mcvParallelMonitor     [输入，传入mcvParallelMonitor环境参数]
* @param pSrc                     [输入，源图像]
* @param pDst                     [输出，结果图像]
* @param width                  [输入，图像的宽]
* @param height                 [输入，图像的高]
* @param pitch                  [输入，图像的每行的步幅]
* @param numChan                [输入，输入，通道数，Y通道是1，UV通道是2]
* @param scaleNoiseShade        [输入，噪声权重图的归一化值]
* @param absDifScale            [输入，差值的归一化值]
* @param weiEachRange           [输入，每个方向上离中心点距离权重]
* @param pShade                    [输入，噪声权重图]
* @param pitchShade             [输入，权重图的pitch]
* @return                       [返回值，0 成功，其他 失败]
*/
MInt32 anis_filtering_process8(MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pDst, MInt32 width, MInt32 height, MInt32 pitch, MInt32 numChan,
                               MInt32 scaleNoiseShade, MInt16 absDifScale, MInt32* weiEachRange, MVoid* pShade, MInt32 pitchShade);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFTSFNR_ANIS_FILTERING_PROCESS8_H
