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
#ifndef ARCSOFTSFNR_BOXFILTERC2_H
#define ARCSOFTSFNR_BOXFILTERC2_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
MRESULT Box_Filter_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
                      MInt32 lSrcLineBytes,MByte *pDstBuf,MInt32 lDstLineBytes, MInt32 lRadius);
static MVoid boxBlurProcessRow_add_C2(MInt32* pSumLine, MByte* addSrc, MInt32 lImgWidth, MInt32 lRadius);
static MVoid boxBlurProcessRow_add_sub_C2(MInt32* pSumLine, MByte* addSrc, MByte* subSrc, MInt32 lImgWidth, MInt32 lRadius);
static MVoid BoxFilterRow_C2(MByte* pDst, MInt32* pBoxSumBuf, MInt32 lImgWidth, MInt32 lRadius, MInt32 invDivNum);
MVoid Image_Box_Stripe_C2(MByte* pSrcImg, MInt32 lImgWidth, MInt32 lImgHeight, MInt32 lSrcPitch,
                          MByte* pDstImg, MInt32 lDstPitch, MInt32 lStartRow, MInt32 lEndRow,
                          MInt32 lRadius, MInt32 *pBoxSumBuf);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFTSFNR_BOXFILTERC2_H
