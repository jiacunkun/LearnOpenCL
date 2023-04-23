#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_VALUEFILTER_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_VALUEFILTER_H

#include <asvloffscreen.h>
#include "single_image_enhancement_define.h"

MInt32 ValueFilter(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MFloat fValue);

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_VALUEFILTER_H
