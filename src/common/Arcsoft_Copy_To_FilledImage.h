#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_COPY_TO_FILLEDIMAGE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_COPY_TO_FILLEDIMAGE_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
    template <typename T>
    MVoid Arcsoft_Copy_To_FilledImage(ImageInfo<T>* pSrcImage, ImageInfo<T>* pDstImage, MInt32 lExpandSize);

    template <typename T>
    MVoid Arcsoft_Copy_To_FilledImage(T* pSrc, MInt32 lWidth, MInt32 lHeight, MInt32 lStep, MInt32 lExpandSize,
                                      T* pDstImgPad, MInt32 lWidthPad, MInt32 lHeightPad, MInt32 lStepPad);

    template<typename T1>
    inline MVoid FillExpandPixels(T1 *pSrc,
                           MInt32 lWidth,
                           MInt32 lHeight,
                           MInt32 lStep,
                           MInt32 lExpandSize);
NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "Arcsoft_Copy_To_FilledImage.inl.h"

#endif
