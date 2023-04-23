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

#include "ArcsoftSharpenHandle.h"
#include "ArcsoftSharpen.h"
#include "single_image_enhancement_define.h"

USING_NS_SINFLE_IMAGE_ENHANCEMENT

    MInt32 Arcsoft_Sharpen_U8(MHandle hMemMgr,
                            MHandle mcvParallelMonitor,
                            MUInt8 *pImage,
                            MInt32 width,
                            MInt32 height,
                            MInt32 pitch,
                            MInt32 intensity)
    {
        MInt32 ret = ArcsoftSharpenU8().Run(hMemMgr,
                mcvParallelMonitor,
                pImage,
                width,
                height,
                pitch,
                intensity);
        return ret;
    }

    MInt32 Arcsoft_Sharpen_U16(MHandle hMemMgr,
                             MHandle mcvParallelMonitor,
                             MUInt16 *pImage,
                             MInt32 width,
                             MInt32 height,
                             MInt32 pitch,
                             MInt32 intensity)
    {
        MInt32 ret = ArcsoftSharpenU16().Run(hMemMgr,
                mcvParallelMonitor,
                pImage,
                width,
                height,
                pitch,
                intensity);
        return ret;
    }

    MInt32 Arcsoft_Sharpen_I16(MHandle hMemMgr,
                             MHandle mcvParallelMonitor,
                             MInt16 *pImage,
                             MInt32 width,
                             MInt32 height,
                             MInt32 pitch,
                             MInt32 intensity)
    {
        MInt32 ret = ArcsoftSharpenI16().Run(hMemMgr,
                 mcvParallelMonitor,
                 pImage,
                 width,
                 height,
                 pitch,
                 intensity);
        return ret;
    }

