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

*******************************************************************************

@brief  均值滤波

@version: 1.0

@author:

@date:

@change:

@note:

@todo:
*******************************************************************************/
#ifndef _BOX_FILTER_NS_H_
#define _BOX_FILTER_NS_H_
#include <stdio.h>
// mpbase
#include "merror.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "amcomdef.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

//function declaration
MRESULT Box_Filter_NS32F(MHandle hMemMgr, MHandle mcvParallelMonitor, MFloat *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
                         MInt32 lSrcLineBytes, MFloat *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius);
MRESULT Box_Filter_NS32I(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
                         MInt32 lSrcLineBytes, MInt32 *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius);


//class BoxFilterNS
//{
//public:
//
//    /// @brief
//    /// @param hMemMgr
//    /// @param pSrcBuf
//    /// @param lWidth
//    /// @param lHeight
//    /// @param lSrcLineBytes
//    /// @param pDstBuf
//    /// @param lDstLineBytes
//    /// @param lRadius
//    /// @return
//    MRESULT BoxFilterNS32F(MHandle hMemMgr,
//                           MHandle mcvParallelMonitor,
//                           MFloat *pSrcBuf,
//                           MInt32 lWidth,
//                           MInt32 lHeight,
//                           MInt32 lSrcLineBytes,
//                           MFloat *pDstBuf,
//                           MInt32 lDstLineBytes,
//                           MInt32 lRadius,
//                           MInt32 nThreadCount = -1);
//
//
//    /// @brief
//    /// @param hMemMgr
//    /// @param pSrcBuf
//    /// @param lWidth
//    /// @param lHeight
//    /// @param lSrcLineBytes
//    /// @param pDstBuf
//    /// @param lDstLineBytes
//    /// @param lRadius
//    /// @return
//    MRESULT BoxFilterNS32I(MHandle hMemMgr,
//                           MInt32 *pSrcBuf,
//                           MInt32 lWidth,
//                           MInt32 lHeight,
//                           MInt32 lSrcLineBytes,
//                           MInt32 *pDstBuf,
//                           MInt32 lDstLineBytes,
//                           MInt32 lRadius,
//                           MInt32 nThreadCount = -1);
//
//private:
//
//    /***********************************************
//    * NS32F
//    ***********************************************/
//
//    MVoid boxBlurProcessRow_add_NS32F(MFloat *pSumLine, MFloat *addSrc, MInt32 lImgWidth, MInt32 lRadius);
//
//    MVoid boxBlurProcessRow_add_sub_NS32F(MFloat *pSumLine, MFloat *addSrc, MFloat *subSrc, MInt32 lImgWidth, MInt32 lRadius);
//
//    MVoid BoxFilterRow_NS32F(MFloat *pDst, MFloat *pBoxSumBuf, MInt32 lImgWidth, MInt32 lRadius);
//
//
//    MVoid Image_Box_Stripe_NS32F(MFloat *pSrcImg,
//                                 MInt32 lImgWidth,
//                                 MInt32 lImgHeight,
//                                 MInt32 lSrcPitch,
//                                 MFloat *pDstImg,
//                                 MInt32 lDstPitch,
//                                 MInt32 lStartRow,
//                                 MInt32 lEndRow,
//                                 MInt32 lRadius,
//                                 MFloat *pBoxSumBuf);
//
//
//    /***********************************************
//    * NS32I
//    ***********************************************/
//
//    MVoid boxBlurProcessRow_add_NS32I(MInt32 *pSumLine, MInt32 *addSrc, MInt32 lImgWidth, MInt32 lRadius);
//
//    MVoid boxBlurProcessRow_add_sub_NS32I(MInt32 *pSumLine, MInt32 *addSrc, MInt32 *subSrc, MInt32 lImgWidth, MInt32 lRadius);
//
//    MVoid BoxFilterRow_NS32I(MInt32 *pDst, MInt32 *pBoxSumBuf, MInt32 lImgWidth, MInt32 lRadius, MFloat invDivNum);
//
//    MVoid Image_Box_Stripe_NS32I(MInt32 *pSrcImg,
//                                 MInt32 lImgWidth,
//                                 MInt32 lImgHeight,
//                                 MInt32 lSrcPitch,
//                                 MInt32 *pDstImg,
//                                 MInt32 lDstPitch,
//                                 MInt32 lStartRow,
//                                 MInt32 lEndRow,
//                                 MInt32 lRadius,
//                                 MInt32 *pBoxSumBuf);
//
//};

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif
