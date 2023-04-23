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
#ifndef _IMG_PYRAMID_BLOCK_NLM_H_
#define _IMG_PYRAMID_BLOCK_NLM_H_

//#include "nlmblock.h"
#include "asvloffscreen.h"
#include "amcomdef.h"
//#include "avecore.h"
#include "single_image_enhancement_define.h"

// ========== configuration for pyramid operations ========== //
#define MAX_LEVELS	3	// the max level of pyramid is (MAX_LEVELS+1)
//#define TOP_LEVEL_DENOISE
// ========== configuration for pyramid operations ========== //
#define USE_GUS_PYR

#define SG_NLM_THR_NUM (4)

typedef struct _tag_IMGPYRAMID {
	MByte	*pImage;
	MInt32	lWidth;
	MInt32	lHeight;
	MInt32	lPitch;
	MInt32 bufSize;
}ImgPyramid_Block_NLM, *LPImgPyramid_Block_NLM;

MInt32 Image_Pyramid_Creat_Build(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPImgPyramid_Block_NLM Y_Data, LPImgPyramid_Block_NLM UVData,
								 MInt32 lLevel, MBool bY_Flag, MBool bUV_Flag);

MInt32 Image_Pyramid_Creat_Build_Haar(MHandle hMemMgr, LPASVLOFFSCREEN pSrcImg, LPImgPyramid_Block_NLM Y_Data,
									  LPImgPyramid_Block_NLM UVData, MInt32 lLevel, MBool bY_Flag, MBool bUV_Flag);

MInt32  Img_Denoise_Block_NLM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImgPyramid_Block_NLM pSrcImg, LPImgPyramid_Block_NLM pDstImg, LPImgPyramid_Block_NLM pShade, MInt32* pMap, MInt32* pInvMap);

MInt32 Alloc_ImgData(MHandle hMemMgr, LPImgPyramid_Block_NLM pImgData, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch);
MVoid  Free_ImgData(MHandle hMemMgr, LPImgPyramid_Block_NLM pImgData);
MInt32 Resize_ImgData(MHandle hMemMgr, LPImgPyramid_Block_NLM pImgData, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch);

MVoid Img_Sub_C1(MHandle mcvParallelMonitor, LPImgPyramid_Block_NLM pSrcDstImg, LPImgPyramid_Block_NLM pSubImg);
MVoid Img_Add_C1(MHandle mcvParallelMonitor, LPImgPyramid_Block_NLM pSrcDstImg, LPImgPyramid_Block_NLM pAddImg);
MVoid Img_Add_C2(MHandle mcvParallelMonitor, LPImgPyramid_Block_NLM pSrcDstImg, LPImgPyramid_Block_NLM pAddImg01, LPImgPyramid_Block_NLM pAddImg02);


#endif // _IMG_PYRAMID_H_
