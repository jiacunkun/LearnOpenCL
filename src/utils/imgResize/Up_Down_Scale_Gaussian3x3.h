#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_UP_DOWN_SCALE_GAUSSIAN3X3_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_UP_DOWN_SCALE_GAUSSIAN3X3_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
/**********************************************************
*  金字塔上采样
**********************************************************/
    template <typename T>
    MInt32 Guass3x3Up2(ImageInfo<T> *pHalfImg, ImageInfo<T> *pFullImg, MInt32 lTopLine, MInt32 lBotLine);

    template <typename T>
    MInt32 Guass3x3Up2Threads(ImageInfo<T> *pHalfImg, ImageInfo<T> *pFullImg, MInt32 nThreadCount = -1);

    template <typename T>
    MInt32 Guass3x3Up2Threads(MHandle mcvParallelMonitor, ImageInfo<T>* pHalfImg, ImageInfo<T>* pFullImg, MInt32 nThreadCount = -1);
/**********************************************************
*  金字塔下采样
**********************************************************/
    template <typename T>
    MInt32 Guass3x3Down2(MHandle hMemMgr,
                         T *pSrcImg,
                         MInt32 lSrcWidth,
                         MInt32 lSrcHeight,
                         MInt32 lSrcPitch,
                         T *pDstImg,
                         MInt32 lDstWidth,
                         MInt32 lDstHeight,
                         MInt32 lDstPitch,
                         MInt32 lTopLine,
                         MInt32 lBotLine,
                         MShort *SmoothBuf);

    template <typename T>
    MInt32 Guass3x3Down2Threads(MHandle hMemMgr,
                                T *pSrcImg,
                                MInt32 lSrcWidth,
                                MInt32 lSrcHeight,
                                MInt32 lSrcPitch,
                                T *pDstImg,
                                MInt32 lDstWidth,
                                MInt32 lDstHeight,
                                MInt32 lDstPitch,
                                MInt32 nThreadCount);
    template <typename T>
    MInt32 Guass3x3Down2Threads(MHandle hMemMgr, ImageInfo<T> *src, ImageInfo<T> *dst, MInt32 nThreadCount = -1);

    template <typename T>
    MInt32 Guass3x3Down2Threads(MHandle hMemMgr,
        MHandle mcvParallelMonitor,
        T* pSrcImg,
        MInt32 lSrcWidth,
        MInt32 lSrcHeight,
        MInt32 lSrcPitch,
        T* pDstImg,
        MInt32 lDstWidth,
        MInt32 lDstHeight,
        MInt32 lDstPitch,
        MInt32 nThreadCount);
    template <typename T>
    MInt32 Guass3x3Down2Threads(MHandle hMemMgr, MHandle mcvParallelMonitor, ImageInfo<T>* src, ImageInfo<T>* dst, MInt32 nThreadCount = -1);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "Up_Down_Scale_Gaussian3x3.inl.h"

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_UP_DOWN_SCALE_GAUSSIAN3X3_H
