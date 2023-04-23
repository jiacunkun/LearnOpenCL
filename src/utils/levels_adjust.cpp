#include "levels_adjust.h"
#include "ArcsoftLog.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

MVoid levels_adjust(MUInt8* pSrc, MInt32 lSrcPitch, MUInt8* pDst, MInt32 lDstPitch, MInt32 lWidth, MInt32 lHeight,
	MInt32 lShadowIn, MInt32 lHighlightIn, MInt32 lShadowOut, MInt32 lHighlightOut)
{
	START_TIME;
	if (pSrc == MNull || pDst == MNull)
	{
		LOGE("pSrc == MNull || pDst == MNull");
		return;
	}

	MUInt8 pVector[256] = { 0 };
	for (MInt32 i = 0; i < 256; i++)
	{
		 MFloat tmp = (1.0*(i - lShadowIn) / (lHighlightIn - lShadowIn)) * (lHighlightOut - lShadowOut) + lShadowOut + 0.5;
		 CLAMP(tmp, 0, 255);
		 pVector[i] = tmp;
	}

	for (int y = 0; y < lHeight; y++)
	{
		auto* pTmpSrc = pSrc + y * lSrcPitch;
		auto* pTmpDst = pDst + y * lDstPitch;
		for (int x = 0; x < lWidth; x++)
		{
			pTmpDst[x] = pVector[pTmpSrc[x]];
		}
	}

	END_TIME;
	return;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END