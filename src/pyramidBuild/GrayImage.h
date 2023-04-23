#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_GRAYIMAGE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_GRAYIMAGE_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

template <typename T = MUInt8>
class GrayImage
{
public:
    GrayImage();
    GrayImage(LPASVLOFFSCREEN pImage, MInt32 lChannel = 0);
    ~GrayImage();

public:
    MInt32 getWidth() const;
    MInt32 getHeight() const;
    MInt32 getStride() const;


private:
    T* pData = MNull;
    MInt32 lWidth = 0;
    MInt32 lHeight = 0;
    MInt32 lStride = 0; // pitch/size(T)
};

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_GRAYIMAGE_H
