#pragma once

#include "single_image_enhancement_define.h"
#include <asvloffscreen.h>

MInt32 GetMeanAB(MHandle hMemMgr,
    MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrcImgPad,
    LPASVLOFFSCREEN pGuideImgPad,
    LPASVLOFFSCREEN pMeanAImg,
    LPASVLOFFSCREEN pMeanBImg,
    MFloat fEps);

MInt32 GetMeanB(MHandle hMemMgr,
    MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrcImgPad,
    LPASVLOFFSCREEN pGuideImgPad,
    LPASVLOFFSCREEN pMeanBImg,
    MFloat fEps);

MInt32 Mul_A_Plus_B(MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrcImg,
    LPASVLOFFSCREEN pDstImg,
    LPASVLOFFSCREEN pMeanAImg,
    LPASVLOFFSCREEN pMeanBImg);