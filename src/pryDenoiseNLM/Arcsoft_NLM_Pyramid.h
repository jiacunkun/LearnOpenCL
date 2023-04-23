#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NLM_PYRAMID_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NLM_PYRAMID_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template <typename T>
    class Arcsoft_NLM_Pyramid
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
        Arcsoft_NLM_Pyramid(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 width, MInt32 height, MInt32 lSrcPitch, MInt32 pitch, MInt32 layer);
        ~Arcsoft_NLM_Pyramid();

        MInt32 init(MInt32 width, MInt32 height, MInt32 lSrcPitch, MInt32 pitch, MInt32 layer);

        MInt32 run(MUInt8* pSrc, MUInt8* pDst, LPASVLOFFSCREEN pShade,
                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                   MFloat* pNoiseVarY);

        MInt32 run(MUInt8* pSrc, MUInt8* pDst, LPASVLOFFSCREEN pShadepShade,
                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                   MFloat fNoiseVarY, MBool isFirstLayerDenoise = false);

        MInt32 run(MInt16 *pSrc, MInt16 *pDst, LPASVLOFFSCREEN pShadepShade,
                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                   MFloat fNoiseVarY, MBool isFirstLayerDenoise = false);


        /**
         * @brief 兼容LPASVLOFFSCREEN格式
         * @param pSrc
         * @param pDst
         * @param feps
         * @param lLayer
         * @return
         */
        MInt32 run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat fNoiseVarY, MBool isFirstLayerDenoise = false, LPASVLOFFSCREEN pShade = MNull);
        
        MInt32 run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat* pfNoiseVarY, LPASVLOFFSCREEN pShade = MNull);


    private:
        inline MVoid CreateDarkShadeMask(ImageInfo<MUInt8> *pMask, ImageInfo<T> *pGrayImg, MInt32 lDarkEnhance, MInt32 lDarkThres);
        inline MVoid CreateRadialMask(MByte *pImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch);
        MVoid BuildPyramid(ImageInfo<T>* pPyramidImage, MInt32 lLayer);
        MVoid ReconstructPyramid(ImageInfo<T> srcImage, ImageInfo<T> tempImage, ImageInfo<T> dstImage);
        inline MVoid ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg);
        inline MVoid ImageAddImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pAddImg);
        inline MVoid ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, MInt32 lTopLine, MInt32 lBotLine);
        inline MVoid ImageAddImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pAddImg, MInt32 lTopLine, MInt32 lBotLine);
    private:
        MHandle m_hMemMgr;
        MHandle m_mcvParallelMonitor;
        MInt32 m_lLayer;
        MInt32 m_lWidth;
        MInt32 m_lHeight;
        MInt32 m_lPitch;

        ImageInfo<T> m_SrcPyrImage[4];
        ImageInfo<T> m_DstPyrImage[4];
        ImageInfo<T> m_TempBuffer[4];
        ImageInfo<MUInt8> m_dnShade[4]; //降噪强度的mask
    };

    typedef Arcsoft_NLM_Pyramid<MInt16>  Arcsoft_NLM_Pyramid_I16;
    typedef Arcsoft_NLM_Pyramid<MUInt8>  Arcsoft_NLM_Pyramid_U8;
NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "Arcsoft_NLM_Pyramid.inl.h"

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_NLM_PYRAMID_H

