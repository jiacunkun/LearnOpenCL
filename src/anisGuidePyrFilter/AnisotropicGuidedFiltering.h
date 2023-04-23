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
@brief  基于引导图像的各向异性滤波，根据scale缩放引导图像进行加速

优化点:
 1、基于导向滤波加入方向的因素，即在计算导向滤波的 ab 系数时，分别计算出4个方向的 ab 系数，再做滤波；
 2、做缩放除了性能上的考虑，更重要的一点是滤掉高频信息，可以获取更大区域像素的方差，平坦区域a 系数降低，滤波效果更好；
边缘区域如果位置满足2的倍数，可以增强梯度，如果不满足，边缘会模糊，出现吃掉白边的现象；
 3、有吃掉孤立点，及边缘的效果，原因在于计算 mean A时，各个方向加了权重， a 的值越大，权重越小；
 
 
 不足点：
 1、会出现细微的条纹, 计算各个方向的方差导致a 系数外扩；
 2、计算 mean AB时，理想情况下噪点的各个方向的方差都比较大，边缘某一个方向的方差很小，
 以此f区分噪点和边缘，但是在实际中，数字计算还不能很好的区分开；
 
 
 特性：
 1、保边效果好于导向滤波
 2、计算 ab的半径固定为4，计算 mean AB的半径固定为2
 3、8方向比4方向效果相差不是很明显
 
 
@version: 1.0

@author:

@date:

@change:

@note:

@todo:
*******************************************************************************/
#ifndef _H_Anisotropic_Guided_Filtering_H_
#define _H_Anisotropic_Guided_Filtering_H_


#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"


NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    /* T=>src数据类型，T1=> guide 图像数据格式  */
    template<typename T = MUInt8, typename T0 = MUInt8>
    class AnisotropicGuidedFiltering
    {
    public:
        AnisotropicGuidedFiltering(MHandle hMemMgr,
                                   MHandle mcvParallelMonitor,
                                   MInt32 lDirection,
                                   MInt32 nThreadCount = 16,
                                   LPASVLOFFSCREEN pShadeMap = MNull, MInt16 nNoiseVar = 30);

        AnisotropicGuidedFiltering(MHandle hMemMgr,
                                   MHandle mcvParallelMonitor,
                                   MInt16 *pMeanA,
                                   MInt16 *pMeanB,
                                   MInt32 AB_Type, // AB是输入还是输出，正数表示输出
                                   MInt32 lDirection,
                                   MInt32 nThreadCount = 16,
                                   LPASVLOFFSCREEN pShadeMap = MNull,
                                   MInt16 nNoiseVar = 30);

        ~AnisotropicGuidedFiltering();

        /**
        * @brief
        * @param hMemMgr                [in]
        * @param mcvParallelMonitor     [in]
        * @param pSrc                   [in]    目前仅支持单通道
        * @param pGuided                [in]    支持与pSrc同一个地址
        * @param lWidth                 [in]
        * @param lHeight                [in]
        * @param lSrcPitch              [in]    扩展的像素个数，不是字节数
        * @param lGuildePitch           [in]    扩展的像素个数，不是字节数
        * @param pDst                   [out]   支持与pSrc同一个地址
        * @param lDstPitch              [in]
        * @param Feps                   [in]    <注意> Feps的取值范围跟 src 的范围相关，例如输入为 uin8的图像，Feps的值要乘以255^2;
        * @param lScale                 [in]
        * @return
        */
        MInt32 Run(MHandle hMemMgr,
                   MHandle mcvParallelMonitor,
                   T *pSrc,
                   T0 *pGuide,
                   MInt32 lWidth,
                   MInt32 lHeight,
                   MInt32 lSrcPitch,
                   MInt32 lGuildePitch,
                   T *pDst,
                   MInt32 lDstPitch,
                   MFloat Feps,
                   MInt32 lScale = 1);

        MInt32 Run(MHandle hMemMgr,
                   MHandle mcvParallelMonitor,
                   T *pSrc,
                   MInt32 lWidth,
                   MInt32 lHeight,
                   MInt32 lSrcPitch,
                   T *pDst,
                   MInt32 lDstPitch,
                   MFloat Feps,
                   MInt32 lScale = 1);

        MInt32 Run(MHandle hMemMgr,
                   MHandle mcvParallelMonitor,
                   T *pSrc,
                   MInt32 lWidth,
                   MInt32 lHeight,
                   MInt32 lSrcPitch,
                   MFloat Feps,
                   MInt32 lScale = 1);


        MVoid CopyTo(MInt16 *pMeanA, MInt16 *pMeanB, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch);

        template<typename T2>
        MInt32 Mul_A_Plus_B_Threads(T *pSrc,
                                    MInt32 lWidth,
                                    MInt32 lHeight,
                                    MInt32 lSrcPitch,
                                    T *pDst,
                                    MInt32 lDstPitch,
                                    T2 *pMeanA,
                                    T2 *pMeanB);

        template<typename T1, typename T2>
        MInt32 GetMeanAB(MHandle hMemMgr,
                         MHandle mcvParallelMonitor,
                         T *pSrc,
                         MInt32 lWidth,
                         MInt32 lHeight,
                         MInt32 lPitch,
                         T0 *pGuide,
                         MInt32 lGuidePitch,
                         T2 *pMeanA,
                         T2 *pMeanB,
                         MFloat Feps);

    private:

        template<typename T1>
        MInt32 GetABThreads(T *pSrc, MInt32 lPitchSrc, T1 **Coff_A, T1 **Coff_B, MInt32 lWidth, MInt32 lHeight, MFloat Feps);

        template<typename T1>
        MInt32 GetABThreads(T *pSrc, MInt32 lPitchSrc, T0 *pGuide, MInt32 lGuidePitch, T1 **Coff_A, T1 **Coff_B, MInt32 lWidth, MInt32 lHeight, MFloat Feps);




        /**
        * @brief 计算AB时，4/8个方向的偏移值
        * @param Shift
        * @param lPitch
        * @return
        */
        inline MVoid Compute_AB_Offset_Direction(MInt32 Shift[][4], MInt32 lPitch);


        /**
        * @brief 计算 meanAB时，4/8个方向的偏移值
        * @param pOffset
        * @param pitch
        * @param nRadius
        * @return
        */
        inline MVoid GetWeiOffset_Direction(MInt32 *pOffset, MInt32 pitch, MInt32 nRadius);


        /**
        * @brief            求AB系数
        * @tparam T1        输出AB系数的数据类型, T=>MUInt8时，T1=>MUInt16
        * @param pSrc       [in]
        * @param lPitchSrc  [in]
        * @param Coff_A     [out]   定点化为7bit
        * @param Coff_B     [out]   定点化为7bit
        * @param lWidth     [in]
        * @param lHeight    [in]
        * @param Feps       [in]    <注意> Feps的取值跟pSrc的取值范围相关
        * @return
        */
        template<typename T1>
        MInt32 GetAB(T *pSrc, MInt32 lPitchSrc, T1 **Coff_A, T1 **Coff_B, MInt32 lWidth, MInt32 lHeight, MFloat Feps,
                    MInt32 startRow, MInt32 endRow);



        template<typename T1>
        MInt32 GetAB(T *pSrc, MInt32 lPitchSrc,
                    T0 *pGuide, MInt32 lGuidePitch,
                    T1 **Coff_A, T1 **Coff_B,
                    MInt32 lWidth, MInt32 lHeight, MFloat Feps,
                    MInt32 startRow, MInt32 endRow);


        /**
        * @brief
        * @tparam T1        ab系数的数据类型
        * @tparam T2        mean ab 系数的数据类型
        * @param pA         [in]
        * @param pB         [in]
        * @param pMeanA     [out]
        * @param pMeanB     [out]
        * @param lWidth     [in]
        * @param lHeight    [in]
        * @return
        */
        template<typename T1, typename T2>
        MInt32 MeanAB(T1 **pA, T1 **pB, T2 *pMeanA, T2 *pMeanB,
                      MInt32 lWidth, MInt32 lHeight,
                      MInt32 startRow,
                      MInt32 endRow);



        template<typename T1, typename T2>
        MInt32 MeanABThreads(T1 **pA,
                             T1 **pB,
                             T2 *pMeanA,
                             T2 *pMeanB,
                             MInt32 lWidth,
                             MInt32 lHeight);

        /**
        * @brief
        * @tparam T1    mean ab 系数的数据类型
        * @param mcvParallelMonitor [in]
        * @param pSrc               [in]
        * @param lWidth             [in]
        * @param lHeight            [in]
        * @param lSrcPitch          [in]
        * @param pDst               [out]   支持与pSrc同一个地址
        * @param lDstPitch          [in]
        * @param pMeanA             [in]    T1=MUInt16时，有效位数7bit, 取值范围[0， 128]
        * @param pMeanB             [in]    T1=MUInt16时，有效位数为7+sizeof(T)
        * @return
        */


        template<typename T2>
        MInt32 Mul_A_Plus_B(
                T *pSrc,
                MInt32 lWidth,
                MInt32 lHeight,
                MInt32 lSrcPitch,
                T *pDst,
                MInt32 lDstPitch,
                T2 *pMeanA,
                T2 *pMeanB,
                MInt32 startRow,
                MInt32 endRow);




        template<typename T1>
        inline MVoid FillExpandPixels(T1 *pSrc,
                               MInt32 lWidth,
                               MInt32 lHeight,
                               MInt32 lPitch,
                               MInt32 lExpandSize);



    private:
        MInt32 m_lDirection;
        MInt32 m_lRadius = 4;
        MUInt8 *m_pShadeMapData = MNull;
        MInt32 m_lShadeMapPitch = 0;
		MHandle m_mcvParallelMonitor = MNull;
		MHandle m_hMemMgr = MNull;
		MInt32 m_nThreadCount;
		MFloat* m_pStrongEdgeMask = MNull;
		MInt32 m_lScale;
		MInt32 m_lWidth = 0;
		MInt32 m_lHeight = 0;
		MInt32 m_lContrast;
		MInt16 m_nEps;
		MInt16 m_nNoiseVar;

        MInt16 *m_pMeanA = MNull; // 传入原尺寸
        MInt16 *m_pMeanB = MNull;
        MInt32 m_AB_Type = 1; // 0表示用外部的输入，1表示内部生成输出
        MInt16 *m_pSmallMeanA = MNull; // 传入小图
        MInt16 *m_pSmallMeanB = MNull;
        MBool m_bIsExternalAB = MFalse;
        MInt16 *m_pMinA = MNull;
    };


    typedef AnisotropicGuidedFiltering<MUInt8> AnisotropicGuidedFilteringU8;
    typedef AnisotropicGuidedFiltering<MInt16> AnisotropicGuidedFilteringI16;
    typedef AnisotropicGuidedFiltering<MUInt16> AnisotropicGuidedFilteringU16;

NS_SINFLE_IMAGE_ENHANCEMENT_END

#include "AnisotropicGuidedFiltering.inl.h"

#endif
