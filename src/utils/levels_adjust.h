#pragma once

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

MVoid levels_adjust(MUInt8 *pSrc, MInt32 lSrcPitch, MUInt8 *pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight,
	MInt32 lShadowIn, MInt32 lHighlightIn, MInt32 lShadowOut = 0, MInt32 lHighlightOut = 255);

NS_SINFLE_IMAGE_ENHANCEMENT_END