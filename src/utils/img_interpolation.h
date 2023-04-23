/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary
and confidential information.

The information and code contained in this file is only for authorized ArcSoft
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy,
distribute, modify, or take any action in reliance on it.

If you have received this file in error, please immediately notify ArcSoft and
permanently delete the original and any copy of any file and any printout
thereof.
*******************************************************************************/
#ifndef ARCSOFTSFNR_IMG_INTERPOLATION_H
#define ARCSOFTSFNR_IMG_INTERPOLATION_H

#include "ammem.h"
#include <math.h>
#include "mthread.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
#ifndef ALPHA_TABLE_BL
#define ALPHA_TABLE_BL
typedef struct _tag_ALPHATABLE_BL {
    MInt32 pre_id;
    MInt32 next_id;
    MInt32 pre_alpha;
    MInt32 next_alpha;
} AlphaTable_BL;
#endif

MInt32 BoxDownSample_C2_rows(MByte* srcdata, MInt32 srcW, MInt32 srcH, MInt32 lSrcPitch, MByte* dstdata, MInt32 dstW, MInt32 dstH, MInt32 lDstPitch,
                             MInt32 startRow, MInt32 endRow);
MInt32 BoxDownSample_C2(MHandle mcvParallelMonitor, MByte* srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch,
                        MByte* dstBuf, MInt32 dstW, MInt32 dstH, MInt32 dstPitch);
MVoid calBiliInterTable(MInt32 srows, MInt32 scols, MInt32 drows, MInt32 dcols, AlphaTable_BL *xofs, AlphaTable_BL *yofs);

MVoid fast_bilinear_8U(MByte *srcBuf, MInt32 srows, MInt32 scols, MInt32 srcPitch, MByte *dstBuf, MInt32 drows, MInt32 dcols,
                       MInt32 dstPitch, MInt32 upScale, MInt32 downScale, MInt32 startRow, MInt32 endRow);
MVoid fast_bilinear_32I(MInt32 *srcBuf, MInt32 srows, MInt32 scols, MInt32 *dstBuf, MInt32 drows, MInt32 dcols,
                        MInt32 upScale, MInt32 downScale, MInt32 startRow, MInt32 endRow);
MInt32 FastBilinear_2X_8UC2(MHandle mcvParallelMonitor, MByte* srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch, MByte* dstBuf, MInt32 dstW,
                            MInt32 dstH, MInt32 dstPitch);
MLong fastBilinearInter_8U(MHandle mcvParallelMonitor, MByte *srcBuf, MInt32 srcH, MInt32 srcW, MInt32 srcPitch, MByte *dstBuf,
                           MInt32 dstH, MInt32 dstW, MInt32 dstPitch, MInt32 upScale, MInt32 downScale);
MLong fastBilinearInter_32I(MHandle mcvParallelMonitor, MInt32 *srcBuf, MInt32 *dstBuf, MInt32 lImgHeight, MInt32 lImgWidth, MInt32 upScale, MInt32 downScale);
MVoid LocalFastBilinear_2X_8UC2(MByte *srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch, MByte *dstBuf, MInt32 dstW, MInt32 dstH,
                                MInt32 dstPitch, MInt32 startRow, MInt32 endRow, MInt32 *expand_size);
MInt32 OrgBilinearInter_8U(MHandle mcvParallelMonitor, MByte *pSrcBuf, MInt32 srows, MInt32 scols, MByte *pDstBuf, MInt32 drows, MInt32 dcols);
MVoid OrgLocalBilinearInter_8U(MByte *srcBuf, MInt32 srows, MInt32 scols, MByte *dstBuf, MInt32 drows, MInt32 dcols,
                               MFloat scale_x, MFloat scale_y, AlphaTable_BL *xofs, AlphaTable_BL *yofs, MInt32 startRow, MInt32 endRow);
MInt32 BoxDownSample_C2(MHandle mcvParallelMonitor, MByte* srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch,
                        MByte* dstBuf, MInt32 dstW, MInt32 dstH, MInt32 dstPitch);
MLong fastBilinearInter_8U(MHandle mcvParallelMonitor, MByte *srcBuf, MInt32 srcH, MInt32 srcW, MInt32 srcPitch, MByte *dstBuf,
                           MInt32 dstH, MInt32 dstW, MInt32 dstPitch, MInt32 upScale, MInt32 downScale);
MLong fastBilinearInter_32I(MHandle mcvParallelMonitor, MInt32 *srcBuf, MInt32 *dstBuf, MInt32 lImgHeight, MInt32 lImgWidth, MInt32 upScale, MInt32 downScale);
MInt32 OrgBilinearInter_8U(MHandle mcvParallelMonitor, MByte *pSrcBuf, MInt32 srows, MInt32 scols, MByte *pDstBuf, MInt32 drows, MInt32 dcols);
MInt32 FastBilinear_2X_8UC2(MHandle mcvParallelMonitor, MByte* srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch, MByte* dstBuf, MInt32 dstW,
                            MInt32 dstH, MInt32 dstPitch);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFTSFNR_IMG_INTERPOLATION_H
