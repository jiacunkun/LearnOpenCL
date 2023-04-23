#include "Arcsoft_Copy_To_FilledImage.h"
#include "CopyImageToImage.h"
NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
    template <typename T>
    MVoid Arcsoft_Copy_To_FilledImage(ImageInfo<T>* pSrcImage, ImageInfo<T>* pDstImage, MInt32 lExpandSize)
    {
        Arcsoft_Copy_To_FilledImage(pSrcImage->pData,
                                    pSrcImage->lWidth,
                                    pSrcImage->lHeight,
                                    pSrcImage->lStride,
                                    lExpandSize,
                                    pDstImage->pData,
                                    pDstImage->lWidth,
                                    pDstImage->lHeight,
                                    pDstImage->lStride);
    }

    template <typename T>
    MVoid Arcsoft_Copy_To_FilledImage(T* pSrc, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lExpandSize,
                                      T* pDstImgPad, MInt32 lWidthPad, MInt32 lHeightPad, MInt32 lPitchPad)
    {
        auto *pDstImg = pDstImgPad + lExpandSize * lPitchPad + lExpandSize; // add offset

        CopyImageToImage(pSrc, lWidth, lHeight, lPitch, pDstImg, lPitchPad);

        FillExpandPixels<T>(pDstImgPad, lWidthPad, lHeightPad, lPitchPad, lExpandSize);
    }

    template<typename T1>
    MVoid FillExpandPixels(T1 *pSrc,
                           MInt32 lWidth,
                           MInt32 lHeight,
                           MInt32 lStride,
                           MInt32 lExpandSize)
    {

        for(MInt32 y = lExpandSize; y < lHeight - lExpandSize; y++)
        {
            T1 *pData = pSrc + y * lStride;
            for(MInt32 k = 0; k < lExpandSize; k++)
            {
                pData[ k ] = pData[ lExpandSize ];
                pData[ lWidth - lExpandSize + k ] = pData[ lWidth - lExpandSize - 1 ];
            }
        }

        for(MInt32 y = 0; y < lExpandSize; y++)
        {
            MMemCpy(pSrc + y * lStride,
                    pSrc + lExpandSize * lStride, lStride * sizeof(T1));
        }

        for(MInt32 y = lHeight - lExpandSize; y < lHeight; y++)
        {
            MMemCpy(pSrc + y * lStride,
                    pSrc + ( lHeight - lExpandSize - 1 ) * lStride, lStride * sizeof(T1));
        }
    }

    template<typename T1>
    MVoid PaddingImage(ImageInfo<T1>* pSrcImage, MInt32 lExpandSize)
    {
        FillExpandPixels<T1>(pSrcImage->pData, pSrcImage->lWidth, pSrcImage->lHeight, pSrcImage->lStride, lExpandSize);
    }

    template<typename T1>
    MVoid PaddingImageInner(ImageInfo<T1>* pSrcImage, MInt32 lExpandSize)
    {
        auto lWidth = pSrcImage->lWidth + lExpandSize*2;
        auto lHeight = pSrcImage->lHeight + lExpandSize*2;
        auto lStride = pSrcImage->lStride;
        auto pData = pSrcImage->pData - lExpandSize - lExpandSize*lStride;

        FillExpandPixels<T1>(pData, lWidth, lHeight, lStride, lExpandSize);
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END