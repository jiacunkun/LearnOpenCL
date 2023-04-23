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

@brief  NLMeans算法高度优化

主要优化方法如下:
1、搜索区域由21x21改为3x3, 可以结合金字塔处理
2、匹配区域由7x7改为4x4
3、由单像素处理改为同时处理4x4的块
4、匹配值计算去掉高斯分布, 由 L2改为 L1
5、neon, 多线程, 定点化

@version: 1.0

@author:

@date:

@change:

@note:

@todo:
*******************************************************************************/
#ifndef _H_NLMEANS_H_
#define _H_NLMEANS_H_

#include <stdio.h>
#include <math.h>
#include <string.h>
// mpbase
#include "merror.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "amcomdef.h"
#include "ImageInfo.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    class NLMeans
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
        NLMeans(MHandle hMemMgr, MHandle mcvParallelMonitor = MNull, MFloat fIntensity = 70.0);

        ~NLMeans();


        /// @brief 更新权重系数
        /// @param fIntensity
        /// @return
        MVoid UpdateWeightMap(MFloat fIntensity);


        /// @brief 多线程主入口
        /// @param pSrcImg      [out]
        /// @param pDstImg      [in]
        /// @param nThreadCount [in]    线程数, 没有传入的话,内部自动设置
        /// @return
        MInt32 Run(ImageInfo<MUInt8> *pSrcImg,
                   ImageInfo<MUInt8> *pDstImg,
                   ImageInfo<MUInt8> *pDnShade = MNull,
                   MFloat fIntensity = -1,
                   MInt32 nThreadCount = -1);

    private:

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

        /// @brief
        /// @param pCurLine
        /// @param pPreLine
        /// @param pNexLine
        /// @param lPitch
        /// @param pMap
        /// @param plW
        /// @param pSharedBuffer
        void GetBlockDiff8Neon(MByte *pCurLine,
                               MByte *pPreLine,
                               MByte *pNexLine,
                               MInt32 lPitch,
                               MInt32 *pMap,
                               MInt32 *plW,
                               MInt16 *pSharedBuffer,
                               MInt32 difScale0,
                               MInt32 difScale1);

        /// @brief
        /// @param pCurLine
        /// @param pPreLine
        /// @param pNexLine
        /// @param lPitch
        /// @param lSumWei
        /// @param plW
        void AddBlockSumByNei3x3Neon(MByte *pCurLine,
                                     MByte *pPreLine,
                                     MByte *pNexLine,
                                     MInt32 lPitch,
                                     MInt32 *lSumWei,
                                     MInt32 *plW);

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


        /// @brief 处理输出值主入口, 必须先调用UpdateWeightMap
        /// @param pSrcImg      [out]
        /// @param pDstImg      [in]
        /// @param lTopLine     [in]
        /// @param lBotLine     [in]    分配多线程时, 除处理最后一行的线程外，在其他线程(lBotLine - lTopLine)必须是4的倍数
        /// @param fIntensity   [in]    大于0时, 重新设置去噪强度
        /// @return
        MInt32 Process(ImageInfo<MUInt8> *pSrcImg,
                       ImageInfo<MUInt8> *pDstImg,
                       ImageInfo<MUInt8> *pDnShade,
                       MInt32 lTopLine,
                       MInt32 lBotLine,
                       MFloat fIntensity = -1);


        /**********************************************************
        * threads
        **********************************************************/


    private:
        MHandle m_hMemMgr = MNull;
        MHandle m_mcvParallelMonitor = MNull;

        ///  查表
        MInt32 *m_pMap = MNull;
        MInt32 *m_pInvMap = MNull;

    };


NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif

