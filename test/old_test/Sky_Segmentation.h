
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_SKY_SEGMENTATION_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_SKY_SEGMENTATION_H

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

MInt32 Sky_Segmentation(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pSkyMask, MInt16 nRotationalAngle);

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_SKY_SEGMENTATION_H
