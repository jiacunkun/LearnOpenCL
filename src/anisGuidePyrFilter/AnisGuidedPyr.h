//
// Created by jck7075 on 2020/6/15.
//

#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ANISGUIDEDPYR_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ANISGUIDEDPYR_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

const MUInt8 LAYER = 4;


NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    MInt32 AnisGuidedPyrHandle(MHandle hMemMgr,
                               MHandle mcvParallelMonitor,
                               MUInt8 *pSrc,
                               MUInt8 *pGuide,
                               MUInt8 *pDst,
                               MInt32 lWidth,
                               MInt32 lHeight,
                               MInt32 lPitch,
                               MInt32 lGuidedPitch,
                               MFloat *feps,
                               MInt32 lDetailLuma,
                               MInt32 *sharpIntensity,
                               MInt32 lScale,
                               LPASVLOFFSCREEN pShadeMap = MNull);




    class AnisGuidedPyr
    {
    public:
        AnisGuidedPyr(MHandle hMemMgr = MNull, MHandle mcvParallelMonitor = MNull, MInt32 width = 0, MInt32 height = 0, MInt32 pitch = 0);
        ~AnisGuidedPyr();

        MVoid Init(MInt32 width, MInt32 height, MInt32 pitch);
        MVoid Realease();
        MInt32 Run(MHandle hMemMgr,
                   MHandle mcvParallelMonitor,
                   MUInt8 *pSrc,
                   MUInt8 *pGuide,
                   MUInt8 *pDst,
                   MInt32 lWidth,
                   MInt32 lHeight,
                   MInt32 lPitch,
                   MInt32 lGuidedPitch,
                   MFloat *feps,
                   MInt32 lDetailLuma,
                   MInt32 *sharpIntensity,
                   MInt32 lScale,
                   LPASVLOFFSCREEN pShadeMap = MNull);

    public:
        MInt32 mixImages(MUInt8* pImage1, MUInt8* pImage2,  MInt16* pAlpha, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MUInt8* pMixedImage);
        template <typename T1 = MUInt8, typename T2 = MUInt8>
        MInt32 mixImages(T1* pImage1, T2* pImage2,  MInt16* pAlpha, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MUInt8* pMixedImage);
        MInt32 getFixMeanA(MInt16* pMeanA,  MInt16* pAlpha, MUInt8* pSrcImage, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt16* pFixMeanA);
        MVoid calcFusedMask(MInt16* pMeanA,  MInt16* pAlpha, MUInt8* pSrcImage, MInt32 lDetailLuma, MInt32 maxThr, MInt32 minThr, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt16* pFixMeanA);
        template <typename T>
        MVoid maxFilter(T* pSrc,  MInt32 lRadius, MInt32 lWidth, MInt32 , MInt32 lPitch, T* pDst);
        template <typename T>
        MInt32 mixImages(T* pImage1, T* pImage2,  MInt16* pAlpha, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, T* pMixedImage);
        MInt32 CalEdgeMask(ImageInfo<MUInt8> &input, ImageInfo<MInt16> &output, MInt32 maxThr, MInt32 minThr);
        MInt32 RefineEdgeMask(ImageInfo<MInt16> &input, ImageInfo<MInt16> &output);
        MVoid GetEdgeThr(MInt32 detailLuma, MInt32 *maxThr, MInt32 *minThr);
    private:
        MHandle m_hMemMgr;
        MHandle m_mcvParallelMonitor;
        MInt32 m_lWidth = 0;
        MInt32 m_lHeight = 0;
        MInt32 m_lPitch = 0;
        MInt32 m_lDetailLuma = 0;
        MFloat m_fEps[LAYER];
        ImageInfo<MUInt8> m_SrcPyrImage[LAYER];
        ImageInfo<MUInt8> m_DstPyrImage[LAYER];
        ImageInfo<MUInt8> m_TempBuffer[LAYER];
        ImageInfo<MInt16> m_MeanA[LAYER];
        ImageInfo<MInt16> m_MeanB[LAYER];
        ImageInfo<MInt16> m_MeanALarge[LAYER];
        ImageInfo<MInt16> m_MeanBLarge[LAYER];
        ImageInfo<MInt16> m_StrongEdgeMask[LAYER];
        ImageInfo<MInt16> m_StrongEdgeMaskLarge[LAYER];
        ImageInfo<MInt16> m_StrongEdgeMaskMax[LAYER];
    };


NS_SINFLE_IMAGE_ENHANCEMENT_END


#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ANISGUIDEDPYR_H
