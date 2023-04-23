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
#include <common/BasicTimer.h>
#include "NLMeans.h"
//#include <thread>  // for c++11 threads
#include "imagebase.h"  // for ABS, MAX...
#include "ArcsoftLog.h"
#include "mobilecv.h" // for threads
#include "single_image_enhancement_define.h"


// NEON 开启开关
//#define USE_NEON

#if defined(USE_NEON) || defined(__ARM_NEON__)
#define USE_NEON_NLM //todo：后续完善neon
#endif

#ifdef USE_NEON_NLM
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif

#define PROCESS_NLM_EDGE //控制NLM的边缘是否进行处理，目前加入padding不需要处理边缘。2021.3.16

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

#define ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pInvMap)        \
{                                                                            \
    lDif = ABS(lCVal - lNVal);                                                \
    lDif = MIN(49,lDif);                                                    \
    lDif = lDif * 9;                                                        \
    lTmpW = pInvMap[lDif];                                                    \
    lTmpW >>= 1;                                                            \
    lWSum += lTmpW;                                                            \
    lSumW += lNVal * lTmpW;                                                    \
}


    NLMeans::NLMeans(MHandle hMemMgr, MHandle mcvParallelMonitor, MFloat fIntensity)
    {
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;

        m_pMap = ( MInt32 * ) MMemAlloc(m_hMemMgr, 16 * 50 * sizeof(MInt32));
        m_pInvMap = ( MInt32 * ) MMemAlloc(m_hMemMgr, ( 256 * 9 + 1 ) * sizeof(MInt32));
        if( MNull == m_pMap || MNull == m_pInvMap )
        {
        }
        MakeDivTable(m_pInvMap, ( 256 * 9 + 1 ));

        if( fIntensity > 0 )
        {
            UpdateWeightMap(fIntensity);
        }
    }

    NLMeans::~NLMeans()
    {
        if( m_pMap )
        {
            MMemFree(m_hMemMgr, m_pMap);
            m_pMap = MNull;
        }
        if( m_pInvMap )
        {
            MMemFree(m_hMemMgr, m_pInvMap);
            m_pInvMap = MNull;
        }

        m_hMemMgr = MNull;
        m_mcvParallelMonitor = MNull;
    }


    /// @brief 计算1.0 / all_weight的值，并量化
    /// @param pTable   [out]
    /// @param lSize    [in]    权重的最大值, 默认为256x9+1, 搜索范围为3x3
    /// @return
    MVoid NLMeans::MakeDivTable(MInt32 *pTable, MInt32 lSize)
    {
        for(MInt32 i = 1; i < lSize; i++) // i表示权重
        {
            pTable[ i ] = ( 1 << 20 ) / i;
        }
        pTable[ 0 ] = pTable[ 1 ];
    }


    /// @brief 制作公式exp(-d/(h^2))的映射表
    /// @param pTable   [out]   存储权重值, 定点化到[0 255]
    /// @param fVar     [in]    滤波系数, 取值越小，曲线越陡
    /// @param lMaxNum  [in]    块匹配的距离和的最大值, 建议参数是4x4的块,每像素距离最大值为49
    /// @return
    MVoid NLMeans::MakeWeightMap(MInt32 *pTable, MFloat fVar, MInt32 lMaxNum)
    {
        //    printf("fVar = %f\n", fVar);
        // 块匹配大小n为4x4, 这里h^2的取值与匹配块的大小成正比
        // 讨论：两个块匹配, 距离最大为49 x 16，则
        // exp(-d/(h^2)) = -（49*16）*（49*16） / (fVar * 2 * 16 * 16) = -1200.5 / fVar
        // 当fVar = 35时, exp(-d/(h^2)) = 1.2697e-15
        MInt32 SumVar = fVar * 2 * 16 * 16;
        pTable[ 0 ] = 256; // TODO
        for(MInt32 x = 1; x < lMaxNum; x++)
        {
            MFloat lVal = x * x; // x表示块距离和距离值, 这里与论文不一样
            lVal = ( MInt32 ) ( 255.0 * exp(-lVal / SumVar) + 0.5f );
            pTable[ x ] = ( MByte ) lVal;
            //printf("%f = %d, %d\n", fVar, x, pTable[ x ]);
        }
    }


    /// @brief 更新权重系数
    /// @param fIntensity
    /// @return
    MVoid NLMeans::UpdateWeightMap(MFloat fIntensity)
    {
        NLMeans::MakeWeightMap(m_pMap, fIntensity, 16 * 50);
    }


    //#pragma mark - Process Bround Point

    /// @brief 处理边缘的当个像素点, 输出一个点, 搜索区域不满足3x3, 匹配区域不满足4x4
    /// @param pCurPoint    [in]
    /// @param pPrePoint    [in]
    /// @param pNexPoint    [in]
    /// @param pDstPoint    [out]
    /// @param pMap         [in]
    /// @param pInvMap      [in]
    /// @param l_add        [in]
    /// @return
    MVoid NLMeans::ProcessPointBround(MByte *pCurPoint,
                                      MByte *pPrePoint,
                                      MByte *pNexPoint,
                                      MByte *pDstPoint,
                                      MInt32 *pMap,
                                      MInt32 *pInvMap,
                                      MInt32 l_add)
    {
        MInt32 lNVal = 0;
        MInt32 lDif = 0, lTmpW = 0;
        MInt32 lInvW = 0, lDVal = 0;

        MInt32 nCurrentVal = pCurPoint[ 0 ];
        MInt32 nWeight = 256;
        MInt32 nAverage = nCurrentVal * 256;

        lNVal = pCurPoint[ l_add ];
        ADD_POINT_WEI(lNVal, nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

        lNVal = pPrePoint[ 0 ];
        ADD_POINT_WEI(lNVal, nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
        lNVal = pPrePoint[ l_add ];
        ADD_POINT_WEI(lNVal, nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

        lNVal = pNexPoint[ 0 ];
        ADD_POINT_WEI(lNVal, nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
        lNVal = pNexPoint[ l_add ];
        ADD_POINT_WEI(lNVal, nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

        lInvW = pInvMap[ nWeight ];
        lDVal = ( nAverage * lInvW + ( 1 << 19 )) >> 20;
        pDstPoint[ 0 ] = lDVal;
    }

    //#pragma mark - Process Point

    /// @brief 处理边缘的当个像素点, 输出一个点, 搜索区域满足3x3, 匹配区域不满足4x4
    /// @param pCurPoint    [in]
    /// @param pPrePoint    [in]
    /// @param pNexPoint    [in]
    /// @param pDstPoint    [out]
    /// @param pMap         [in]
    /// @param pInvMap
    /// @return
    MVoid NLMeans::ProcessPoint(MByte *pCurPoint,
                                MByte *pPrePoint,
                                MByte *pNexPoint,
                                MByte *pDstPoint,
                                MInt32 *pMap,
                                MInt32 *pInvMap)
    {
        MInt32 lDif = 0;
        MInt32 lTmpW = 0;

        MInt32 nCurrentVal = pCurPoint[ 0 ];
        MInt32 nWeight = 256;
        MInt32 nAverage = nCurrentVal * nWeight;

        ADD_POINT_WEI(pCurPoint[ -1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
        ADD_POINT_WEI(pCurPoint[ 1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

        ADD_POINT_WEI(pPrePoint[ -1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
        ADD_POINT_WEI(pPrePoint[ 0 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
        ADD_POINT_WEI(pPrePoint[ 1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

        ADD_POINT_WEI(pNexPoint[ -1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
        ADD_POINT_WEI(pNexPoint[ 0 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
        ADD_POINT_WEI(pNexPoint[ 1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

        MInt32 lInvW = pInvMap[ nWeight ];
        MInt32 lDVal = ( nAverage * lInvW + ( 1 << 19 )) >> 20;
        pDstPoint[ 0 ] = lDVal;
    }


    //#pragma mark - Process Block

    /// @brief 计算块匹配距离
    ///    4x4代替7x7, 为了加速！！！
    ///    距离计算方式为SAD, 为了加速！！！
    ///    用均值加权代替高斯加权, 为了加速！！！
    ///    距离超过49的值都设置为49, h 值设置需要注意, 值过大会导致模糊！！！
    /// @param pCurBlock    [in]
    /// @param pNeiBlock    [in]
    /// @param lPitch       [in]
    /// @return 匹配距离和
    MInt32 NLMeans::GetBlockDiff(MByte *pCurBlock, MByte *pNeiBlock, MInt32 lPitch)
    {
        MInt32 lDif = 0;
        MInt32 lPDif = 0;

        lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        pCurBlock += lPitch;
        pNeiBlock += lPitch;

        lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        pCurBlock += lPitch;
        pNeiBlock += lPitch;

        lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        pCurBlock += lPitch;
        pNeiBlock += lPitch;

        lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;
        lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
        lPDif = MIN(49, lPDif);
        lDif += lPDif;

        return lDif;
    }


    /// @brief 记录 average 和 sweight的值
    /// @param pNeiBlock    [in]
    /// @param lPitch       [in]
    /// @param lSumWei      [out]
    /// @param lW           [in]
    /// @return
    MVoid NLMeans::AddBlockSum(MByte *pNeiBlock, MInt32 lPitch, MInt32 *lSumWei, MInt32 lW)
    {
        lSumWei[ 0 ] += lW;

        lSumWei[ 1 ] += pNeiBlock[ 0 ] * lW;
        lSumWei[ 2 ] += pNeiBlock[ 1 ] * lW;
        lSumWei[ 3 ] += pNeiBlock[ 2 ] * lW;
        lSumWei[ 4 ] += pNeiBlock[ 3 ] * lW;
        pNeiBlock += lPitch;

        lSumWei[ 5 ] += pNeiBlock[ 0 ] * lW;
        lSumWei[ 6 ] += pNeiBlock[ 1 ] * lW;
        lSumWei[ 7 ] += pNeiBlock[ 2 ] * lW;
        lSumWei[ 8 ] += pNeiBlock[ 3 ] * lW;
        pNeiBlock += lPitch;

        lSumWei[ 9 ] += pNeiBlock[ 0 ] * lW;
        lSumWei[ 10 ] += pNeiBlock[ 1 ] * lW;
        lSumWei[ 11 ] += pNeiBlock[ 2 ] * lW;
        lSumWei[ 12 ] += pNeiBlock[ 3 ] * lW;
        pNeiBlock += lPitch;

        lSumWei[ 13 ] += pNeiBlock[ 0 ] * lW;
        lSumWei[ 14 ] += pNeiBlock[ 1 ] * lW;
        lSumWei[ 15 ] += pNeiBlock[ 2 ] * lW;
        lSumWei[ 16 ] += pNeiBlock[ 3 ] * lW;
    }


    /// @brief 记录领域 average 和 sweight的值
    /// @param pCurBlock    [in]
    /// @param pNeiBlock    [in]
    /// @param lPitch       [in]
    /// @param lSumWei      [out]
    /// @param pMap         [in]
    /// @return
    MVoid NLMeans::AddBlockSumByNei(MByte *pCurBlock,
                                    MByte *pNeiBlock,
                                    MInt32 lPitch,
                                    MInt32 *lSumWei,
                                    MInt32 *pMap,
                                    MInt32 difScale0)
    {
        MInt32 lBDif = GetBlockDiff(pCurBlock, pNeiBlock, lPitch); // 计算块匹配值d,
        difScale0 = difScale0 < 64 ? 64 : difScale0;
        difScale0 = (256 - difScale0)/3;
        lBDif = lBDif * (difScale0) >> 6;
        CLAMP(lBDif, 1, 799);
        MInt32 lW = pMap[lBDif]; // 获取权重, 查表法, exp(-d/(h^2))
        lW >>= 1;  // ？？？
        AddBlockSum(pNeiBlock, lPitch, lSumWei, lW);
    }


    /// @brief 计算 average / sweight, 同时处于4x4区域
    /// @param pDstBlock    [out]
    /// @param lPitch       [in]
    /// @param lSumWei      [in]
    /// @param pInvMap      [in]
    /// @return
    MVoid NLMeans::GetBlockResult(MByte *pDstBlock,
                                  MInt32 lPitch,
                                  MInt32 *lSumWei,
                                  MInt32 *pInvMap)
    {
        MInt32 lSW = lSumWei[ 0 ];
        MInt32 lInvW = pInvMap[ lSW ];
        lSumWei++;
#ifdef USE_NEON_NLM
        MInt32 resData[4];
        int32x4_t sumweidata, invwdata, consdata, tmpdata;
        consdata = vdupq_n_s32(1 << 19);
        invwdata = vdupq_n_s32(lInvW);

        sumweidata = vld1q_s32(lSumWei);
        tmpdata = vmulq_s32(sumweidata, invwdata);
        tmpdata = vaddq_s32(tmpdata, consdata);
        tmpdata = vshrq_n_s32(tmpdata, 20);
        vst1q_s32(resData, tmpdata);
        pDstBlock[0] = resData[0];
        pDstBlock[1] = resData[1];
        pDstBlock[2] = resData[2];
        pDstBlock[3] = resData[3];
        pDstBlock += lPitch;
        lSumWei += 4;

        sumweidata = vld1q_s32(lSumWei);
        tmpdata = vmulq_s32(sumweidata, invwdata);
        tmpdata = vaddq_s32(tmpdata, consdata);
        tmpdata = vshrq_n_s32(tmpdata, 20);
        vst1q_s32(resData,tmpdata);
        pDstBlock[0] = resData[0];
        pDstBlock[1] = resData[1];
        pDstBlock[2] = resData[2];
        pDstBlock[3] = resData[3];
        pDstBlock += lPitch;
        lSumWei += 4;

        sumweidata = vld1q_s32(lSumWei);
        tmpdata = vmulq_s32(sumweidata, invwdata);
        tmpdata = vaddq_s32(tmpdata, consdata);
        tmpdata = vshrq_n_s32(tmpdata, 20);
        vst1q_s32(resData,tmpdata);
        pDstBlock[0] = resData[0];
        pDstBlock[1] = resData[1];
        pDstBlock[2] = resData[2];
        pDstBlock[3] = resData[3];
        pDstBlock += lPitch;
        lSumWei += 4;

        sumweidata = vld1q_s32(lSumWei);
        tmpdata = vmulq_s32(sumweidata, invwdata);
        tmpdata = vaddq_s32(tmpdata, consdata);
        tmpdata = vshrq_n_s32(tmpdata, 20);
        vst1q_s32(resData,tmpdata);
        pDstBlock[0] = resData[0];
        pDstBlock[1] = resData[1];
        pDstBlock[2] = resData[2];
        pDstBlock[3] = resData[3];

#else

        pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock += lPitch;
        lSumWei += 4;

        pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock += lPitch;
        lSumWei += 4;

        pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock += lPitch;
        lSumWei += 4;

        pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
        pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
#endif
    }


    /// @brief 处理4x4个像素点, 输出4x4个点, 搜索区域满足3x3, 匹配区域满足4x4
    /// @param pCurLine
    /// @param pPreLine
    /// @param pNexLine
    /// @param pDstLine
    /// @param pMap
    /// @param lPitch
    /// @param lSumWei
    /// @param pInvMap
    /// @return
    MVoid NLMeans::ProcessBlock4x4(MByte *pCurLine,
                                   MByte *pPreLine,
                                   MByte *pNexLine,
                                   MByte *pDstLine,
                                   MInt32 *pMap,
                                   MInt32 lSrcPitch,
                                   MInt32 lDstPitch,
                                   MInt32 *lSumWei,
                                   MInt32 *pInvMap,
                                   MInt32 difScale0)
    {
        // 权重表清零
        MMemSet(&lSumWei[ 0 ], 0, 17 * sizeof(MInt32));

        // 当前块, 权重设置为最大值256
        AddBlockSum(pCurLine, lSrcPitch, lSumWei, 256);

        // 领域8个位置
        AddBlockSumByNei(pCurLine, pCurLine - 1, lSrcPitch, lSumWei, pMap, difScale0);
        AddBlockSumByNei(pCurLine, pCurLine + 1, lSrcPitch, lSumWei, pMap, difScale0);

        AddBlockSumByNei(pCurLine, pPreLine - 1, lSrcPitch, lSumWei, pMap, difScale0);
        AddBlockSumByNei(pCurLine, pPreLine, lSrcPitch, lSumWei, pMap, difScale0);
        AddBlockSumByNei(pCurLine, pPreLine + 1, lSrcPitch, lSumWei, pMap, difScale0);

        AddBlockSumByNei(pCurLine, pNexLine - 1, lSrcPitch, lSumWei, pMap, difScale0);
        AddBlockSumByNei(pCurLine, pNexLine, lSrcPitch, lSumWei, pMap, difScale0);
        AddBlockSumByNei(pCurLine, pNexLine + 1, lSrcPitch, lSumWei, pMap, difScale0);

        // average / sweight
        GetBlockResult(pDstLine, lDstPitch, lSumWei, pInvMap);
    }

    //#pragma mark - Process Block Neon


#ifdef  USE_NEON_NLM

    /// @brief 搜索区域3x3一次性处理完毕
    /// @param pCurLine
    /// @param pPreLine
    /// @param pNexLine
    /// @param lPitch
    /// @param pMap
    /// @param plW
    /// @param pSharedBuffer
    void NLMeans::GetBlockDiff4Neon(MByte *pCurLine,
                                    MByte *pPreLine,
                                    MByte *pNexLine,
                                    MInt32 lPitch,
                                    MInt32 *pMap,
                                    MInt32 *plW,
                                    MInt16 *pSharedBuffer)
    {
        //block_dif的优化
        MInt32 sharedBufferSize = 4 * 8 * sizeof(MInt16);
    //#ifdef USE_STD_LIB
    //    memset(pSharedBuffer , 0 , sharedBufferSize);
    //#else
        MMemSet(pSharedBuffer, 0, sharedBufferSize);
    //#endif
        uint8_t array_tbl[] = {1, 2, 3, 4, 0, 0, 0, 0};
        uint8x8_t V_tbl_mid = vld1_u8(array_tbl);

        uint8x8_t V_const_1 = vdup_n_u8(( uint8_t ) 1);
        int16x4_t V_const_49 = vdup_n_s16(( int16_t ) 49);
        int16x4_t V_diffSum16x4 = vdup_n_s16(( int16_t ) 0);    //累加和初始化为0
        uint8x8_t V_tbl_right = vadd_u8(V_tbl_mid, V_const_1);

        MByte *pCurLine_Left = pCurLine - 1;
        MByte *pPreLine_Left = pPreLine - 1;
        MByte *pNextLine_Left = pNexLine - 1;

        int16_t *pRoot = pSharedBuffer;    //这一块Buffer数据需要被初始化为0

        int row;
        for(row = 0; row < 4; row++)
        {
            int16_t *pStore = pRoot;

            //计算4 , 6
            uint8x8_t V_curLeft8x8 = vld1_u8(pCurLine_Left);
            uint8x8_t V_cur8x8 = vtbl1_u8(V_curLeft8x8, V_tbl_mid);
            uint8x8_t V_curRight8x8 = vtbl1_u8(V_curLeft8x8, V_tbl_right);
            int16x4_t V_curLeft16x4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_curLeft8x8)));
            int16x4_t V_cur16x4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_cur8x8)));
            int16x4_t V_curRight16x4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_curRight8x8)));

            int16x4_t V_diff16x4 = vmin_s16(vabs_s16(vsub_s16(V_cur16x4, V_curLeft16x4)), V_const_49);
            V_diffSum16x4 = vadd_s16(vld1_s16(pStore), V_diff16x4);
            vst1_s16(pStore, V_diffSum16x4);
            pStore += 4;    //一次保存4个数据，数据的间隔为8个单位(与周围8个block做差)
            V_diff16x4 = vmin_s16(vabs_s16(vsub_s16(V_cur16x4, V_curRight16x4)), V_const_49);
            V_diffSum16x4 = vadd_s16(vld1_s16(pStore), V_diff16x4);
            vst1_s16(pStore, V_diffSum16x4);
            pStore += 4;    //一次保存4个数据

            //计算1 , 2 , 3
            uint8x8_t V_preLeft8x8 = vld1_u8(pPreLine_Left);
            uint8x8_t V_pre8x8 = vtbl1_u8(V_preLeft8x8, V_tbl_mid);
            uint8x8_t V_preRight8x8 = vtbl1_u8(V_preLeft8x8, V_tbl_right);
            int16x4_t V_preLeft16x4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_preLeft8x8)));
            int16x4_t V_pre16x4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_pre8x8)));
            int16x4_t V_preRight16x4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_preRight8x8)));

            V_diff16x4 = vmin_s16(vabs_s16(vsub_s16(V_cur16x4, V_preLeft16x4)), V_const_49);
            V_diffSum16x4 = vadd_s16(vld1_s16(pStore), V_diff16x4);
            vst1_s16(pStore, V_diffSum16x4);
            pStore += 4;    //一次保存4个数据
            V_diff16x4 = vmin_s16(vabs_s16(vsub_s16(V_cur16x4, V_pre16x4)), V_const_49);
            V_diffSum16x4 = vadd_s16(vld1_s16(pStore), V_diff16x4);
            vst1_s16(pStore, V_diffSum16x4);
            pStore += 4;    //一次保存4个数据
            V_diff16x4 = vmin_s16(vabs_s16(vsub_s16(V_cur16x4, V_preRight16x4)), V_const_49);
            V_diffSum16x4 = vadd_s16(vld1_s16(pStore), V_diff16x4);
            vst1_s16(pStore, V_diffSum16x4);
            pStore += 4;    //一次保存4个数据

            //计算7 , 8 , 9
            uint8x8_t V_nextLeft8x8 = vld1_u8(pNextLine_Left);
            uint8x8_t V_next8x8 = vtbl1_u8(V_nextLeft8x8, V_tbl_mid);
            uint8x8_t V_nextRight8x8 = vtbl1_u8(V_nextLeft8x8, V_tbl_right);
            int16x4_t V_nextLeft16x4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_nextLeft8x8)));
            int16x4_t V_next16x4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_next8x8)));
            int16x4_t V_nextRight16x4 = vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_nextRight8x8)));

            V_diff16x4 = vmin_s16(vabs_s16(vsub_s16(V_cur16x4, V_nextLeft16x4)), V_const_49);
            V_diffSum16x4 = vadd_s16(vld1_s16(pStore), V_diff16x4);
            vst1_s16(pStore, V_diffSum16x4);
            pStore += 4;    //一次保存4个数据
            V_diff16x4 = vmin_s16(vabs_s16(vsub_s16(V_cur16x4, V_next16x4)), V_const_49);
            V_diffSum16x4 = vadd_s16(vld1_s16(pStore), V_diff16x4);
            vst1_s16(pStore, V_diffSum16x4);
            pStore += 4;    //一次保存4个数据
            V_diff16x4 = vmin_s16(vabs_s16(vsub_s16(V_cur16x4, V_nextRight16x4)), V_const_49);
            V_diffSum16x4 = vadd_s16(vld1_s16(pStore), V_diff16x4);
            vst1_s16(pStore, V_diffSum16x4);
            pStore += 4;    //一次保存4个数据

            pCurLine_Left += lPitch;
            pPreLine_Left += lPitch;
            pNextLine_Left += lPitch;
        }
        //已经输出8个周围区域的lBDif值
        //把4个值加在一起的值即为lBDif的值
        //vpaddl_s16 + vpaddl_s32
        for(int i = 0; i < 8; i++, pRoot += 4)
        {
            int64x1_t result = vpaddl_s32(vpaddl_s16(vld1_s16(pRoot)));
            int64_t lBdif = vget_lane_s64(result, 0);
            MInt32 lW = pMap[ lBdif ];
            lW = lW >> 1;

            plW[ i ] = lW;    //记录lW的值，顺序为4,6,1,2,3,7,8,9
        }
        return;
    }


    /// @brief
    /// @param pCurLine
    /// @param pPreLine
    /// @param pNexLine
    /// @param lPitch
    /// @param pMap
    /// @param plW
    /// @param pSharedBuffer
    void NLMeans::GetBlockDiff8Neon(MByte *pCurLine,
                                    MByte *pPreLine,
                                    MByte *pNexLine,
                                    MInt32 lPitch,
                                    MInt32 *pMap,
                                    MInt32 *plW,
                                    MInt16 *pSharedBuffer,
                                    MInt32 difScale0,
                                    MInt32 difScale1)
    {
        //block_dif的优化
        MInt32 sharedBufferSize = 8 * 8 * sizeof(MInt16);
    //#ifdef USE_STD_LIB
    //    memset(pSharedBuffer , 0 , sharedBufferSize);
    //#else
        MMemSet(pSharedBuffer, 0, sharedBufferSize);
    //#endif

        uint8x8_t V_const_1 = vdup_n_u8(( uint8_t ) 1);
        int16x8_t V_const_49 = vdupq_n_s16(( int16_t ) 49);
        int16x8_t V_diffSum16x8;

        int16_t *pRoot = pSharedBuffer;    //这一块Buffer数据需要被初始化为0

        int row;
        for(row = 0; row < 4; row++)    //4x4 , line 4
        {
            int16_t *pStore = pRoot;

            //计算4 , 6
            uint8x8_t V_curLeft8x8 = vld1_u8(pCurLine - 1);
            uint8x8_t V_cur8x8 = vld1_u8(pCurLine);
            uint8x8_t V_curRight8x8 = vld1_u8(pCurLine + 1);
            int16x8_t V_curLeft16x8 = vreinterpretq_s16_u16(vmovl_u8(V_curLeft8x8));
            int16x8_t V_cur16x8 = vreinterpretq_s16_u16(vmovl_u8(V_cur8x8));
            int16x8_t V_curRight16x8 = vreinterpretq_s16_u16(vmovl_u8(V_curRight8x8));

            int16x8_t V_diff16x8 = vminq_s16(vabsq_s16(vsubq_s16(V_cur16x8, V_curLeft16x8)), V_const_49);
            V_diffSum16x8 = vld1q_s16(pStore);
            V_diffSum16x8 = vaddq_s16(V_diffSum16x8, V_diff16x8);
            vst1q_s16(pStore, V_diffSum16x8);
            pStore += 8;    //一次保存8个(两个块并行数据做差)，数据的间隔为8个单位(与周围8个block做差)
            V_diff16x8 = vminq_s16(vabsq_s16(vsubq_s16(V_cur16x8, V_curRight16x8)), V_const_49);
            V_diffSum16x8 = vld1q_s16(pStore);
            V_diffSum16x8 = vaddq_s16(V_diffSum16x8, V_diff16x8);
            vst1q_s16(pStore, V_diffSum16x8);
            pStore += 8;

            //计算1  2  3
            uint8x8_t V_preLeft8x8 = vld1_u8(pPreLine - 1);
            uint8x8_t V_pre8x8 = vld1_u8(pPreLine);
            uint8x8_t V_preRight8x8 = vld1_u8(pPreLine + 1);
            int16x8_t V_preLeft16x8 = vreinterpretq_s16_u16(vmovl_u8(V_preLeft8x8));
            int16x8_t V_pre16x8 = vreinterpretq_s16_u16(vmovl_u8(V_pre8x8));
            int16x8_t V_preRight16x8 = vreinterpretq_s16_u16(vmovl_u8(V_preRight8x8));

            V_diff16x8 = vminq_s16(vabsq_s16(vsubq_s16(V_cur16x8, V_preLeft16x8)), V_const_49);
            V_diffSum16x8 = vld1q_s16(pStore);
            V_diffSum16x8 = vaddq_s16(V_diffSum16x8, V_diff16x8);
            vst1q_s16(pStore, V_diffSum16x8);
            pStore += 8;    //一次保存8个(两个块并行数据做差)，数据的间隔为8个单位(与周围8个block做差)
            V_diff16x8 = vminq_s16(vabsq_s16(vsubq_s16(V_cur16x8, V_pre16x8)), V_const_49);
            V_diffSum16x8 = vld1q_s16(pStore);
            V_diffSum16x8 = vaddq_s16(V_diffSum16x8, V_diff16x8);
            vst1q_s16(pStore, V_diffSum16x8);
            pStore += 8;
            V_diff16x8 = vminq_s16(vabsq_s16(vsubq_s16(V_cur16x8, V_preRight16x8)), V_const_49);
            V_diffSum16x8 = vld1q_s16(pStore);
            V_diffSum16x8 = vaddq_s16(V_diffSum16x8, V_diff16x8);
            vst1q_s16(pStore, V_diffSum16x8);
            pStore += 8;

            //计算7  8  9
            uint8x8_t V_nextLeft8x8 = vld1_u8(pNexLine - 1);
            uint8x8_t V_next8x8 = vld1_u8(pNexLine);
            uint8x8_t V_nextRight8x8 = vld1_u8(pNexLine + 1);
            int16x8_t V_nextLeft16x8 = vreinterpretq_s16_u16(vmovl_u8(V_nextLeft8x8));
            int16x8_t V_next16x8 = vreinterpretq_s16_u16(vmovl_u8(V_next8x8));
            int16x8_t V_nextRight16x8 = vreinterpretq_s16_u16(vmovl_u8(V_nextRight8x8));

            V_diff16x8 = vminq_s16(vabsq_s16(vsubq_s16(V_cur16x8, V_nextLeft16x8)), V_const_49);
            V_diffSum16x8 = vld1q_s16(pStore);
            V_diffSum16x8 = vaddq_s16(V_diffSum16x8, V_diff16x8);
            vst1q_s16(pStore, V_diffSum16x8);
            pStore += 8;    //一次保存8个(两个块并行数据做差)，数据的间隔为8个单位(与周围8个block做差)
            V_diff16x8 = vminq_s16(vabsq_s16(vsubq_s16(V_cur16x8, V_next16x8)), V_const_49);
            V_diffSum16x8 = vld1q_s16(pStore);
            V_diffSum16x8 = vaddq_s16(V_diffSum16x8, V_diff16x8);
            vst1q_s16(pStore, V_diffSum16x8);
            pStore += 8;
            V_diff16x8 = vminq_s16(vabsq_s16(vsubq_s16(V_cur16x8, V_nextRight16x8)), V_const_49);
            V_diffSum16x8 = vld1q_s16(pStore);
            V_diffSum16x8 = vaddq_s16(V_diffSum16x8, V_diff16x8);
            vst1q_s16(pStore, V_diffSum16x8);
            pStore += 8;

            pCurLine += lPitch;
            pPreLine += lPitch;
            pNexLine += lPitch;    //hope this can hit cache
        }
        //在这里可以对数组的组织形式重组
        //已经输出8个周围区域的lBDif值(并行计算2个block，因此是16个)
        //把4个值加在一起的值即为lBDif的值
        //vpaddl_s16 + vpaddl_s32
        for(int i = 0; i < 8; i++, pRoot += 8)
        {
            int16x8_t paraller_lW = vld1q_s16(pRoot);
            int64x2_t result = vpaddlq_s32(vpaddlq_s16(paraller_lW));
            int64_t F_lBdif = vgetq_lane_s64(result, 0);

            F_lBdif = F_lBdif * (difScale0) >> 6;
            CLAMP(F_lBdif, 1, 799);
            MInt32 lW = pMap[F_lBdif]; // 获取权重, 查表法, exp(-d/(h^2))

            lW = lW >> 1;
            plW[ i ] = lW;        //记录lW的值，顺序为4,6,1,2,3,7,8,9

            int64_t S_lBdif = vgetq_lane_s64(result, 1);
            S_lBdif = S_lBdif * (difScale1) >> 6;
            CLAMP(S_lBdif, 1, 799);
            lW = pMap[S_lBdif];
            lW = lW >> 1;
            plW[ 8 + i ] = lW;    //记录lW的值，顺序为4,6,1,2,3,7,8,9
        }
    }

    /// @brief
    /// @param pCurLine
    /// @param pPreLine
    /// @param pNexLine
    /// @param lPitch
    /// @param lSumWei
    /// @param plW
    void NLMeans::AddBlockSumByNei3x3Neon(MByte *pCurLine,
                                          MByte *pPreLine,
                                          MByte *pNexLine,
                                          MInt32 lPitch,
                                          MInt32 *lSumWei,
                                          MInt32 *plW)
    {
        uint8_t array_tbl[] = {1, 2, 3, 4, 0, 0, 0, 0};
        uint8x8_t V_tbl_mid = vld1_u8(array_tbl);

        uint8x8_t V_const_1 = vdup_n_u8(( uint8_t ) 1);
        uint8x8_t V_tbl_right = vadd_u8(V_tbl_mid, V_const_1);

        //首先读出8个区域的lw值到寄存器当中，以免重复加载
        int32_t lw_Sum = 0;

        MInt32 *lW = plW;
        int32x4_t V_lW = vld1q_s32(lW);
        int32_t curLeftlW = vgetq_lane_s32(V_lW, 0);
        lw_Sum += curLeftlW;
        int32_t curRightlW = vgetq_lane_s32(V_lW, 1);
        lw_Sum += curRightlW;
        int32_t preLeftlW = vgetq_lane_s32(V_lW, 2);
        lw_Sum += preLeftlW;
        int32_t prelW = vgetq_lane_s32(V_lW, 3);
        lw_Sum += prelW;
        lW += 4;

        V_lW = vld1q_s32(lW);
        int32_t preRightlW = vgetq_lane_s32(V_lW, 0);
        lw_Sum += preRightlW;
        int32_t nextLeftlW = vgetq_lane_s32(V_lW, 1);
        lw_Sum += nextLeftlW;
        int32_t nextlW = vgetq_lane_s32(V_lW, 2);
        lw_Sum += nextlW;
        int32_t nextRightlW = vgetq_lane_s32(V_lW, 3);
        lw_Sum += nextRightlW;
        //如果嵌套blockResult，可以考虑修改lSumWei的顺序
        lSumWei[ 0 ] += lw_Sum;

        //加载出lSumWei的值
        MInt32 *p_lSumWei = lSumWei + 1;
        MInt32 *pRoot = p_lSumWei;
        int row;    //以行计算的方式进行迭代
        for(row = 0; row < 4; row++, pRoot += 4)
        {
            int32x4_t V_sumWei = vld1q_s32(pRoot);

            //计算4 , 6
            uint8x8_t V_curLeft8x8 = vld1_u8(pCurLine - 1);
            uint8x8_t V_curRight8x8 = vld1_u8(pCurLine + 1);
            int32x4_t V_curLeft32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_curLeft8x8))));
            V_sumWei = vaddq_s32(V_sumWei, vmulq_n_s32(V_curLeft32x4, curLeftlW));
            int32x4_t V_curRight32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_curRight8x8))));
            V_sumWei = vaddq_s32(V_sumWei, vmulq_n_s32(V_curRight32x4, curRightlW));

            //计算1 , 2 , 3
            uint8x8_t V_preLeft8x8 = vld1_u8(pPreLine - 1);
            uint8x8_t V_pre8x8 = vld1_u8(pPreLine);
            uint8x8_t V_preRight8x8 = vld1_u8(pPreLine + 1);
            int32x4_t V_preLeft32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_preLeft8x8))));
            V_sumWei = vaddq_s32(V_sumWei, vmulq_n_s32(V_preLeft32x4, preLeftlW));
            int32x4_t V_pre32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_pre8x8))));
            V_sumWei = vaddq_s32(V_sumWei, vmulq_n_s32(V_pre32x4, prelW));
            int32x4_t V_preRight32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_preRight8x8))));
            V_sumWei = vaddq_s32(V_sumWei, vmulq_n_s32(V_preRight32x4, preRightlW));

            //计算7 , 8 , 9
            uint8x8_t V_nextLeft8x8 = vld1_u8(pNexLine - 1);
            uint8x8_t V_next8x8 = vld1_u8(pNexLine);
            uint8x8_t V_nextRight8x8 = vld1_u8(pNexLine + 1);
            int32x4_t V_nextLeft32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_nextLeft8x8))));
            V_sumWei = vaddq_s32(V_sumWei, vmulq_n_s32(V_nextLeft32x4, nextLeftlW));
            int32x4_t V_next32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_next8x8))));
            V_sumWei = vaddq_s32(V_sumWei, vmulq_n_s32(V_next32x4, nextlW));
            int32x4_t V_nextRight32x4 = vmovl_s16(vget_low_s16(vreinterpretq_s16_u16(vmovl_u8(V_nextRight8x8))));
            V_sumWei = vaddq_s32(V_sumWei, vmulq_n_s32(V_nextRight32x4, nextRightlW));

            //这个累加和已经可以直接计算dstLine了

            vst1q_s32(pRoot, V_sumWei);    //保存结果的数据

            pCurLine += lPitch;
            pPreLine += lPitch;
            pNexLine += lPitch;    //hope this can hit cache
        }
    }


    /// @brief
    /// @param pCurLine
    /// @param pPreLine
    /// @param pNexLine
    /// @param lSumWei
    /// @param lPitch
    /// @param pMap
    /// @param pSharedBuffer
    /// @param pDstLine
    /// @param pInvMap
    void NLMeans::ProcessBlock4x4Neon(MByte *pCurLine,
                                      MByte *pPreLine,
                                      MByte *pNexLine,
                                      MInt32 *lSumWei,
                                      MInt32 lSrcPitch, 
                                      MInt32 lDstPitch,
                                      MInt32 *pMap,
                                      MInt16 *pSharedBuffer,
                                      MByte *pDstLine,
                                      MInt32 *pInvMap)
    {
        memset(&lSumWei[ 0 ], 0, 17 * sizeof(MInt32));

        AddBlockSum(pCurLine, lSrcPitch, lSumWei, 256);

        MInt32 lW[8];    //一次只需要保存8个diff的值
        GetBlockDiff4Neon(pCurLine, pPreLine, pNexLine,
            lSrcPitch, pMap, lW, pSharedBuffer);

        AddBlockSumByNei3x3Neon(pCurLine, pPreLine, pNexLine, lSrcPitch, lSumWei, lW);

        GetBlockResult(pDstLine, lDstPitch, lSumWei, pInvMap);
    }

