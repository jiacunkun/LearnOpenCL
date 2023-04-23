#include <ammem.h>
#include <ArcsoftSharpen.h>
#include <up16_fix.h>
#include "Arcsoft_Up_Down_Scale_Handle.h"
#include "ArcsoftLog.h"
#include "DefineForDebug.h"
#include "AnisotropicGuidedFiltering.h"
#include "Arcsoft_Sharpen_Pyramid.h"


NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template<typename T, typename T0>
    Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::Arcsoft_AnisGuided_Pyramid_Up2Down(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 layer, MInt32 nThreadCount)
    {
        LOGD("Arcsoft_AnisGuided_Pyramid_Up2Down++");
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_nThreadCount = nThreadCount;
        m_lDirection = 4;
        m_lLayer = layer;
        m_lWidth = lWidth;
        m_lHeight = lHeight;
        m_lPitch = lPitch/sizeof(T);
        LOGD("Arcsoft_AnisGuided_Pyramid_Up2Down m_lDirection = %d", m_lDirection);

        m_SrcPyrImage0.pData = MNull; // 0层为传入图像
        m_SrcPyrImage0.lWidth = lWidth;
        m_SrcPyrImage0.lHeight = lHeight;
        m_SrcPyrImage0.lStride = m_lPitch;

        m_DstPyrImage0.pData = MNull; // 0层为传入图像
        m_DstPyrImage0.lWidth = lWidth;
        m_DstPyrImage0.lHeight = lHeight;
        m_DstPyrImage0.lStride = m_lPitch;

        m_GuidedImage[0].lWidth = lWidth;
        m_GuidedImage[0].lHeight = lHeight;
        m_GuidedImage[0].lStride = lWidth;
        m_GuidedImage[0].pData = MNull;



        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            m_GuidedImage[i].lWidth   = lWidth >> i;
            m_GuidedImage[i].lHeight  = lHeight >> i;
            m_GuidedImage[i].lStride   = lWidth >> i;
            m_GuidedImage[i].pData = (MUInt8*)MMemAlloc(hMemMgr, m_GuidedImage[i].lHeight * m_GuidedImage[i].lStride * sizeof(MUInt8));

        }

        for (MInt32 i = 0; i <= m_lLayer - 1; i++)
        {
            m_SrcPyrImage[i].lWidth  = lWidth >>  (i+1);
            m_SrcPyrImage[i].lHeight = lHeight >> (i+1);
            m_SrcPyrImage[i].lStride  = lWidth >> (i + 1);
            m_SrcPyrImage[i].pData = (T0*)MMemAlloc(hMemMgr, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(T0));

            m_TempBuffer[i].lWidth  = lWidth >> (i);
            m_TempBuffer[i].lHeight = lHeight >> (i);
            m_TempBuffer[i].lStride  = lWidth >> (i);
            m_TempBuffer[i].pData = (T0*)MMemAlloc(hMemMgr, m_TempBuffer[i].lHeight * m_TempBuffer[i].lStride * sizeof(T0));

            m_pMeanA[i] = (MInt16*)MMemAlloc(hMemMgr, m_TempBuffer[i].lHeight * m_TempBuffer[i].lStride * sizeof(MInt16));
            m_pMeanB[i] = (MInt16*)MMemAlloc(hMemMgr, m_TempBuffer[i].lHeight * m_TempBuffer[i].lStride * sizeof(MInt16));
        
            m_DstPyrImage[i].lWidth = lWidth >> (i + 1);
            m_DstPyrImage[i].lHeight = lHeight >> (i + 1);
            m_DstPyrImage[i].lStride = lWidth >> (i + 1);
            m_DstPyrImage[i].pData = (T0*)m_pMeanA[i];
        }
        m_DstPyrImage[m_lLayer - 1].pData = (T0*)m_pMeanB[m_lLayer - 1];
    }

    template<typename T, typename T0>
    Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::~Arcsoft_AnisGuided_Pyramid_Up2Down()
    {
#if 0
        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            SAFE_FREE_ARRAY(m_hMemMgr, m_GuidedImage[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_dnShade[i].pData);
        }
#endif
        for (MInt32 i = 0; i <= m_lLayer - 1; i++)
        {
            SAFE_FREE_ARRAY(m_hMemMgr, m_SrcPyrImage[i].pData);
            //SAFE_FREE_ARRAY(m_hMemMgr, m_DstPyrImage[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_TempBuffer[i].pData);


            SAFE_FREE_ARRAY(m_hMemMgr, m_pMeanA[i]);
            SAFE_FREE_ARRAY(m_hMemMgr, m_pMeanB[i]);
        }

    }

    template<typename T, typename T0>
    MInt32 Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::run(MUInt8 *pSrc, MUInt8 *pGuided, MUInt8 *pDst, LPASVLOFFSCREEN pShade,
                                                      MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lGuidedPitch,
                                                      MFloat* pEps, MInt32* pSharpenIntensity, MInt32 lScale)

    {
        START_TIME;
        MInt32 lret = 0;

        MBool isNewDst = false;
        if (pDst == pSrc)
        {
            LOGD("pDst == pSrc");
            isNewDst = true;
            pSrc = (T*)MMemAlloc(m_hMemMgr, lHeight*lPitch*sizeof(T));
            MMemCpy(pSrc, pDst, lHeight*lPitch*sizeof(T));
            if (pDst == pGuided)
            {
                pGuided = pSrc;
            }
        }

        m_SrcPyrImage0.pData = pSrc;
        m_SrcPyrImage0.lStride = lPitch;
        m_DstPyrImage0.pData = pDst;
        m_DstPyrImage0.lStride = lPitch;

        m_GuidedImage[0].pData = pGuided;
        m_GuidedImage[0].lStride = lGuidedPitch;


#ifdef  BUILD_OPENCV
        char filename[255];
#endif
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 获取图像金字塔
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor);

		if (m_lLayer > 1)
        {
            MInt32 i = 1;
            up_down_scale.downScale2(m_SrcPyrImage0, m_SrcPyrImage[0], gaussian3x3);

            MMemCpy(m_TempBuffer[i].pData, m_SrcPyrImage[0].pData,
                    m_SrcPyrImage[0].lStride * m_SrcPyrImage[0].lHeight * sizeof(T0));

            if (pSrc != pGuided)
            {
                up_down_scale.downScale2(m_GuidedImage[i - 1], m_GuidedImage[i], gaussian3x3);
            }
        }

		for (MInt32 i = 2; i < m_lLayer; i++)
		{

			up_down_scale.downScale2(m_SrcPyrImage[i - 2], m_SrcPyrImage[i - 1], gaussian3x3);

			MMemCpy(m_TempBuffer[i].pData, m_SrcPyrImage[i - 1].pData,
                    m_SrcPyrImage[i - 1].lStride * m_SrcPyrImage[i - 1].lHeight * sizeof(T0));

			if (pSrc != pGuided)
			{
				up_down_scale.downScale2(m_GuidedImage[i - 1], m_GuidedImage[i], gaussian3x3);
			}
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 每层金字塔进行操作
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 如果最大层scale大于1，则用对应scale大小层的AB值
        // 从小到大迭代滤波
        for (MInt32 i = m_lLayer - 1; i > 0; i--)
        {
#if CALCULATE_TIME
            BasicTimer time2;
#endif
            ///////////////////////////////////////////////////////////////////
            // 单层降噪
            if (pEps[i] > 0)
            {
                if (pSrc == pGuided)
                {
                    LOGD("pSrc == pGuided!!");
                    MInt32 lTemp = sizeof(T0) > 1 ? 16 : 1;
                    auto obj = AnisotropicGuidedFiltering<T0, T0>(m_hMemMgr, m_mcvParallelMonitor, m_pMeanA[i], m_pMeanB[i], 1, m_lDirection, m_nThreadCount, MNull);
                    lret = obj.Run(m_hMemMgr,
                                   m_mcvParallelMonitor,
                                   m_TempBuffer[i].pData,
                                   m_TempBuffer[i].lWidth,
                                   m_TempBuffer[i].lHeight,
                                   m_TempBuffer[i].lStride,
                                   m_DstPyrImage[i-1].pData,
                                   m_DstPyrImage[i-1].lStride,
                                   pEps[ i ]*lTemp,
                                   1);

                }
                else
                {
                    auto obj = AnisotropicGuidedFiltering<T0, MUInt8>(m_hMemMgr, m_mcvParallelMonitor, m_pMeanA[i], m_pMeanB[i], 1, m_lDirection, m_nThreadCount, MNull);
                    lret = obj.Run(m_hMemMgr,
                                   m_mcvParallelMonitor,
                                   m_TempBuffer[i].pData,
                                   m_GuidedImage[i].pData,
                                   m_TempBuffer[i].lWidth,
                                   m_TempBuffer[i].lHeight,
                                   m_TempBuffer[i].lStride,
                                   m_GuidedImage[i].lStride,
                                   m_DstPyrImage[i-1].pData,
                                   m_DstPyrImage[i-1].lStride,
                                   pEps[ i ],
                                   1);

                }
            }
            else
            {

                MMemCpy(m_DstPyrImage[i-1].pData, m_TempBuffer[i].pData, m_SrcPyrImage[i-1].lHeight * m_SrcPyrImage[i-1].lStride *
                                                                            sizeof(T0));
            }


            // 得到拉普拉斯结果图
            ImageSubImage<T0>(&m_DstPyrImage[i-1], &m_SrcPyrImage[i - 1]);

            // 拉普拉斯上采样后，加到原图像中

            if (i == 1)
            {
                up_down_scale.upScale2(m_DstPyrImage[i-1], m_DstPyrImage0, gaussian3x3);
                //ImageAddImage<MUInt8>(&m_TempBuffer[0], &m_DstPyrImage0);
                ImageAddImage<MUInt8>(&m_SrcPyrImage0, &m_DstPyrImage0, &m_TempBuffer[0], 0, lHeight);
            }
            else
            {
                up_down_scale.upScale2(m_DstPyrImage[i-1], m_DstPyrImage[i-2], gaussian3x3);
                ImageAddImage<T0>(&m_TempBuffer[i-1], &m_DstPyrImage[i-2]);
            }

#if 1 //逐步释放内存
            SAFE_FREE_ARRAY(m_hMemMgr, m_GuidedImage[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_TempBuffer[i].pData);
#endif
#if CALCULATE_TIME
            LOGD("The %d layer is finished timer count = %fms!\n", i, time2.UpdateAndGetDelta());
#endif
        }

        {
#if CALCULATE_TIME
            BasicTimer time2;
#endif
            // 对0层进行降噪
            MInt32 i = 0;

            // 单层降噪
            if (pEps[i] > 0)
            {
                // 将小图的AB上采样到大图尺寸
                int AB_type = (lScale == 2) && m_lLayer > 1;
                if (AB_type)
                {
                    ImageInfo<MInt16> LargeMeanA(m_pMeanA[i], m_lWidth, m_lHeight, m_lWidth);
                    ImageInfo<MInt16> LargeMeanB(m_pMeanB[i], m_lWidth, m_lHeight, m_lWidth);
                    ImageInfo<MInt16> MeanA(m_pMeanA[i + 1], m_lWidth / 2, m_lHeight / 2, m_lWidth / 2);
                    ImageInfo<MInt16> MeanB(m_pMeanB[i + 1], m_lWidth / 2, m_lHeight / 2, m_lWidth / 2);

                    up_down_scale.upScale2(MeanA, LargeMeanA, gaussian3x3);
                    up_down_scale.upScale2(MeanB, LargeMeanB, gaussian3x3);

                    mat_write255(m_lHeight/2, m_lWidth/2, CV_16SC1, m_pMeanA[i+1], "pSmallMeanA.png", 1.0);
                    mat_write255(m_lHeight/2, m_lWidth/2, CV_16SC1, m_pMeanB[i+1], "pSmallMeanB.png", 1.0);
                    mat_write255(m_lHeight, m_lWidth, CV_16SC1, m_pMeanA[i], "pLargeMeanA.png", 1.0);
                    mat_write255(m_lHeight, m_lWidth, CV_16SC1, m_pMeanB[i], "pLargeMeanB.png", 1.0);
                }

                if (pSrc == pGuided)
                {
                    LOGD("pSrc == pGuided!!");
                    //auto obj = AnisotropicGuidedFiltering<MUInt8, MUInt8>(m_hMemMgr, m_mcvParallelMonitor, m_lDirection+1, m_nThreadCount, &ShadeTemp);
                    auto obj = AnisotropicGuidedFiltering<MUInt8, MUInt8>(m_hMemMgr, m_mcvParallelMonitor,
                                                                          m_pMeanA[i], m_pMeanB[i], AB_type<=0,
                                                                          m_lDirection,
                                                                          m_nThreadCount, MNull);
                    lret = obj.Run(m_hMemMgr,
                                   m_mcvParallelMonitor,
                                   m_TempBuffer[0].pData,
                                   m_TempBuffer[0].lWidth,
                                   m_TempBuffer[0].lHeight,
                                   m_TempBuffer[0].lStride,
                                   m_DstPyrImage0.pData,
                                   m_DstPyrImage0.lStride,
                                   pEps[i],
                                   lScale);
                }
                else
                {

                    //auto obj = AnisotropicGuidedFiltering<MUInt8, MUInt8>(m_hMemMgr, m_mcvParallelMonitor, m_lDirection+1, m_nThreadCount, &ShadeTemp);
                    auto obj = AnisotropicGuidedFiltering<MUInt8, MUInt8>(m_hMemMgr, m_mcvParallelMonitor,
                                                                          m_pMeanA[i], m_pMeanB[i], AB_type <= 0, m_lDirection,  m_nThreadCount, MNull);
                    lret = obj.Run(m_hMemMgr,
                                   m_mcvParallelMonitor,
                                   m_TempBuffer[0].pData,
                                   m_GuidedImage[i].pData,
                                   m_TempBuffer[0].lWidth,
                                   m_TempBuffer[0].lHeight,
                                   m_TempBuffer[0].lStride,
                                   m_GuidedImage[i].lStride,
                                   m_DstPyrImage0.pData,
                                   m_DstPyrImage0.lStride,
                                   pEps[ i ],
                                   lScale);
                }

            }
            else
            {

                MMemCpy(m_DstPyrImage0.pData, m_TempBuffer[0].pData, m_SrcPyrImage0.lHeight * m_SrcPyrImage0.lStride *
                                                                    sizeof(T));

            }


#ifdef  BUILD_OPENCV
            sprintf(filename, "m_DstPyrImage0_Up2Down_Restore.png");
            mat_write255(m_DstPyrImage0.lHeight, m_DstPyrImage0.lStride, CV_8UC1, m_DstPyrImage0.pData, filename, 1.0);
            sprintf(filename, "m_GuidedImage0.png");
            mat_write255(m_GuidedImage[i].lHeight, m_GuidedImage[i].lWidth, CV_8UC1, m_GuidedImage[i].pData, filename, 1.0);
#endif

#if CALCULATE_TIME
            LOGD("The 0 layer pyramid is finished timer count = %fms!\n", time2.UpdateAndGetDelta());
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

        END_TIME;
        return lret;
    }

    template<typename T, typename T0>
    MInt32 Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pGuided, LPASVLOFFSCREEN pDst, MFloat* pEps, MInt32* pSharpenIntensity, LPASVLOFFSCREEN pShade, MInt32 lScale)
    {
        MInt32 lret = 0;

        lret = run(pSrc->ppu8Plane[0], pGuided->ppu8Plane[0], pDst->ppu8Plane[0], pShade,
                   pSrc->i32Width, pSrc->i32Height, pSrc->pi32Pitch[0], pGuided->pi32Pitch[0],
                   pEps, pSharpenIntensity, lScale);

        return lret;

    }

    template<class T, class T0>
    template<typename T1>
    MVoid Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::ImageSubImage(ImageInfo<T1>* pImage, ImageInfo<T1>* pSubImg)
    {
        LOGD("ImageSubImage++");
        MInt32 nHeight = pImage->lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)

        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight >= 1024 ? 16 : 8;
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
        LOGD("ImageSubImage--");
    }

    template<class T, class T0>
    template<typename T1>
    MVoid Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::ImageSubImage(ImageInfo<T1>* m_pImage, ImageInfo<T1>* pSubImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt32 lOffset = sizeof(T1) > 1 ? 512 : 128;

        auto *pSrcDstData = m_pImage->pData;
        auto *pSubData = pSubImg->pData;
        MInt32 lWidth = m_pImage->lWidth;
        MInt32 lHeight = m_pImage->lHeight;
        MInt32 lPitch = m_pImage->lStride;
        MInt32 lSubPitch = pSubImg->lStride;
        MInt32 x, y;

#ifdef USE_NEON_PYRAMID00
        uint8x16_t srcdata, subdata;
        uint16x8_t tmpdata;
        uint8x8_t resdata;
#endif
        for(y = lTopLine; y < lBotLine; y++)
        {
            auto *tmpSrcDst = pSrcDstData + y * lPitch;
            auto *tmpSub = pSubData + y * lSubPitch;

            x = 0;
#ifdef USE_NEON_PYRAMID00
            for(; x < lWidth - 15; x += 16)
            {
                srcdata = vld1q_u8(tmpSrcDst + x);
                subdata = vld1q_u8(tmpSub + x);

                tmpdata = vaddl_u8(vget_low_u8(srcdata), vdup_n_u8(128));
                tmpdata = vsubw_u8(tmpdata, vget_low_u8(subdata));
                resdata = vmovn_u16(tmpdata);
                vst1_u8(tmpSrcDst + x, resdata);

                tmpdata = vaddl_u8(vget_high_u8(srcdata), vdup_n_u8(128));
                tmpdata = vsubw_u8(tmpdata, vget_high_u8(subdata));
                resdata = vmovn_u16(tmpdata);
                vst1_u8(tmpSrcDst + x + 8, resdata);
            }

#else
            for(; x < lWidth; x++)
            {
                MInt32 lVal = (MInt32)tmpSrcDst[ x ] - (MInt32)tmpSub[ x ] + lOffset;
                CLAMP(lVal, 0, lOffset*2-1);
                tmpSrcDst[ x ] = lVal;
            }
#endif
        }

    }


    template<class T, class T0>
    template<typename T1>
    MVoid Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::ImageAddImage(ImageInfo<T1>* pImage, ImageInfo<T1>* pAddImg)
    {
        LOGD("ImageAddImage++");
        MInt32 nHeight = pImage->lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)

        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight >= 1024 ? 16 : 8;
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
        LOGD("ImageAddImage--");
    }


    template<class T, class T0>
    template<typename T1>
    MVoid Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::ImageAddImage(ImageInfo<T1>* m_pImage, ImageInfo<T1>* pAddImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt32 lOffset = sizeof(T1) > 1 ? 512 : 128;

        auto *pSrcDstData = m_pImage->pData;
        auto *pAddData = pAddImg->pData;
        MInt32 lWidth = m_pImage->lWidth;
        MInt32 lHeight = m_pImage->lHeight;
        MInt32 lPitch = m_pImage->lStride;
        MInt32 lAddPitch = pAddImg->lStride;
        MInt32 x, y;

        for(y = lTopLine; y < lBotLine; y++)
        {
            auto *tmpSrcDst = pSrcDstData + y * lPitch;
            auto *tmpAdd = pAddData + y * lAddPitch;
            for(x = 0; x < lWidth; x++)
            {
                MInt32 lVal = tmpSrcDst[ x ] + tmpAdd[ x ] - lOffset;
                CLAMP(lVal, 0, lOffset*2-1);
                tmpSrcDst[ x ] = lVal;
            }
        }
        return;
    }

    template<class T, class T0>
    template<typename T1>
    MVoid Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::ImageAddImage(ImageInfo<T1>* pImage, ImageInfo<T1>* pAddImg, ImageInfo<T1>* pDst, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt32 lOffset = sizeof(T1) > 1 ? 512 : 128;

        auto* pSrcData = pImage->pData;
        auto* pAddData = pAddImg->pData;
        auto* pDstData = pDst->pData;
        MInt32 lWidth = pImage->lWidth;
        MInt32 lHeight = pImage->lHeight;
        MInt32 lPitch = pImage->lStride;
        MInt32 lAddPitch = pAddImg->lStride;
        MInt32 lDstPitch = pDst->lStride;
        MInt32 x, y;

        for (y = lTopLine; y < lBotLine; y++)
        {
            auto* tmpSrc = pSrcData + y * lPitch;
            auto* tmpAdd = pAddData + y * lAddPitch;
            auto* tmpDst = pDstData + y * lDstPitch;
            for (x = 0; x < lWidth; x++)
            {
                MInt32 lVal = tmpSrc[x] + tmpAdd[x] - lOffset;
                CLAMP(lVal, 0, lOffset * 2 - 1);
                tmpDst[x] = lVal;
            }
        }
        return;
    }


    template<class T, class T0>
    MVoid Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::GetEdgeThr(MInt32 nLuma, MInt32 nLumaDetail, MInt32* max_threshold, MInt32* min_threshold, MInt16* contrast_param)
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


        //明亮度参数计算
        double dLumaVal = 0.0;
        if (nLuma > 25)
        {
            dLumaVal = (nLuma * 0.01 - 0.25) / 0.75;
            dLumaVal = MAX(0.0, MIN(1.0, dLumaVal)) * 0.5 + 0.5;
        }
        else
        {
            dLumaVal = MAX(0.0, MIN(0.5, 2.0 * nLuma * 0.01));
        }


        double weight_list[4] = {1.0, 0.43, 0.21, 0.12};

        for (int k = 0; k < 4; k++)
        {
            double dWeightedLumDetailVal = dLumaDetailVal * weight_list[k];
            max_threshold[k] = ROUND(dWeightedLumDetailVal * 4 * 1.414);
            min_threshold[k] = ROUND(dWeightedLumDetailVal * 4);
        }

        double diff_val = 1.0 - dLumaVal;
        contrast_param[0] = 128 * dLumaVal;
        contrast_param[1] = 128 * 0.33 * diff_val + 128 * dLumaVal;
        contrast_param[2] = 128 * 0.66 * diff_val + 128 * dLumaVal;
        contrast_param[3] = 128 * 1.0;

    }

    template<class T, class T0>
    template<typename T1>
    MInt32 Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::CalEdgeMask(ImageInfo<T1> &input, ImageInfo<MInt16> &output, MInt32 maxThr, MInt32 minThr)
    {
        LOGD("CalEdgeMask++");
        MInt32 lret = MOK;

        MInt32 lHeight = input.lHeight;
        MInt32 lWidth = input.lWidth;
        MInt32 lPitch = input.lStride;

        MInt32 offset[4] = { 3, 2 * lPitch + 2, 3 * lPitch, 2 * lPitch - 2 };

        auto *pTempSrc = input.pData + 3*lPitch;
        MInt16 *pTempDst = output.pData + 3*lWidth;

        MMemSet(output.pData, 0, output.lHeight * output.lStride * sizeof(MInt16));

        for (int y = 3; y < lHeight-3; y++)
        {
            for (int x = 3; x < lWidth-3; x++)
            {
                MInt32 sumMax = 0;
                MInt32 sumMin = 0;
                MUInt8 nCur = pTempSrc[x];
                for (int i = 0; i < 4; i++)
                {
                    sumMax += (ABS(pTempSrc[x+offset[i]] - nCur) > maxThr);
                    sumMin += (ABS(pTempSrc[x+offset[i]] - nCur) > minThr);
                    sumMax += (ABS(pTempSrc[x-offset[i]] - nCur) > maxThr);
                    sumMin += (ABS(pTempSrc[x-offset[i]] - nCur) > minThr);
                }
                pTempDst[x] = sumMax > 0 ? 128 : (sumMin > 0 ? 100 : 0); // 强边缘设置为128，弱纹理设置为64
            }
            pTempSrc += lPitch;
            pTempDst += lWidth;
        }

        LOGD("CalEdgeMask--");
        return lret;
    }

    template<class T, class T0>
    MInt32 Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::RefineEdgeMask(ImageInfo<MInt16> &input, ImageInfo<MInt16> &output)
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

                pTempDst[x] = (nCur == 0 && sumMax >= 4) ? 100 : ((nCur > 0 && sumMax <= 1) ? 0 : nCur);
            }
            pTempSrc += lWidth;
            pTempDst += lWidth;
        }


        return lret;
    }

    template<class T, class T0>
    template<typename T2, typename T3>
    MVoid Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::MixImages(ImageInfo<T2>* pImage1, ImageInfo<T3>* pImage2,  ImageInfo<MInt16>* pAlpha, ImageInfo<T3>* pDstImage)
    {
        LOGD("MixImages++");
        MInt32 lWidth = pDstImage->lWidth;
        MInt32 lHeight = pDstImage->lHeight;
        MInt32 lPitch = pDstImage->lStride;
        MInt32 lAlphaPitch = pAlpha->lStride;

        for (MInt32 y = 0; y < lHeight; y++)
        {
            auto *pCurSrc1 = pImage1->pData + y * lPitch;
            auto *pCurSrc2 = pImage2->pData + y * lPitch;
            auto *pCurDst = pDstImage->pData + y * lPitch;
            MInt16 *pA = pAlpha->pData + y * lAlphaPitch;

            for (MInt32 x = 0; x < lWidth; x++)
            {
                MFloat tempA = pA[x];
                tempA = tempA > 128 ? 128 : tempA;
                MInt32 val;

                val = (1.0*(128 - tempA) * pCurSrc1[x]) + (1.0*tempA * pCurSrc2[x]);
                val = (val + 64)/128;
                val = val > 255 ? 255 : val;
                pCurDst[x] = val;
            }
        }
        LOGD("MixImages--");
    }

    template<class T, class T0>
    template<typename T2, typename T3>
    MVoid Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::MixImages(ImageInfo<T2>* pImage1, ImageInfo<T3>* pImage2,  MInt16 pAlpha, ImageInfo<T3>* pDstImage)
    {
        LOGD("MixImages++");
        MInt32 lWidth = pDstImage->lWidth;
        MInt32 lHeight = pDstImage->lHeight;
        MInt32 lPitch = pDstImage->lStride;

        for (MInt32 y = 0; y < lHeight; y++)
        {
            auto *pCurSrc1 = pImage1->pData + y * lPitch;
            auto *pCurSrc2 = pImage2->pData + y * lPitch;
            auto *pCurDst = pDstImage->pData + y * lPitch;


            for (MInt32 x = 0; x < lWidth; x++)
            {
                MFloat tempA = pAlpha;
                tempA = tempA > 128 ? 128 : tempA;
                MInt32 val;

                val = (1.0*(128 - tempA) * pCurSrc1[x]) + (1.0*tempA * pCurSrc2[x]);
                val = (val + 64)/128;
                val = val > 255 ? 255 : val;
                pCurDst[x] = val;
            }
        }
        LOGD("MixImages--");
    }

    template<class T, class T0>
    MVoid Arcsoft_AnisGuided_Pyramid_Up2Down<T, T0>::ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, MInt32 lTopLine, MInt32 lBotLine, ImageInfo<MInt16>* pLaplaceImg)
    {
        T *pSrcData = m_pImage->pData;
        T *pSubData = pSubImg->pData;
        MInt32 lWidth = m_pImage->lWidth;
        MInt32 lHeight = m_pImage->lHeight;
        MInt32 lPitch = m_pImage->lStride;
        MInt32 lSubPitch = pSubImg->lStride;
        MInt32 x, y;

        for(y = lTopLine; y < lBotLine; y++)
        {
            T *tmpSrc = pSrcData + y * lPitch;
            T *tmpSub = pSubData + y * lSubPitch;
            MInt16 *pTempDst = pLaplaceImg->pData + y * pLaplaceImg->lStride;

            x = 0;

            for(; x < lWidth; x++)
            {
                MInt32 lVal = (MInt16)tmpSrc[ x ] - (MInt16)tmpSub[ x ];
                pTempDst[ x ] = lVal;
            }

        }
        return;
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END
