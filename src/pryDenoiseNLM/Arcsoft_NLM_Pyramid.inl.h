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
#include <thread>
#include "ammem.h"
#include "ArcsoftLog.h"
#include "DefineForDebug.h"
#include "Arcsoft_Up_Down_Scale_Handle.h"
#include "Up_Down_Scale_Gaussian3x3.h"
#include "NLMeans16.h"
#include "NLMeans.h"
#include "BasicTimer.h"
#include "Arcsoft_Copy_To_FilledImage.h"
#define NLM16 // 开启16bitNLM处理

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN


    template <typename T>
    Arcsoft_NLM_Pyramid<T>::Arcsoft_NLM_Pyramid(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcPitch, MInt32 lPitch, MInt32 layer)
    {
        LOGD("Arcsoft_NLM_Pyramid(T) = %d", sizeof(T));
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;

        for (MInt32 i = 0; i < layer; i++)
        {
            MMemSet(&m_SrcPyrImage[i], 0, sizeof(ImageInfo<T>));
            MMemSet(&m_DstPyrImage[i], 0, sizeof(ImageInfo<T>));
            MMemSet(&m_TempBuffer[i], 0, sizeof(ImageInfo<T>));
            MMemSet(&m_dnShade[i], 0, sizeof(ImageInfo<MUInt8>));
        }
        LOGD("lWidth = %d, lHeight = %d", lWidth, lHeight);
        init(lWidth, lHeight, lSrcPitch, lPitch, layer);
    }

    template <typename T>
    Arcsoft_NLM_Pyramid<T>::~Arcsoft_NLM_Pyramid()
    {
        for (MInt32 i = 1; i < m_lLayer; i++)
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

            if (m_dnShade[i].pData)
            {
                MMemFree(m_hMemMgr, m_dnShade[i].pData);
                m_dnShade[i].pData = MNull;
            }
        }

        for (MInt32 i = 0; i < m_lLayer; i++)
        {
            if (m_TempBuffer[i].pData)
            {
                MMemFree(m_hMemMgr, m_TempBuffer[i].pData);
                m_TempBuffer[i].pData = MNull;
            }
        }
    }

    template <typename T>
    MInt32 Arcsoft_NLM_Pyramid<T>::init(MInt32 lWidth, MInt32 lHeight, MInt32 lSrcPitch, MInt32 lPitch, MInt32 layer)
    {
        MInt32 lret = 0;

        m_lLayer = layer;
        m_lWidth = lWidth;
        m_lHeight = lHeight;
        m_lPitch = lPitch;

        m_SrcPyrImage[0].pData = MNull; // 0层为传入图像
        m_SrcPyrImage[0].lWidth = lWidth;
        m_SrcPyrImage[0].lHeight = lHeight;
        m_SrcPyrImage[0].lStride = lSrcPitch / sizeof(T);

        m_DstPyrImage[0].pData = MNull; // 0层为传入图像
        m_DstPyrImage[0].lWidth = lWidth;
        m_DstPyrImage[0].lHeight = lHeight;
        m_DstPyrImage[0].lStride = lPitch / sizeof(T);

        m_TempBuffer[0].lWidth = lWidth;
        m_TempBuffer[0].lHeight = lHeight;
        m_TempBuffer[0].lStride = lSrcPitch / sizeof(T);
        m_TempBuffer[0].pData = (T*)MMemAlloc(m_hMemMgr, m_TempBuffer[0].lStride * m_TempBuffer[0].lHeight * sizeof(T));

        m_dnShade[0].lWidth = lWidth >> 2;
        m_dnShade[0].lHeight = lHeight >> 2;
        m_dnShade[0].lStride = lWidth >> 2;
        m_dnShade[0].pData = MNull;

        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            m_SrcPyrImage[i].lWidth = lWidth >> i;
            m_SrcPyrImage[i].lHeight = lHeight >> i;
            m_SrcPyrImage[i].lStride = lWidth >> i;
            m_SrcPyrImage[i].pData = (T*)MMemAlloc(m_hMemMgr, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(T));

            m_DstPyrImage[i].lWidth = lWidth >> i;
            m_DstPyrImage[i].lHeight = lHeight >> i;
            m_DstPyrImage[i].lStride = lWidth >> i;
            m_DstPyrImage[i].pData = (T*)MMemAlloc(m_hMemMgr, m_DstPyrImage[i].lHeight * m_DstPyrImage[i].lStride * sizeof(T));

            m_TempBuffer[i].lWidth = lWidth >> i;
            m_TempBuffer[i].lHeight = lHeight >> i;
            m_TempBuffer[i].lStride = lWidth >> i;
            m_TempBuffer[i].pData = (T*)MMemAlloc(m_hMemMgr, m_TempBuffer[i].lHeight * m_TempBuffer[i].lStride * sizeof(T));

            m_dnShade[i].lWidth = lWidth >> (i + 2);
            m_dnShade[i].lHeight = lHeight >> (i + 2);
            m_dnShade[i].lStride = lWidth >> (i + 2);
            m_dnShade[i].pData = (MUInt8*)MMemAlloc(m_hMemMgr, m_dnShade[i].lHeight * m_dnShade[i].lStride * sizeof(MUInt8));
        }



        return lret;
    }

    template <typename T>
    MInt32 Arcsoft_NLM_Pyramid<T>::run(MUInt8* pSrc, MUInt8* pDst, LPASVLOFFSCREEN pShade,
        MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
        MFloat* pNoiseVarY)
    {
        START_TIME;

        MInt32 lret = 0;

        lPitch = lPitch / sizeof(T); //数据强转后，字节数变成字数，需要除以2

        MBool isNewDst = false;
        if (pDst == pSrc)
        {
            isNewDst = true;
            pSrc = (T*)MMemAlloc(m_hMemMgr, lHeight * lPitch * sizeof(T));
            MMemCpy(pSrc, pDst, lHeight * lPitch * sizeof(T));
        }

        m_SrcPyrImage[0].pData = pSrc;
        m_DstPyrImage[0].pData = pDst;

#ifdef  BUILD_OPENCV
        char filename[255];
#endif
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 获取图像金字塔
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor);
        for (MInt32 i = 1; i < m_lLayer; i++)
        {
#if defined(GAUSS5X5)
            up_down_scale.downScale(m_SrcPyrImage[i - 1], m_SrcPyrImage[i]);
            up_down_scale.downScale(m_SrcPyrImage[i - 1], m_TempBuffer[i]);
#else
            Guass3x3Down2Threads<T>(m_hMemMgr, m_mcvParallelMonitor, &m_SrcPyrImage[i - 1], &m_SrcPyrImage[i]);

            MMemCpy(m_TempBuffer[i].pData, m_SrcPyrImage[i].pData,
                m_SrcPyrImage[i].lStride * m_SrcPyrImage[i].lHeight * sizeof(T));

            //Guass3x3Down2Threads<T>(m_hMemMgr, &m_SrcPyrImage[i - 1], &m_TempBuffer[i]);
#endif
        }
        //MMemCpy(m_TempBuffer[0].pData, m_SrcPyrImage[0].pData,
        //        m_SrcPyrImage[0].lStride * m_SrcPyrImage[0].lHeight * sizeof(T));
        CopyImageToImage<T>(&m_SrcPyrImage[0], &m_TempBuffer[0]);
        /*******************************外部没有shade时，内部分配*******************************************/
        MBool isNewMem = false;
        if (pShade == MNull)
        {
            isNewMem = true;
            MInt32 lDarkDenoiseEnhance = 0;
            MInt32 lDarkThresLev = 0;
            m_dnShade[0].pData = (MUInt8*)MMemAlloc(m_hMemMgr, (lHeight / 4) * (lPitch / 4) * sizeof(MUInt8));
            m_dnShade[0].lWidth = lWidth / 4;
            m_dnShade[0].lHeight = lHeight / 4;
            m_dnShade[0].lStride = lWidth / 4;

            CreateRadialMask(m_dnShade[0].pData, m_dnShade[0].lWidth, m_dnShade[0].lHeight, m_dnShade[0].lStride);
            if (lDarkDenoiseEnhance > 0)
            {
                CreateDarkShadeMask(&m_dnShade[0], &m_SrcPyrImage[2], lDarkDenoiseEnhance, lDarkThresLev);
            }
        }
        else
        {
            m_dnShade[0].pData = pShade->ppu8Plane[0];
            m_dnShade[0].lWidth = pShade->i32Width;
            m_dnShade[0].lHeight = pShade->i32Height;
            m_dnShade[0].lStride = pShade->pi32Pitch[0];
        }

        // 获取shade金字塔
        for (MInt32 i = 1; i < m_lLayer; i++)
        {
#if defined(GAUSS5X5)
            up_down_scale.downScale(m_dnShade[i - 1], m_dnShade[i]);
#else
            Guass3x3Down2Threads<MUInt8>(m_hMemMgr, m_mcvParallelMonitor, &m_dnShade[i - 1], &m_dnShade[i]);
#endif
        }
        /*************************************************************************************************/

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 每层金字塔进行操作
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        NLMeans nlmObj(m_hMemMgr, m_mcvParallelMonitor);
        MBool isFirstLayerDenoise = pNoiseVarY[0] > 0.0;

        for (MInt32 i = m_lLayer - 1; i > 0; i--)
        {
#if CALCULATE_TIME
            BasicTimer time2;
#endif
            MFloat fTmpVarY = pNoiseVarY[i];
            fTmpVarY = MAX(1.0f, fTmpVarY);
            // 单层降噪
            CopyImageToImage<T>(&m_SrcPyrImage[i], &m_DstPyrImage[i]);
            //MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData,
            //    m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lWidth * sizeof(T)); //输入和输出一样，防止内部数据不处理时，可以原图考出
#if 1
            NLMeans::PyInfo_t src, dst, mask;
            src.pData = (MUInt8*)m_SrcPyrImage[i].pData;
            src.lWidth = m_SrcPyrImage[i].lWidth;
            src.lHeight = m_SrcPyrImage[i].lHeight;
            src.lStride = m_SrcPyrImage[i].lStride;

            dst.pData = (MUInt8*)m_DstPyrImage[i].pData;
            dst.lWidth = m_DstPyrImage[i].lWidth;
            dst.lHeight = m_DstPyrImage[i].lHeight;
            dst.lStride = m_DstPyrImage[i].lStride;

            mask.pData = (MUInt8*)m_dnShade[i].pData;
            mask.lWidth = m_dnShade[i].lWidth;
            mask.lHeight = m_dnShade[i].lHeight;
            mask.lStride = m_dnShade[i].lStride;

            lret = nlmObj.Run(&m_SrcPyrImage[i], &m_DstPyrImage[i], &m_dnShade[i], fTmpVarY);


#else // 验证金字塔效果
            MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData,
                m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lWidth * sizeof(T));
