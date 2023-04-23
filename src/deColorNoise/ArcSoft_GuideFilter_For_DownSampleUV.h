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

#ifndef ARCSOFTSFNR_ARCSOFT_GUIDEFILTER_FOR_DOWNSAMPLEUV_H
#define ARCSOFTSFNR_ARCSOFT_GUIDEFILTER_FOR_DOWNSAMPLEUV_H\

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
/**
 * @brief 针对UV的降噪，先对UV进行下采样，采用引导滤波以对应的RGB图像为引导图进行降噪，降噪后上采用回去
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrcImg   [输入和输出，共用一块内存]
 * @param kernelSizeUV  [滤波半径]
 * @param lUVIntensity [降噪强度]
 * @return
 */
MInt32 ArcSoft_GuideFilter_For_DownSampleUV(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, MInt32 kernelSizeUV, MInt32 lUVIntensity, LPASVLOFFSCREEN pShadeMap, MBool isBF);

MInt32 ArcSoft_GuideFilter_For_DownSampleUV(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 kernelSizeUV, MInt32 lUVIntensity, LPASVLOFFSCREEN pShadeMap);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFTSFNR_ARCSOFT_GUIDEFILTER_FOR_DOWNSAMPLEUV_H
