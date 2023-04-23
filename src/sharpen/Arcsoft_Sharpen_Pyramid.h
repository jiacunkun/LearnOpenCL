#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SHARPEN_PYRAMID_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SHARPEN_PYRAMID_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

#define DETAIL_U8

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template <typename T>
    class Arcsoft_Sharpen_Pyramid
    {
    public:
        Arcsoft_Sharpen_Pyramid(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 width, MInt32 height, MInt32 pitch, MInt32 layer, MInt32 lThreadCount = 8);
        ~Arcsoft_Sharpen_Pyramid();

        MInt32 init();
        MVoid  release();

        MInt32 run(T *pSrcDst,
                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                   MInt32 lIntensity[], MInt32 lRange[], MInt32 lFilterVal = 50);

    private:
        template<class T1>
        MVoid ImageAddImage(ImageInfo<T1>* m_pImage, ImageInfo<T1>* pAddImg, MInt32 lIntensity, MInt32 lRange, MInt32 lTopLine, MInt32 lBotLine);

    private:
        MHandle m_hMemMgr;
        MHandle m_mcvParallelMonitor;
        MInt32 m_lThreadCount;
        MInt32 m_lLayer;
        MInt32 m_lWidth;
        MInt32 m_lHeight;
        MInt32 m_lPitch;

        ImageInfo<T> m_SrcPyrImage[4] = { MNull };
        ImageInfo<T> m_DstPyrImage[4] = { MNull };
#ifdef DETAIL_U8
        ImageInfo<MUInt8> m_DetailImage[4] = { MNull };
        ImageInfo<MUInt8> m_TempBuffer[4] = { MNull };
#else
        ImageInfo<MInt16> m_DetailImage[4] = { MNull };
        ImageInfo<MInt16> m_TempBuffer[4] = { MNull };
#endif
        MInt16* m_pMeanA[4] = {MNull};
        MInt16* m_pMeanB[4] = {MNull};
    };

NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "Arcsoft_Sharpen_Pyramid.inl.h"

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_SHARPEN_PYRAMID_H
