#pragma once

#include <asvloffscreen.h>
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

MVoid SobelFilter_Hor(MUInt8* pSrc, MInt32 lSrcStride, MInt16* pDst, MInt32 lDstStride, MInt32 lWidth, MInt32 lHeight);

MVoid SobelFilter_Ver(MUInt8* pSrc, MInt32 lSrcStride, MInt16* pDst, MInt32 lDstStride, MInt32 lWidth, MInt32 lHeight);

MVoid CalcSobelIntensity(MInt16* pSrcHor, MInt16* pSrcVer, MInt32 lSrcStride, MUInt8* pDst, MInt32 lDstStride, MInt32 lWidth, MInt32 lHeight);

MVoid CalcSobelDirection(MInt16* pSrcHor, MInt16* pSrcVer, MInt32 lSrcStride, MUInt8* pDst, MInt32 lDstStride, MInt32 lWidth, MInt32 lHeight);

NS_SINFLE_IMAGE_ENHANCEMENT_END