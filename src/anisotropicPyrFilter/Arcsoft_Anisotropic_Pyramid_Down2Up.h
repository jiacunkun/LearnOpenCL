#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISOTROPIC_PYRAMID_DOWN2UP_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISOTROPIC_PYRAMID_DOWN2UP_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    class Arcsoft_Anisotropic_Pyramid_Down2Up
    {
    public:

        Arcsoft_Anisotropic_Pyramid_Down2Up(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 layer = 4, MInt32 nThreadCount = 16);
        ~Arcsoft_Anisotropic_Pyramid_Down2Up();


        MInt32 run(MUInt8 *pSrc, MUInt8 *pDst, LPASVLOFFSCREEN pShade,
                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                   MFloat* pEps, MInt32* pSharpenIntensity,
                   MInt32 *absDifScale = MNull, MInt32 *weiEachRange = MNull);



        MInt32 run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst,
                   MFloat* pEps, MInt32* pSharpenIntensity, LPASVLOFFSCREEN pShade = MNull,
                   MInt32 *absDifScale = MNull, MInt32 *weiEachRange = MNull);


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
    };

NS_SINFLE_IMAGE_ENHANCEMENT_END


#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_ANISOTROPIC_PYRAMID_DOWN2UP_H
