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
*******************************************************************************
*
*
* @brief
*
* @version: 1.0
*
* @author:
*
* @date:
*
* @change:
*
* @note:
*
* @TODO:
*
******************************************************************/
#pragma once

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    typedef enum
    {
        kMeanDown2 = 0,
        kMeanDown4 = 1,
        kBilinearUp2 = 2,
        kBilinearUp4 = 3,
    } ScaleType_t;

    template<typename T = MUInt8>
    class Up_Down_Scale_Mean2x2_4x4
    {

    public:
        Up_Down_Scale_Mean2x2_4x4(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 nThreadCount);
        ~Up_Down_Scale_Mean2x2_4x4();

    public:
        MInt32 Run(T *pSrc,
                   MInt32 lWidthSrc,
                   MInt32 lHeightSrc,
                   MInt32 lPitchSrc,
                   T *pDst,
                   MInt32 lWidthDst,
                   MInt32 lHeightDst,
                   MInt32 lPitchDst,
                   MInt32 cn,
                   ScaleType_t type);

    private:

        typedef struct _tag_IMAGE_MEAN_DOWN
        {
            MByte *pSrc;
            MInt32 lPitchSrc;

            MByte *pDst;
            MInt32 lPitchDst;

            MByte *pDstU;
            MInt32 lPitchDstU;

            MByte *pDstV;
            MInt32 lPitchDstV;

            MInt32 lDstWidth;
            MInt32 lDstHeight;
            MInt32 startRow;
            MInt32 endRow;
        } PARAM_IMAGE_MEAN_DOWN;


    private:

        MInt32 Mean_Down2_C1(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                             T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst);

        MInt32 Mean_Down2_C1(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                             T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst,
                             MInt32 startRow, MInt32 endRow);

        MInt32 Mean_Down2_C2(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                             T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst,
                             MInt32 startRow, MInt32 endRow);

        MInt32 Mean_Down4_C1(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                             T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst);

        MInt32 Mean_Down4_C1(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                             T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst,
                             MInt32 startRow, MInt32 endRow);

        MInt32 Mean_Down4_C2(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                             T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst,
                             MInt32 startRow, MInt32 endRow);



        MInt32 Bilinear_Up2_C1(T *pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
                               T *pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst,
                               MInt32 startRow, MInt32 endRow);

        MInt32 Bilinear_Up2_C1_Threads(T *pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
                                       T *pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst);

        MInt32 Bilinear_Up4_C1(T *pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
                               T *pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst,
                               MInt32 startRow, MInt32 endRow);

        MInt32 Bilinear_Up4_C1_Threads(T *pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
                                       T *pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst);

    private:
        MHandle m_hMemMgr;
        MHandle m_mcvParallelMonitor;
        MInt32 m_nThreadCount;
    };


NS_SINFLE_IMAGE_ENHANCEMENT_END


#include "Up_Down_Scale_Mean2x2_4x4.inl.h"
