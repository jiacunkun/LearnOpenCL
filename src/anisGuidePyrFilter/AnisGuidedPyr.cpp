//
// Created by jck7075 on 2020/6/15.
//
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <vector>

#include "adlcore.h"
#include "ammem.h"
#include "ArcsoftLog.h"

#include "imageproc.h"
#include "down16_fix.h"
#include "down8_fix.h"
#include "up16_fix.h"
#include "up8_fix.h"
#include "single_image_enhancement_define.h"

#include "AnisotropicGuidedFiltering.h"
#include "PyramidLayer.h"

#include "LaplacianSharpen.h"
#include "DefineForDebug.h"
#include "ArcsoftSharpen.h"
#include "AnisGuidedPyr.h"
#include "PyramidLayer.h"
#include "gaussian_filter.h"
#include "Up_Down_Scale_Mean2x2_4x4.h"
#include "img_dilate_erode.h"
#include "ArcsoftSharpenHandle.h"
#include "Arcsoft_Up_Down_Scale_Handle.h"
//#define DETAIL_LUMA 20

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN


    MInt32 AnisGuidedPyrHandle(MHandle hMemMgr,
                               MHandle mcvParallelMonitor,
                               MUInt8 *pSrc,
                               MUInt8 *pGuide,
                               MUInt8 *pDst,
                               MInt32 lWidth,
                               MInt32 lHeight,
                               MInt32 lPitch,
                               MInt32 lGuidedPitch,
                               MFloat *feps,
                               MInt32 lDetailLuma,
                               MInt32 *sharpIntensity,
                               MInt32 lScale,
                               LPASVLOFFSCREEN pShadeMap)
    {
        return AnisGuidedPyr(hMemMgr, mcvParallelMonitor, lWidth, lHeight, lPitch).Run(hMemMgr, mcvParallelMonitor, pSrc, pGuide, pDst, lWidth,
                 lHeight, lPitch, lGuidedPitch,  feps, lDetailLuma, sharpIntensity, lScale, pShadeMap);
    }

    AnisGuidedPyr::AnisGuidedPyr(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 width, MInt32 height, MInt32 pitch)
    {
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_lWidth = width;
        m_lHeight = height;
        m_lPitch = pitch;
        Init(width, height, pitch);
    }

    AnisGuidedPyr::~AnisGuidedPyr()
    {
        Realease();
    }

    MVoid AnisGuidedPyr::Init(MInt32 width, MInt32 height, MInt32 pitch)
    {


        m_SrcPyrImage[0].pData = MNull; // 0层为传入图像
        m_SrcPyrImage[0].lWidth = width;
        m_SrcPyrImage[0].lHeight = height;
        m_SrcPyrImage[0].lStride = pitch;

        m_DstPyrImage[0].pData = MNull; // 0层为传入图像
        m_DstPyrImage[0].lWidth = width;
        m_DstPyrImage[0].lHeight = height;
        m_DstPyrImage[0].lStride = pitch;
        for (MInt32 i = 1; i < LAYER; i++)
        {
            m_SrcPyrImage[i].lWidth = width >> i;
            m_SrcPyrImage[i].lHeight = height >> i;
            m_SrcPyrImage[i].lStride = width >> i;
            m_SrcPyrImage[i].pData = (MUInt8*)MMemAlloc(m_hMemMgr, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(MUInt8));

            m_DstPyrImage[i].lWidth = width >> i;
            m_DstPyrImage[i].lHeight = height >> i;
            m_DstPyrImage[i].lStride = width >> i;
            m_DstPyrImage[i].pData = (MUInt8*)MMemAlloc(m_hMemMgr, m_DstPyrImage[i].lHeight * m_DstPyrImage[i].lStride * sizeof(MUInt8));
        }

        for (MInt32 i = 0; i < LAYER; i++)
        {
            m_MeanA[i].lWidth = width >> i;
            m_MeanA[i].lHeight = height >> i;
            m_MeanA[i].lStride = width >> i;
            m_MeanA[i].pData = (MInt16 *) MMemAlloc(m_hMemMgr, m_MeanA[i].lWidth * m_MeanA[i].lHeight * sizeof(MInt16));

            m_MeanB[i].lWidth = width >> i;
            m_MeanB[i].lHeight = height >> i;
            m_MeanB[i].lStride = width >> i;
            m_MeanB[i].pData = (MInt16 *) MMemAlloc(m_hMemMgr, m_MeanB[i].lWidth * m_MeanB[i].lHeight * sizeof(MInt16));

            m_StrongEdgeMask[i].lWidth = width >> i;
            m_StrongEdgeMask[i].lHeight = height >> i;
            m_StrongEdgeMask[i].lStride = width >> i;
            m_StrongEdgeMask[i].pData = (MInt16 *) MMemAlloc(m_hMemMgr, m_StrongEdgeMask[i].lWidth * m_StrongEdgeMask[i].lHeight * sizeof(MInt16));

            m_StrongEdgeMaskLarge[i].lWidth = width;
            m_StrongEdgeMaskLarge[i].lHeight = height;
            m_StrongEdgeMaskLarge[i].lStride = width;
            m_StrongEdgeMaskLarge[i].pData = (MInt16 *) MMemAlloc(m_hMemMgr, m_StrongEdgeMaskLarge[i].lWidth * m_StrongEdgeMaskLarge[i].lHeight * sizeof(MInt16));

            m_MeanALarge[i].lWidth = width;
            m_MeanALarge[i].lHeight = height;
            m_MeanALarge[i].lStride = width;
            m_MeanALarge[i].pData = (MInt16 *) MMemAlloc(m_hMemMgr, m_MeanALarge[i].lWidth * m_MeanALarge[i].lHeight * sizeof(MInt16));

            m_MeanBLarge[i].lWidth = width;
            m_MeanBLarge[i].lHeight = height;
            m_MeanBLarge[i].lStride = width;
            m_MeanBLarge[i].pData = (MInt16 *) MMemAlloc(m_hMemMgr, m_MeanBLarge[i].lWidth * m_MeanBLarge[i].lHeight * sizeof(MInt16));

            m_StrongEdgeMaskMax[i].lWidth = width>>i;
            m_StrongEdgeMaskMax[i].lHeight = height>>i;
            m_StrongEdgeMaskMax[i].lStride = width >> i;
            m_StrongEdgeMaskMax[i].pData = (MInt16 *) MMemAlloc(m_hMemMgr, m_StrongEdgeMaskMax[i].lWidth * m_StrongEdgeMaskMax[i].lHeight * sizeof(MInt16));

            m_TempBuffer[i].lWidth = width>>i;
            m_TempBuffer[i].lHeight = height>>i;
            m_TempBuffer[i].lStride = width >> i;
            m_TempBuffer[i].pData = (MUInt8*) MMemAlloc(m_hMemMgr, m_TempBuffer[i].lWidth * m_TempBuffer[i].lHeight * sizeof(MUInt8));
        }
    }

    MVoid AnisGuidedPyr::Realease()
    {
        for (MInt32 i = 1; i < LAYER; i++)
        {
            if (m_SrcPyrImage[i].pData)
            {
                MMemFree(m_hMemMgr, m_SrcPyrImage[i].pData);
                m_SrcPyrImage[i].pData = MNull;
            }

            if (m_DstPyrImage[i].pData)
            {
                MMemFree(m_hMemMgr, m_DstPyrImage[i].pData);
                m_DstPyrImage[i].pData = MNull;
            }
			if (m_MeanALarge[i].pData)
			{
				MMemFree(m_hMemMgr, m_MeanALarge[i].pData);
				m_MeanALarge[i].pData = MNull;
			}
			if (m_MeanBLarge[i].pData)
			{
				MMemFree(m_hMemMgr, m_MeanBLarge[i].pData);
				m_MeanBLarge[i].pData = MNull;
			}
        }

        for (MInt32 i = 0; i < LAYER; i++)
        {
            if (m_MeanA[i].pData)
            {
                MMemFree(m_hMemMgr, m_MeanA[i].pData);
                m_MeanA[i].pData = MNull;
            }
            if (m_MeanB[i].pData)
            {
                MMemFree(m_hMemMgr, m_MeanB[i].pData);
                m_MeanB[i].pData = MNull;
            }
            
            if (m_StrongEdgeMask[i].pData)
            {
                MMemFree(m_hMemMgr, m_StrongEdgeMask[i].pData);
                m_StrongEdgeMask[i].pData = MNull;
            }
            if (m_StrongEdgeMaskLarge[i].pData)
            {
                MMemFree(m_hMemMgr, m_StrongEdgeMaskLarge[i].pData);
                m_StrongEdgeMaskLarge[i].pData = MNull;
            }
            if (m_StrongEdgeMaskMax[i].pData)
            {
                MMemFree(m_hMemMgr, m_StrongEdgeMaskMax[i].pData);
                m_StrongEdgeMaskMax[i].pData = MNull;
            }
            if (m_TempBuffer[i].pData)
            {
                MMemFree(m_hMemMgr, m_TempBuffer[i].pData);
                m_TempBuffer[i].pData = MNull;
            }
        }
    }

    MInt32 AnisGuidedPyr::Run(MHandle hMemMgr,
                              MHandle mcvParallelMonitor,
                              MUInt8 *pSrc,
                              MUInt8 *pGuide,
                              MUInt8 *pDst,
                              MInt32 lWidth,
                              MInt32 lHeight,
                              MInt32 lPitch,
                              MInt32 lGuidedPitch,
                              MFloat *feps,
                              MInt32 lDetailLuma,
                              MInt32 *sharpIntensity,
                              MInt32 lScale,
                              LPASVLOFFSCREEN pShadeMap)
    {
        MInt32 lret = 0;

#ifdef  BUILD_OPENCV
        char filename[255];
#endif

        for (int i = 0; i < LAYER; i++)
        {
            m_fEps[i] = feps[i];
        }

        m_SrcPyrImage[0].pData = pSrc;
        m_DstPyrImage[0].pData = pDst;

        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor);
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 获取图像金字塔
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (MInt32 i = 1; i < LAYER; i++)
        {
            up_down_scale.downScale2(m_SrcPyrImage[i - 1], m_SrcPyrImage[i], mean2x2);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 每层进行操作
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        auto obj = AnisotropicGuidedFiltering<>(hMemMgr, mcvParallelMonitor, 8, 8, pShadeMap);
        // 获取各个尺度的meanab和强边缘mask
        m_MeanALarge[0] = m_MeanA[0];
        m_MeanBLarge[0] = m_MeanB[0];

        /******************************* 0层需要加入引导图来计算MeanA和MeanB *******************************************/
        {
            MInt16 i = 0;
            obj.GetMeanAB<MInt16, MInt16>(hMemMgr,
                                          mcvParallelMonitor,
                                          m_SrcPyrImage[i].pData,
                                          m_SrcPyrImage[i].lWidth,
                                          m_SrcPyrImage[i].lHeight,
                                          m_SrcPyrImage[i].lStride,
                                          pGuide,
                                          lGuidedPitch,
                                          m_MeanA[i].pData,
                                          m_MeanB[i].pData,
                                          feps[i]);

//            obj.getStrongEdgeMask(m_StrongEdgeMask[i].pData, m_StrongEdgeMask[i].lWidth, m_StrongEdgeMask[i].lHeight);

#ifdef  BUILD_OPENCV
            mat_write255(m_MeanA[i].lHeight, m_MeanA[i].lWidth, CV_8UC1, pGuide, "pGuide.jpg", 1.0);
            sprintf(filename, "MeanAPyr[%d].jpg", i);
            mat_write255(m_MeanA[i].lHeight, m_MeanA[i].lWidth, CV_16SC1, m_MeanA[i].pData, filename, 1.0);
            sprintf(filename, "MeanBPyr[%d].jpg", i);
            mat_write255(m_MeanA[i].lHeight, m_MeanA[i].lWidth, CV_16SC1, m_MeanB[i].pData, filename, 1.0);
#endif
        }

        /***************************** 其他层不需要另外加入引导图，计算计算MeanA和MeanB ********************************/
        for (MInt32 i = 1; i < LAYER; i++)
        {
            obj.GetMeanAB<MInt16, MInt16>(hMemMgr,
                                          mcvParallelMonitor,
                                          m_SrcPyrImage[i].pData,
                                          m_SrcPyrImage[i].lWidth,
                                          m_SrcPyrImage[i].lHeight,
                                          m_SrcPyrImage[i].lStride,
                                          m_SrcPyrImage[i].pData,
                                          m_SrcPyrImage[i].lStride,
                                          m_MeanA[i].pData,
                                          m_MeanB[i].pData,
                                          feps[i]);

//            obj.getStrongEdgeMask(m_StrongEdgeMask[i].pData, m_StrongEdgeMask[i].lWidth, m_StrongEdgeMask[i].lHeight);

#ifdef  BUILD_OPENCV
            sprintf(filename, "MeanAPyr[%d].jpg", i);
            mat_write255(m_MeanA[i].lHeight, m_MeanA[i].lWidth, CV_16SC1, m_MeanA[i].pData, filename, 1.0);
            sprintf(filename, "MeanBPyr[%d].jpg", i);
            mat_write255(m_MeanA[i].lHeight, m_MeanA[i].lWidth, CV_16SC1, m_MeanB[i].pData, filename, 1.0);
#endif
        }

        /******************************* 乘A加B，得到图像 *******************************************/
        for (MInt32 i = LAYER - 1; i >= 0; i--)
        {
            // 每层单独降噪
#if 0
            // 用强边缘修正MeanA
            MInt16* pFixMeanA = (MInt16*)MMemAlloc(m_hMemMgr, m_MeanA[i].lHeight*m_MeanA[i].lStride* sizeof(MInt16));
            if (i == 0)
            {
                // 膨胀强边缘

                maxFilter<MInt16>(m_StrongEdgeMask[i].pData, 1, m_StrongEdgeMaskMax[i].lWidth, m_StrongEdgeMaskMax[i].lHeight, m_StrongEdgeMaskMax[i].lWidth,
                                  m_StrongEdgeMaskMax[i].pData); // 膨胀

#ifdef  BUILD_OPENCV
                sprintf(filename, "pStrongEdgeMaskMax[%d].jpg", i);
                mat_write255(m_StrongEdgeMaskMax[i].lHeight, m_StrongEdgeMaskMax[i].lWidth, CV_16SC1, m_StrongEdgeMask[i].pData, filename, 1.0);
#endif
                getFixMeanA(m_MeanA[i].pData, m_StrongEdgeMask[i].pData, m_SrcPyrImage[i].pData,
                        m_MeanA[i].lWidth, m_MeanA[i].lHeight, m_MeanA[i].lStride, pFixMeanA);
            }
            else
            {
                MMemCpy(pFixMeanA, m_MeanA[i].pData, m_MeanA[i].lWidth*m_MeanA[i].lHeight* sizeof(MInt16));
            }
#ifdef  BUILD_OPENCV
            sprintf(filename, "pFixMeanA[%d].jpg", i);
            mat_write255(m_MeanA[i].lHeight, m_MeanA[i].lWidth, CV_16SC1, pFixMeanA, filename, 1.0);
#endif
#endif
            // 用MeanA融合图像
#ifdef USING_MIX
            mixImages<MInt16 , MUInt8>(m_MeanB[i].pData , m_SrcPyrImage[i].pData, m_MeanA[i].pData,
                                       m_SrcPyrImage[i].lWidth, m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, m_DstPyrImage[i].pData);
#else
            obj.Mul_A_Plus_B_Threads<MInt16>(m_SrcPyrImage[i].pData, m_SrcPyrImage[i].lWidth, m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride,
                                             m_DstPyrImage[i].pData, m_DstPyrImage[i].lStride, m_MeanA[i].pData, m_MeanB[i].pData);
#endif

#ifdef  BUILD_OPENCV
            sprintf(filename, "m_SrcPyrImage[%d].jpg", i);
            mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
            sprintf(filename, "m_DstPyrImage[%d].jpg", i);
            mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
#endif

#if 0
            MMemFree(m_hMemMgr, pFixMeanA);
            pFixMeanA = MNull;
#endif
        }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 多层融合
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0 // 采用camera的细节图计算方法
        // 动态获取计算强弱纹理的阈值
        MInt32 maxThr[4] ; // 强纹理阈值
        MInt32 minThr[4] ; // 弱纹理阈值

        m_lDetailLuma = lDetailLuma;

        GetEdgeThr(m_lDetailLuma, maxThr, minThr);


        //计算融合mask
        for (MInt32 i = LAYER - 2; i >= 0; i--)
        {
//            calcFusedMask(m_MeanA[i].pData, m_StrongEdgeMaskMax[i].pData, m_SrcPyrImage[i].pData, lDetailLuma, maxThr[i], minThr[i],
//                        m_MeanA[i].lWidth, m_MeanA[i].lHeight, m_MeanA[i].lStride, m_MeanA[i].pData);
//
//            char filename[255];
//            sprintf(filename, "m_MeanANew[%d].jpg", i);
//            mat_write255(m_MeanA[i].lHeight, m_MeanA[i].lWidth, CV_16SC1, m_MeanA[i].pData, filename, 1.0);
        }

#endif

        for (MInt32 i = LAYER - 2; i >= 0; i--)
        {
#if 0 // 采用camera的细节图计算方法
            CalEdgeMask(m_SrcPyrImage[i], m_StrongEdgeMask[i], maxThr[i], minThr[i]); // 得到细节图mask

#ifdef  BUILD_OPENCV
            sprintf(filename, "CalEdgeMask[%d].jpg", i);
            mat_write255(m_StrongEdgeMask[i].lHeight, m_StrongEdgeMask[i].lWidth, CV_16SC1, m_StrongEdgeMask[i].pData, filename, 1.0);
#endif

#if 1 // 采用camera的去除孤立点的方法
            RefineEdgeMask(m_StrongEdgeMask[i], m_StrongEdgeMask[i]); // 去除细节图的孤立点
//            RefineEdgeMask(m_StrongEdgeMask[i], m_StrongEdgeMask[i]); // 去除细节图的孤立点，一次不够，再来一次
#else // 用方向引导去除孤立点
            AnisotropicGuidedFiltering<MInt16, MUInt8> obj2(8);
            obj2.Run(hMemMgr, mcvParallelMonitor,
                    m_StrongEdgeMask[i].pData, m_SrcPyrImage[i].pData,
                    m_SrcPyrImage[i].lWidth, m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, m_SrcPyrImage[i].lStride,
                    m_StrongEdgeMask[i].pData, m_StrongEdgeMask[i].lStride,
                             10, 1);
#endif

#ifdef  BUILD_OPENCV
            sprintf(filename, "RefineEdgeMask[%d].jpg", i);
            mat_write255(m_StrongEdgeMask[i].lHeight, m_StrongEdgeMask[i].lWidth, CV_16SC1, m_StrongEdgeMask[i].pData, filename, 1.0);
#endif
#endif
            // 上层降噪结果上采样
            up_down_scale.upScale2(m_DstPyrImage[i + 1], m_TempBuffer[i], mean2x2);
            mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_8UC1, m_TempBuffer[i].pData, "TempBuffer.jpg", 1.0);

            // 融合两层结果
            mixImages<MUInt8 , MUInt8>(m_TempBuffer[i].pData, m_DstPyrImage[i].pData, m_MeanA[i].pData,
                                       m_DstPyrImage[i].lWidth, m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, m_DstPyrImage[i].pData);
        }

        return lret;
    }



    template <typename T>
    MVoid AnisGuidedPyr::maxFilter(T* pSrc,  MInt32 lRadius, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, T* pDst)
    {
//        for (MInt32 y = lRadius; y < lHeight - lRadius; y++)
//        {
//            T *pTempSrc = pSrc;
//            T *pTempDst = pDst;
//
//            for (MInt32 x = lRadius; x < lWidth - lRadius; x++)
//            {
//
//            }
//            pTempSrc += lStride;
//            pTempDst += lStride;
//        }

#ifdef BUILD_OPENCV0 //todo:暂时用opencv来仿真
        cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2*lRadius + 1, 2*lRadius + 1));

        if (sizeof(T) > 1)
        {
            cv::Mat src(lHeight, lWidth, CV_16SC1, pSrc);
            cv::Mat dst(lHeight ,lWidth, CV_16SC1, pDst);
            cv::dilate(src, dst, element);
        }
        else
        {
            cv::Mat src(lHeight, lWidth, CV_8UC1, pSrc);
            cv::Mat dst(lHeight ,lWidth, CV_8UC1, pDst);
            cv::dilate(src, dst, element);
        }
