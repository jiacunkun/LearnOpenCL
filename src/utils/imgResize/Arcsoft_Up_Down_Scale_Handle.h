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
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_UP_DOWN_SCALE_HANDLE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_UP_DOWN_SCALE_HANDLE_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

enum ScaleType
{
    mean2x2 = 0,
    gaussian3x3 = 1,
    gaussian5x5 = 2,
    other = 3,
};


class Arcsoft_Up_Down_Scale_Handle
{
public:
    Arcsoft_Up_Down_Scale_Handle(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 nThreadCount = 16);
    ~Arcsoft_Up_Down_Scale_Handle();

    MInt32 downScale2(ImageInfo<MUInt8> largeImage, ImageInfo<MInt16> smallImage, ScaleType type);
    MInt32 downScale2(ImageInfo<MInt16> largeImage, ImageInfo<MInt16> smallImage, ScaleType type);
    MInt32 downScale2(ImageInfo<MUInt8> largeImage, ImageInfo<MUInt8> smallImage, ScaleType type);

    MInt32 upScale2(ImageInfo<MInt16> smallImage, ImageInfo<MUInt8> largeImage, ScaleType type);
    MInt32 upScale2(ImageInfo<MInt16> smallImage, ImageInfo<MInt16> largeImage, ScaleType type, MInt16 nWeight = 1);
    MInt32 upScale2(ImageInfo<MUInt8> smallImage, ImageInfo<MUInt8> largeImage, ScaleType type);

    static MVoid ImageInfo2ASVLOFFSCREEN(ImageInfo<MUInt8> srcImage, ASVLOFFSCREEN& dstImage0);
    static MVoid ImageInfo2ASVLOFFSCREEN(ImageInfo<MInt16> srcImage, ASVLOFFSCREEN& dstImage0);
private:
    MHandle m_hMemMgr;
    MHandle m_mcvParallelMonitor;
    MInt32 m_nThreadCount;
};

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_UP_DOWN_SCALE_HANDLE_H
