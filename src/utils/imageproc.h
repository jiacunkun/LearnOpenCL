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
#ifndef _ARCSOFT_IMAGEPROC_H_
#define _ARCSOFT_IMAGEPROC_H_

#include "asvloffscreen.h"
#include "DefineForDebug.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
/// @brief
/// @param szFile
/// @param pData
/// @param nWidth
/// @param nHeight
/// @param nLineBytes
/// @param nBitCount
/// @return
MRESULT SaveBMP(MChar *szFile,
                MUInt8 *pData,
                MLong nWidth,
                MLong nHeight,
                MLong nLineBytes,
                MLong nBitCount);

/// @brief
/// @param szFile
/// @param img
/// @param serial
/// @param gamma
/// @param ispGain
/// @return
MRESULT Save_ASVL(MChar *szFile,
                  ASVLOFFSCREEN img,
                  MInt32 serial = -1);

/// @brief
/// @param p
/// @param width
/// @param height
/// @param pitch
/// @param fname
/// @param folderPath
/// @return
MVoid SaveImg8B(MVoid *p,
                MInt32 width,
                MInt32 height,
                MInt32 pitch,
                const char *fname,
                const char* folderPath = DebugPath);

/// @brief
/// @param pSrc
/// @param width
/// @param height
/// @param pitch
/// @param fname
/// @param folderPath
/// @return
MVoid SaveImg10B(MVoid *pSrc, MInt32 width, MInt32 height, MInt32 pitch, const char *fname, const char* folderPath = DebugPath);





/// @brief 申请图片内存空间, 目前仅支持以下格式:
//                            ASVL_PAF_GRAY
//                            ASVL_PAF_I444
//                            ASVL_PAF_I420
//                            ASVL_PAF_I422H
//                            ASVL_PAF_NV21
//                            ASVL_PAF_NV12
//                            ASVL_PAF_LPI422H
//                            ASVL_PAF_LPI422H2
//                            ASVL_PAF_YUYV
//                            ASVL_PAF_UYVY
/// @param hMemMgr  [in]        context句柄
/// @param lWidth   [in]        图片宽度, 见下面注意事项
/// @param lHeight  [in]        图片高度, 见下面注意事项
/// @param lPAF     [in]        单像素格式，例如 ASVL_PAF_I420 等
/// @param pImgOut  [in,out]    图片格式信息
/// @param lYPitch  [in]        Y通道 stride; 若lYPitch = 0, 内部自己计算, 否则传入自定义的 stride值
/// @param lUVPitch [in]        UV通道 stride, lUVPitch = 0, 内部自己计算, 否则传入自定义的 stride值
/// @return MOK => 创建内存成功
/// 注意：宽高是奇数时, 例如(1921 * 1081)时,
/// LPASVLOFFSCREEN信息如下：pImgOut->i32Width = 1920, pImgOut->i32Height = 1080;
/// 但是实际申请的内存为（1922 * 1082）
/// 目的是避免内存拷贝时，参数传入错误导致的内存越界崩溃;
MRESULT AllocOffscreenMemory(MHandle hMemMgr,
                             MInt32 lWidth,
                             MInt32 lHeight,
                             MInt32 lPAF,
                             LPASVLOFFSCREEN pImgOut,
                             MInt32 lYPitch,
                             MInt32 lUVPitch);

MRESULT AllocOffscreenMemory(MHandle hMemMgr,
                             MInt32 lWidth,
                             MInt32 lHeight,
                             MInt32 lPAF,
                             LPASVLOFFSCREEN pImgOut,
                             MInt32 lYPitch);

MRESULT AllocOffscreenMemory(MHandle hMemMgr,
                             MInt32 lWidth,
                             MInt32 lHeight,
                             MInt32 lPAF,
                             LPASVLOFFSCREEN pImgOut);


/// @brief
/// @param hMemMgr
/// @param pImgIn
/// @return
MVoid FreeOffscreenMemory(MHandle hMemMgr, LPASVLOFFSCREEN pImgIn);


/// @brief 图像数据拷贝, 目前仅支持以下格式
//                        ASVL_PAF_I420
//                        ASVL_PAF_YUYV
//                        ASVL_PAF_UYVY
//                        ASVL_PAF_NV21
//                        ASVL_PAF_NV12
//                        ASVL_PAF_I422H
//                        ASVL_PAF_LPI422H
//                        ASVL_PAF_LPI422H2
//                        ASVL_PAF_GRAY
/// @param pDstImg  [out]
/// @param pSrcImg  [in]
/// @return MOK => 创建内存成功
MRESULT CopyImageData(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg);


NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif // _ARCSOFT_IMAGEPROC_H_
