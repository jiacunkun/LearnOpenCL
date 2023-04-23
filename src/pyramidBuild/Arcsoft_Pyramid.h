#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_PYRAMID_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_PYRAMID_H


#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    struct ImageParam
    {
        MFloat fIntensity;
        MInt32 lScale;
    };

    template <typename T>
    class Arcsoft_Pyramid
    {
    public:
        Arcsoft_Pyramid(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lStride, MInt32 lLayer, MInt32 lThreadCount);
        ~Arcsoft_Pyramid();

        MInt32 init();
        MVoid  release();

        MInt32 run(ImageInfo<T>* pSrc, ImageInfo<T>* pDst, ImageInfo<T>* pShade, ImageParam *pParamArray, MInt32 lMethod);

    public:
        /**
         * @brief 将单层图像数据，做起金字塔图像数据组，外部有扩充，但未做扩充赋值
         * @param pSrc              [in]
         * @param pPyramid          [out]
         * @param pPyramidInner     [out]
         * @param lLayer            [in]
         * @param lExpandSize       [in]
         * @return
         */
        MInt32 BuildPyramid(ImageInfo<T>* pSrc, ImageInfo<T> *pPyramid, ImageInfo<T>* pPyramidInner, MInt32 lLayer, MInt32 lExpandSize);
        MInt32 RestorePyramid(ImageInfo<T>* pSrcPyramid, ImageInfo<T>* pDstPyramid, ImageInfo<T>* pTempPyramid, MInt32 iLayer, MInt32 lExpandSize);

    private:
        MInt32 SingleLayerDenoise(ImageInfo<T>* pSrc, ImageInfo<T>* pDst, ImageInfo<T>* pShade, ImageParam param);

    private:
        MHandle m_hMemMgr;
        MHandle m_mcvParallelMonitor;
        MInt32 m_lThreadCount;
        MInt32 m_lLayer;
        MInt32 m_lWidth;
        MInt32 m_lHeight;
        MInt32 m_lStride;
        MInt32 m_lExpandSize;
        MInt32 m_lMethod;


        ImageInfo<T> m_SrcPyrImage[4];
        ImageInfo<T> m_DstPyrImage[4];
        ImageInfo<T> m_TempBuffer[4];
        ImageInfo<T> m_ShadePyrImage[4];
        ImageInfo<MUInt8> m_dnShade[4]; //降噪强度的mask
    };

NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "Arcsoft_Pyramid.inl.h"

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_PYRAMID_H