#endif



#ifdef  BUILD_OPENCV
            sprintf(filename, "m_dnShade[%d].png", i);
            mat_write255(m_dnShade[i].lHeight, m_dnShade[i].lWidth, CV_8UC1, m_dnShade[i].pData, filename, 1.0);

            if (sizeof(T) > 1)
            {
                sprintf(filename, "m_SrcPyrImage7[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_16SC1, m_SrcPyrImage[i].pData, filename, 1.0 / 4);
                sprintf(filename, "m_DstPyrImage7[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0 / 4);
                sprintf(filename, "TempBuffer7[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_16SC1, m_TempBuffer[i].pData, filename, 1.0 / 4);

            }
            else
            {
                sprintf(filename, "m_SrcPyrImage7[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "m_DstPyrImage7[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "TempBuffer7[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_8UC1, m_TempBuffer[i].pData, filename, 1.0);

            }
#endif


            // 得到拉普拉斯结果图
            ImageSubImage(&m_DstPyrImage[i], &m_TempBuffer[i]);

            // 拉普拉斯上采样后，加到原图像中
#if defined(GAUSS5X5)
            up_down_scale.upScale(m_DstPyrImage[i], m_DstPyrImage[i - 1]);
#else
            Guass3x3Up2Threads<T>(m_mcvParallelMonitor, &m_DstPyrImage[i], &m_DstPyrImage[i - 1]);
#endif
            if (i == 1 && !isFirstLayerDenoise)
            {
                ImageAddImage(&m_DstPyrImage[i - 1], &m_SrcPyrImage[i - 1]);
            }
            else
            {
                ImageAddImage(&m_SrcPyrImage[i - 1], &m_DstPyrImage[i - 1]);
            }


#if CALCULATE_TIME
            LOGD("The %d layer is finished timer count = %fms!\n", i, time2.UpdateAndGetDelta());
#endif

        }

        if (isFirstLayerDenoise)
        {
#if CALCULATE_TIME
            BasicTimer time2;
#endif
            // 对0层进行降噪
            MInt32 i = 0;

            MFloat fTmpVarY = pNoiseVarY[i];
            fTmpVarY = MAX(1.0f, fTmpVarY);

            CopyImageToImage<T>(&m_SrcPyrImage[i], &m_DstPyrImage[i]);
            //MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData,
            //        m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(T));

            NLMeans::PyInfo_t src, dst, mask;
            src.pData = (MUInt8*)m_SrcPyrImage[i].pData;
            src.lWidth = m_SrcPyrImage[i].lWidth;
            src.lHeight = m_SrcPyrImage[i].lHeight;
            src.lStride = m_SrcPyrImage[i].lStride;

            dst.pData = (MUInt8*)m_DstPyrImage[i].pData;
            dst.lWidth = m_DstPyrImage[i].lWidth;
            dst.lHeight = m_DstPyrImage[i].lHeight;
            dst.lStride = m_DstPyrImage[i].lStride;

            mask.pData = (MUInt8*)m_dnShade[i].pData;
            mask.lWidth = m_dnShade[i].lWidth;
            mask.lHeight = m_dnShade[i].lHeight;
            mask.lStride = m_dnShade[i].lStride;

            lret = nlmObj.Run(&m_SrcPyrImage[i], &m_DstPyrImage[i], &m_dnShade[i], fTmpVarY);

            if (m_SrcPyrImage[0].pData != m_DstPyrImage[0].pData)
            {
                MMemCpy(m_SrcPyrImage[i].pData, m_TempBuffer[i].pData,
                    m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(T));
            }


#if CALCULATE_TIME
            LOGD("The %d layer is finished timer count = %fms!\n", i, time2.UpdateAndGetDelta());
#endif



#ifdef  BUILD_OPENCV
            sprintf(filename, "m_dnShade[%d].png", i);
            mat_write255(m_dnShade[i].lHeight, m_dnShade[i].lWidth, CV_8UC1, m_dnShade[i].pData, filename, 1.0);

            if (sizeof(T) > 1)
            {
                sprintf(filename, "m_SrcPyrImage7[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_16SC1, m_SrcPyrImage[i].pData, filename, 1.0 / 4);
                sprintf(filename, "m_DstPyrImage7[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0 / 4);
                sprintf(filename, "TempBuffer7[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_16SC1, m_TempBuffer[i].pData, filename, 1.0 / 4);

            }
            else
            {
                sprintf(filename, "m_SrcPyrImage3[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "m_DstPyrImage3[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "TempBuffer[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_8UC1, m_TempBuffer[i].pData, filename, 1.0);

            }
#endif
        }

#ifdef  BUILD_OPENCV
        int i = 0;
        sprintf(filename, "m_dnShade[%d].png", i);
        mat_write255(m_dnShade[i].lHeight, m_dnShade[i].lWidth, CV_8UC1, m_dnShade[i].pData, filename, 1.0);


        sprintf(filename, "m_SrcPyrImage3[%d].png", i);
        mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
        sprintf(filename, "m_DstPyrImage3[%d].png", i);
        mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
        sprintf(filename, "TempBuffer[%d].png", i);
        mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_8UC1, m_TempBuffer[i].pData, filename, 1.0);
#endif

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 释放内存
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (isNewDst)
        {
            MMemFree(m_hMemMgr, pSrc);
            pSrc = pDst;
        }

        if (isNewMem)
        {
            MMemFree(m_hMemMgr, m_dnShade[0].pData);
            m_dnShade[0].pData = MNull;
        }

        END_TIME;

        return lret;
    }

    template <typename T>
    MInt32 Arcsoft_NLM_Pyramid<T>::run(MUInt8* pSrc, MUInt8* pDst, LPASVLOFFSCREEN pShade,
        MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
        MFloat fNoiseVarY, MBool isFirstLayerDenoise)
    {
        START_TIME;
        MFloat fPow[] = { 1.0, 0.5, 0.25, 0.125, 0.0625 };

        MInt32 lret = 0;

        lPitch = lPitch / sizeof(T); //数据强转后，字节数变成字数，需要除以2

        MBool isNewDst = false;
        if (pDst == pSrc)
        {
            isNewDst = true;
            pSrc = (T*)MMemAlloc(m_hMemMgr, lHeight * lPitch * sizeof(T));
            MMemCpy(pSrc, pDst, lHeight * lPitch * sizeof(T));
        }

        m_SrcPyrImage[0].pData = pSrc;
        m_DstPyrImage[0].pData = pDst;

#ifdef  BUILD_OPENCV
        char filename[255];
#endif
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 获取图像金字塔
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor);
        for (MInt32 i = 1; i < m_lLayer; i++)
        {
#if defined(GAUSS5X5)
            up_down_scale.downScale(m_SrcPyrImage[i - 1], m_SrcPyrImage[i]);
            up_down_scale.downScale(m_SrcPyrImage[i - 1], m_TempBuffer[i]);
#else
            Guass3x3Down2Threads<T>(m_hMemMgr, m_mcvParallelMonitor, &m_SrcPyrImage[i - 1], &m_SrcPyrImage[i]);

            MMemCpy(m_TempBuffer[i].pData, m_SrcPyrImage[i].pData,
                    m_SrcPyrImage[i].lStride * m_SrcPyrImage[i].lHeight * sizeof(T));

            //Guass3x3Down2Threads<T>(m_hMemMgr, &m_SrcPyrImage[i - 1], &m_TempBuffer[i]);
#endif
        }
        //MMemCpy(m_TempBuffer[0].pData, m_SrcPyrImage[0].pData,
        //        m_SrcPyrImage[0].lStride * m_SrcPyrImage[0].lHeight * sizeof(T));
        CopyImageToImage<T>(&m_SrcPyrImage[0], &m_TempBuffer[0]);
        /*******************************外部没有shade时，内部分配*******************************************/
        MBool isNewMem = false;
        if (pShade == MNull)
        {
            isNewMem = true;
            MInt32 lDarkDenoiseEnhance = 0;
            MInt32 lDarkThresLev = 0;
            m_dnShade[0].pData = (MUInt8*)MMemAlloc(m_hMemMgr, (lHeight / 4) * (lPitch / 4) * sizeof(MUInt8));
            m_dnShade[0].lWidth = lWidth / 4;
            m_dnShade[0].lHeight = lHeight / 4;
            m_dnShade[0].lStride = lWidth / 4;

            CreateRadialMask(m_dnShade[0].pData, m_dnShade[0].lWidth, m_dnShade[0].lHeight, m_dnShade[0].lStride);
            if (lDarkDenoiseEnhance > 0)
            {
                CreateDarkShadeMask(&m_dnShade[0], &m_SrcPyrImage[2], lDarkDenoiseEnhance, lDarkThresLev);
            }
        }
        else
        {
            m_dnShade[0].pData = pShade->ppu8Plane[0];
            m_dnShade[0].lWidth = pShade->i32Width;
            m_dnShade[0].lHeight = pShade->i32Height;
            m_dnShade[0].lStride = pShade->pi32Pitch[0];
        }

        // 获取shade金字塔
        for (MInt32 i = 1; i < m_lLayer; i++)
        {
#if defined(GAUSS5X5)
            up_down_scale.downScale(m_dnShade[i - 1], m_dnShade[i]);
#else
            Guass3x3Down2Threads<MUInt8>(m_hMemMgr, m_mcvParallelMonitor, &m_dnShade[i - 1], &m_dnShade[i]);
#endif
        }
        /*************************************************************************************************/

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 每层金字塔进行操作
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        NLMeans nlmObj(m_hMemMgr, m_mcvParallelMonitor);


        for (MInt32 i = m_lLayer - 1; i > 0; i--)
        {
#if CALCULATE_TIME
            BasicTimer time2;
#endif
            MFloat fTmpVarY = fNoiseVarY * fPow[i];
            fTmpVarY = MAX(1.0f, fTmpVarY);
            // 单层降噪
            CopyImageToImage<T>(&m_SrcPyrImage[i], &m_DstPyrImage[i]);
            //MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData,
            //    m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lWidth * sizeof(T)); //输入和输出一样，防止内部数据不处理时，可以原图考出
#if 1
            NLMeans::PyInfo_t src, dst, mask;
            src.pData = (MUInt8*)m_SrcPyrImage[i].pData;
            src.lWidth = m_SrcPyrImage[i].lWidth;
            src.lHeight = m_SrcPyrImage[i].lHeight;
            src.lStride = m_SrcPyrImage[i].lStride;

            dst.pData = (MUInt8*)m_DstPyrImage[i].pData;
            dst.lWidth = m_DstPyrImage[i].lWidth;
            dst.lHeight = m_DstPyrImage[i].lHeight;
            dst.lStride = m_DstPyrImage[i].lStride;

            mask.pData = (MUInt8*)m_dnShade[i].pData;
            mask.lWidth = m_dnShade[i].lWidth;
            mask.lHeight = m_dnShade[i].lHeight;
            mask.lStride = m_dnShade[i].lStride;

            lret = nlmObj.Run(&m_SrcPyrImage[i], &m_DstPyrImage[i], &m_dnShade[i], fTmpVarY);


#else // 验证金字塔效果
            MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData,
                m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lWidth * sizeof(T));
#endif



#ifdef  BUILD_OPENCV
            sprintf(filename, "m_dnShade[%d].png", i);
            mat_write255(m_dnShade[i].lHeight, m_dnShade[i].lWidth, CV_8UC1, m_dnShade[i].pData, filename, 1.0);

            if (sizeof(T) > 1)
            {
                sprintf(filename, "m_SrcPyrImage7[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_16SC1, m_SrcPyrImage[i].pData, filename, 1.0 / 4);
                sprintf(filename, "m_DstPyrImage7[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0 / 4);
                sprintf(filename, "TempBuffer7[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_16SC1, m_TempBuffer[i].pData, filename, 1.0 / 4);

            }
            else
            {
                sprintf(filename, "m_SrcPyrImage7[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "m_DstPyrImage7[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "TempBuffer7[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_8UC1, m_TempBuffer[i].pData, filename, 1.0);

            }
#endif


            // 得到拉普拉斯结果图
            ImageSubImage(&m_DstPyrImage[i], &m_TempBuffer[i]);

            // 拉普拉斯上采样后，加到原图像中
#if defined(GAUSS5X5)
            up_down_scale.upScale(m_DstPyrImage[i], m_DstPyrImage[i - 1]);
#else
            Guass3x3Up2Threads<T>(m_mcvParallelMonitor, &m_DstPyrImage[i], &m_DstPyrImage[i - 1]);
#endif
            if (i == 1 && !isFirstLayerDenoise)
            {
                ImageAddImage(&m_DstPyrImage[i - 1], &m_SrcPyrImage[i - 1]);
            }
            else
            {
                ImageAddImage(&m_SrcPyrImage[i - 1], &m_DstPyrImage[i - 1]);
            }


#if CALCULATE_TIME
            LOGD("The %d layer is finished timer count = %fms!\n", i, time2.UpdateAndGetDelta());
#endif

        }

        if (isFirstLayerDenoise)
        {
#if CALCULATE_TIME
            BasicTimer time2;
#endif
            // 对0层进行降噪
            MInt32 i = 0;

            MFloat fTmpVarY = fNoiseVarY * fPow[i];
            fTmpVarY = MAX(1.0f, fTmpVarY);

            CopyImageToImage<T>(&m_SrcPyrImage[i], &m_DstPyrImage[i]);
            //MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData,
            //        m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(T));

            NLMeans::PyInfo_t src, dst, mask;
            src.pData = (MUInt8*)m_SrcPyrImage[i].pData;
            src.lWidth = m_SrcPyrImage[i].lWidth;
            src.lHeight = m_SrcPyrImage[i].lHeight;
            src.lStride = m_SrcPyrImage[i].lStride;

            dst.pData = (MUInt8*)m_DstPyrImage[i].pData;
            dst.lWidth = m_DstPyrImage[i].lWidth;
            dst.lHeight = m_DstPyrImage[i].lHeight;
            dst.lStride = m_DstPyrImage[i].lStride;

            mask.pData = (MUInt8*)m_dnShade[i].pData;
            mask.lWidth = m_dnShade[i].lWidth;
            mask.lHeight = m_dnShade[i].lHeight;
            mask.lStride = m_dnShade[i].lStride;

            lret = nlmObj.Run(&m_SrcPyrImage[i], &m_DstPyrImage[i], &m_dnShade[i], fTmpVarY);

            if (m_SrcPyrImage[0].pData != m_DstPyrImage[0].pData)
            {
                MMemCpy(m_SrcPyrImage[i].pData, m_TempBuffer[i].pData,
                        m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(T));
            }
            

#if CALCULATE_TIME
            LOGD("The %d layer is finished timer count = %fms!\n", i, time2.UpdateAndGetDelta());
#endif



#ifdef  BUILD_OPENCV
            sprintf(filename, "m_dnShade[%d].png", i);
            mat_write255(m_dnShade[i].lHeight, m_dnShade[i].lWidth, CV_8UC1, m_dnShade[i].pData, filename, 1.0);

            if (sizeof(T) > 1)
            {
                sprintf(filename, "m_SrcPyrImage7[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_16SC1, m_SrcPyrImage[i].pData, filename, 1.0 / 4);
                sprintf(filename, "m_DstPyrImage7[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0 / 4);
                sprintf(filename, "TempBuffer7[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_16SC1, m_TempBuffer[i].pData, filename, 1.0 / 4);

            }
            else
            {
                sprintf(filename, "m_SrcPyrImage3[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "m_DstPyrImage3[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "TempBuffer[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_8UC1, m_TempBuffer[i].pData, filename, 1.0);

        }
#endif
    }

#ifdef  BUILD_OPENCV
        int i = 0;
        sprintf(filename, "m_dnShade[%d].png", i);
        mat_write255(m_dnShade[i].lHeight, m_dnShade[i].lWidth, CV_8UC1, m_dnShade[i].pData, filename, 1.0);


        sprintf(filename, "m_SrcPyrImage3[%d].png", i);
        mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
        sprintf(filename, "m_DstPyrImage3[%d].png", i);
        mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
        sprintf(filename, "TempBuffer[%d].png", i);
        mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_8UC1, m_TempBuffer[i].pData, filename, 1.0);       
#endif

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 释放内存
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (isNewDst)
        {
            MMemFree(m_hMemMgr, pSrc);
            pSrc = pDst;
        }

        if (isNewMem)
        {
            MMemFree(m_hMemMgr, m_dnShade[0].pData);
            m_dnShade[0].pData = MNull;
        }

        END_TIME;

        return lret;
    }

    template <typename T>
    MInt32 Arcsoft_NLM_Pyramid<T>::run(MInt16 *pSrc, MInt16 *pDst, LPASVLOFFSCREEN pShade,
                                       MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                                       MFloat fNoiseVarY, MBool isFirstLayerDenoise )
    {
        MFloat fPow[] = {1.0, 0.5, 0.25, 0.125, 0.0625};
        LOGD("Arcsoft_NLM_Pyramid++");
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lret = 0;

        lPitch = lPitch / sizeof(T); //数据强转后，字节数变成字数，需要除以2

        MBool isNewDst = false;
        if (pDst == pSrc)
        {
            isNewDst = true;
            pSrc = (T*)MMemAlloc(m_hMemMgr, lHeight*lPitch*sizeof(T));
            MMemCpy(pSrc, pDst, lHeight*lPitch*sizeof(T));
        }
        m_SrcPyrImage[0].pData = pSrc;
        m_DstPyrImage[0].pData = pDst;

#ifdef  BUILD_OPENCV
        char filename[255];
#endif
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 获取图像金字塔
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor);
        for (MInt32 i = 1; i < m_lLayer; i++)
        {
#if defined(GAUSS5X5)
            up_down_scale.downScale(m_SrcPyrImage[i - 1], m_SrcPyrImage[i]);
            up_down_scale.downScale(m_SrcPyrImage[i - 1], m_TempBuffer[i]);
#else
            Guass3x3Down2Threads<T>(m_hMemMgr, m_mcvParallelMonitor, &m_SrcPyrImage[i - 1], &m_SrcPyrImage[i]);

            MMemCpy(m_TempBuffer[i].pData, m_SrcPyrImage[i].pData,
                    m_SrcPyrImage[i].lStride * m_SrcPyrImage[i].lHeight * sizeof(T));

            //Guass3x3Down2Threads<T>(m_hMemMgr, &m_SrcPyrImage[i - 1], &m_TempBuffer[i]);
#endif
        }

        MMemCpy(m_TempBuffer[0].pData, m_SrcPyrImage[0].pData,
                m_SrcPyrImage[0].lStride * m_SrcPyrImage[0].lHeight * sizeof(T));

        /*******************************外部没有shade时，内部分配*******************************************/
        MBool isNewMem = false;
        if (pShade == MNull)
        {
            isNewMem = true;
            MInt32 lDarkDenoiseEnhance = 0;
            MInt32 lDarkThresLev = 0;
            m_dnShade[0].pData = (MUInt8*)MMemAlloc(m_hMemMgr, (lHeight/4)*(lPitch/4)*sizeof(MUInt8));
            m_dnShade[0].lWidth = lWidth/4;
            m_dnShade[0].lHeight = lHeight/4;
            m_dnShade[0].lStride = lWidth / 4;

            CreateRadialMask(m_dnShade[0].pData, m_dnShade[0].lWidth, m_dnShade[0].lHeight, m_dnShade[0].lStride);
            if (lDarkDenoiseEnhance > 0)
            {
                CreateDarkShadeMask(&m_dnShade[0], &m_SrcPyrImage[2], lDarkDenoiseEnhance, lDarkThresLev);
            }
        }
        else
        {
            m_dnShade[0].pData   = pShade->ppu8Plane[0];
            m_dnShade[0].lWidth  = pShade->i32Width;
            m_dnShade[0].lHeight = pShade->i32Height;
            m_dnShade[0].lStride  = pShade->pi32Pitch[0];
        }

        // 获取shade金字塔
        for (MInt32 i = 1; i < m_lLayer; i++)
        {
#if defined(GAUSS5X5)
            up_down_scale.downScale(m_dnShade[i - 1], m_dnShade[i]);
#else
            Guass3x3Down2Threads<MUInt8>(m_hMemMgr, m_mcvParallelMonitor, &m_dnShade[i - 1], &m_dnShade[i]);
#endif
        }
        /*************************************************************************************************/

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 每层金字塔进行操作
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(NLM16)
        NLMeans16 nlmObj16(m_hMemMgr, m_mcvParallelMonitor);
#else
        NLMeans nlmObj(m_hMemMgr, m_mcvParallelMonitor);
#endif

        for (MInt32 i = m_lLayer - 1; i > 0; i--)
        {
            MFloat fTmpVarY = fNoiseVarY * fPow[i];;
            fTmpVarY = MAX(1.0f, fTmpVarY);
            // 单层降噪
            CopyImageToImage<T>(&m_SrcPyrImage[i], &m_DstPyrImage[i]);
            //MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData,
            //        m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(T));
#if 1
#if defined(NLM16)
            nlmObj16.Run(&m_SrcPyrImage[i], &m_DstPyrImage[i], &m_dnShade[i], fTmpVarY);
#else
            NLMeans::PyInfo_t src, dst, mask;
            src.pData  = (MUInt8*)m_SrcPyrImage[i].pData;
            src.lWidth  = m_SrcPyrImage[i].lWidth;
            src.lHeight = m_SrcPyrImage[i].lHeight;
            src.lStride  = m_SrcPyrImage[i].lStride;

            dst.pData  = (MUInt8*)m_DstPyrImage[i].pData;
            dst.lWidth  = m_DstPyrImage[i].lWidth;
            dst.lHeight = m_DstPyrImage[i].lHeight;
            dst.lStride  = m_DstPyrImage[i].lStride;

            mask.pData  = (MUInt8*)m_dnShade[i].pData;
            mask.lWidth  = m_dnShade[i].lWidth;
            mask.lHeight = m_dnShade[i].lHeight;
            mask.lStride  = m_dnShade[i].lStride;

            lret = nlmObj.Run(&src, &dst, &mask, fTmpVarY);
#endif

#else // 验证金字塔效果
            MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData,
                        m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lWidth * sizeof(T));
#endif



#ifdef  BUILD_OPENCV
            if (sizeof(T) > 1)
            {
                sprintf(filename, "m_SrcPyrImage7[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_16SC1, m_SrcPyrImage[i].pData, filename, 1.0/4);
                sprintf(filename, "m_DstPyrImage7[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0/4);
                sprintf(filename, "TempBuffer7[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_16SC1, m_TempBuffer[i].pData, filename, 1.0/4);

            }
            else
            {
                sprintf(filename, "m_SrcPyrImage7[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "m_DstPyrImage7[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "TempBuffer7[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_8UC1, m_TempBuffer[i].pData, filename, 1.0);

            }
#endif


            // 得到拉普拉斯结果图
            ImageSubImage(&m_DstPyrImage[i], &m_TempBuffer[i]);

            // 拉普拉斯上采样后，加到原图像中
#if defined(GAUSS5X5)
            up_down_scale.upScale(m_DstPyrImage[i], m_DstPyrImage[i-1]);
#else
            Guass3x3Up2Threads<T>(m_mcvParallelMonitor, &m_DstPyrImage[i], &m_DstPyrImage[i-1]);
#endif

            if (i == 1 && !isFirstLayerDenoise)
            {
                ImageAddImage(&m_DstPyrImage[i - 1], &m_SrcPyrImage[i - 1]);
            }
            else
            {
                ImageAddImage(&m_SrcPyrImage[i - 1], &m_DstPyrImage[i - 1]);
            }
        }

        if (isFirstLayerDenoise)
        {
            // 对0层进行降噪
            MInt32 i = 0;

            MFloat fTmpVarY = fNoiseVarY * fPow[i];
            fTmpVarY = MAX(1.0f, fTmpVarY);

            CopyImageToImage<T>(&m_SrcPyrImage[i], &m_DstPyrImage[i]);
            //MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData,
            //        m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(T));


#if defined(NLM16)
            nlmObj16.Run(&m_SrcPyrImage[i], &m_DstPyrImage[i], &m_dnShade[i], fTmpVarY);
#else
            NLMeans::PyInfo_t src, dst, mask;
            src.pData = (MUInt8*)m_SrcPyrImage[i].pData;
            src.lWidth = m_SrcPyrImage[i].lWidth;
            src.lHeight = m_SrcPyrImage[i].lHeight;
            src.lStride = m_SrcPyrImage[i].lStride;

            dst.pData = (MUInt8*)m_DstPyrImage[i].pData;
            dst.lWidth = m_DstPyrImage[i].lWidth;
            dst.lHeight = m_DstPyrImage[i].lHeight;
            dst.lStride = m_DstPyrImage[i].lStride;

            mask.pData = (MUInt8*)m_dnShade[i].pData;
            mask.lWidth = m_dnShade[i].lWidth;
            mask.lHeight = m_dnShade[i].lHeight;
            mask.lStride = m_dnShade[i].lStride;

            lret = nlmObj.Run(&src, &dst, &mask, fTmpVarY);
#endif

            MMemCpy(m_SrcPyrImage[0].pData, m_TempBuffer[0].pData,
                    m_SrcPyrImage[0].lStride * m_SrcPyrImage[0].lHeight * sizeof(T));

#ifdef  BUILD_OPENCV
            if (sizeof(T) > 1)
            {
                sprintf(filename, "m_SrcPyrImage7[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_16SC1, m_SrcPyrImage[i].pData, filename, 1.0 / 4);
                sprintf(filename, "m_DstPyrImage7[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0 / 4);
                sprintf(filename, "TempBuffer7[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_16SC1, m_TempBuffer[i].pData, filename, 1.0 / 4);

            }
            else
            {
                sprintf(filename, "m_SrcPyrImage3[%d].png", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "m_DstPyrImage3[%d].png", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "TempBuffer[%d].png", i);
                mat_write255(m_TempBuffer[i].lHeight, m_TempBuffer[i].lWidth, CV_8UC1, m_TempBuffer[i].pData, filename, 1.0);

            }
#endif
        }



        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 释放内存
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (isNewDst)
        {
            MMemFree(m_hMemMgr, pSrc);
            pSrc = pDst;
        }

        if (isNewMem)
        {
            MMemFree(m_hMemMgr, m_dnShade[0].pData);
            m_dnShade[0].pData = MNull;
        }
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("Arcsoft_NLM_Pyramid--");
        return lret;
    }

    template <typename T>
    MInt32 Arcsoft_NLM_Pyramid<T>::run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat fNoiseVarY, MBool isFirstLayerDenoise, LPASVLOFFSCREEN pShade)
    {
        MInt32 lret = 0;

        T* pSrcImage = (T*)pSrc->ppu8Plane[0];
        T* pDstImage = (T*)pDst->ppu8Plane[0];

        MInt32 lWidth = pSrc->i32Width;
        MInt32 lHeight = pSrc->i32Height;
        MInt32 lPitch = pSrc->pi32Pitch[0];

        lret = run(pSrcImage, pDstImage, pShade,
                   lWidth, lHeight, lPitch,
                   fNoiseVarY, isFirstLayerDenoise);

        return lret;
    }

    template <typename T>
    MInt32 Arcsoft_NLM_Pyramid<T>::run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat* pNoiseVarY, LPASVLOFFSCREEN pShade)
    {
        MInt32 lret = 0;

        T* pSrcImage = (T*)pSrc->ppu8Plane[0];
        T* pDstImage = (T*)pDst->ppu8Plane[0];

        MInt32 lWidth = pSrc->i32Width;
        MInt32 lHeight = pSrc->i32Height;
        MInt32 lPitch = pSrc->pi32Pitch[0];

        lret = run(pSrcImage, pDstImage, pShade,
            lWidth, lHeight, lPitch,
            pNoiseVarY);

        return lret;
    }

    template <typename T>
    MVoid Arcsoft_NLM_Pyramid<T>::ReconstructPyramid(ImageInfo<T> srcImage, ImageInfo<T> tempImage, ImageInfo<T> dstImage)
    {
        MInt32 srcWidth = srcImage.lWidth;
        MInt32 srcHeight = srcImage.lHeight;
        MInt32 srcPitch = srcImage.lStride;

        MInt16 min = 0;
        MInt16 max = sizeof(T) > 1 ? 1020:255;

        T *pTempSrc = srcImage.pData;
        T *pTempDst = dstImage.pData;
        T *pTempBuffer = tempImage.pData;
        for (MInt32 y = 0; y < srcHeight; y++)
        {
            for (MInt32 x = 0; x < srcWidth; x++)
            {
                MInt32 temp = 1.0*pTempDst[x] + 1.0*pTempSrc[x] - 1.0*pTempBuffer[x];
                temp = temp < min ? min:temp;
                temp = temp > max ? max:temp;
                pTempDst[x] = temp;
            }

            pTempSrc     += srcPitch;
            pTempDst     += srcPitch;
            pTempBuffer  += srcPitch;
        }
    }

    template <typename T>
    MVoid Arcsoft_NLM_Pyramid<T>::CreateDarkShadeMask(ImageInfo<MUInt8> *pMask, ImageInfo<T> *pGrayImg, MInt32 lDarkEnhance, MInt32 lDarkThres)
    {
        START_TIME;
        MInt32 lWidth = pMask->lWidth;
        MInt32 lHeight = pMask->lHeight;

        MFloat fDarkEnhance = 1.0f / ( 1 + lDarkEnhance / 2.5f ); // 10 -> 1/5
        lDarkThres = lDarkThres * 10 + 16;
        for(MInt32 y = 0; y < lHeight; y++)
        {
            T *pGray = pGrayImg->pData + y * pGrayImg->lStride;
            MByte *pDataShade = pMask->pData + y * pMask->lStride;

            for(MInt32 x = 0; x < lWidth; x++)
            {
                MInt32 temp = pGray[ x ];
                if( temp > 150 )
                {
                    continue;
                }

                if( temp < lDarkThres )
                {
                    pDataShade[ x ] *= fDarkEnhance;
                    pDataShade[ x ] = pDataShade[ x ] >= 63 ? 63 : pDataShade[ x ];
                    pDataShade[ x ] = pDataShade[ x ] < 0 ? 0 : pDataShade[ x ];
                }
                else
                {
                    MFloat w = ( MFloat ) ( temp - lDarkThres ) / ( 150 - lDarkThres );
                    MFloat fCurEnhance = fDarkEnhance * ( 1 - w ) + 1.0f * w;
                    pDataShade[ x ] *= fCurEnhance;
                    pDataShade[ x ] = pDataShade[ x ] >= 63 ? 63 : pDataShade[ x ];
                    pDataShade[ x ] = pDataShade[ x ] < 0 ? 0 : pDataShade[ x ];
                }
            }
        }
        END_TIME;
    }

    template<class T>
    MVoid Arcsoft_NLM_Pyramid<T>::ImageSubImage(ImageInfo<T>* pImage, ImageInfo<T>* pSubImg)
#ifdef USE_STD_THREAD
    {
        LOGD("ImageSubImage++");
#if CALCULATE_TIME
        BasicTimer time;
#endif

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        MInt32 nHeight = pImage->lHeight;
        int threadCount =  nHeight >= 1024 ? 16 : 8;
        auto expand_functor = [&](int currentThreadId)
        {
            // 线程分割
            int startHeight = 0;
            int endHeight = nHeight;
            if( threadCount > 1 )
            {
                int countStride = nHeight / threadCount;
                countStride = ( countStride >> 2 ) << 2; // 必须是4的倍数

                startHeight = currentThreadId * countStride;
                if( currentThreadId != threadCount - 1 )
                {
                    endHeight = startHeight + countStride;
                }
            }

            ImageSubImage(pImage, pSubImg, startHeight, endHeight);
        };

        std::thread *expand_thread = new std::thread[threadCount - 1];
        for(int i = 0; i < threadCount - 1; ++i)
        {
            expand_thread[ i ] = std::thread(expand_functor, i);
        }
        expand_functor(threadCount - 1);
        for(int i = 0; i < threadCount - 1; ++i)
        {
            expand_thread[ i ].join();
        }
        if( expand_thread )
        {
            delete[] expand_thread;
            expand_thread = MNull;
        }

#else
            ImageSubImage(pImage,  pSubImg, 0, nHeight);
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("ImageSubImage--");
    }
#else

    {
        MInt32 nHeight = pImage->lHeight;
        ImageSubImage(pImage, pSubImg, 0, nHeight);
    }

#endif

    template<class T>
    MVoid Arcsoft_NLM_Pyramid<T>::ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt32 lOffset = sizeof(T) > 1 ? 512 : 128;

        T *pSrcDstData = m_pImage->pData;
        T *pSubData = pSubImg->pData;
        MInt32 lWidth = m_pImage->lWidth;
        MInt32 lHeight = m_pImage->lHeight;
        MInt32 lPitch = m_pImage->lStride;
        MInt32 lSubPitch = pSubImg->lStride;
        MInt32 x, y;

#ifdef USE_NEON
        uint8x16_t srcdata, subdata;
        uint16x8_t tmpdata;
        uint8x8_t resdata;
#endif
        for(y = lTopLine; y < lBotLine; y++)
        {
            T *tmpSrcDst = pSrcDstData + y * lPitch;
            T *tmpSub = pSubData + y * lSubPitch;

            x = 0;
#ifdef USE_NEON
            if (sizeof(T) == 1)
            {
                for (; x < lWidth - 15; x += 16)
                {
                    srcdata = vld1q_u8((MUInt8 *) tmpSrcDst + x);
                    subdata = vld1q_u8((MUInt8 *) tmpSub + x);

                    tmpdata = vaddl_u8(vget_low_u8(srcdata), vdup_n_u8(128));
                    tmpdata = vsubw_u8(tmpdata, vget_low_u8(subdata));
                    resdata = vmovn_u16(tmpdata);
                    vst1_u8((MUInt8 *) tmpSrcDst + x, resdata);

                    tmpdata = vaddl_u8(vget_high_u8(srcdata), vdup_n_u8(128));
                    tmpdata = vsubw_u8(tmpdata, vget_high_u8(subdata));
                    resdata = vmovn_u16(tmpdata);
                    vst1_u8((MUInt8 *) tmpSrcDst + x + 8, resdata);
                }
            }
#endif
            for(; x < lWidth; x++)
            {
                MInt32 lVal = (MInt16)tmpSrcDst[ x ] - (MInt16)tmpSub[ x ] + lOffset;
                CLAMP(lVal, 0, lOffset*2-1);
                tmpSrcDst[ x ] = lVal;
            }

        }

    }


    template<class T>
    MVoid Arcsoft_NLM_Pyramid<T>::ImageAddImage(ImageInfo<T>* pImage, ImageInfo<T>* pAddImg)
#ifdef USE_STD_THREAD
    {
        START_TIME;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        MInt32 nHeight = pImage->lHeight;
        int threadCount = nHeight >= 1024 ? 16 : 8;
        auto expand_functor = [&](int currentThreadId)
        {
            // 线程分割
            int startHeight = 0;
            int endHeight = nHeight;
            if( threadCount > 1 )
            {
                int countStride = nHeight / threadCount;
                countStride = ( countStride >> 2 ) << 2; // 必须是4的倍数

                startHeight = currentThreadId * countStride;
                if( currentThreadId != threadCount - 1 )
                {
                    endHeight = startHeight + countStride;
                }
            }

            ImageAddImage(pImage, pAddImg, startHeight, endHeight);
        };

        std::thread *expand_thread = new std::thread[threadCount - 1];
        for(int i = 0; i < threadCount - 1; ++i)
        {
            expand_thread[ i ] = std::thread(expand_functor, i);
        }
        expand_functor(threadCount - 1);
        for(int i = 0; i < threadCount - 1; ++i)
        {
            expand_thread[ i ].join();
        }
        if( expand_thread )
        {
            delete[] expand_thread;
            expand_thread = MNull;
        }

#else
            ImageAddImage(pImage,  pAddImg, 0, nHeight);
#endif

            END_TIME;
    }
#else

    {
        START_TIME;
        MInt32 nHeight = pImage->lHeight;
        ImageAddImage(pImage, pAddImg, 0, nHeight);
        END_TIME;
    }

#endif

    template<class T>
    MVoid Arcsoft_NLM_Pyramid<T>::ImageAddImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pAddImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt16 lOffset = sizeof(T) > 1 ? 512 : 128;

        T *pSrcDstData = m_pImage->pData;
        T *pAddData = pAddImg->pData;
        MInt32 lWidth = m_pImage->lWidth;
        MInt32 lHeight = m_pImage->lHeight;
        MInt32 lPitch = m_pImage->lStride;
        MInt32 lAddPitch = pAddImg->lStride;
        MInt32 x, y;

        for(y = lTopLine; y < lBotLine; y++)
        {
            T *tmpSrcDst = pSrcDstData + y * lPitch;
            T *tmpAdd = pAddData + y * lAddPitch;
            x = 0;
#ifdef USE_NEON
            if (sizeof(T) == 1)
            {
                uint8x8_t srcDst_8x8;
                uint8x8_t add_8x8;
                for(; x < lWidth - 8; x+=8)
                {
                    srcDst_8x8 = vld1_u8((MUInt8*)tmpSrcDst+x);
                    add_8x8 = vld1_u8((MUInt8*)tmpAdd+x);
                    uint16x8_t val_16x8 = vaddl_u8(srcDst_8x8, add_8x8);
                    val_16x8 = vqsubq_u16(val_16x8, vdupq_n_u16(128));
                    val_16x8 = vminq_u16(val_16x8, vdupq_n_u16(255));

                    vst1_u8((MUInt8*)tmpSrcDst+x, vmovn_u16(val_16x8));
                }
            }
#endif

            for(; x < lWidth; x++)
            {
                MInt16 lVal = tmpSrcDst[ x ] + tmpAdd[ x ] - lOffset;
                CLAMP(lVal, 0, lOffset*2-1);
                tmpSrcDst[ x ] = lVal;
            }
        }
    }

    template <typename T>
    MVoid Arcsoft_NLM_Pyramid<T>::CreateRadialMask(MByte *pImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch)
    {
        MInt32 xCenter = lWidth >> 1;
        MInt32 yCenter = lHeight >> 1;
        MInt32 maxDist = xCenter + yCenter;

        for(MInt32 y = 0; y < lHeight; y++)
        {
            MByte *pData = pImg + y * lPitch;
            MInt32 yDist = ABS(y - yCenter);
            for(MInt32 x = 0; x < lWidth; x++)
            {
                MInt32 xDist = ABS(x - xCenter);
                MInt32 sumDist = ( yDist + xDist ) * 21 / maxDist;
                pData[ x ] = 64 - sumDist; // 43~63
            }
        }
    }
NS_SINFLE_IMAGE_ENHANCEMENT_END