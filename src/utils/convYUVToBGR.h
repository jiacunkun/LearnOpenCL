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
#ifndef ARCSOFTSFNR_CONVYUVTOBGR_H
#define ARCSOFTSFNR_CONVYUVTOBGR_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
/**
 * @brief 只能将I444的数据类型转成RGB图像
 * @param mcvParallelMonitor
 * @param pYdata
 * @param pUVdata
 * @param lYPitch
 * @param lUVPitch
 * @param pDstBGR
 * @return
 */
    MRESULT convYUVToBGR(MHandle mcvParallelMonitor, MByte *pYdata, MByte *pUVdata, MInt32 lYPitch, MInt32 lUVPitch, LPASVLOFFSCREEN pDstBGR);

    MVoid BGRToYUV444Planar(MByte* pBGR, MInt32 nW, MInt32 nH, MInt32 nBGRStride,
                            MByte* pY, MInt32 lYStride, MByte* pU, MInt32 lUStride, MByte* pV, MInt32 lVStride);

    MVoid YUV444ToBGRPlanar(MByte* pY, MInt32 lYStride, MByte* pU, MInt32 lUStride, MByte* pV, MInt32 lVStride,
                            MByte* pBGR, MInt32 nW, MInt32 nH, MInt32 nBGRStride);

// 私用函数
    typedef struct __tag_localConvYUVToBGR_para{
        MUInt8 *pY;
        MInt32 lStepY;
        MUInt8 *pUV;
        MInt32 lStepUV;
        MUInt8 *pBGR;
        MInt32 lStepBGR;
        MInt32 lWidth;
        MInt32 lStart;
        MInt32 lEnd;
    }ConvYUVToBGRPara;
    MVoid localConvYUVToBGR(MUInt8 *pY, MInt32 lStepY, MUInt8 *pUV, MInt32 lStepUV,
                            MUInt8 *pBGR, MInt32 lStepBGR, MInt32 lWidth, MInt32 lStart, MInt32 lEnd);
    MVoid thread_localConvYUVToBGR(MVoid *para);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFTSFNR_CONVYUVTOBGR_H