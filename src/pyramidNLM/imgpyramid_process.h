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
#include "imagebase.h"

#define Img_Guass3x3_Down2_C1				PX(Img_Guass3x3_Down2_C1)
#define Img_Guass3x3_Down2_C2				PX(Img_Guass3x3_Down2_C2)
#define Img_Haar_Down2_C1					PX(Img_Haar_Down2_C1)
#define Img_Haar_Down2_C2					PX(Img_Haar_Down2_C2)
#define Img_expand_C2						PX(Img_expand_C2)
#define Img_expand_C1						PX(Img_expand_C1)
#define Img_UpScale2_C1						PX(Img_UpScale2_C1)
#define Img_UpScale2_Haar_C1				PX(Img_UpScale2_Haar_C1)


MInt32 Img_Guass3x3_Down2_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch);

MInt32 Img_Guass3x3_Down2_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImgC1, MByte* pDstImgC2, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch);


MInt32 Img_Haar_Down2_C1(MHandle hMemMgr, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch);

MInt32 Img_Haar_Down2_C2(MHandle hMemMgr, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImgC1, MByte* pDstImgC2, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch);


MInt32 Img_expand_C2(MHandle hMemMgr, LPImg_Plane_Data pFullImgUV, LPImg_Plane_Data pHalfImgU, LPImg_Plane_Data pHalfImgV);
MInt32 Img_expand_C1(MHandle hMemMgr, LPImg_Plane_Data pFullImg, LPImg_Plane_Data pHalfImg);


MInt32 Img_UpScale2_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImg_Plane_Data pHalfImg, LPImg_Plane_Data pFullImg);
MInt32 Img_UpScale2_Haar_C1(MHandle hMemMgr, LPImg_Plane_Data pHalfImg, LPImg_Plane_Data pFullImg);


#endif // _IMG_PYRAMID_H_