#else
        // 先转到8bit处理，然后转会16bit
        MUInt8 *pTempSrcData = (MUInt8*)MMemAlloc(m_hMemMgr, lHeight * lPitch);
        MUInt8 *pTempDstData = (MUInt8*)MMemAlloc(m_hMemMgr, lHeight * lPitch);

        for (MInt32 i = 0; i < lHeight*lPitch; i++)
        {
			pTempSrcData[i] = pSrc[i];
        }


        if (lRadius == 3)
        {
            FilterDilate7x7u8(m_hMemMgr, m_mcvParallelMonitor, pTempSrcData, lPitch, pTempDstData, lPitch, lWidth, lHeight);
        }
        else if (lRadius == 2)
        {
            FilterDilate5x5u8(m_hMemMgr, m_mcvParallelMonitor, pTempSrcData, lPitch, pTempDstData, lPitch, lWidth, lHeight);
        }
        else if (lRadius == 1)
        {
            FilterDilate3x3u8(m_hMemMgr, m_mcvParallelMonitor, pTempSrcData, lPitch, pTempDstData, lPitch, lWidth, lHeight);
        }
        else
        {
            MMemCpy(pTempDstData, pTempSrcData, lHeight*lPitch* sizeof(T));
        }

        for (MInt32 i = 0; i < lHeight*lPitch; i++)
        {
            pDst[i] = pTempDstData[i];
        }

        MMemFree(m_hMemMgr, pTempDstData);
        MMemFree(m_hMemMgr, pTempSrcData);