#endif

    //#pragma mark - main


    /// @brief 处理第一行或最后一行，搜索区域不满足3x3
    /// @param pCurLine
    /// @param pPreLine
    /// @param pDstLine
    /// @param lWidth
    /// @param pMap
    /// @param pInvMap
    /// @return
    MVoid NLMeans::ProcessLinesBroundMain(MByte *pCurLine,
                                          MByte *pPreLine,
                                          MByte *pDstLine,
                                          MInt32 lWidth,
                                          MInt32 *pMap,
                                          MInt32 *pInvMap)
    {
        MInt32 lWSum = 0;
        MInt32 lSumW = 0;
        MInt32 lCVal = 0, lNVal = 0;
        MInt32 lDVal = 0;
        MInt32 lTmpW = 0;
        MInt32 lInvW = 0;
        MInt32 lDif = 0;
        MInt32 x = 0;
        //left point
        lCVal = pCurLine[ 0 ];
        lWSum = 256;
        lSumW = 256 * lCVal;
        lNVal = pCurLine[ 1 ];

        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
        lNVal = pPreLine[ 0 ];
        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
        lNVal = pPreLine[ 1 ];
        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

        lInvW = pInvMap[ lWSum ];
        lDVal = ( lSumW * lInvW + ( 1 << 19 )) >> 20;
        //pDstLine[0] = (lDVal - lCVal >> 1) + 128;
        pDstLine[ 0 ] = lDVal;

        //med point
        for(x = 1; x < lWidth - 1; x++)
        {
            lCVal = pCurLine[ x ];
            lWSum = 256;
            lSumW = 256 * lCVal;

            lNVal = pCurLine[ x - 1 ];
            ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
            lNVal = pCurLine[ x + 1 ];
            ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

            lNVal = pPreLine[ x - 1 ];
            ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
            lNVal = pPreLine[ x ];
            ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
            lNVal = pPreLine[ x + 1 ];
            ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

            lInvW = pInvMap[ lWSum ];
            lDVal = ( lSumW * lInvW + +( 1 << 19 )) >> 20;
            pDstLine[ x ] = lDVal;
            //pDstLine[x] = (lDVal - lCVal >> 1) + 128;
        }

        //right point
        lCVal = pCurLine[ lWidth - 1 ];
        lWSum = 256;
        lSumW = 256 * lCVal;

        lNVal = pCurLine[ lWidth - 2 ];
        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
        lNVal = pPreLine[ lWidth - 2 ];
        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
        lNVal = pPreLine[ lWidth - 1 ];
        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

        lInvW = pInvMap[ lWSum ];
        lDVal = ( lSumW * lInvW + ( 1 << 19 )) >> 20;
        pDstLine[ lWidth - 1 ] = lDVal;
    }


    /// @brief 处理中间行，每次处理1行
    /// @param pCurLine
    /// @param pPreLine
    /// @param pNexLine
    /// @param pDstLine
    /// @param lWidth
    /// @param pMap
    /// @param pInvMap
    /// @return
    MVoid NLMeans::ProcessLines1Main(MByte *pCurLine,
                                     MByte *pPreLine,
                                     MByte *pNexLine,
                                     MByte *pDstLine,
                                     MInt32 lWidth,
                                     MInt32 *pMap,
                                     MInt32 *pInvMap)
    {
        //left point

        ProcessPointBround(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, 1);

        //med point
        pCurLine++;
        pPreLine++;
        pNexLine++;
        pDstLine++;
        for(MInt32 x = 1; x < lWidth - 1; x++)
        {
            ProcessPoint(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap);
            pCurLine++;
            pPreLine++;
            pNexLine++;
            pDstLine++;
        }

        //right point
        ProcessPointBround(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, -1);
    }


    /// @brief 处理中间行，每次处理4行
    /// @param pCurLine
    /// @param pPreLine
    /// @param pNexLine
    /// @param pDstLine
    /// @param lWidth
    /// @param lPitch
    /// @param pMap
    /// @param pInvMap
    /// @return
    MVoid NLMeans::ProcessLines4Main(MByte *pCurLine,
                                     MByte *pPreLine,
                                     MByte *pNexLine,
                                     MByte *pDstLine,
                                     MInt32 lWidth,
                                     MInt32 lPitch,
                                     MInt32 lDstPitch,
                                     MInt32 *pMap,
                                     MInt32 *pInvMap,
                                     MByte *pDnShadeLine,
                                     MInt32 shadeWidth)
    {
        MInt32 lSumWei[17] = {0};
        MInt32 x = 1;


        /// 最左边一个点
        {
#ifdef PROCESS_NLM_EDGE
            MInt32 lShift = 0;
            MInt32 lDstShift = 0;
            ProcessPointBround(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, 1);
            lShift += lPitch;
            lDstShift += lDstPitch;
            ProcessPointBround(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lDstShift, pMap, pInvMap, 1);
            lShift += lPitch;
            lDstShift += lDstPitch;
            ProcessPointBround(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lDstShift, pMap, pInvMap, 1);
            lShift += lPitch;
            lDstShift += lDstPitch;
            ProcessPointBround(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lDstShift, pMap, pInvMap, 1);
            lShift += lPitch;
            lDstShift += lDstPitch;
#endif
            pCurLine++;
            pPreLine++;
            pNexLine++;
            pDstLine++;
        }


        
#ifdef USE_NEON_NLM

        MInt32 sharedBufferSize = 8 * 8 * sizeof(MInt16);
        MInt16 *pSharedBuffer = ( MInt16 * ) MMemAlloc(MNull, sharedBufferSize);
        if( MNull == pSharedBuffer )
        {
            return;
        }

        // 一次计算的offset为8
        MInt32 lW[16] = {0};    //保存中间返回的数据(设制为缓存buffer)
        for(x = 1; x < lWidth - 12; x += 8)
        {
            MInt32 difScale0 = 64;
            MInt32 difScale1 = 64;
            if (pDnShadeLine)
            {
                MInt32 xshade = MIN(x >> 2, shadeWidth - 1);
                difScale0 = pDnShadeLine[xshade];

                xshade = MIN((x + 4) >> 2, shadeWidth - 1);
                difScale1 = pDnShadeLine[xshade];
            }


            if (difScale0 || difScale1)
            {
                MInt32 difScale00 = (difScale0 < 64 && difScale0 > 0) ? 64 : difScale0;
                difScale00 = difScale00 > 0 ? (256 - difScale00) / 3 : 0;
                MInt32 difScale11 = (difScale1 < 64 && difScale1 > 0) ? 64 : difScale1;
                difScale11 = difScale11 > 0 ? (256 - difScale11) / 3 : 0;

                memset(&lSumWei[0], 0, 17 * sizeof(MInt32));
                AddBlockSum(pCurLine, lPitch, lSumWei, 256);

                GetBlockDiff8Neon(pCurLine, pPreLine, pNexLine,
                    lPitch, pMap, lW, pSharedBuffer, difScale00, difScale11);
            }


            if (difScale0)
            {


                AddBlockSumByNei3x3Neon(pCurLine, pPreLine, pNexLine,
                                    lPitch, lSumWei, lW);

                GetBlockResult(pDstLine, lDstPitch, lSumWei, pInvMap);
            }



            pCurLine += 4;
            pPreLine += 4;
            pNexLine += 4;
            pDstLine += 4;


            //第二次迭代计算
            if (difScale1)
            {
                memset(&lSumWei[ 0 ], 0, 17 * sizeof(MInt32));
                AddBlockSum(pCurLine, lPitch, lSumWei, 256);

                AddBlockSumByNei3x3Neon(pCurLine, pPreLine, pNexLine,
                                    lPitch, lSumWei, lW + 8);
                GetBlockResult(pDstLine, lDstPitch, lSumWei, pInvMap);
            }

            pCurLine += 4;
            pPreLine += 4;
            pNexLine += 4;
            pDstLine += 4;
        }

#if 0
        //epilog, 一次计算一个diff(暂时不使用NEON的优化)
        for(; x < lWidth - 4; x += 4)
        {
            ProcessBlock4x4Neon(pCurLine,
                                pPreLine,
                                pNexLine,
                                lSumWei,
                                lPitch,
                                lDstPitch,
                                pMap,
                                pSharedBuffer,
                                pDstLine,
                                pInvMap);

            pCurLine += 4;
            pPreLine += 4;
            pNexLine += 4;
            pDstLine += 4;
        }
#endif

        if( pSharedBuffer )
        {
            MMemFree(MNull, pSharedBuffer);
        }
#else

        // 中间点，是4的倍数, 同时处理4x4
        for(; x < lWidth - 4; x += 4)// x = 1
        {
            MInt32 difScale0 = 64;
            MInt32 difScale1 = 64;
            if( pDnShadeLine )
            {
                MInt32 xshade = MIN(x >> 2, shadeWidth - 1);
                difScale0 = pDnShadeLine[ xshade ];
                xshade = MIN(x + 4 >> 2, shadeWidth - 1);
                difScale1 = pDnShadeLine[ xshade ];
            }

            if (difScale0 != 0) // todo: 先用判断语句来控制，对于0是否进入流程
            {
                ProcessBlock4x4(pCurLine,
                                pPreLine,
                                pNexLine,
                                pDstLine,
                                pMap,
                                lPitch,
                                lDstPitch,
                                lSumWei,
                                pInvMap,
                                difScale0);
            }


            pCurLine += 4;
            pPreLine += 4;
            pNexLine += 4;
            pDstLine += 4;
        }
#endif

#ifdef PROCESS_NLM_EDGE
        /// 中间点，不是4的倍数, 单像素处理
        for(; x < lWidth - 1; x++)
        {
            MInt32 lShift = 0;
            MInt32 lDstShift = 0;
            ProcessPoint(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap);
            lShift += lPitch;
            lDstShift += lDstPitch;
            ProcessPoint(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lDstShift, pMap, pInvMap);
            lShift += lPitch;
            lDstShift += lDstPitch;
            ProcessPoint(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lDstShift, pMap, pInvMap);
            lShift += lPitch;
            lDstShift += lDstPitch;
            ProcessPoint(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lDstShift, pMap, pInvMap);
            lShift += lPitch;
            lDstShift += lDstPitch;

            pCurLine++;
            pPreLine++;
            pNexLine++;
            pDstLine++;
        }


        /// 最右边一个点
        {
            MInt32 lShift = 0;
            MInt32 lDstShift = 0;
            ProcessPointBround(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, -1);
            lShift += lPitch;
            lDstShift += lDstPitch;
            ProcessPointBround(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lDstShift, pMap, pInvMap, -1);
            lShift += lPitch;
            lDstShift += lDstPitch;
            ProcessPointBround(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lDstShift, pMap, pInvMap, -1);
            lShift += lPitch;
            lDstShift += lDstPitch;
            ProcessPointBround(pCurLine + lShift, pPreLine + lShift, pNexLine + lShift, pDstLine + lDstShift, pMap, pInvMap, -1);
            lShift += lPitch;
            lDstShift += lDstPitch;
        }
#endif
    }


    /// @brief 处理输出值主入口
    /// @param pSrcImg      [out]
    /// @param pDstImg      [in]
    /// @param lTopLine     [in]
    /// @param lBotLine     [in]    分配多线程时, 除处理最后一行的线程外，在其他线程(lBotLine - lTopLine)必须是4的倍数
    /// @param fIntensity   [in]    大于0时, 重新设置去噪强度
    /// @return
    MInt32 NLMeans::Process(ImageInfo<MUInt8> *pSrcImg,
                            ImageInfo<MUInt8> *pDstImg,
                            ImageInfo<MUInt8> *pDnShade,
                            MInt32 lTopLine,
                            MInt32 lBotLine,
                            MFloat fIntensity /*= -1*/)
    {
#ifdef USE_NEON_NLM
        LOGD("USE_NEON_NLM!");
#endif

        if( m_pMap == MNull || m_pInvMap == MNull )
        {
            return MERR_INVALID_PARAM;
        }

        MInt32 *pMap = m_pMap;
        MInt32 *pInvMap = m_pInvMap;

        MInt32 lWidth = pSrcImg->lWidth;
        MInt32 lHeight = pSrcImg->lHeight;
        MInt32 lSrcPitch = pSrcImg->lStride;
        MInt32 lDstPitch = pDstImg->lStride;
        MByte *pSrcData = pSrcImg->pData;
        MByte *pDstData = pDstImg->pData;

        MByte *pCurLine;
        MByte *pPreLine;
        MByte *pNexLine;
        MByte *pDstLine;
        MInt16 lBlock_Bot = MIN(lBotLine, lHeight - 4);

        /// 更新权重值
        if( fIntensity > 0 )
        {
            UpdateWeightMap(fIntensity);
        }

        MInt32 y = lTopLine;


        /// 处理第一行
        if( 0 == lTopLine )
        {
#ifdef PROCESS_NLM_EDGE
            pCurLine = pSrcData;
            pNexLine = pSrcData + lSrcPitch;
            pDstLine = pDstData;
            ProcessLinesBroundMain(pCurLine, pNexLine, pDstLine, lWidth, pMap, pInvMap);
#endif
            y = 1;
        }


        /// 每次处理4行
        for(; y < lBlock_Bot; y += 4)
        {
            MByte *pDnShadeLine = MNull;
            MInt32 shadeWidth = 0;
            if( pDnShade && pDnShade->pData )
            {
                MInt32 shadeY = MIN(y >> 2, pDnShade->lHeight - 1);
                shadeWidth = pDnShade->lWidth;
                pDnShadeLine = pDnShade->pData + shadeY * pDnShade->lStride;
            }

            pCurLine = pSrcData + lSrcPitch * y;
            pPreLine = pCurLine - lSrcPitch;
            pNexLine = pCurLine + lSrcPitch;
            pDstLine = pDstData + lDstPitch * y;
            ProcessLines4Main(pCurLine, pPreLine, pNexLine, pDstLine, lWidth, lSrcPitch, lDstPitch, pMap, pInvMap, pDnShadeLine, shadeWidth);
        }

#ifdef PROCESS_NLM_EDGE
        /// 处理最后几行, 不满足4的倍数
        /// TODO 负载分配可以再优化一下
        if( lHeight == lBotLine )
        {
            for(; y < lHeight - 1; y++)
            {
                pCurLine = pSrcData + lSrcPitch * y;
                pPreLine = pCurLine - lSrcPitch;
                pNexLine = pCurLine + lSrcPitch;
                pDstLine = pDstData + lDstPitch * y;
                ProcessLines1Main(pCurLine, pPreLine, pNexLine, pDstLine, lWidth, pMap, pInvMap);
            }

            pCurLine = pSrcData + lSrcPitch * ( lHeight - 1 );
            pPreLine = pCurLine - lSrcPitch;
            pDstLine = pDstData + lDstPitch * ( lHeight - 1 );
            ProcessLinesBroundMain(pCurLine, pPreLine, pDstLine, lWidth, pMap, pInvMap);
        }
#endif

        return MOK;
    }

    typedef struct _tag_IMG_SG_NLM_ST
    {
        MVoid *obj;
        MInt32 task_ID;
        MHandle hMemMgr;
        MInt32 lRet;

        ImageInfo<MUInt8> *pSrcImg;
        ImageInfo<MUInt8> *pDstImg;
        ImageInfo<MUInt8> *pDnShade;

        MInt32 startRow;
        MInt32 endRow;

        MFloat fIntensity;
        MInt32 lTaskHeight;
        MInt32 lTotal_TaskNum;
    } IMG_SG_NLM_ST, *LpIMG_SG_NLM_ST;


    /// @brief 多线程主入口
    /// @param pSrcImg      [out]
    /// @param pDstImg      [in]
    /// @return
    MInt32 NLMeans::Run(ImageInfo<MUInt8> *pSrcImg,
                        ImageInfo<MUInt8> *pDstImg,
                        ImageInfo<MUInt8> *pDnShade,
                        MFloat fIntensity,
                        MInt32 nThreadCount)
    {
        START_TIME;

        MInt32 res = MOK;

        MInt32 lHeight = pSrcImg->lHeight;
        MBool isRunThreads = false;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        isRunThreads = ( lHeight > 64 ) && ( m_mcvParallelMonitor != MNull );
#endif

        if( isRunThreads )
        {
            /// 设置回调函数
            auto func_lamda = [](MVoid *HParam) -> MVoid
            {
                LpIMG_SG_NLM_ST SG_NLM_sturct = ( LpIMG_SG_NLM_ST ) HParam;
                ImageInfo<MUInt8> *pSrcImg = SG_NLM_sturct->pSrcImg;
                ImageInfo<MUInt8> *pDstImg = SG_NLM_sturct->pDstImg;
                ImageInfo<MUInt8> *pDnShade = SG_NLM_sturct->pDnShade;
                MFloat fIntensity = SG_NLM_sturct->fIntensity;
                MInt32 lret = MOK;

                NLMeans *obj = ( NLMeans * ) SG_NLM_sturct->obj;
                lret = obj->Process(pSrcImg,
                                    pDstImg,
                                    pDnShade,
                                    SG_NLM_sturct->startRow,
                                    SG_NLM_sturct->endRow,
                                    fIntensity);

                SG_NLM_sturct->lRet = lret;
            };
            MVoid (*func)(MVoid *) = func_lamda;



            /// 设置参数
            MInt32 lTaskNum = nThreadCount > 0 ? nThreadCount : lHeight > 16 ? 16 : 8;
            lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = ( lTaskHeight >> 2 ) << 2;

            IMG_SG_NLM_ST pParam[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].startRow = lTaskHeight * lnum + 1;
                pParam[ lnum ].endRow = lTaskHeight * ( lnum + 1 );
            }
            pParam[0].startRow = 0;
            pParam[ lTaskNum - 1 ].endRow = lHeight;


            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].obj = this;
                pParam[ lnum ].task_ID = lnum;
                pParam[ lnum ].hMemMgr = m_hMemMgr;

                pParam[ lnum ].pSrcImg = pSrcImg;
                pParam[ lnum ].pDstImg = pDstImg;
                pParam[ lnum ].pDnShade = pDnShade;

                pParam[ lnum ].fIntensity = fIntensity;
                pParam[ lnum ].lTaskHeight = lTaskHeight;
                pParam[ lnum ].lTotal_TaskNum = lTaskNum;
            }


            /// 创建线程
            MInt32 lTaskID[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                lTaskID[ lnum ] = mcvAddTask(m_mcvParallelMonitor, func, ( MVoid * ) &pParam[ lnum ]);
                if( lTaskID[ lnum ] < 0 )
                {
                    res = MERR_BAD_STATE;
                    goto exit;
                }
            }

            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                mcvWaitTask(m_mcvParallelMonitor, lTaskID[ lnum ]);
            }
        }
        else
        {
            res = Process(pSrcImg, pDstImg, pDnShade, 0, lHeight, fIntensity);
        }

        exit:
        END_TIME;
        return res;
    }


NS_SINFLE_IMAGE_ENHANCEMENT_END


