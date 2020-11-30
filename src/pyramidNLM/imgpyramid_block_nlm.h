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

#include "nlmblock.h"
#include "asvloffscreen.h"
#include "amcomdef.h"
#include "imagebase.h"
#include "mobilecv.h"

// ========== configuration for pyramid operations ========== //
#define MAX_LEVELS	3	// the max level of pyramid is (MAX_LEVELS+1)
//#define TOP_LEVEL_DENOISE
// ========== configuration for pyramid operations ========== //
#define USE_GUS_PYR

#define SG_NLM_THR_NUM (4)

#ifndef TRIMBYTE
#define TRIMBYTE(x) (MByte)( (x) < 0 ? 0 :( (x) >255 ? 255 :(x) ))
#endif

//typedef struct _tag_IMGPYRAMID {
//	MByte	*pImage;
//	MInt32	lWidth;
//	MInt32	lHeight;
//	MInt32	lPitch;
//	MInt32  lCn;
//}ImgPyramid_Block_NLM, *LPImgPyramid_Block_NLM;

typedef struct _tag_IMGPLANE {
    MByte	*pImage;
    MInt32	lWidth;
    MInt32	lHeight;
    MInt32	lPitch;
    MInt32  lCn;
    MVoid Img_Plane_Data()
    {
        pImage = MNull;
        lWidth = 0;
        lHeight = 0;
        lPitch = 0;
        lCn = 0;
    }
}Img_Plane_Data, *LPImg_Plane_Data;

#define Image_Pyramid_Creat_Build			PX(Image_Pyramid_Creat_Build)
#define Image_Pyramid_Creat_Build_Haar		PX(Image_Pyramid_Creat_Build_Haar)
#define Img_Denoise_Block_NLM				PX(Img_Denoise_Block_NLM)
#define Local_ImgData						PX(Local_ImgData)
#define Free_ImgData						PX(Free_ImgData)
#define Img_Sub_C1							PX(Img_Sub_C1)
#define Img_Add_C1							PX(Img_Add_C1)
#define Img_Add_C2							PX(Img_Add_C2)

MInt32 Image_Pyramid_Creat_Build(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPImg_Plane_Data Y_Data, LPImg_Plane_Data UVData,
                                 MInt32 lLevel, MBool bY_Flag, MBool bUV_Flag);

MInt32 MaskImage_Pyramid_Creat_Build(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pMaskImg, LPImg_Plane_Data Mask_Data, MInt32 lLevel);

MInt32 Image_Pyramid_Creat_Build_Haar(MHandle hMemMgr, LPASVLOFFSCREEN pSrcImg, LPImg_Plane_Data Y_Data,
                                      LPImg_Plane_Data UVData, MInt32 lLevel, MBool bY_Flag, MBool bUV_Flag);

MInt32  Img_Denoise_Block_NLM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pDstImg, MInt32* pMap, MInt32* pInvMap);

MInt32 Img_Denoise_Block_Mask_NLM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pDstImg,
                                  LPImg_Plane_Data pMask_Data, MInt32* pMap, MInt32* pInvMap);

MInt32 Img_Denoise_Guide_Block_Mask_NLM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pDstImg,
                                        LPImg_Plane_Data pGuideImg, LPImg_Plane_Data pMask_Data, MInt32* pMap, MInt32* pInvMap);

MInt32 Local_ImgData(MHandle hMemMgr, LPImg_Plane_Data pImgData, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lCn);

MVoid  Free_ImgData(MHandle hMemMgr, LPImg_Plane_Data pImgData);

MVoid Img_Sub_C1(MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcDstImg, LPImg_Plane_Data pSubImg);
MVoid Img_Add_C1(MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcDstImg, LPImg_Plane_Data pAddImg);
MVoid Img_Add_C2(MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcDstImg, LPImg_Plane_Data pAddImg01, LPImg_Plane_Data pAddImg02);

MVoid Img_Add_C2_To_C1(MHandle mcvParallelMonitor, LPImg_Plane_Data pSrcImg, LPImg_Plane_Data pAddDstImg01, LPImg_Plane_Data pAddDstImg02);
MVoid Img_Meger_C1_To_C2(MHandle mcvParallelMonitor, LPImg_Plane_Data pDstImgC2, LPImg_Plane_Data pSrcImg01, LPImg_Plane_Data pSrcImg02);

MInt32 Alloc_Image_Pyramid_Mem(MHandle hMemMgr, LPImg_Plane_Data Y_Data, LPImg_Plane_Data UVData,
                               MInt32 lWidth, MInt32 lHeight, MInt32 lLevel, MBool  bY_Flag, MBool bUV_Flag);

MVoid Free_Image_Pyramid_Mem(MHandle hMemMgr, LPImg_Plane_Data Y_Data, LPImg_Plane_Data UVData, MInt32 lLevel);




#endif // _IMG_PYRAMID_H_
