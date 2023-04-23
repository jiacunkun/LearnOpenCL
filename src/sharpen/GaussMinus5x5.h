#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_GAUSSMINUS5X5_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_GAUSSMINUS5X5_H

#include "single_image_enhancement_define.h"
#include "asvloffscreen.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    MInt32 GaussMinus5x5(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt16* pDetail, MInt32 lPitchDetail);

    MInt32 GaussMinus5x5_U8(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MUInt8 * pDetail, MInt32 lPitchDetail);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_GAUSSMINUS5X5_H

