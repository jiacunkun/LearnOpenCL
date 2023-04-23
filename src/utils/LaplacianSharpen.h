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
*******************************************************************************
 
 @brief  类似拉普拉斯锐化的锐化算法

kernel :
     1   1   -4  1   1
     1   1   -4  1   1
     -4  -4  16  -4  -4
     1   1   -4  1   1
     1   1   -4  1   1
  

 @version: 1.0

 @author:

 @date:

 @change:

 @note:

 @todo:
 *******************************************************************************/

#ifndef LaplacianSharpen_h
#define LaplacianSharpen_h

#include <stdio.h>
#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN


    template<typename T = MUInt8>
    class LaplacianSharpen
    {
    public:
        LaplacianSharpen();

        ~LaplacianSharpen();


        /**
         * @brief 8bit图像锐化
         * @param hMemMgr
         * @param mcvParallelMonitor
         * @param srcdst
         * @param pBufSharpen
         * @param width
         * @param height
         * @param pitch
         * @param cn
         * @param intensity
         * @return
         */
        MInt32 sharpen(MHandle hMemMgr,
                       MHandle mcvParallelMonitor,
                       T *srcdst,
                       T *pBufSharpen,
                       MInt32 width,
                       MInt32 height,
                       MInt32 pitch,
                       MInt32 cn,
                       MInt32 intensity,
                       MInt32 nThreadCount = -1);


    private:

        /**
        * @brief
        * @tparam T             只支持 u8 和 u6
        * @param pDataBuf       [in,out]
        * @param pTmpBuf        [in,out]    存放每一行像素值的临时内存,必须左右各扩展2个像素
        * @param width          [in]
        * @param height         [in]
        * @param pitch          [in]        以字节为单位
        * @param channels       [in]        通道数，1表示 y 通道，2表示 uv 通道
        * @param sharpScale     [in]        锐化程度，取值范围[0, 255]
        * @param startRow       [in]
        * @param endRow         [in]
        * @return
        */
        MVoid processRow(T *pDataBuf,
                         T *pTmpBuf,
                         MInt32 width,
                         MInt32 height,
                         MInt32 pitch,
                         MInt32 channels,
                         MInt32 sharpScale,
                         MInt32 startRow,
                         MInt32 endRow);


        MInt32 processRowThreads(MHandle hMemMgr,
                                 MHandle mcvParallelMonitor,
                                 T *pDataBuf,
                                 MInt32 width,
                                 MInt32 height,
                                 MInt32 pitch,
                                 MInt32 channels,
                                 MInt32 sharpScale,
                                 MInt32 nThreadCount = -1);


        MVoid processCol(T *pSrcBuf,
                         T *pDstBuf,
                         MInt32 width,
                         MInt32 height,
                         MInt32 pitch,
                         MInt32 sharpScale,
                         MInt32 startRow,
                         MInt32 endRow);

        MInt32 processColThreads(MHandle hMemMgr,
                                 MHandle mcvParallelMonitor,
                                 T *pSrcBuf,
                                 T *pDstBuf,
                                 MInt32 width,
                                 MInt32 height,
                                 MInt32 pitch,
                                 MInt32 sharpScale,
                                 MInt32 nThreadCount = -1);
    };


    typedef LaplacianSharpen<MUInt8> LaplacianSharpenU8;
    typedef LaplacianSharpen<MInt16> LaplacianSharpenI16;


NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif /* LaplacianSharpen_hpp */
