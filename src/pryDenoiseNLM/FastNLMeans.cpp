
#include <cmath>
#include <BasicTimer.h>
#include <thread>
#include "ammem.h"
#include "ArcsoftLog.h"
#include "DefineForDebug.h"
#include "Arcsoft_Copy_To_FilledImage.h"
#include "FastNLMeans.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    FastNLMeans::FastNLMeans(MHandle hMemMgr, MHandle mcvParallelMonitor)
    {
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
    }

    FastNLMeans::~FastNLMeans()
    {

    }

    MInt32 FastNLMeans::run(ImageInfo<MUInt8>* pSrcImage,
                            ImageInfo<MUInt8>* pDstImage,
                            MInt16 nSearchRadius,
                            MInt16 nNeighborRadius,
                            MFloat fIntensity)
    {
        LOGD("FastNLMeans::run++");

#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 lRet = 0;

        if (pSrcImage->pData == MNull || pDstImage->pData == MNull)
        {
            LOGD("pSrcImage->pData == MNull || pDstImage->pData == MNull");
            return -1;
        }

        m_nSearchRadius = nSearchRadius; // 搜索区域半径
        m_nNeighborRadius = nNeighborRadius; // 邻域半径
        m_nPadRadius = m_nSearchRadius + m_nNeighborRadius;

        m_lHeight = pSrcImage->lHeight;
        m_lWidth = pSrcImage->lWidth;
        m_lPitch = pSrcImage->lStride;

        m_fIntensity = fIntensity;
        m_fSigmaSqrt = fIntensity * fIntensity * (m_nNeighborRadius * 2 + 1) * (m_nNeighborRadius * 2 + 1);

        // 对图像进行填充
        MInt32 lWidthPad = m_lWidth + m_nPadRadius * 2;
        MInt32 lHeightPad = m_lHeight + m_nPadRadius * 2;
        auto *pSrcPad = SAFE_MALLOC(m_hMemMgr, MUInt8, lWidthPad * lHeightPad);

        ImageInfo<MUInt8> SrcImagePad(pSrcPad, lWidthPad, lHeightPad, lWidthPad);
        Arcsoft_Copy_To_FilledImage<MUInt8>(pSrcImage, &SrcImagePad, m_nPadRadius);

        // 过程
        Process(&SrcImagePad, pSrcImage, pDstImage);

        ////////////////////////////////////////////
        // 释放内存
        ////////////////////////////////////////////
        SAFE_FREE_ARRAY(m_hMemMgr, pSrcPad);

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("FastNLMeans::run--");
        return lRet;
    }

    void FastNLMeans::Process(ImageInfo<MUInt8>* pSrcImagePad,
                              ImageInfo<MUInt8>* pSrcImage,
                              ImageInfo<MUInt8>* pDstImage)
    {
        LOGD("FastNLMeans::Process++");

#if CALCULATE_TIME
        BasicTimer time;
#endif

        auto *pSumWeight = SAFE_MALLOC(m_hMemMgr, MFloat, m_lWidth * m_lHeight);
        MMemSet(pSumWeight, 0, m_lWidth * m_lHeight * sizeof(MFloat));
        auto *pSum = SAFE_MALLOC(m_hMemMgr, MFloat, m_lWidth * m_lHeight);
        MMemSet(pSum, 0, m_lWidth * m_lHeight * sizeof(MFloat));

        MInt32 lPitch_Integral = m_lWidth + m_nNeighborRadius * 2 + 1;
        auto *pIntegral = SAFE_MALLOC(m_hMemMgr, MUInt32, lPitch_Integral * (m_lHeight + 1 + m_nNeighborRadius * 2));
        MMemSet(pIntegral, 0, lPitch_Integral * (m_lHeight + 1 + m_nNeighborRadius * 2) * sizeof(MUInt32));

        // 积分图计算均值的矩形框
        m_lTempValY = (m_nNeighborRadius + m_nNeighborRadius + 1) * lPitch_Integral;
        m_lTempValX = (m_nNeighborRadius + m_nNeighborRadius + 1);

        // 计算得到搜索窗中的每个点的权重值，和对应的权重乘以该像素点值
        CalWeight(pSrcImagePad, pSrcImage, pIntegral, lPitch_Integral, pSumWeight, pSum);

        // 进行加权平均，得到结果
        GetResult(pSumWeight, pSum, pDstImage->pData);

        SAFE_FREE_ARRAY(m_hMemMgr, pSumWeight);
        SAFE_FREE_ARRAY(m_hMemMgr, pSum);
        SAFE_FREE_ARRAY(m_hMemMgr, pIntegral);

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("FastNLMeans::Process--");
    }

    void FastNLMeans::CalWeight(ImageInfo<MUInt8>* pSrcImagePad,
                                ImageInfo<MUInt8>* pSrcImage,
                                MUInt32 *pIntegral,
                                MInt32 lPitch_Integral,
                                MFloat *pSumWeight,
                                MFloat *pSum)
    {
        LOGD("FastNLMeans::CalWeight++");

#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lPitch = pSrcImagePad->lStride;

        MInt16 lSearchArea = (m_nSearchRadius*2+1)*(m_nSearchRadius*2+1);
        auto *pOffsetXY = SAFE_MALLOC(m_hMemMgr, MInt32, lSearchArea);

        for (MInt16 ty = -m_nSearchRadius, i = 0; ty <= m_nSearchRadius; ty++)
        {
            for (MInt16 tx = -m_nSearchRadius; tx <= m_nSearchRadius; tx++, i++)
            {
                // todo: 后续可以结合方向滤波思想，只对某个方向进行处理
                pOffsetXY[i] = tx + ty * lPitch; // 独立出来 TODO：方便后续并行
            }
        }


        CalBlockWeight(pSrcImagePad,
                       pSrcImage,
                       pIntegral,
                       lPitch_Integral,
                       pOffsetXY,
                       lSearchArea,
                       pSumWeight,
                       pSum,
                       0, lSearchArea);

        SAFE_FREE_ARRAY(m_hMemMgr, pOffsetXY);

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("FastNLMeans::CalWeight--");
    }

    void FastNLMeans::CalBlockWeight(ImageInfo<MUInt8>* pSrcImagePad,
                                     ImageInfo<MUInt8>* pSrcImage,
                                     MUInt32 *pIntegral,
                                     MInt32 lPitch_Integral,
                                     MInt32 *pOffsetXY,
                                     MInt16 lSearchArea,
                                     MFloat *pSumWeight,
                                     MFloat *pSum)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 nHeight = m_lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        int threadCount = 16;
        auto expand_functor = [&](int currentThreadId)
        {
            // 线程分割
            int startHeight = 0;
            int endHeight = nHeight;
            if( threadCount > 1 )
            {
                int countStride = nHeight / threadCount;
//                countStride = ( countStride >> 2 ) << 2; // 必须是4的倍数

                startHeight = currentThreadId * countStride;
                if( currentThreadId != threadCount - 1 )
                {
                    endHeight = startHeight + countStride;
                }
            }

            CalBlockWeight(pSrcImagePad,
                           pSrcImage,
                           pIntegral,
                           lPitch_Integral,
                           pOffsetXY,
                           lSearchArea,
                           pSumWeight,
                           pSum,
                           startHeight, endHeight);

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
        CalBlockWeight(pSrcImagePad,
                           pSrcImage,
                           pIntegral,
                           lPitch_Integral,
                           pOffsetXY,
                           lSearchArea,
                           pSumWeight,
                           pSum,
                           0, nHeight);
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
    }

    void FastNLMeans::CalBlockWeight(ImageInfo<MUInt8>* pSrcImagePad,
                                     ImageInfo<MUInt8>* pSrcImage,
                                     MUInt32 *pIntegral,
                                     MInt32 lPitch_Integral,
                                     MInt32 *pOffsetXY,
                                     MInt16 lSearchArea,
                                     MFloat *pSumWeight,
                                     MFloat *pSum,
                                     MInt32 lTopLine, MInt32 lBotLine)
    {
        for (MInt16 i = 0; i < lSearchArea; i++) // todo: 后续可以结合方向滤波思想，只对某个方向进行处理
        {
            CalIntegralImgSqDiff(pSrcImagePad, pIntegral, lPitch_Integral, pOffsetXY[i], 0, m_lHeight + m_nNeighborRadius * 2); // 得到偏移位置，和中心点的匹配度
            // 0, m_lHeight + m_nNeighborRadius * 2

            AddBlockSum(pSrcImagePad, pSrcImage, pIntegral, lPitch_Integral, pOffsetXY[i], pSumWeight, pSum);
        }
    }

    void FastNLMeans::CalIntegralImgSqDiff(ImageInfo<MUInt8>* pSrcImagePad,
                                           MUInt32 *pIntegral,
                                           MInt32 lPitch_Integral,
                                           MInt32 lOffsetXY)
    {
#if CALCULATE_TIME
        //BasicTimer time;
#endif

        MInt32 nHeight = m_lHeight + m_nNeighborRadius * 2;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
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

            CalIntegralImgSqDiff(pSrcImagePad, pIntegral, lPitch_Integral, lOffsetXY, startHeight, endHeight);
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
        CalIntegralImgSqDiff(pSrcImagePad, pIntegral, lPitch_Integral, lOffsetXY, 0, nHeight);
#endif

#if CALCULATE_TIME
        //LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
    }

    void FastNLMeans::CalIntegralImgSqDiff(ImageInfo<MUInt8>* pSrcImagePad,
                                           MUInt32 *pIntegral,
                                           MInt32 lPitch_Integral,
                                           MInt32 lOffsetXY,
                                           MInt32 lTopLine, MInt32 lBotLine)
    {
        //LOGD("FastNLMeans::CalIntegralImgSqDiff++");

#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 lPith = pSrcImagePad->lStride;
        auto *pData = pSrcImagePad->pData;

        auto *ColSum = SAFE_MALLOC(MNull, MUInt32, lPitch_Integral-1);
        MMemSet(ColSum, 0, (lPitch_Integral-1) * sizeof(MUInt32));

        //MMemSet(pIntegral + lTopLine*lPitch_Integral, 0, lPitch_Integral * sizeof(MUInt32)); // 第一行都为0
        //for (MInt32 y = 0; y < m_lHeight + m_nNeighborRadius * 2; y++)
        for (MInt32 y = lTopLine; y < lBotLine; y++)
        {
            auto *pTempData = pData + (y + m_nSearchRadius) * lPith + m_nSearchRadius;
            //auto *pLinePL = pIntegral + (y) * lPitch_Integral + 1; // 上一行位置
            auto *pLinePD = pIntegral + (y + 1) * lPitch_Integral + 1; // 当前位置，注意每行的第一列的值都为0
            pLinePD[-1] = 0;

            for (MInt32 x = 0; x < m_lWidth + m_nNeighborRadius * 2; x++)
            {
                ColSum[x] += ABS(pTempData[x] - pTempData[x + lOffsetXY]); // 行方向累加
                pLinePD[x] = pLinePD[x-1] + ColSum[x]; // 更新积分图
            }
        }

#ifdef BUILD_OPENCV00
        MInt32 lHeight = pSrcImagePad->lHeight;
        MInt32 lWidth = pSrcImagePad->lWidth;
        cv::Mat SrcImg(lHeight, lWidth, CV_8UC1, pData);
        cv::Mat img(m_lHeight+1+ m_nNeighborRadius*2, m_lWidth+1+ m_nNeighborRadius*2, CV_32SC1, pIntegral);
#endif
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        //LOGD("FastNLMeans::CalIntegralImgSqDiff--");
    }

    void FastNLMeans::AddBlockSum(ImageInfo<MUInt8>* pSrcImagePad,
                                  ImageInfo<MUInt8>* pSrcImage,
                                  MUInt32 *pIntegral,
                                  MInt32 lPitch_Integral,
                                  MInt32 lOffsetXY,
                                  MFloat *pSumWeight, MFloat *pSum)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 nHeight = m_lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
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

            AddBlockSum(pSrcImagePad, pSrcImage, pIntegral, lPitch_Integral, lOffsetXY, pSumWeight, pSum, startHeight, endHeight);
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
            AddBlockSum(pSrcImagePad, pSrcImage, pIntegral, lPitch_Integral, lOffsetXY, pSumWeight, pSum, 0, nHeight);
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
    }

    void FastNLMeans::AddBlockSum(ImageInfo<MUInt8>* pSrcImagePad,
                                  ImageInfo<MUInt8>* pSrcImage,
                                  MUInt32 *pIntegral,
                                  MInt32 lPitch_Integral,
                                  MInt32 lOffsetXY,
                                  MFloat *pSumWeight, MFloat *pSum,
                                  MInt32 lTopLine, MInt32 lBotLine)
    {
        auto *pSrcData = pSrcImagePad->pData;
        MInt32 lPitch = pSrcImagePad->lStride;
        MInt32 lTempOffset = m_nPadRadius * lPitch + m_nPadRadius;


        for (MInt32 y = lTopLine; y < lBotLine; y++)
        {
            auto *pTempIntegral = pIntegral + y * lPitch_Integral;
            auto *pTempSrcData = pSrcData + y * lPitch + lTempOffset + lOffsetXY;
            auto *pTempWeight = pSumWeight + y * m_lWidth;
            auto *pTempSum = pSum + y * m_lWidth;;
            for (MInt32 x = 0; x < m_lWidth; x++)
            {
//                        MInt16 nDist2 = pIntegral[x + m_nNeighborRadius + m_nNeighborRadius + (y + m_nNeighborRadius + m_nNeighborRadius+1)*lPithc_Integral+1]
//                                      + pIntegral[x - m_nNeighborRadius + m_nNeighborRadius + (y + m_nNeighborRadius - m_nNeighborRadius)*lPithc_Integral]
//                                      - pIntegral[x - m_nNeighborRadius + m_nNeighborRadius + (y + m_nNeighborRadius + m_nNeighborRadius+1)*lPithc_Integral]
//                                      - pIntegral[x + m_nNeighborRadius + m_nNeighborRadius + (y + m_nNeighborRadius - m_nNeighborRadius)*lPithc_Integral+1];

                MInt16 nDist2 = pTempIntegral[x + m_lTempValX + m_lTempValY]
                                + pTempIntegral[x]
                                - pTempIntegral[x + m_lTempValY]
                                - pTempIntegral[x + m_lTempValX];


                MFloat nWeight = exp(-nDist2 / m_fSigmaSqrt)+1e-6;
//                if (nWeight <= 0)
//                {
//                    LOGD("nWeight <= 0");
//                }
                pTempWeight[x] += nWeight;
                pTempSum[x] += pTempSrcData[x] * nWeight;
            }
        }
    }

    void FastNLMeans::GetResult(MFloat *pSumWeight,
                                MFloat *pSum,
                                MUInt8* pDstData)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 nHeight = m_lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
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

            GetResult(pSumWeight, pSum, pDstData, startHeight, endHeight);
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
            GetResult(pSumWeight, pSum, pDstData, 0, nHeight);
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif

    }

    void FastNLMeans::GetResult(MFloat *pSumWeight,
                                MFloat *pSum,
                                MUInt8* pDstData,
                                MInt32 lTopLine, MInt32 lBotLine)
    {
#if CALCULATE_TIME
        //BasicTimer time;
#endif

        for (MInt32 y = lTopLine; y < lBotLine; y++)
        {
            auto *pTempDstData = pDstData + y * m_lPitch;
            auto *pTempWeight = pSumWeight + y * m_lWidth;
            auto *pTempSum = pSum + y * m_lWidth;
            for (MInt32 x = 0; x < m_lWidth; x++)
            {
                MInt16 val = pTempSum[x] / (pTempWeight[x]) + 0.5;
                CLAMP(val, 0, 255);
                pTempDstData[x] = val;
            }
        }
#if CALCULATE_TIME
        //LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
    }


NS_SINFLE_IMAGE_ENHANCEMENT_END