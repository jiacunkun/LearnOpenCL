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

#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_REMOVEBLOCKNOISE_PYRAMID_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_REMOVEBLOCKNOISE_PYRAMID_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    const MInt32 LAYER = 2;

/**
 *
 * @tparam T 金字塔的数据类型
 */
    template <typename T>
    class RemoveBlockNoise_Pyramid
    {
    public:
        /**
         * @brief 构造函数，用来初始化金字塔内存
         * @param hMemMgr
         * @param mcvParallelMonitor
         * @param width
         * @param height
         * @param pitch
         */
        RemoveBlockNoise_Pyramid(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 width, MInt32 height, MInt32 pitch, MInt32 dst_pitch);
        ~RemoveBlockNoise_Pyramid();

        MInt32 init();
        MVoid  release();

        /**
         * @brief 去除块状噪声
         * @param pSrc
         * @param pDst
         * @param feps
         * @param lLayer
         * @return
         */
        MInt32 run(LPASVLOFFSCREEN pSrc,LPASVLOFFSCREEN pDst, MFloat feps, MInt32 lLayer);

        MInt32 run(MUInt8 *pSrc, MUInt8 *pDst,
                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MFloat feps, MInt32 lLayer);
    private:

        template <typename T0>
        MVoid ReconstructPyramid(ImageInfo<T0> srcImage, ImageInfo<T0> tempImage, ImageInfo<T0> dstImage);
        inline MVoid ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg);
        inline MVoid ImageAddImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pAddImg);
        inline MVoid ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, MInt32 lTopLine, MInt32 lBotLine);
        inline MVoid ImageAddImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pAddImg, MInt32 lTopLine, MInt32 lBotLine);
    private:
        MHandle m_hMemMgr;
        MHandle m_mcvParallelMonitor;
        MInt32 m_lThreadCount;
        MInt32 m_lLayer;
        MInt32 m_lWidth;
        MInt32 m_lHeight;
        MInt32 m_lPitch;
        MInt32 m_lDstPitch;

        ImageInfo<MUInt8> m_SrcPyrImage0;
        ImageInfo<MUInt8> m_DstPyrImage0;
        ImageInfo<MUInt8> m_TempBuffer0; // 小图原图上采样结果
        ImageInfo<T> m_SrcPyrImage[LAYER] = { MNull };
        ImageInfo<T> m_DstPyrImage[LAYER] = { MNull };
        ImageInfo<T> m_TempBuffer[LAYER] = { MNull };
    };

    typedef RemoveBlockNoise_Pyramid<MUInt8> RemoveBlockNoise_Pyramid_U8;
    typedef RemoveBlockNoise_Pyramid<MInt16> RemoveBlockNoise_Pyramid_I16;

NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "RemoveBlockNoise_Pyramid.inl.h"

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_REMOVEBLOCKNOISE_PYRAMID_H