#endif

    }

    MInt32 AnisGuidedPyr::getFixMeanA(MInt16* pMeanA,  MInt16* pAlpha, MUInt8* pSrcImage, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt16* pFixMeanA)
    {
        MInt32 lret = MOK;

        for (MInt32 y = 0; y < lHeight; y++)
        {
            auto *pCurSrc = pMeanA + y * lWidth;
            auto *pCurDst = pFixMeanA + y * lWidth;
            MUInt8 *pTempSrcImage = pSrcImage + y * lWidth;
            MInt16 *pA = pAlpha + y * lWidth;

            for (MInt32 x = 0; x < lWidth; x++)
            {
                MInt32 tempA = pA[x];
                MInt32 val;
                val = pCurSrc[x];

//                val = (128 - tempA) * val; // 强边缘区域
//                val = (val + 64)/128;
//                val = val > 50 ? val:val/2;
//                val = val > 40 ? val:val/2;

//                val = tempA > 40 ? (128 - tempA) : val; // 强边缘区域


//                val = tempA > 20 ? val/2 : val;
//                val = tempA > 30 ? val/4 : val;
//                val = tempA > 40 ? val/8 : val;
//                val = (tempA > 30 || tempA < 20 ) ? val*1.2 : val;

//                val = tempA < 40 ? val/4 : val;//平坦区域慢慢过渡
//                val = tempA < 30 ? val/2 : val;//平坦区域慢慢过渡
//                val = tempA < 20 ? val/4 : val;//平坦区域慢慢过渡
                val = tempA < 10 ? val/2 : val;
                val = tempA < 5 ? val/4 : val;
                val = tempA < 1 ? 0 : val; //平坦区域
//
               val = pTempSrcImage[x] > 180 ? 100 : val; // 明亮区域细节保留多一些
               val = pTempSrcImage[x] > 220 ? 128 : val; // 过曝区域保留原图

//               val += 21; // 让噪声回来一些

                val = val > 128 ? 128 : val;
                pCurDst[x] = val;
            }
        }

        return lret;
    }

    MVoid AnisGuidedPyr::calcFusedMask(MInt16* pMeanA,  MInt16* pAlpha, MUInt8* pSrcImage, MInt32 lDetailLuma, MInt32 maxThr, MInt32 minThr,
                                        MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt16* pFixMeanA)
    {
        LOGD("maxThr = %d", maxThr);
        LOGD("minThr = %d", minThr);

        for (MInt32 y = 0; y < lHeight; y++)
        {
            auto *pCurSrc = pMeanA + y * lWidth;
            auto *pCurDst = pFixMeanA + y * lWidth;
            MUInt8 *pTempSrcImage = pSrcImage + y * lPitch;
            MInt16 *pTempAlpha = pAlpha + y * lWidth;

            for (MInt32 x = 0; x < lWidth; x++)
            {
                MInt32 tempA = pTempAlpha[x];
                MInt32 val = pCurSrc[x];
//                val = val > 50 ? 128*7/8:0; // 只融合
//                val = val < 40 ? 128/4:128/2;
//                val = val < 30 ? 0:val;
//                val = val < 20 ? 128/4:val;
//                val = val < 20 ? 0:val;

//                val = (128 - tempA) * val; // 强边缘区域
//                val = (val + 64)/128;

//                val = tempA > 32 ? (128 - tempA) : val; // 强边缘区域


//                val = tempA > 20 ? val/2 : val;
//                val = tempA > 30 ? val/4 : val;
//                val = tempA > 40 ? val/8 : val;
//                val = (tempA > 30 || tempA < 20 ) ? val*1.2 : val;

//                val = tempA < 40 ? val/4 : val;//平坦区域慢慢过渡
//                val = tempA < 30 ? val/2 : val;//平坦区域慢慢过渡
//                val = tempA < 20 ? val/4 : val;//平坦区域慢慢过渡
//                val = tempA < 10 ? val/2 : val;
//                val = tempA < 5 ? val/4 : val;
//                val = tempA < 1 ? 0 : val; //平坦区域
//

//                val = val + 16;
//                if (val > maxThr*4)
//                {
//                    val = 100;
//                }
//                else if (val > minThr*4)
//                {
//                    val = 60;
//                }
//                else
//                {
//                    val = 0;
//                }
                val = pTempSrcImage[x] > 180 ? val*1.5 : val; // 明亮区域细节保留多一些
                val = pTempSrcImage[x] > 220 ? 128 : val; // 过曝区域保留原图
//               val += 21; // 让噪声回来一些

                val = val > 128 ? 128 : val;
                pCurDst[x] = val;
            }
        }
    }

    template <typename T1, typename T2>
    MInt32 AnisGuidedPyr::mixImages(T1* pImage1, T2* pImage2,  MInt16* pAlpha, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MUInt8* pMixedImage)
    {
        MInt32 lret = MOK;

        MUInt8 nBitWidth1 = sizeof(T1);
        MUInt8 nBitWidth2 = sizeof(T2);

        MInt32 divFactor1 = nBitWidth1 > 1 ? 1 : 1;
        MInt32 divFactor2 = nBitWidth2 > 1 ? 1 : 1;

        for (MInt32 y = 0; y < lHeight; y++)
        {
            T1 *pCurSrc1 = pImage1 + y * lPitch;
            T2 *pCurSrc2 = pImage2 + y * lPitch;
            MUInt8 *pCurDst = pMixedImage + y * lPitch;
            MInt16 *pA = pAlpha + y * lWidth;

            for (MInt32 x = 0; x < lWidth; x++)
            {
                MFloat tempA = pA[x]*(1.0+m_lDetailLuma*1.0/20.0);
                tempA = tempA > 128 ? 128 : tempA;
                MInt32 val;

                val = (1.0*(128 - tempA) * pCurSrc1[x]/divFactor1) + (1.0*tempA * pCurSrc2[x]/divFactor2);
                val = (val + 64)/128;
                val = val > 255 ? 255 : val;
                pCurDst[x] = val;
            }
        }

        return lret;
    }

    template <typename T>
    MInt32 AnisGuidedPyr::mixImages(T* pImage1, T* pImage2,  MInt16* pAlpha, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, T* pMixedImage)
    {
        MInt32 lret = MOK;

        for (MInt32 y = 0; y < lHeight; y++)
        {
            T *pCurSrc1 = pImage1 + y * lPitch;
            T *pCurSrc2 = pImage2 + y * lPitch;
            T *pCurDst = pMixedImage + y * lPitch;
            MInt16 *pA = pAlpha + y * lWidth;

            for (MInt32 x = 0; x < lWidth; x++)
            {
                MInt32 tempA = pA[x];
                MInt32 val;

                val = (1.0*(128 - tempA) * pCurSrc1[x]) + (1.0*tempA * pCurSrc2[x]);
                val = (val + 64)/128;
//                val = val > 255 ? 255 : val;
                pCurDst[x] = val;
            }
        }

        return lret;
    }


    MInt32 AnisGuidedPyr::CalEdgeMask(ImageInfo<MUInt8> &input, ImageInfo<MInt16> &output, MInt32 maxThr, MInt32 minThr)
    {
        LOGD("CalEdgeMask++");
        MInt32 lret = MOK;

        MInt32 lHeight = input.lHeight;
        MInt32 lWidth = input.lWidth;
        MInt32 lPitch = input.lStride;

		MInt32 offset[4] = { 3, 2 * lPitch + 2, 3 * lPitch, 2 * lPitch - 2 };

        MUInt8 *pTempSrc = input.pData + 3*lPitch;
        MInt16 *pTempDst = output.pData + 3*lWidth;

        for (int y = 3; y < lHeight-3; y++)
        {
            for (int x = 3; x < lWidth-3; x++)
            {
                MInt32 sumMax = 0;
                MInt32 sumMin = 0;
				MUInt8 nCur = pTempSrc[x];
                for (int i = 0; i < 4; i++)
                {
                    sumMax += ((pTempSrc[x+offset[i]] - nCur) > maxThr);
                    sumMin += ((pTempSrc[x+offset[i]] - nCur) > minThr);
                    sumMax += ((pTempSrc[x-offset[i]] - nCur) > maxThr);
                    sumMin += ((pTempSrc[x-offset[i]] - nCur) > minThr);
                }
                pTempDst[x] = sumMax > 0 ? 100 : (sumMin > 0 ? 64 : 0); // 强边缘设置为128，弱纹理设置为64
            }
            pTempSrc += lPitch;
            pTempDst += lWidth;
        }

        LOGD("CalEdgeMask--");
        return lret;
    }

    MInt32 AnisGuidedPyr::RefineEdgeMask(ImageInfo<MInt16> &input, ImageInfo<MInt16> &output)
    {
        MInt32 lret = 0;

        MInt32 lHeight = input.lHeight;
        MInt32 lWidth = input.lWidth;
        MInt32 lPitch = input.lStride;

        MInt32 offset[4] = { 1, lWidth + 1, lWidth, lWidth - 1};

        MInt16 *pTempSrc = input.pData + 1*lWidth;
        MInt16 *pTempDst = output.pData + 1*lWidth;

        for (int y = 1; y < lHeight-1; y++)
        {
            for (int x = 1; x < lWidth-1; x++)
            {
                MInt32 sumMax = 0;
                MUInt8 nCur = pTempSrc[x];
                for (int i = 0; i < 4; i++)
                {
                    sumMax += ((pTempSrc[x+offset[i]]) > 0);
                }

                pTempDst[x] = (nCur == 0 && sumMax >= 4) ? 64 : ((nCur > 0 && sumMax <= 1) ? 0 : nCur);
            }
            pTempSrc += lWidth;
            pTempDst += lWidth;
        }


        return lret;
    }

    MVoid AnisGuidedPyr::GetEdgeThr(MInt32 nLumaDetail, MInt32* max_threshold, MInt32* min_threshold)
    {
        //明亮度大于0时才会进入,明亮度取值范围为[0, 100]，默认构建4层金字塔
        //明亮度细节参数计算
        double dLumaDetailVal = 0.0;
        if (nLumaDetail > 50)
        {
            dLumaDetailVal = 2 * nLumaDetail * 0.01 - 1.0;
            dLumaDetailVal = 0.01525902189669642 - MAX(0.0, MIN(1.0, dLumaDetailVal)) * 0.01144426642252232;
        }
        else
        {
            dLumaDetailVal = 2 * nLumaDetail * 0.01;
            dLumaDetailVal = 0.06103608758678569 - MAX(0.0, MIN(1.0, dLumaDetailVal)) * 0.04577706569008926;
        }
        dLumaDetailVal = dLumaDetailVal * 255;

        double weight_list[4] = {1.0, 0.43, 0.21, 0.12};

        for (int k = 0; k < 4; k++)
        {
            double dWeightedLumDetailVal = dLumaDetailVal * weight_list[k];
            max_threshold[k] = round(dWeightedLumDetailVal * 4 * 1.414);
            min_threshold[k] = round(dWeightedLumDetailVal * 4);
        }

    }

NS_SINFLE_IMAGE_ENHANCEMENT_END