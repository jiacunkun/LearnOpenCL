#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_IMAGEINFO_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_IMAGEINFO_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

struct Rect
{
    MInt32 x;
    MInt32 y;
    MInt32 lWidth;
    MInt32 lHeight;
};

template <typename T = MUInt8>
class ImageInfo
{
public:
    ImageInfo(T* pData, MInt32 lWidth, MInt32 lHeight, MInt32 lStride);
    ImageInfo(LPASVLOFFSCREEN pData);
    ImageInfo(MHandle hMemMgr, MInt32 lWidth, MInt32 lHeight, MInt32 lStride);
    ImageInfo(const ImageInfo<T> srcImage, const Rect& roi);
    ImageInfo();
    ~ImageInfo();

public:
    MInt32 CopyTo(ImageInfo<T>& dstImage);
    MInt32 CopyFrom(ImageInfo<T>& srcImage);

public:
    static MVoid ImageInfo2ASVLOFFSCREEN(ImageInfo<MUInt8>* pSrcImage, ASVLOFFSCREEN& dstImage0);
    static MVoid ImageInfo2ASVLOFFSCREEN(ImageInfo<MInt16>* pSrcImage, ASVLOFFSCREEN& dstImage0);
    static MVoid ASVLOFFSCREEN2ImageInfo(ASVLOFFSCREEN srcImage, ImageInfo<MUInt8>& dstImage);
    static MVoid ASVLOFFSCREEN2ImageInfo(ASVLOFFSCREEN srcImage, ImageInfo<MInt16>& dstImage);

public:
    static MVoid ImageSubImage(ImageInfo<T>* pSrcDst, ImageInfo<T>* pSubImg, MInt32 lThreadCount = -1);
    static MVoid ImageAddImage(ImageInfo<T>* pSrcDst, ImageInfo<T>* pAddImg, MInt32 lThreadCount = -1);
    static MVoid ImageSubImage(ImageInfo<T>* pSrcDst, ImageInfo<T>* pSubImg, MInt32 lTopLine, MInt32 lBotLine);
    static MVoid ImageAddImage(ImageInfo<T>* pSrcDst, ImageInfo<T>* pAddImg, MInt32 lTopLine, MInt32 lBotLine);

public:
    T* pData = MNull;
    MInt32 lWidth = 0;
    MInt32 lHeight = 0;
    MInt32 lStride = 0;
private:
    MBool isNewMem = false;
    MHandle m_hMemMgr = MNull;
};

NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "ImageInfo.inl.h"

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_IMAGEINFO_H
