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
#include "Up_Down_Scale_Mean2x2_4x4.h"
#include "up_down_scale_gaussian5x5.h"
#include "Up_Down_Scale_Gaussian3x3.h"
#include "down8_fix.h"
#include "Arcsoft_Up_Down_Scale_Handle.h"
#include "up8_fix.h"
#include "up_down_scale_filter3x3.h"
#include "BasicTimer.h"
#include "ArcsoftLog.h"
#include "up_down_scale.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN


    Arcsoft_Up_Down_Scale_Handle::Arcsoft_Up_Down_Scale_Handle(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 nThreadCount)
    {
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_nThreadCount = nThreadCount;
    }

    Arcsoft_Up_Down_Scale_Handle::~Arcsoft_Up_Down_Scale_Handle()
    {

    }

    MInt32 Arcsoft_Up_Down_Scale_Handle::downScale2(ImageInfo<MUInt8> largeImage, ImageInfo<MInt16> smallImage, ScaleType type)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lret = 0;
        switch (type)
        {
            case 0:
            {
                lret = down8(m_mcvParallelMonitor, largeImage.pData, smallImage.pData,
                             largeImage.lWidth, largeImage.lHeight, largeImage.lStride, smallImage.lStride, 1);
                break;
            }

            case 1:
            {
                break;
            }

            case 2:
            {
                ASVLOFFSCREEN pSrcImg;
                MInt16 *pTempBuffer = (MInt16*)MMemAlloc(m_hMemMgr, largeImage.lWidth*largeImage.lHeight* sizeof(MInt16));
                {
                    pSrcImg.ppu8Plane[0] = (MUInt8*)pTempBuffer;
                    pSrcImg.i32Width = largeImage.lWidth;
                    pSrcImg.i32Height = largeImage.lHeight;
                    pSrcImg.pi32Pitch[0] = largeImage.lStride << 1;
                    pSrcImg.u32PixelArrayFormat = ASVL_PAF_GRAY;
                }

                MInt32 imageSum = pSrcImg.i32Width * pSrcImg.i32Height;
                MUInt8 *pTempLargeImage = largeImage.pData;
                MInt16 *pTempDstImage = (MInt16*)pSrcImg.ppu8Plane[0];
                for (MInt32 i = 0; i < imageSum; i++)
                {
                    pTempDstImage[i] = (pTempLargeImage[i] << 2);
                }

                /////////////////////
                ASVLOFFSCREEN pDstImg;
                ImageInfo2ASVLOFFSCREEN(smallImage, pDstImg);

                Img_Guass5x5_Down2_u16(m_hMemMgr, m_mcvParallelMonitor, &pSrcImg, &pDstImg);
                break;
            }

            case 3:
            {
                MInt16 kernel[] = {
                        1, 1, 1,
                        1, 8, 1,
                        1, 1, 1};

                MInt32 lSumWeight = 16/4;
                filter2D3x3Down2<MUInt8, MInt16>(m_hMemMgr, m_mcvParallelMonitor, largeImage, smallImage, kernel, lSumWeight);

            }

            default:
                break;
        }
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return lret;

    }


    MInt32 Arcsoft_Up_Down_Scale_Handle::downScale2(ImageInfo<MInt16> largeImage, ImageInfo<MInt16> smallImage, ScaleType type)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lret = 0;

        switch (type)
        {
            case 0:
            {
                Up_Down_Scale_Mean2x2_4x4<MInt16>(m_hMemMgr, m_mcvParallelMonitor, m_nThreadCount).Run(largeImage.pData, largeImage.lWidth, largeImage.lHeight, largeImage.lStride,
                                                                                                       smallImage.pData, smallImage.lWidth, smallImage.lHeight, smallImage.lStride, 1, kMeanDown2);
                break;
            }

            case 1:
            {
                Guass3x3Down2Threads<MInt16>(m_hMemMgr, m_mcvParallelMonitor, &largeImage, &smallImage);
                break;
            }

            case 2:
            {
                ASVLOFFSCREEN pSrcImg;
                ImageInfo2ASVLOFFSCREEN(largeImage, pSrcImg);

                ASVLOFFSCREEN pDstImg;
                ImageInfo2ASVLOFFSCREEN(smallImage, pDstImg);

                Img_Guass5x5_Down2_u16(m_hMemMgr, m_mcvParallelMonitor, &pSrcImg, &pDstImg);
                break;
            }

            case 3:
            {
                MInt16 kernel[] = {
                        1, 1, 1,
                        1, 8, 1,
                        1, 1, 1};

                MInt32 lSumWeight = 16;
                filter2D3x3Down2<MInt16 , MInt16>(m_hMemMgr, m_mcvParallelMonitor, largeImage, smallImage, kernel, lSumWeight);

            }
            default:
                break;
        }
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return lret;
    }

    MInt32 Arcsoft_Up_Down_Scale_Handle::downScale2(ImageInfo<MUInt8> largeImage, ImageInfo<MUInt8> smallImage, ScaleType type)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lret = 0;

        switch (type)
        {
            case 0:
            {
                Up_Down_Scale_Mean2x2_4x4<>(m_hMemMgr, m_mcvParallelMonitor, m_nThreadCount).Run(largeImage.pData, largeImage.lWidth, largeImage.lHeight, largeImage.lStride,
                                                                                                 smallImage.pData, smallImage.lWidth, smallImage.lHeight, smallImage.lStride, 1, kMeanDown2);
                break;
            }

            case 1:
            {
                lret = Guass3x3Down2Threads<MUInt8>(m_hMemMgr, m_mcvParallelMonitor, &largeImage, &smallImage);
                break;
            }

            case 2:
            {
                ASVLOFFSCREEN pSrcImg;
                ImageInfo2ASVLOFFSCREEN(largeImage, pSrcImg);

                ASVLOFFSCREEN pDstImg;
                ImageInfo2ASVLOFFSCREEN(smallImage, pDstImg);
                lret = Img_Guass5x5_Down2_u8(m_hMemMgr, m_mcvParallelMonitor, &pSrcImg, &pDstImg, MNull);
                break;
            }

            case 3:
            {
#if 0
                MInt16 kernel[] = {
                        1, 1, 1,
                        1, 8, 1,
                        1, 1, 1};
#else
                MInt16 kernel[] = {
                        1, 2, 1,
                        2, 4, 2,
                        1, 2, 1};
#endif

                filter2D3x3Down2(m_hMemMgr, m_mcvParallelMonitor, largeImage, smallImage, kernel, 16);

            }
            default:
                break;
        }

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return lret;
    }

    MInt32 Arcsoft_Up_Down_Scale_Handle::upScale2(ImageInfo<MInt16> smallImage, ImageInfo<MUInt8> largeImage, ScaleType type)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lret = 0;

        switch (type)
        {
            case 0:
            {
                MInt16 *dstSmall = (MInt16*)MMemAlloc(m_hMemMgr, smallImage.lHeight * smallImage.lStride * sizeof(MInt16));
                up8(m_hMemMgr, m_mcvParallelMonitor, (MVoid*)smallImage.pData, (MVoid*)dstSmall, (MVoid*)largeImage.pData, largeImage.lWidth, largeImage.lHeight,
                    smallImage.lStride, largeImage.lStride, 1);
                SAFE_FREE_ARRAY(m_hMemMgr, dstSmall);
                break;
            }

            case 1:
            {
                break;
            }

            case 2:
            {
                ASVLOFFSCREEN pSrcImg;
                ImageInfo2ASVLOFFSCREEN(smallImage, pSrcImg);

                ASVLOFFSCREEN pDstImg;
                MInt16 *pTempBuffer = (MInt16*)MMemAlloc(m_hMemMgr, largeImage.lWidth*largeImage.lHeight* sizeof(MInt16));
                {
                    pDstImg.ppu8Plane[0] = (MUInt8*)pTempBuffer;
                    pDstImg.i32Width = largeImage.lWidth;
                    pDstImg.i32Height = largeImage.lHeight;
                    pDstImg.pi32Pitch[0] = largeImage.lStride << 1;
                    pDstImg.u32PixelArrayFormat = ASVL_PAF_GRAY;
                }

                lret = Img_Guass5x5_Up2_u16(m_hMemMgr, m_mcvParallelMonitor, &pSrcImg, &pDstImg);

                MInt32 imageSum = pDstImg.i32Width * pDstImg.i32Height;
                MUInt8 *pTempLargeImage = largeImage.pData;
                MInt16 *pTempDstImage = (MInt16*)pDstImg.ppu8Plane[0];
                for (MInt32 i = 0; i < imageSum; i++)
                {
                    pTempLargeImage[i] = (pTempDstImage[i] + 2) >> 2;
                }

                MMemFree(m_hMemMgr, pTempBuffer);
                pTempBuffer = MNull;
                break;
            }

            case 3:
            {
                MInt16 kernel[] = {
                        1, 2, 1,
                        2, 4, 2,
                        1, 2, 1};

                MInt32 lSumWeight = 4;

                filter2D3x3Up2<MInt16, MUInt8>(m_hMemMgr, m_mcvParallelMonitor, smallImage, largeImage, kernel, lSumWeight, 0, largeImage.lHeight);

            }

            default:
                break;
        }
