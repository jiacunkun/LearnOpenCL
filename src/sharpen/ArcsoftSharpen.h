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
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_TEMPLATE_SHARPEN_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_TEMPLATE_SHARPEN_H

#include "single_image_enhancement_define.h"
#include "asvloffscreen.h"
#include "ImageInfo.h"


NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN


    template<typename T = MUInt8>
    class ArcsoftSharpen
    {
    public:
        ArcsoftSharpen(MInt32 nThreadCount = 16);
        ~ArcsoftSharpen();

    public:
        /**
        * @brief
        * @param hMemMgr
        * @param mcvParallelMonitor
        * @param pImage             [in,out]
        * @param width              [in]
        * @param height             [in]
        * @param pitch              [in]
        * @param intensity          [in]        锐化强度，取值范围[0,  20.0],
        * @param lRange             [in]        残差低于lRange的像素不做锐化
        * @param lMaskRange         [in]        暂时没用
        * @return
        */
        MInt32 Run(MHandle hMemMgr,
                   MHandle mcvParallelMonitor,
                   T *pImage,
                   MInt32 width,
                   MInt32 height,
                   MInt32 pitch,
                   MInt32 intensity,
                   MInt32 lRange = 20,
                   MInt32 lMaskRange = 4);

        MInt32 Run(MHandle hMemMgr,
                   MHandle mcvParallelMonitor,
                   T *pImage,
                   T *pAddDetailImage,
                   MInt32 width,
                   MInt32 height,
                   MInt32 pitch,
                   MInt32 intensity,
                   MInt32 lRange = 20,
                   MInt32 lMaskRange = 20);


    public:
        MInt32 GetMask(MHandle hMemMgr, MHandle mcvParallelMonitor,
            T* pImage, MInt32 width, MInt32 height, MInt32 pitch,
            MInt32 lMaskRange, MByte* pSharpenMask);

        MInt32 GetDetailImage(MHandle hMemMgr,
                              MHandle mcvParallelMonitor,
                              T *pImage,
                              MInt32 width,
                              MInt32 height,
                              MInt32 pitch,
                              MInt16 *pDetailImage);

        MInt32 GetDetailImage_U8(MHandle hMemMgr,
                                 MHandle mcvParallelMonitor,
                                 T* pImage,
                                 MInt32 width,
                                 MInt32 height,
                                 MInt32 pitch,
                                 MUInt8* pDetailImage);

        MInt32 GetAnisotropicDetailImage(MHandle hMemMgr,
                                         MHandle mcvParallelMonitor,
                                         MInt16 *pDetailImage,
                                         T *pImage,
                                         MInt32 srcPitch,
                                         MInt32 width,
                                         MInt32 height,
                                         MInt32 guidedPitch,
                                         MInt16 *pAnisotropicDetailImage,
                                         MInt32 dstPitch,
                                         MFloat fEps,
                                         MInt32 lScale,
                                         MInt16* pMeanA = MNull,
                                         MInt16* pMeanB = MNull,
                                         MInt32 AB_type = 1);

        MInt32 GetAnisotropicDetailImage_U8(MHandle hMemMgr,
                                            MHandle mcvParallelMonitor,
                                            MUInt8* pDetailImage,
                                            T* pImage,
                                            MInt32 srcPitch,
                                            MInt32 width,
                                            MInt32 height,
                                            MInt32 guidedPitch,
                                            MUInt8* pAnisotropicDetailImage,
                                            MInt32 dstPitch,
                                            MFloat fEps,
                                            MInt32 lScale,
                                            MInt16 *pMeanA = MNull,
                                            MInt16 *pMeanB = MNull,
                                            MInt32 AB_type = 1);

        MInt32 AddDetail(MHandle hMemMgr,
                         MHandle mcvParallelMonitor,
                         T *pImage,
                         MInt16 *pDetailImage,
                         MInt32 width,
                         MInt32 height,
                         MInt32 pitch,
                         MInt32 intensity,
                         MInt32 lRange);

        MInt32 AddDetail_U8(MHandle hMemMgr,
                            MHandle mcvParallelMonitor,
                            T* pImage,
                            MUInt8* pDetailImage,
                            MInt32 width,
                            MInt32 height,
                            MInt32 pitch,
                            MInt32 intensity,
                            MInt32 lRange);

        MInt32 AddDetail_U8(MHandle hMemMgr,
                         MHandle mcvParallelMonitor,
                         T* pImage,
                         MByte* pDetailImage,
                         MByte* pSharpenMask,
                         MInt32 width,
                         MInt32 height,
                         MInt32 pitch,
                         MInt32 intensity,
                         MInt32 lRange,
                         MInt32 lMaskRange);

    private:
        template<typename T1>
        MVoid filter2D5x5(MHandle hMemMgr,
                          MHandle mcvParallelMonitor,
                          T *pImage,
                          MInt32 width,
                          MInt32 height,
                          MInt32 pitch,
                          MInt16 *offset,
                          MInt16 *kernel,
                          MInt32 lSumWeight,
                          T1 *pFilteredImage);



        MInt32 AddDetail(MHandle hMemMgr,
                         T *pImage,
                         MInt16 *pDetailImage,
                         MInt32 width,
                         MInt32 height,
                         MInt32 pitch,
                         MInt32 intensity,
                         MInt32 lRange,
                         MInt16 starLine,
                         MInt16 endLine);


        MInt32 AddDetail_U8(MHandle hMemMgr,
            T* pImage,
            MUInt8* pDetailImage,
            MInt32 width,
            MInt32 height,
            MInt32 pitch,
            MInt32 intensity,
            MInt32 lRange,
            MInt16 starLine,
            MInt16 endLine);

        MInt32 GetSharpenMask(MHandle hMemMgr, MHandle mcvParallelMonitor,
                              T *pImage, MInt32 width, MInt32 height, MInt32 pitch,
                              MInt32 lMaskRange,
                              MByte *pSharpenMask);


        template<typename T1 = MInt16>
        MInt32 GetABForDetail(T1 **pSrcDetail,
                              T *pGuid,
                              MInt32 lPitchSrc,
                              T1 **Coff_A,
                              T1 **Coff_B,
                              MInt32 lWidth,
                              MInt32 lHeight,
                              MFloat Feps);

        template<typename T1 = MInt16>
        MInt32 GetABForDetail(T1 **pSrcDetail,
                              T *pGuid,
                              MInt32 lPitchSrc,
                              T1 **Coff_A,
                              T1 **Coff_B,
                              MInt32 lWidth,
                              MInt32 lHeight,
                              MFloat Feps,
                              MInt32 startRow /*= -1*/,
                              MInt32 endRow /*= -1*/);

        template<typename T1, typename T2>
        MInt32 MeanABForDetail(T1 **pA,
                               T1 **pB,
                               T2 **pMeanA,
                               T2 **pMeanB,
                               MInt32 lWidth,
                               MInt32 lHeight);

        template<typename T1 = MInt16, typename T2 = MInt16>
        MInt32 MeanABForDetail(T1 **pA,
                               T1 **pB,
                               T2 **pMeanA,
                               T2 **pMeanB,
                               MInt32 lWidth,
                               MInt32 lHeight,
                               MInt32 startRow /*= -1*/,
                               MInt32 endRow /*= -1*/);

        template<typename T1, typename T2>
        MInt32 MeanDetail(T1 **pA,
                          T2 **pB,
                          T2 *pMeanB,
                          MInt32 lWidth,
                          MInt32 lHeight);

        template<typename T1, typename T2>
        MInt32 MeanDetail(T1 **pA,
                          T2 **pMeanB,
                          T2 *pDst,
                          MInt32 lWidth,
                          MInt32 lHeight,
                          MInt32 startRow /*= -1*/,
                          MInt32 endRow /*= -1*/);

        MInt32 GetDirectionDetailImage(MHandle hMemMgr,
                                       MHandle mcvParallelMonitor,
                                       T *pImage,
                                       MInt32 width,
                                       MInt32 height,
                                       MInt32 pitch,
                                       MInt16 *pDetailImage);



        MVoid filter2D5x1(MHandle hMemMgr, MHandle mcvParallelMonitor,
                           T *pImage, MInt32 width, MInt32 height, MInt32 pitch,
                           MInt16 *pOffset,
                           MInt16 kernel[],
                           MInt32 lSumWeight,
                           MInt16 *pFilteredImage);

        MVoid filter2D5x1(MHandle hMemMgr, MHandle mcvParallelMonitor,
                          T *pImage, MInt32 width, MInt32 height, MInt32 pitch,
                          MInt16 *pOffset, MInt16 kernel[], MInt32 lSumWeight,
                          MInt16 *pFilteredImage, MInt32 starLine, MInt32 endLine);

        template<typename T1>
        MVoid filter2D5x5(MHandle hMemMgr, MHandle mcvParallelMonitor,
                           T *pImage, MInt32 width, MInt32 height, MInt32 pitch,
                           MInt16 *offset,
                           MInt16 kernel[],
                           MInt32 lSumWeight,
                           T1 *pFilteredImage,
                           MInt16 starLine,
                           MInt16 endLine);

        MInt32 filter2D3x3(MHandle hMemMgr, MHandle mcvParallelMonitor,
                           T *pImage, MInt32 width, MInt32 height, MInt32 pitch,
                           MInt16 kernel[],
                           MInt16 *pFilteredImage);

        MInt32 gaussionFilter2D3x3(MHandle hMemMgr, MHandle mcvParallelMonitor,
                                   T *pImage, MInt32 width, MInt32 height, MInt32 pitch,
                                   MByte *pGaussionImage);


        MInt32 getGradientMap(MHandle hMemMgr, MHandle mcvParallelMonitor,
                              T *pImage, MInt32 width, MInt32 height, MInt32 pitch,
                              MByte *pGradeienMap);

        inline MVoid Compute_AB_Offset_Direction(MInt32 Shift[][4], MInt32 lPitch);

        inline MVoid ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, ImageInfo<MInt16>* pDstImg);

        inline MVoid ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, ImageInfo<MInt16>* pDstImg, MInt32 lTopLine, MInt32 lBotLine);

    private:
        MInt32 m_nThreadCount;
        MInt32 m_lRange;
        MInt32 m_lMaskRange;
    };


    typedef ArcsoftSharpen<MUInt8> ArcsoftSharpenU8;
    typedef ArcsoftSharpen<MUInt16> ArcsoftSharpenU16;
    typedef ArcsoftSharpen<MInt16> ArcsoftSharpenI16;

NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "ArcsoftSharpen.inl.h"

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_TEMPLATE_SHARPEN_H
