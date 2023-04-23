#pragma once

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "merror.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "amcomdef.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    class NLMeans2
    {
    public:
        //
        typedef struct _tag_NLM_PYRAMID_INFO
        {
            MByte *pData;
            MInt32 lWidth;
            MInt32 lHeight;
            MInt32 lStride;
        } PyInfo_t;

    public:
        NLMeans2(MHandle hMemMgr, MHandle mcvParallelMonitor);

        ~NLMeans2();

        MVoid UpdateWeightMap(MFloat fIntensity);

        /**
         * @brief 没有对边缘特殊处理，需要外部padding
         * @param pSrcImg
         * @param pDstImg
         * @param pDnShade
         * @param fIntensity
         * @param lRadius
         * @param nThreadCount
         * @return
         */
        MInt32 run_new(LPASVLOFFSCREEN pSrcImg,
            LPASVLOFFSCREEN pDstImg,
            LPASVLOFFSCREEN pDnShade,
            MFloat fIntensity,
            MInt32 lRadius,
            MInt32 nThreadCount = -1);

        /**
         * @brief 对边缘有特殊处理
         * @param pSrcImg
         * @param pDstImg
         * @param pDnShade
         * @param fIntensity
         * @param lRadius
         * @param nThreadCount
         * @return
         */
        MInt32 Run(LPASVLOFFSCREEN pSrcImg,
                   LPASVLOFFSCREEN pDstImg,
                   LPASVLOFFSCREEN pDnShade,
                   MFloat fIntensity,
                   MInt32 lRadius,
                   MInt32 nThreadCount = -1);

    private:
        MInt32 RunNoMask(LPASVLOFFSCREEN pSrcImg,
            LPASVLOFFSCREEN pDstImg,
            LPASVLOFFSCREEN pDnShade,
            MFloat fIntensity,
            MInt32 lRadius,
            MInt32 nThreadCount = -1);

        MInt32 Process3x3(LPASVLOFFSCREEN pSrcImg,
            LPASVLOFFSCREEN pDstImg,
            LPASVLOFFSCREEN pDnShade,
            MInt32 lTopLine,
            MInt32 lBotLine);

        MInt32 Process5x5(LPASVLOFFSCREEN pSrcImg,
            LPASVLOFFSCREEN pDstImg,
            LPASVLOFFSCREEN pDnShade,
            MInt32 lTopLine,
            MInt32 lBotLine);

        MInt32 Process7x7(LPASVLOFFSCREEN pSrcImg,
            LPASVLOFFSCREEN pDstImg,
            LPASVLOFFSCREEN pDnShade,
            MInt32 lTopLine,
            MInt32 lBotLine);

    private:
        /// @brief 处理输出值主入口, 必须先调用UpdateWeightMap
        /// @param pSrcImg      [out]
        /// @param pDstImg      [in]
        /// @param lTopLine     [in]
        /// @param lBotLine     [in]    分配多线程时, 除处理最后一行的线程外，在其他线程(lBotLine - lTopLine)必须是4的倍数
        /// @param fIntensity   [in]    大于0时, 重新设置去噪强度
        /// @return
        MInt32 Process(LPASVLOFFSCREEN pSrcImg,
            LPASVLOFFSCREEN pDstImg,
            LPASVLOFFSCREEN pDnShade,
            MInt32 lTopLine,
            MInt32 lBotLine,
            MFloat fIntensity = -1);

        MInt32 ProcessNoMask(LPASVLOFFSCREEN pSrcImg,
            LPASVLOFFSCREEN pDstImg,
            MInt32 lTopLine,
            MInt32 lBotLine,
            MFloat fIntensity = -1);

        /// @brief 制作公式exp(-d/(h^2))的映射表
        /// @param pTable   [out]   存储权重值, 定点化到[0 255]
        /// @param fVar     [in]    滤波系数, 取值越小，曲线越陡
        /// @param lMaxNum  [in]    块匹配的距离和的最大值, 建议参数是4x4的块,每像素距离最大值为49
        /// @return
        MVoid MakeWeightMap(MInt32 *pTable, MFloat fVar, MInt32 lMaxNum);

        /// @brief 计算1.0 / all_weight的值，并量化
        /// @param pTable   [out]
        /// @param lSize    [in]    权重的最大值, 默认为256x9+1, 搜索范围为3x3
        /// @return
        MVoid MakeDivTable(MInt32 *pTable, MInt32 lSize);


        /**********************************************************
        * Process Bround Point
        **********************************************************/
        /// @brief 处理边缘的当个像素点, 输出一个点, 搜索区域不满足3x3, 匹配区域不满足4x4
        /// @param pCurPoint    [in]
        /// @param pPrePoint    [in]
        /// @param pNexPoint    [in]
        /// @param pDstPoint    [out]
        /// @param pMap         [in]
        /// @param pInvMap      [in]
        /// @param l_add        [in]
        /// @return
        MVoid ProcessPointBround(MByte *pCurPoint,
                                 MByte *pPrePoint,
                                 MByte *pNexPoint,
                                 MByte *pDstPoint,
                                 MInt32 *pMap,
                                 MInt32 *pInvMap,
                                 MInt32 l_add);


        /**********************************************************
        * Process Point
        **********************************************************/
        /// @brief 处理边缘的当个像素点, 输出一个点, 搜索区域满足3x3, 匹配区域不满足4x4
        /// @param pCurPoint    [in]
        /// @param pPrePoint    [in]
        /// @param pNexPoint    [in]
        /// @param pDstPoint    [out]
        /// @param pMap         [in]
        /// @param pInvMap
        /// @return
        MVoid ProcessPoint(MByte *pCurPoint,
                           MByte *pPrePoint,
                           MByte *pNexPoint,
                           MByte *pDstPoint,
                           MInt32 *pMap,
                           MInt32 *pInvMap);


        /**********************************************************
        * Process Block
        **********************************************************/

        /// @brief 计算块匹配距离
        ///    4x4代替7x7, 为了加速！！！
        ///    距离计算方式为SAD, 为了加速！！！
        ///    用均值加权代替高斯加权, 为了加速！！！
        ///    距离超过49的值都设置为49, h 值设置需要注意, 值过大会导致模糊！！！
        /// @param pCurBlock    [in]
        /// @param pNeiBlock    [in]
        /// @param lPitch       [in]
        /// @return 匹配距离和
        MInt32 GetBlockDiff(MByte *pCurBlock, MByte *pNeiBlock, MInt32 lPitch);


        /// @brief 记录 average 和 sweight的值
        /// @param pNeiBlock    [in]
        /// @param lPitch       [in]
        /// @param lSumWei      [out]
        /// @param lW           [in]
        /// @return
        MVoid AddBlockSum(MByte *pNeiBlock, MInt32 lPitch, MInt32 *lSumWei, MInt32 lW);


        /// @brief 记录领域 average 和 sweight的值
        /// @param pCurBlock    [in]
        /// @param pNeiBlock    [in]
        /// @param lPitch       [in]
        /// @param lSumWei      [out]
        /// @param pMap         [in]
        /// @return
        MVoid AddBlockSumByNei(MByte *pCurBlock,
                               MByte *pNeiBlock,
                               MInt32 lPitch,
                               MInt32 *lSumWei,
                               MInt32 *pMap,
                               MInt32 difScale0);

        /// @brief 计算 average / sweight, 同时处于4x4区域
        /// @param pDstBlock    [out]
        /// @param lPitch       [in]
        /// @param lSumWei      [in]
        /// @param pInvMap      [in]
        /// @return
        MVoid GetBlockResult(MByte *pDstBlock,
                             MInt32 lPitch,
                             MInt32 *lSumWei,
                             MInt32 *pInvMap);


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
        MVoid ProcessBlock4x4(MByte *pCurLine,
                              MByte *pPreLine,
                              MByte *pNexLine,
                              MByte *pDstLine,
                              MInt32 *pMap,
                              MInt32 lPitch,
                              MInt32 lDstPitch,
                              MInt32 *lSumWei,
                              MInt32 *pInvMap,
                              MInt32 difScale0);


        /**********************************************************
        * Process Block Neon
        **********************************************************/

        /// @brief 搜索区域3x3一次性处理完毕
        /// @param pCurLine
        /// @param pPreLine
        /// @param pNexLine
        /// @param lPitch
        /// @param pMap
        /// @param plW
        /// @param pSharedBuffer
        void GetBlockDiff4Neon(MByte *pCurLine,
                               MByte *pPreLine,
                               MByte *pNexLine,
                               MInt32 lPitch,
                               MInt32 *pMap,
                               MInt32 *plW,
                               MInt16 *pSharedBuffer);


        void GetBlockDiff8Neon(MByte *pCurLine,
                               MByte *pPreLine,
                               MByte *pNexLine,
                               MInt32 lPitch,
                               MInt32 *pMap,
                               MInt32 *plW,
                               MInt16 *pSharedBuffer,
                               MInt32 difScale0,
                               MInt32 difScale1);

        void GetBlockDiff8Neon3x3(MByte* pCurLine,
            MByte* pPreLine,
            MByte* pNexLine,
            MByte* pDstLine,
            MInt32 lPitch,
            MInt32 lDstPitch,
            MInt32* pMap,
            MInt32* pInvMap,
            MInt32* plW,
            MInt32 difScale0,
            MInt32 difScale1);

        void GetBlockDiff8Neon5x5(MByte* pCurLine,
            MByte* pPreLine,
            MByte* pNexLine,
            MByte* pDstLine,
            MInt32 lPitch,
            MInt32 lDstPitch,
            MInt32* pMap,
            MInt32* pInvMap,
            MInt32* plW,
            MInt32 difScale0,
            MInt32 difScale1);

        void GetBlockDiff4Neon5x5(MByte* pCurLine,
            MByte* pPreLine,
            MByte* pNexLine,
            MByte* pDstLine,
            MInt32 lPitch,
            MInt32 lDstPitch,
            MInt32* pMap,
            MInt32* pInvMap,
            MInt32* plW,
            MInt32 difScale0,
            MInt32 difScale1);

        void GetBlockDiff8Neon7x7(MByte* pCurLine,
            MByte* pPreLine,
            MByte* pNexLine,
            MByte* pDstLine,
            MInt32 lPitch,
            MInt32 lDstPitch,
            MInt32* pMap,
            MInt32* pInvMap,
            MInt32* plW,
            MInt32 difScale0,
            MInt32 difScale1);


        void AddBlockSumByNei3x3Neon(MByte *pCurLine,
                                     MByte *pPreLine,
                                     MByte *pNexLine,
                                     MInt32 lPitch,
                                     MInt32 *lSumWei,
                                     MInt32 *plW);

    
        void ProcessBlock4x4Neon(MByte *pCurLine,
                                 MByte *pPreLine,
                                 MByte *pNexLine,
                                 MInt32 *lSumWei,
                                 MInt32 lPitch,
                                 MInt32 lDstPitch,
                                 MInt32 *pMap,
                                 MInt16 *pSharedBuffer,
                                 MByte *pDstLine,
                                 MInt32 *pInvMap);


        /**********************************************************
        * main
        **********************************************************/
        /// @brief 处理第一行或最后一行，搜索区域不满足3x3
        /// @param pCurLine
        /// @param pPreLine
        /// @param pDstLine
        /// @param lWidth
        /// @param pMap
        /// @param pInvMap
        /// @return
        MVoid ProcessLinesBroundMain(MByte *pCurLine,
                                     MByte *pPreLine,
                                     MByte *pDstLine,
                                     MInt32 lWidth,
                                     MInt32 *pMap,
                                     MInt32 *pInvMap);


        /// @brief 处理中间行，每次处理1行
        /// @param pCurLine
        /// @param pPreLine
        /// @param pNexLine
        /// @param pDstLine
        /// @param lWidth
        /// @param pMap
        /// @param pInvMap
        /// @return
        MVoid ProcessLines1Main(MByte *pCurLine,
                                MByte *pPreLine,
                                MByte *pNexLine,
                                MByte *pDstLine,
                                MInt32 lWidth,
                                MInt32 *pMap,
                                MInt32 *pInvMap);

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
        MVoid ProcessLines4Main(MByte *pCurLine,
                                MByte *pPreLine,
                                MByte *pNexLine,
                                MByte *pDstLine,
                                MInt32 lWidth,
                                MInt32 lPitch,
                                MInt32 lDstPitch,
                                MInt32 *pMap,
                                MInt32 *pInvMap,
                                MByte *pDnShadeLine,
                                MInt32 shadeWidth);

        MVoid ProcessLines4MainNew(MByte* pCurLine,
            MByte* pPreLine,
            MByte* pNexLine,
            MByte* pDstLine,
            MInt32 lWidth,
            MInt32 lPitch,
            MInt32 lDstPitch,
            MInt32* pMap,
            MInt32* pInvMap,
            MByte* pDnShadeLine,
            MInt32 shadeWidth);

        /**********************************************************
        * threads
        **********************************************************/


    private:
        MHandle m_hMemMgr = MNull;
        MHandle m_mcvParallelMonitor = MNull;
        MUInt16 m_pTaskNum[64] = { 0 };
        MInt32 m_lCallFunc = 0;
        MUInt16* m_pNum = MNull;

        ///  查表
        MInt32 *m_pMap = MNull;
        MInt32 *m_pInvMap = MNull;

    };


NS_SINFLE_IMAGE_ENHANCEMENT_END



