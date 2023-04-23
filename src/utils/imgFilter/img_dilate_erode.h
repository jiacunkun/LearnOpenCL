//
// Created by jck7075 on 2020/6/21.
//

#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_IMG_DILATE_ERODE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_IMG_DILATE_ERODE_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

MInt32 FilterDilate3x3u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch,
                         MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight);

MInt32 FilterDilate5x5u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch,
                         MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight);

MInt32 FilterDilate7x7u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch,
                         MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight);

MInt32 FilterErode3x3u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch,
                        MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight);

MInt32 FilterErode5x5u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch,
                        MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight);

MInt32 FilterErode9x9u8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lSrcPitch,
                        MByte* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight);

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_IMG_DILATE_ERODE_H
