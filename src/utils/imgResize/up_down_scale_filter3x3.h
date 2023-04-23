
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_UP_DOWN_SCALE_FILTER3X3_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_UP_DOWN_SCALE_FILTER3X3_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"
#include "ImageInfo.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
    template<typename T=MUInt8, typename T1=MUInt8>
    MInt32 filter2D3x3(MHandle hMemMgr, MHandle mcvParallelMonitor,
                       ImageInfo<T>& SrcImage, ImageInfo<T1> DstImage,
                       MInt16 kernel[], MInt32 sumWeight);

    template<typename T=MUInt8, typename T1=MUInt8>
    MInt32 filter2D3x3(MHandle hMemMgr, MHandle mcvParallelMonitor,
                       ImageInfo<T>& SrcImage, ImageInfo<T1> DstImage,
                       MInt16 kernel[], MInt32 sumWeight,
                       MInt16 starLine,
                       MInt16 endLine);

    template<typename T=MUInt8, typename T1=MUInt8>
    MInt32 filter2D3x3Down2(MHandle hMemMgr, MHandle mcvParallelMonitor,
                            ImageInfo<T> largeImage, ImageInfo<T1> smallImage,
                            MInt16 kernel[], MInt32 sumWeight);
    template<typename T=MUInt8, typename T1=MUInt8>
    MInt32 filter2D3x3Down2(MHandle hMemMgr, MHandle mcvParallelMonitor,
                            ImageInfo<T> largeImage, ImageInfo<T1> smallImage,
                            MInt16 kernel[], MInt32 sumWeight,
                            MInt16 starLine,
                            MInt16 endLine);

    template<typename T=MUInt8, typename T1=MUInt8>
    MInt32 filter2D3x3Up2(MHandle hMemMgr, MHandle mcvParallelMonitor,
                          ImageInfo<T> smallImage, ImageInfo<T1> largeImage,
                          MInt16 kernel[], MInt32 sumWeight,
                          MInt16 starLine,
                          MInt16 endLine);
NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "up_down_scale_filter3x3.inl.h"
#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_UP_DOWN_SCALE_FILTER3X3_H