#if CALCULATE_TIME
        LOGD("%s MInt16 -> MUInt8 [%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return lret;
    }

    MInt32 Arcsoft_Up_Down_Scale_Handle::upScale2(ImageInfo<MInt16> smallImage, ImageInfo<MInt16> largeImage, ScaleType type, MInt16 nWeight)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lret = 0;

        switch (type)
        {
            case 0:
            {
                Up_Down_Scale_Mean2x2_4x4<MInt16>(m_hMemMgr, m_mcvParallelMonitor, m_nThreadCount).Run(smallImage.pData, smallImage.lWidth, smallImage.lHeight, smallImage.lStride,
                                                                                                       largeImage.pData, largeImage.lWidth, largeImage.lHeight, largeImage.lStride, 1, kBilinearUp2);
                break;
            }

            case 1:
            {
                Guass3x3Up2Threads<MInt16>(m_mcvParallelMonitor, &smallImage, &largeImage);
                break;
            }

            case 2:
            {
                ASVLOFFSCREEN pSrcImg;
                ImageInfo2ASVLOFFSCREEN(smallImage, pSrcImg);

                ASVLOFFSCREEN pDstImg;
                ImageInfo2ASVLOFFSCREEN(largeImage, pDstImg);

                lret = Img_Guass5x5_Up2_u16(m_hMemMgr, m_mcvParallelMonitor, &pSrcImg, &pDstImg);
                break;
            }

            case 3:
            {
                MInt16 kernel[] = {
                        1, 2, 1,
                        2, 4, 2,
                        1, 2, 1};

                filter2D3x3Up2<MInt16, MInt16>(m_hMemMgr, m_mcvParallelMonitor, smallImage, largeImage, kernel, 4*nWeight, 0, largeImage.lHeight);

            }
            default:
                break;
        }

