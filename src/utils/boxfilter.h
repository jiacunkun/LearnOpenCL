#ifndef VIDEODEBLUR_BOXFILTER_H
#define VIDEODEBLUR_BOXFILTER_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

MInt32  Box_Filter_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 lRadius, MInt32* boxRowBuf = MNull);

MRESULT Box_Filter_C1(MHandle hMemMgr, MHandle mcvParallelMonitor,
                      MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcLineBytes,
                      MByte *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius, MInt32* boxRowBuf);

MVoid Box_Filter_RowBuf_C2(MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcLineBytes,
                           MByte *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius, MInt32 *boxRowBuf);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //VIDEODEBLUR_BOXFILTER_H
