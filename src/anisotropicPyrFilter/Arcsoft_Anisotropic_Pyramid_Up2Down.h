#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISOTROPIC_PYRAMID_UP2DOWN_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISOTROPIC_PYRAMID_UP2DOWN_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template <typename T = MUInt8 >
    class Arcsoft_Anisotropic_Pyramid_Up2Down
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
        Arcsoft_Anisotropic_Pyramid_Up2Down(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 layer = 4, MInt32 nThreadCount = 16);
        ~Arcsoft_Anisotropic_Pyramid_Up2Down();


        MInt32 run(MUInt8 *pSrc, MUInt8 *pDst, LPASVLOFFSCREEN pShade,
                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                   MFloat* pEps, MInt32* pSharpenIntensity,
                   MInt32 *absDifScale = MNull, MInt32 *weiEachRange = MNull);



        MInt32 run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst,
                   MFloat* pEps, MInt32* pSharpenIntensity, LPASVLOFFSCREEN pShade = MNull,
                   MInt32 *absDifScale = MNull, MInt32 *weiEachRange = MNull);

    private:
        MVoid ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, MInt32 lTopLine, MInt32 lBotLine);
        MVoid ImageAddImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pAddImg, MInt32 lTopLine, MInt32 lBotLine);
    private:
        MHandle m_hMemMgr;
        MHandle m_mcvParallelMonitor;
        MInt32 m_nThreadCount;
        MInt32 m_lDirection;
        MInt32 m_lLayer;
        MInt32 m_lWidth;
        MInt32 m_lHeight;
        MInt32 m_lPitch;

        ImageInfo<MUInt8> m_SrcPyrImage[4];
        ImageInfo<MUInt8> m_DstPyrImage[4];
        ImageInfo<MUInt8> m_TempBuffer[4];
    };

NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "Arcsoft_Anisotropic_Pyramid_Up2Down.inl.h"

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISOTROPIC_PYRAMID_UP2DOWN_H
