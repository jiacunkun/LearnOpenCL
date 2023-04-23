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
#ifndef _IMG_PYRAMID_H_
#define _IMG_PYRAMID_H_

#include "asvloffscreen.h"
#include "merror.h"
#include "amcomdef.h"
//#include "night_shot.h"
#include "single_image_enhancement_define.h"


#define MAX_LEVELS			3
#define STRIPE_HEIGHT		240
#define STRIPE_TOP			0
#define STRIPE_MIDDLE		1
#define STRIPE_BOTTOM		2
// =============== Function definition =============== //
// #define Downscale2x2					PX(Downscale2x2)
// #define Upscale2x2						PX(Upscale2x2)
// #define GetPyramidSize					PX(GetPyramidSize)
// #define CreateGaussPyramid				PX(CreateGaussPyramid)
// #define CreateGaussPyramid_YUYV			PX(CreateGaussPyramid_YUYV)
// #define DestroyPyramid					PX(DestroyPyramid)
// #define ImgPyramidDenoise_Global		PX(ImgPyramidDenoise_Global)
// #define ImgPyramidDenoise_Stripe		PX(ImgPyramidDenoise_Stripe)
// #define ReduceBlock						PX(ReduceBlock)
// #define ExpandBlock						PX(ExpandBlock)
// #define ExpandBlock2					PX(ExpandBlock2)
// =============== Function definition =============== //

#ifndef TRIMBYTE
    #define TRIMBYTE(x)	    (MByte)( (x) < 0 ? 0 :( (x) >255 ? 255 :(x) ))
#endif

typedef struct _tag_IMGPYRAMID {
	MVoid	*pImage;
	MInt32	lWidth;
	MInt32	lHeight;
	MInt32	lPitch;
	struct _tag_IMGPYRAMID	*pPrev;
}ImgPyramid, *LPImgPyramid;


MVoid Downscale2x2(const MByte* pInImage, MInt32 irows, MInt32 icols, MInt32 ipitch, MByte* pOutImage, MInt32 orows, MInt32 ocols, MInt32 opitch);

MVoid Upscale2x2(const MByte* pInImage, MInt32 irows, MInt32 icols, MInt32 ipitch, MByte* pOutImage, MInt32 orows, MInt32 ocols, MInt32 opitch);

MInt32 GetPyramidSize(MInt32 lWidth, MInt32 lHeight, MInt32 lPydLevels);

LPImgPyramid CreateGaussPyramid(MHandle hMMemMgr, MByte *pImg, MInt32 lWidth, MInt32 lHeight, MInt32 lLineBytes, 
								MInt32 lPydLevels, MByte *pTmpBuf, MInt32 lBufSize);
LPImgPyramid CreateGaussPyramid_YUYV(MHandle hMMemMgr, MByte *pImg, MInt32 lWidth, MInt32 lHeight, MInt32 lLineBytes, 
								MInt32 lOffset, MInt32 lStep, MInt32 lPydLevels, MByte *pTmpBuf, MInt32 lBufSize);
MVoid DestroyPyramid(MHandle hMMemMgr, LPImgPyramid pImgPyd);

MRESULT ImgPyramidDenoise_Global(MHandle hMemMgr, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, 
								 MFloat fNoiseVarY, MFloat fNoiseVarUV);
MRESULT ImgPyramidDenoise_Stripe(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg,
								 MRECT rtInside, MFloat fNoiseVarY, MFloat fNoiseVarUV, MBool Flag_Y);

MInt32 ImgPyramidDenoise_Block_NLM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN  pSrcImg, LPASVLOFFSCREEN pDstImg, MFloat fNoiseVarY, MFloat fNoiseVarUV, MByte *pFlatMask = MNull);

MRESULT ReduceBlock(const MVoid *pExpandData, MDWord dwExpandLine, MInt32 lExpandW, MInt32 lExpandH, 
				  MInt32 lReduceL, MInt32 lReduceR, MInt32 lReduceT, MInt32 lReduceB, 
				  MVoid *pReduceData, MDWord dwReduceLine);
MVoid ExpandBlock(MVoid *pExpandData, MInt32 lExpandW, MInt32 lExpandH, MDWord dwExpandLine,
				  MInt32 lLeft, MInt32 lRight, MInt32 lTop, MInt32 lBottom,
				  const MVoid *pReduceData, MDWord dwReduceLine, MInt32 lReduceW, MInt32 lReduceH);
MVoid ExpandBlock2(const MUInt8 *pExpandSrc, MUInt8 *pExpandDst, MInt32 lExpandW, MInt32 lExpandH, MDWord dwExpandLine,
				  MInt32 lLeft, MInt32 lRight, MInt32 lTop, MInt32 lBottom,
				  const MInt8 *pReduceData, MDWord dwReduceLine, MInt32 lReduceW, MInt32 lReduceH);



#endif // _IMG_PYRAMID_H_
