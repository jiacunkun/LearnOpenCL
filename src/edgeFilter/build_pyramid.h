#ifndef DIS_OPTICALFLOW_BUILD_PYRAMID_H
#define DIS_OPTICALFLOW_BUILD_PYRAMID_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

MInt32 BuildGauPyrs_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, ASVLOFFSCREEN* pyrGau, MInt32 nlevGau, MUInt16** tmpGuassBuf = MNull);

MVoid BuildLapPyrs_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, ASVLOFFSCREEN* pyrGau, ASVLOFFSCREEN* pyrLap, MInt32 nlevLap);

MInt32 BuildGauPyrs_s16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, ASVLOFFSCREEN* pyrGau, MInt32 nlevGau);

MVoid BuildLapPyrs_s16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, ASVLOFFSCREEN* pyrGau, ASVLOFFSCREEN* pyrLap, MInt32 nlevLap);

MRESULT ReconstructLapLacianPyramid_S16(MHandle hMemMgr, MHandle mcvParallelMonitor, ASVLOFFSCREEN* pLapPyr, LPASVLOFFSCREEN pDstImg, MInt32 nLevs);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //DIS_OPTICALFLOW_BUILD_PYRAMID_H
