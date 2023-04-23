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
#ifndef _IMG_PYRAMID_PROCESS_H_
#define _IMG_PYRAMID_PROCESS_H_
#include "imgpyramid_block_nlm.h"
#include "single_image_enhancement_define.h"

MInt32 Img_Guass3x3_Down2_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch);

MInt32 Img_Guass3x3_Down2_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImgC1, MByte* pDstImgC2, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch);


MInt32 Img_Haar_Down2_C1(MHandle hMemMgr, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch);

MInt32 Img_Haar_Down2_C2(MHandle hMemMgr, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImgC1, MByte* pDstImgC2, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch);


MInt32 Img_expand_C2(MHandle hMemMgr, LPImgPyramid_Block_NLM pFullImgUV, LPImgPyramid_Block_NLM pHalfImgU, LPImgPyramid_Block_NLM pHalfImgV);
MInt32 Img_expand_C1(MHandle hMemMgr, LPImgPyramid_Block_NLM pFullImg, LPImgPyramid_Block_NLM pHalfImg);


MInt32 Img_UpScale2_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImgPyramid_Block_NLM pHalfImg, LPImgPyramid_Block_NLM pFullImg);
MInt32 Img_UpScale2_Haar_C1(MHandle hMemMgr, LPImgPyramid_Block_NLM pHalfImg, LPImgPyramid_Block_NLM pFullImg);

MInt32 Img_Sub_UpScale2_Add_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImgPyramid_Block_NLM pFullOutImg,
	LPImgPyramid_Block_NLM pHalfSubImg0, LPImgPyramid_Block_NLM pHalfSubImg1, LPImgPyramid_Block_NLM pFullAddImg);


#endif // _IMG_PYRAMID_H_
