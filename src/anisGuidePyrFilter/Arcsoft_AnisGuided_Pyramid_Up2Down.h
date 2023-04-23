
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISGUIDED_PYRAMID_UP2DOWN_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISGUIDED_PYRAMID_UP2DOWN_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template <typename T = MUInt8, typename T0 = MUInt8>
    class Arcsoft_AnisGuided_Pyramid_Up2Down
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
        Arcsoft_AnisGuided_Pyramid_Up2Down(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 layer = 4, MInt32 nThreadCount = 16);
        ~Arcsoft_AnisGuided_Pyramid_Up2Down();


        MInt32 run(MUInt8 *pSrc, MUInt8 *pGuided, MUInt8 *pDst, LPASVLOFFSCREEN pShade,
                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lGuidedPitch,
                   MFloat* pEps, MInt32* pSharpenIntensity, MInt32 lScale = 1);



        MInt32 run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pGuided, LPASVLOFFSCREEN pDst,
                   MFloat* pEps, MInt32* pSharpenIntensity, LPASVLOFFSCREEN pShade = MNull, MInt32 lScale = 1);

        static MVoid GetEdgeThr(MInt32 nLuma, MInt32 nLumaDetail, MInt32* max_threshold, MInt32* min_threshold, MInt16* contrast_param);

        template<typename T1>
        static MInt32 CalEdgeMask(ImageInfo<T1> &input, ImageInfo<MInt16> &output, MInt32 maxThr, MInt32 minThr);
        static MInt32 RefineEdgeMask(ImageInfo<MInt16> &input, ImageInfo<MInt16> &output);

    private:
        template<typename T1>
        inline MVoid ImageSubImage(ImageInfo<T1>* pImage, ImageInfo<T1>* pSubImg);
        template<typename T1>
        inline MVoid ImageSubImage(ImageInfo<T1>* pImage, ImageInfo<T1>* pSubImg, MInt32 lTopLine, MInt32 lBotLine);

        inline MVoid ImageSubImage(ImageInfo<T>* pImage, ImageInfo<T>* pSubImg, MInt32 lTopLine, MInt32 lBotLine, ImageInfo<MInt16>* pLaplaceImg);

        template<typename T1>
        inline MVoid ImageAddImage(ImageInfo<T1>* pImage, ImageInfo<T1>* pAddImg);
        template<typename T1>
        inline MVoid ImageAddImage(ImageInfo<T1>* pImage, ImageInfo<T1>* pAddImg, MInt32 lTopLine, MInt32 lBotLine);

        template<typename T1>
        inline MVoid ImageAddImage(ImageInfo<T1>* pImage, ImageInfo<T1>* pAddImg, ImageInfo<T1>* pDst, MInt32 lTopLine, MInt32 lBotLine);

        template<typename T2, typename T3>
        MVoid MixImages(ImageInfo<T2>* pImage1, ImageInfo<T3>* pImage2,  ImageInfo<MInt16>* pAlpha, ImageInfo<T3>* pDstImage);

        template<typename T2, typename T3>
        MVoid MixImages(ImageInfo<T2>* pImage1, ImageInfo<T3>* pImage2,  MInt16 pAlpha, ImageInfo<T3>* pDstImage);

    private:
        MHandle m_hMemMgr;
        MHandle m_mcvParallelMonitor;
        MInt32 m_nThreadCount;
        MInt32 m_lDirection;
        MInt32 m_lLayer;
        MInt32 m_lWidth;
        MInt32 m_lHeight;
        MInt32 m_lPitch;

        ImageInfo<MUInt8> m_SrcPyrImage0;
        ImageInfo<MUInt8> m_DstPyrImage0;
        ImageInfo<T0> m_SrcPyrImage[5] = { MNull };
        ImageInfo<T0> m_DstPyrImage[5] = { MNull };
        ImageInfo<MUInt8> m_GuidedImage[5] = { MNull };
        ImageInfo<MUInt8> m_MeanA[5] = { MNull };
        ImageInfo<MUInt8> m_MeanB[5] = { MNull };
        ImageInfo<T0> m_TempBuffer[5];

        MInt16* m_pMeanA[4] = { MNull };
        MInt16* m_pMeanB[4] = { MNull };
    };

NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "Arcsoft_AnisGuided_Pyramid_Up2Down.inl.h"

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISGUIDED_PYRAMID_UP2DOWN_H
