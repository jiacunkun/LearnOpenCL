
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISGUIDED_PYRAMID_DOWN2UP_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISGUIDED_PYRAMID_DOWN2UP_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    class Arcsoft_AnisGuided_Pyramid_Down2Up
    {
    public:

        Arcsoft_AnisGuided_Pyramid_Down2Up(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 layer = 4, MInt32 nThreadCount = 16);
        ~Arcsoft_AnisGuided_Pyramid_Down2Up();


        MInt32 run(MUInt8 *pSrc, MUInt8 *pGuided, MUInt8 *pDst, LPASVLOFFSCREEN pShade,
                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lGuidedPitch,
                   MFloat* pEps, MInt32* pSharpenIntensity, MInt32 lScale = 1);



        MInt32 run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pGuided, LPASVLOFFSCREEN pDst,
                   MFloat* pEps, MInt32* pSharpenIntensity, LPASVLOFFSCREEN pShade = MNull, MInt32 lScale = 1);


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
        ImageInfo<MInt16> m_SrcPyrImage[3];
        ImageInfo<MInt16> m_DstPyrImage[3];
        ImageInfo<MUInt8> m_GuidedImage[4];
        ImageInfo<MUInt8> m_dnShade[4]; //降噪强度的mask
    };

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISGUIDED_PYRAMID_DOWN2UP_H