#if CALCULATE_TIME
        LOGD("%s MInt16 -> MInt16 [%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return lret;
    }

    MInt32 Arcsoft_Up_Down_Scale_Handle::upScale2(ImageInfo<MUInt8> smallImage, ImageInfo<MUInt8> largeImage, ScaleType type)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lret = 0;

        switch (type)
        {
            case 0:
            {

                Up_Down_Scale_Mean2x2_4x4<MUInt8>(m_hMemMgr, m_mcvParallelMonitor, m_nThreadCount).Run(smallImage.pData, smallImage.lWidth, smallImage.lHeight, smallImage.lStride,
                                                                                                       largeImage.pData, largeImage.lWidth, largeImage.lHeight, largeImage.lStride, 1, kBilinearUp2);
                break;
            }

            case 1:
            {
                Guass3x3Up2Threads<MUInt8>(m_mcvParallelMonitor, &smallImage, &largeImage);
                break;
            }

            case 2:
            {
                ASVLOFFSCREEN pSrcImg;
                ImageInfo2ASVLOFFSCREEN(smallImage, pSrcImg);

                ASVLOFFSCREEN pDstImg;
                ImageInfo2ASVLOFFSCREEN(largeImage, pDstImg);

                lret = Img_Guass5x5_Up2_u8(m_hMemMgr, &pSrcImg, &pDstImg);
                break;
            }

            case 3:
            {
                MInt16 kernel[] = {
                        1, 2, 1,
                        2, 4, 2,
                        1, 2, 1};

                filter2D3x3Up2<MUInt8, MUInt8>(m_hMemMgr, m_mcvParallelMonitor, smallImage, largeImage, kernel, 4, 0, largeImage.lHeight);

            }
            default:
                break;
        }

#if CALCULATE_TIME
        LOGD("%s MUInt8 -> MUInt8 [%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif

        return lret;
    }


    MVoid Arcsoft_Up_Down_Scale_Handle::ImageInfo2ASVLOFFSCREEN(ImageInfo<MUInt8> srcImage, ASVLOFFSCREEN& dstImage0)
    {
        dstImage0.u32PixelArrayFormat = ASVL_PAF_GRAY;

        dstImage0.i32Height = srcImage.lHeight;
        dstImage0.i32Width = srcImage.lWidth;
        dstImage0.pi32Pitch[0] = srcImage.lStride;
        dstImage0.ppu8Plane[0] = srcImage.pData;
    }


    MVoid Arcsoft_Up_Down_Scale_Handle::ImageInfo2ASVLOFFSCREEN(ImageInfo<MInt16> srcImage, ASVLOFFSCREEN& dstImage0)
    {
        dstImage0.u32PixelArrayFormat = ASVL_PAF_GRAY;

        dstImage0.i32Height = srcImage.lHeight;
        dstImage0.i32Width = srcImage.lWidth;
        dstImage0.pi32Pitch[0] = srcImage.lStride << 1;
        dstImage0.ppu8Plane[0] = (MUInt8*)(srcImage.pData);
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END