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
#ifndef _IMG_ROTATION_H_
#define _IMG_ROTATION_H_

#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "imagebase.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
MRESULT ImgRotateRestrictAngle_C3(MHandle mcvParEngine, MInt32 tdNum, MLong lDegree,
	MUInt8* pSrc, MLong lSrcP, MLong lSrcW, MLong lSrcH,
	MUInt8* pDst, MLong lDstP, MLong lDstW, MLong lDstH);

MRESULT ImgRotateRestrictAngle_C1(MHandle mcvParEngine, MInt32 tdNum, MLong lDegree,
	MUInt8* pSrc, MLong lSrcP, MLong lSrcW, MLong lSrcH,
	MUInt8* pDst, MLong lDstP, MLong lDstW, MLong lDstH);

NS_SINFLE_IMAGE_ENHANCEMENT_END
#endif // _ARCSOFT_CONV_PIXFMT_H_
