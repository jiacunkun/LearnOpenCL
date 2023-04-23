#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_COPYIMAGETOIMAGE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_COPYIMAGETOIMAGE_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template <typename T>
    MInt32 CopyImageToImage(T* pSrc, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcPitch,
                            T* pDst,  MInt32 lDstPitch);

    template <typename T>
    MInt32 CopyImageToImage(ImageInfo<T>* pSrc, ImageInfo<T>* pDst);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "CopyImageToImage.inl.h"

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_COPYIMAGETOIMAGE_H
