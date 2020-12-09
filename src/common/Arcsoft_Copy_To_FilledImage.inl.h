#include "Arcsoft_Copy_To_FilledImage.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
    template <typename T>
    MVoid Arcsoft_Copy_To_FilledImage(ImageInfo<T>* pSrcImage, ImageInfo<T>* pDstImage, MInt32 lExpandSize)
    {
        Arcsoft_Copy_To_FilledImage(pSrcImage->pData,
                                    pSrcImage->lWidth,
                                    pSrcImage->lHeight,
                                    pSrcImage->lPitch,
                                    lExpandSize,
                                    pDstImage->pData,
                                    pDstImage->lWidth,
                                    pDstImage->lHeight,
                                    pDstImage->lPitch);
    }

    template <typename T>
    MVoid Arcsoft_Copy_To_FilledImage(T* pSrc, MInt32 lWidth, MInt32 lHeight, MInt32 lStep, MInt32 lExpandSize,
                                      T* pDstImgPad, MInt32 lWidthPad, MInt32 lHeightPad, MInt32 lStepPad)
    {
        auto *pDstImg = pDstImgPad + lExpandSize * lStepPad + lExpandSize;

        for (MInt32 y = 0; y < lHeight; y++)
        {
            MMemCpy(pDstImg + y*lStepPad, pSrc + y*lStep, lWidth * sizeof(T));
        }
        FillExpandPixels<T>(pDstImgPad, lWidthPad, lHeightPad, lStepPad, lExpandSize);
    }

    template<typename T1>
    MVoid FillExpandPixels(T1 *pSrc,
                           MInt32 lWidth,
                           MInt32 lHeight,
                           MInt32 lPitch,
                           MInt32 lExpandSize)
    {

        for(MInt32 y = lExpandSize; y < lHeight - lExpandSize; y++)
        {
            T1 *pData = pSrc + y * lPitch;
            for(MInt32 k = 0; k < lExpandSize; k++)
            {
                pData[ k ] = pData[ lExpandSize ];
                pData[ lWidth - lExpandSize + k ] = pData[ lWidth - lExpandSize - 1 ];
            }
        }

        for(MInt32 y = 0; y < lExpandSize; y++)
        {
            MMemCpy(pSrc + y * lPitch,
                    pSrc + lExpandSize * lPitch, lPitch * sizeof(T1));
        }

        for(MInt32 y = lHeight - lExpandSize; y < lHeight; y++)
        {
            MMemCpy(pSrc + y * lPitch,
                    pSrc + ( lHeight - lExpandSize - 1 ) * lPitch, lPitch * sizeof(T1));
        }
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END