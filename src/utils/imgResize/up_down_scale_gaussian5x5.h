#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_UP_DOWN_SCALE_GAUSSIAN5X5_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_UP_DOWN_SCALE_GAUSSIAN5X5_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    MInt32 Img_Guass5x5_Down2_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);
    MRESULT Img_Guass5x5_Up2_u8(MHandle hMemMgr, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);
    MRESULT Img_Guass5x5_Up2AndSub_u8(MHandle hMemMgr, LPASVLOFFSCREEN pSmallSrcImg, LPASVLOFFSCREEN pLargeRefImg, LPASVLOFFSCREEN pLargeDstImg);
    MInt32 Img_Guass5x5_Down4(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
                              MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch);

    MRESULT Img_Guass5x5_Down2_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg,
                                   LPASVLOFFSCREEN pDstImg);
    MRESULT Img_Guass5x5_Up2_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);
    MRESULT Img_Guass5x5_Up2_sub_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pRefImg, LPASVLOFFSCREEN pDstImg);
    MRESULT Img_Guass5x5_Up2_add_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pRefImg, LPASVLOFFSCREEN pDstImg);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_UP_DOWN_SCALE_GAUSSIAN5X5_H
