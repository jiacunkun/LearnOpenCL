
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_TEST_SKY_SEGMENTATION_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_TEST_SKY_SEGMENTATION_H

#include "asvloffscreen.h"

MInt32 Process_SkySegment(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg,
                          LPASVLOFFSCREEN pDispImg,  MInt32 lImageOrient);

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_TEST_SKY_SEGMENTATION_H
