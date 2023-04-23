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
#include "ammem.h"
#include "ArcsoftLog.h"
#include "imagebase.h"
#include "Up_Down_Scale_Mean2x2_4x4.h"
#include "DefineForDebug.h"
#include "Arcsoft_Copy_To_FilledImage.h"
#include "BasicTimer.h"
#include "CopyImageToImage.h"
#include <algorithm>
#include "Up_Down_Scale_Gaussian3x3.h"

#if defined(__ARM_NEON__) || defined(USE_NEON)
#define USE_NEON_GUIDED
#endif

#ifdef USE_NEON_GUIDED
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif

#define RADIUS 4 // 滤波半径
#define STEP 2 // mean ab时的均值半径
//#define USING_LWEI_TABLE // 对MeanAB中采用查表
//#define NEW_CALCU_VAR // 改变A计算方式
//#define AUTO_RADIUS // 若开启，滤波半径会根据图像尺寸改变；不开启，滤波半径固定为4
//#define DYNAMIC_DENOISE

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

static MInt32 lWei_Table[] = { 1033,1017,1001,985,970,954,939,924,909,894,879,864,849,835,820,806,792,778,764,750,737,723,710,697,684,671,658,645,632,620,607,595,583,571,559,547,536,524,513,502,491,480,469,458,447,437,426,416,406,396,386,376,367,357,348,339,330,321,312,303,294,286,277,269,261,253,245,237,230,222,215,208,201,194,187,180,173,167,160,154,148,142,136,130,125,119,114,109,104,99,94,89,84,80,75,71,67,63,59,55,52,48,45,42,39,36,33,30,27,25,22,20,18,16,14,12,11,9,8,7,6,5,4,3,2,2,1,1,1 };

    typedef struct _tag_Ani_Guided_Filter_ST
    {
        MVoid *obj;
        MInt32 task_ID;
        MHandle hMemMgr;
        MInt32 lRet;

        MInt32 startRow;
        MInt32 endRow;

        MInt32 lWidth;
        MInt32 lHeight;
        MInt32 lPitchSrc;
        MInt32 lPitchGuided;

        MFloat Feps;

        MVoid *p1;
        MVoid *p2;
        MVoid *p3;
        MVoid *p4;

    } Ani_Guided_Filter_ST, *LAni_Guided_Filter_ST;


    typedef struct _tag_Ani_Guided_Filter_ST_With_Guided
    {
        MVoid *obj;
        MInt32 task_ID;
        MHandle hMemMgr;
        MInt32 lRet;

        MInt32 startRow;
        MInt32 endRow;

        MInt32 lWidth;
        MInt32 lHeight;
        MInt32 lPitchSrc;
        MInt32 lPitchGuided;

        MFloat Feps;

        MVoid *p1;
        MVoid *p2;
        MVoid *p3;
        MVoid *p4;

    } Ani_Guided_Filter_ST_With_Guided, *LAni_Guided_Filter_ST_With_Guided;


    template<typename T, typename T0>
    AnisotropicGuidedFiltering<T, T0>::AnisotropicGuidedFiltering(MHandle hMemMgr,
                                                                  MHandle mcvParallelMonitor,
                                                                  MInt32 lDirection,
                                                                  MInt32 nThreadCount,
                                                                  LPASVLOFFSCREEN pShadeMap, MInt16 nNoiseVar)
    {
        m_lContrast = 0; // 控制最大层是否保留噪声
        if (lDirection > 8) {
            lDirection = 8;
            m_lContrast = 64;
        }
        else if (lDirection > 4 && lDirection != 8)
        {
            lDirection = 4;
            m_lContrast = 64;
        }

        m_lDirection = lDirection;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_hMemMgr = hMemMgr;
        m_nThreadCount = nThreadCount;
        LOGI("AnisotropicGuidedFiltering m_lDirection = %d", m_lDirection);
        LOGI("m_nThreadCount = %d", m_nThreadCount);
        if (pShadeMap != MNull)
        {
            m_lShadeMapPitch = pShadeMap->pi32Pitch[0];
            m_pShadeMapData = pShadeMap->ppu8Plane[0];
        }
        else
        {
            LOGD("pShadeMap == MNull1");
            m_pShadeMapData = (MUInt8 *) MMemAlloc(m_hMemMgr, sizeof(MUInt8) * 10240);
            MMemSet(m_pShadeMapData, 0, 10240);
            m_lShadeMapPitch = 0;
        }

        m_nNoiseVar = nNoiseVar;

        m_pMeanA = MNull;
        m_pMeanB = MNull;
    }

    template<typename T, typename T0>
    AnisotropicGuidedFiltering<T, T0>::AnisotropicGuidedFiltering(MHandle hMemMgr,
                                                                  MHandle mcvParallelMonitor,
                                                                  MInt16 *pMeanA,
                                                                  MInt16 *pMeanB,
                                                                  MInt32 AB_Type,
                                                                  MInt32 lDirection,
                                                                  MInt32 nThreadCount,
                                                                  LPASVLOFFSCREEN pShadeMap, MInt16 nNoiseVar)
    {
        m_lContrast = 0; // 控制最大层是否保留噪声
        if (lDirection > 8)
        {
            lDirection = 8;
            m_lContrast = 64;
        }
        else if (lDirection > 4 && lDirection != 8)
        {
            lDirection = 4;
            m_lContrast = 64;
        }

        m_lDirection = lDirection;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_hMemMgr = hMemMgr;
        m_nThreadCount = nThreadCount;
        LOGI("AnisotropicGuidedFiltering m_lDirection = %d", m_lDirection);
        LOGI("m_nThreadCount = %d", m_nThreadCount);
        if (pShadeMap != MNull)
        {
            m_lShadeMapPitch = pShadeMap->pi32Pitch[0];
            m_pShadeMapData = pShadeMap->ppu8Plane[0];
        }
        else
        {
            LOGD("pShadeMap == MNull2");
            m_pShadeMapData = (MUInt8 *) MMemAlloc(m_hMemMgr, sizeof(MUInt8) * 10240);
            MMemSet(m_pShadeMapData, 0, 10240);
            m_lShadeMapPitch = 0;
        }

        m_nNoiseVar = nNoiseVar;
        m_AB_Type = AB_Type;

        if (pMeanA != MNull && pMeanB != MNull)
        {
            LOGD("Use external AB");

            m_bIsExternalAB = MTrue;
            m_pMeanA = pMeanA;
            m_pMeanB = pMeanB;

        }
    }

    template<typename T, typename T0>
    AnisotropicGuidedFiltering<T, T0>::~AnisotropicGuidedFiltering()
    {
        if (m_pStrongEdgeMask)
        {
            MMemFree(m_hMemMgr, m_pStrongEdgeMask);
            m_pStrongEdgeMask = MNull;
        }
        if (m_lShadeMapPitch == 0 && m_pShadeMapData)
        {
            MMemFree(m_hMemMgr, m_pShadeMapData);
            m_pShadeMapData = MNull;
        }

        if (!m_bIsExternalAB)
        {
            SAFE_FREE_ARRAY(m_hMemMgr, m_pMeanA);
            SAFE_FREE_ARRAY(m_hMemMgr, m_pMeanB);
        }

        SAFE_FREE_ARRAY(m_hMemMgr, m_pMinA);
    }



    template<typename T, typename T0>
    MVoid AnisotropicGuidedFiltering<T, T0>::Compute_AB_Offset_Direction(MInt32 Shift[][4], MInt32 lPitch)
    {
        Shift[ 0 ][ 0 ] = 1; // hori
        Shift[ 0 ][ 1 ] = 2;
        Shift[ 0 ][ 2 ] = 3;
        Shift[ 0 ][ 3 ] = 4;

        Shift[ 1 ][ 0 ] = 1 + lPitch; // diag
        Shift[ 1 ][ 1 ] = 2 + lPitch * 2;
        Shift[ 1 ][ 2 ] = 3 + lPitch * 3;
        Shift[ 1 ][ 3 ] = 4 + lPitch * 4;

        Shift[ 2 ][ 0 ] = lPitch; // vect
        Shift[ 2 ][ 1 ] = lPitch * 2;
        Shift[ 2 ][ 2 ] = lPitch * 3;
        Shift[ 2 ][ 3 ] = lPitch * 4;

        Shift[ 3 ][ 0 ] = -1 + lPitch; // anti
        Shift[ 3 ][ 1 ] = -2 + lPitch * 2;
        Shift[ 3 ][ 2 ] = -3 + lPitch * 3;
        Shift[ 3 ][ 3 ] = -4 + lPitch * 4;

        Shift[ 4 ][ 0 ] = 1;
        Shift[ 4 ][ 1 ] = 2 + lPitch;
        Shift[ 4 ][ 2 ] = 3 + lPitch * 2;
        Shift[ 4 ][ 3 ] = 4 + lPitch * 2;
//        Shift[ 4 ][ 0 ] = 2 + lStride;
//        Shift[ 4 ][ 1 ] = 2 + lStride;
//        Shift[ 4 ][ 2 ] = 4 + lStride * 2;
//        Shift[ 4 ][ 3 ] = 4 + lStride * 2;

        Shift[ 5 ][ 0 ] = lPitch;
        Shift[ 5 ][ 1 ] = 1 + lPitch * 2;
        Shift[ 5 ][ 2 ] = 2 + lPitch * 3;
        Shift[ 5 ][ 3 ] = 2 + lPitch * 4;
//        Shift[ 5 ][ 0 ] = 1 + lStride * 2;
//        Shift[ 5 ][ 1 ] = 1 + lStride * 2;
//        Shift[ 5 ][ 2 ] = 2 + lStride * 4;
//        Shift[ 5 ][ 3 ] = 2 + lStride * 4;

        Shift[ 6 ][ 0 ] = lPitch;
        Shift[ 6 ][ 1 ] = -1 + lPitch * 2;
        Shift[ 6 ][ 2 ] = -2 + lPitch * 3;
        Shift[ 6 ][ 3 ] = -2 + lPitch * 4;
//        Shift[ 6 ][ 0 ] = -1 + lStride * 2;
//        Shift[ 6 ][ 1 ] = -1 + lStride * 2;
//        Shift[ 6 ][ 2 ] = -2 + lStride * 4;
//        Shift[ 6 ][ 3 ] = -2 + lStride * 4;

        Shift[ 7 ][ 0 ] = -1;
        Shift[ 7 ][ 1 ] = -2 + lPitch;
        Shift[ 7 ][ 2 ] = -3 + lPitch * 2;
        Shift[ 7 ][ 3 ] = -4 + lPitch * 2;
//        Shift[ 7 ][ 0 ] = -2 + lStride;
//        Shift[ 7 ][ 1 ] = -2 + lStride;
//        Shift[ 7 ][ 2 ] = -4 + lStride * 2;
//        Shift[ 7 ][ 3 ] = -4 + lStride * 2;
// 再加斜斜斜对角8个方向
        Shift[ 8 ][ 0 ] = 1;
        Shift[ 8 ][ 1 ] = 2;
        Shift[ 8 ][ 2 ] = 3 + lPitch;
        Shift[ 8 ][ 3 ] = 4 + lPitch;

        Shift[ 9 ][ 0 ] = 1 + lPitch;
        Shift[ 9 ][ 1 ] = 2 + lPitch * 2;
        Shift[ 9 ][ 2 ] = 3 + lPitch * 2;
        Shift[ 9 ][ 3 ] = 4 + lPitch * 3;

        Shift[ 10 ][ 0 ] = 1 + lPitch;
        Shift[ 10 ][ 1 ] = 2 + lPitch * 2;
        Shift[ 10 ][ 2 ] = 3 + lPitch * 3;
        Shift[ 10 ][ 3 ] = 3 + lPitch * 4;

        Shift[ 11 ][ 0 ] = lPitch;
        Shift[ 11 ][ 1 ] = lPitch * 2;
        Shift[ 11 ][ 2 ] = 1 + lPitch * 3;
        Shift[ 11 ][ 3 ] = 1 + lPitch * 4;

        Shift[ 12 ][ 0 ] = lPitch;
        Shift[ 12 ][ 1 ] = lPitch * 2;
        Shift[ 12 ][ 2 ] = -1 + lPitch * 3;
        Shift[ 12 ][ 3 ] = -1 + lPitch * 4;

        Shift[ 13 ][ 0 ] = lPitch;
        Shift[ 13 ][ 1 ] = -1 + lPitch * 2;
        Shift[ 13 ][ 2 ] = -2 + lPitch * 3;
        Shift[ 13 ][ 3 ] = -2 + lPitch * 4;

        Shift[ 14 ][ 0 ] = -1 + lPitch;
        Shift[ 14 ][ 1 ] = -2 + lPitch * 2;
        Shift[ 14 ][ 2 ] = -3 + lPitch * 2;
        Shift[ 14 ][ 3 ] = -4 + lPitch * 3;

        Shift[ 15 ][ 0 ] = -1;
        Shift[ 15 ][ 1 ] = -2;
        Shift[ 15 ][ 2 ] = -3 + lPitch;
        Shift[ 15 ][ 3 ] = -4 + lPitch;
    }


    template<typename T, typename T0>
    MVoid AnisotropicGuidedFiltering<T, T0>::GetWeiOffset_Direction(MInt32 *pOffset, MInt32 pitch, MInt32 nRadius)
    {
        pOffset[ 0 ] = -nRadius; //水平
        pOffset[ 1 ] = nRadius;

        pOffset[ 2 ] = -nRadius - pitch * nRadius; // 斜对角
        pOffset[ 3 ] = nRadius + pitch * nRadius;

        pOffset[ 4 ] = -pitch * nRadius; // 竖着
        pOffset[ 5 ] = pitch * nRadius;

        pOffset[ 6 ] = nRadius - pitch * nRadius; // 反斜对角
        pOffset[ 7 ] = -nRadius + pitch * nRadius;

        pOffset[ 8 ] = -nRadius - pitch * nRadius / 2;
        pOffset[ 9 ] = nRadius + pitch * nRadius / 2;

        pOffset[ 10 ] = -1 * nRadius / 2 - pitch * nRadius;
        pOffset[ 11 ] = 1 * nRadius / 2 + pitch * nRadius;

        pOffset[ 12 ] = 1 * nRadius / 2 - pitch * nRadius;
        pOffset[ 13 ] = -1 * nRadius / 2 + pitch * nRadius;

        pOffset[ 14 ] = nRadius - pitch * nRadius / 2;
        pOffset[ 15 ] = -nRadius + pitch * nRadius / 2;
#if 0// 再加8个方向
        pOffset[ 16 ] = -nRadius - nRadius * pitch / 4;
        pOffset[ 17 ] = nRadius + nRadius * pitch / 4;

        pOffset[ 18] = -nRadius - pitch * nRadius * 3 / 4;
        pOffset[ 19] = nRadius + pitch * nRadius * 3 / 4;

        pOffset[ 20 ] = -nRadius * 3 / 4 - pitch * nRadius;
        pOffset[ 21 ] = nRadius * 3 / 4 + pitch * nRadius;

        pOffset[ 22 ] = -nRadius * 1 / 4 - pitch * nRadius;
        pOffset[ 23 ] = nRadius * 1 / 4 + pitch * nRadius;

        pOffset[ 24] = nRadius * 1 / 4 - pitch * nRadius;
        pOffset[ 25] = -nRadius * 1 / 4 + pitch * nRadius;

        pOffset[ 26 ] = 1 * nRadius * 3 / 4 - pitch * nRadius;
        pOffset[ 27 ] = -1 * nRadius * 3 / 4 + pitch * nRadius;

        pOffset[ 28 ] = nRadius - pitch * nRadius * 3 / 4;
        pOffset[ 29 ] = -nRadius + pitch * nRadius * 3 / 4;

        pOffset[ 30 ] = nRadius - pitch * nRadius * 1 / 4;
        pOffset[ 31 ] = -nRadius + pitch * nRadius * 1 / 4;
#endif
    }

    /**
    * @brief            求AB系数,  每个方向半径固定为4
    * @tparam T1        输出AB系数的数据类型, T=>MUInt8时，T1=>MUInt16
    * @param pSrc       [in]
    * @param lPitchSrc  [in]
    * @param Coff_A     [out]   定点化为7bit
    * @param Coff_B     [out]   定点化为7bit
    * @param lWidth     [in]
    * @param lHeight    [in]
    * @param Feps       [in]
    * @return
    */
    template<typename T, typename T0>
    template<typename T1>
    MInt32 AnisotropicGuidedFiltering<T, T0>::GetAB(T *pSrc,
                                                    MInt32 lPitchSrc,
                                                    T1 **Coff_A,
                                                    T1 **Coff_B,
                                                    MInt32 lWidth,
                                                    MInt32 lHeight,
                                                    MFloat Feps,
                                                    MInt32 startRow /*= -1*/,
                                                    MInt32 endRow /*= -1*/)
    {
#ifdef USE_NEON_GUIDED
        LOGD("USE_NEON_GUIDED");
#endif
#if CALCULATE_TIME
        BasicTimer time;
#endif
        //LOGD("GetAB++");
        const MInt32 nRadius = RADIUS; // 这里半径固定为4
        MInt32 lKnlSize = m_lRadius * 2 + 1;
        //Feps = MAX(0.001, Feps);
        Feps = Feps * lKnlSize * lKnlSize;
        const MFloat FTempEps = Feps;

        MInt32 AB_Offset_Src[32][4] = {MNull};

        Compute_AB_Offset_Direction(AB_Offset_Src, lPitchSrc);

        MInt32 startRow2 = MAX(nRadius, startRow);
        MInt32 endRow2 = MIN(lHeight - nRadius, endRow);

        MFloat fDivFactor = 1.0 / lKnlSize;

        for (MInt32 y = startRow2; y < endRow2; y++)
        {
            T *pDataSrc = pSrc + y * lPitchSrc;
            MInt16 *pTempMinA = m_pMinA + y * lWidth;
            MInt32 x = nRadius;

#ifdef USE_NEON_GUIDED

            float32x4_t fRoundValue = vdupq_n_f32(0.49999997f);
            float32x4_t fRoundValue9 = vdupq_n_f32(fDivFactor);
            

            float32x4_t fRoundValue128 = vdupq_n_f32(128.0f);
            float32x4_t FTempEps_32x4 = vdupq_n_f32(FTempEps);

            //////////////////////////////////// MUInt8 ///////////////////////////////////////////////////
            if (sizeof(T) == 1)
            {
                //uint8x8_t lShade_8x8;

                uint8x8_t lCur_8x8;

                uint8x8_t lS0_8x8;
                uint8x8_t lS1_8x8;
                uint8x8_t lS2_8x8;
                uint8x8_t lS3_8x8;
                uint8x8_t lS4_8x8;
                uint8x8_t lS5_8x8;
                uint8x8_t lS6_8x8;
                uint8x8_t lS7_8x8;

                uint16x8_t src_sqrt0_16x8;
                uint16x8_t src_sqrt1_16x8;
                uint16x8_t src_sqrt2_16x8;
                uint16x8_t src_sqrt3_16x8;
                uint16x8_t src_sqrt4_16x8;
                uint16x8_t src_sqrt5_16x8;
                uint16x8_t src_sqrt6_16x8;
                uint16x8_t src_sqrt7_16x8;

                for (; x < lWidth - nRadius - 1; x += 8)
                {
                    // lode data

                    // shade mask
                    //lShade_8x8= vld1_u8(pTempShadeMapData + x);
                    //float32x4_t fEps_high_32x4 = vcvtq_f32_s32(vmovl_u16(vget_high_u16(vmovl_u8(lShade_8x8))));
                    //fEps_high_32x4 = vmulq_n_f32(vmulq_n_f32(fEps_high_32x4, FTempEps), 0.015625f);
                    //float32x4_t fEps_low_32x4 = vcvtq_f32_s32(vmovl_u16(vget_low_u16(vmovl_u8(lShade_8x8))));
                    //fEps_low_32x4 = vmulq_n_f32(vmulq_n_f32(fEps_low_32x4, FTempEps), 0.015625f);

                    lCur_8x8 = vld1_u8((MUInt8*)pDataSrc + x);

                    uint16x8_t tempMin_16x8 = vdupq_n_u16(128);

                    for (MInt32 i = 0; i < m_lDirection; i++)
                    {
                        MInt32 s_0 = AB_Offset_Src[i][0];
                        MInt32 s_1 = AB_Offset_Src[i][1];
                        MInt32 s_2 = AB_Offset_Src[i][2];
                        MInt32 s_3 = AB_Offset_Src[i][3];

                        lS0_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_0);
                        lS1_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_0);
                        lS2_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_1);
                        lS3_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_1);
                        lS4_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_2);
                        lS5_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_2);
                        lS6_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_3);
                        lS7_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_3);


                        // 和的平方
                        uint16x8_t sum_src_16x8 = vaddl_u8(lS0_8x8, lS1_8x8);

                        if (m_lRadius >= 2)
                        {
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS2_8x8);
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS3_8x8);
                        }

                        if (m_lRadius >= 3)
                        {
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS4_8x8);
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS5_8x8);
                        }

                        if (m_lRadius >= 4)
                        {
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS6_8x8);
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS7_8x8);
                        }

                        sum_src_16x8 = vaddw_u8(sum_src_16x8, lCur_8x8);


                        uint16x4_t sum_src_high_16x4 = vget_high_u16(sum_src_16x8);
                        uint16x4_t sum_src_low_16x4 = vget_low_u16(sum_src_16x8);
                        uint32x4_t sum_src_sqrt_high_32x4 = vmull_u16(sum_src_high_16x4, sum_src_high_16x4);
                        uint32x4_t sum_src_sqrt_low_32x4 = vmull_u16(sum_src_low_16x4, sum_src_low_16x4);
#ifdef NEW_CALCU_VAR
                        uint16x8_t diff_16x8;
                        diff_16x8 = vabdl_u8(lS0_8x8, lCur_8x8);
                        diff_16x8 = vabal_u8(diff_16x8, lS1_8x8, lCur_8x8);
                       
                        
                        if (m_lRadius >= 2)
                        {
                            diff_16x8 = vabal_u8(diff_16x8, lS2_8x8, lCur_8x8);
                            diff_16x8 = vabal_u8(diff_16x8, lS3_8x8, lCur_8x8);
                        }

                        if (m_lRadius >= 3)
                        {
                            diff_16x8 = vabal_u8(diff_16x8, lS4_8x8, lCur_8x8);
                            diff_16x8 = vabal_u8(diff_16x8, lS5_8x8, lCur_8x8);
                        }

                        if (m_lRadius >= 4)
                        {
                            diff_16x8 = vabal_u8(diff_16x8, lS6_8x8, lCur_8x8);
                            diff_16x8 = vabal_u8(diff_16x8, lS7_8x8, lCur_8x8);
                        }

                        uint32x4_t src_sqrt_sum_high_32x4 = vmull_u16(vget_high_u16(diff_16x8), vget_high_u16(diff_16x8));
                        uint32x4_t src_sqrt_sum_low_32x4 = vmull_u16(vget_low_u16(diff_16x8), vget_low_u16(diff_16x8));

#else
                        // 平方的和
                        src_sqrt0_16x8 = vmull_u8(lS0_8x8, lS0_8x8);
                        src_sqrt1_16x8 = vmull_u8(lS1_8x8, lS1_8x8);
                        src_sqrt2_16x8 = vmull_u8(lS2_8x8, lS2_8x8);
                        src_sqrt3_16x8 = vmull_u8(lS3_8x8, lS3_8x8);
                        src_sqrt4_16x8 = vmull_u8(lS4_8x8, lS4_8x8);
                        src_sqrt5_16x8 = vmull_u8(lS5_8x8, lS5_8x8);
                        src_sqrt6_16x8 = vmull_u8(lS6_8x8, lS6_8x8);
                        src_sqrt7_16x8 = vmull_u8(lS7_8x8, lS7_8x8);

                        uint16x8_t src_sqrt_16x8 = vmull_u8(lCur_8x8, lCur_8x8);


                        uint32x4_t src_sqrt_sum_high_32x4 = vaddl_u16(vget_high_u16(src_sqrt0_16x8), vget_high_u16(src_sqrt1_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt2_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt3_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt4_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt5_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt6_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt7_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt_16x8));
                        src_sqrt_sum_high_32x4 = vmulq_n_u32(src_sqrt_sum_high_32x4, lKnlSize);

                        uint32x4_t src_sqrt_sum_low_32x4 = vaddl_u16(vget_low_u16(src_sqrt0_16x8), vget_low_u16(src_sqrt1_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt2_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt3_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt4_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt5_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt6_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt7_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt_16x8));
                        src_sqrt_sum_low_32x4 = vmulq_n_u32(src_sqrt_sum_low_32x4, lKnlSize);

                        // 计算方差
                        src_sqrt_sum_high_32x4 = vsubq_u32(src_sqrt_sum_high_32x4, sum_src_sqrt_high_32x4);
                        src_sqrt_sum_low_32x4 = vsubq_u32(src_sqrt_sum_low_32x4, sum_src_sqrt_low_32x4);
#endif
                        float32x4_t fVar_high_32x4 = vcvtq_f32_u32(src_sqrt_sum_high_32x4);
                        float32x4_t fVar_low_32x4 = vcvtq_f32_u32(src_sqrt_sum_low_32x4);

                        // 计算AB


                        //float32x4_t fVarEps_high_32x4 = vaddq_f32(fVar_high_32x4, fEps_high_32x4);
                        float32x4_t fVarEps_high_32x4 = vaddq_f32(fVar_high_32x4, FTempEps_32x4);
                        float32x4_t fInvVar_high_32x4 = vrecpeq_f32(fVarEps_high_32x4); // 倒数
                        //fInvVar_high_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_high_32x4, fInvVar_high_32x4), fInvVar_high_32x4);
                        fInvVar_high_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_high_32x4, fInvVar_high_32x4), fInvVar_high_32x4); // 精确化
                        float32x4_t fCoefA_high_32x4 = vmulq_f32(fInvVar_high_32x4, vmulq_f32(fVar_high_32x4, fRoundValue128));
                        uint16x4_t lCoefA_high_16x4 = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fCoefA_high_32x4, fRoundValue)));


                        //float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, fEps_low_32x4);
                        float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, FTempEps_32x4);
                        float32x4_t fInvVar_low_32x4 = vrecpeq_f32(fVarEps_low_32x4);
                        //fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4), fInvVar_low_32x4);
                        fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4), fInvVar_low_32x4); // 精确化
                        float32x4_t fCoefA_low_32x4 = vmulq_f32(fInvVar_low_32x4, vmulq_f32(fVar_low_32x4, fRoundValue128));
                        uint16x4_t lCoefA_low_16x4 = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fCoefA_low_32x4, fRoundValue)));

                        tempMin_16x8 = vminq_u16(vcombine_u16(lCoefA_low_16x4, lCoefA_high_16x4), tempMin_16x8);

                        // store
                        // A
                        //lCoefA_high_16x4 = vdup_n_u16(0);
                        //lCoefA_low_16x4 = vdup_n_u16(0);
                        vst1_s16((Coff_A[i]+y * lWidth + x), vreinterpret_s16_u16(lCoefA_low_16x4));
                        vst1_s16((Coff_A[i]+y * lWidth + x + 4), vreinterpret_s16_u16(lCoefA_high_16x4));

                        // B
                        float32x4_t fMean_high_32x4 = vcvtq_f32_s32(vmovl_s16(vget_high_s16(sum_src_16x8)));
                        fMean_high_32x4 = vmulq_f32(fMean_high_32x4, fRoundValue9);
                        int16x4_t mean_src_16x4 = vmovn_s32(vcvtq_s32_f32(vaddq_f32(fMean_high_32x4, fRoundValue)));
                        vst1_s16((Coff_B[i]+y * lWidth + x + 4), (mean_src_16x4));

                        float32x4_t fMean_low_32x4 = vcvtq_f32_s32(vmovl_s16(vget_low_s16(sum_src_16x8)));
                        fMean_low_32x4 = vmulq_f32(fMean_low_32x4, fRoundValue9);
                        mean_src_16x4 = vmovn_s32(vcvtq_s32_f32(vaddq_f32(fMean_low_32x4, fRoundValue)));
                        vst1_s16((Coff_B[i]+y * lWidth + x), (mean_src_16x4));
                    }

                    vst1q_s16((pTempMinA + x), vreinterpretq_s16_u16(tempMin_16x8));

                }
            }

                //////////////////////////////////// MInt16 ///////////////////////////////////////////////////
            else if (sizeof(T) == 2)
            {
                uint8x8_t lShade_8x8;

                int16x4_t lCur_16x4;

                int16x4_t lS0_16x4;
                int16x4_t lS1_16x4;
                int16x4_t lS2_16x4;
                int16x4_t lS3_16x4;
                int16x4_t lS4_16x4;
                int16x4_t lS5_16x4;
                int16x4_t lS6_16x4;
                int16x4_t lS7_16x4;

                uint32x4_t src_sqrt0_32x4;
                uint32x4_t src_sqrt1_32x4;
                uint32x4_t src_sqrt2_32x4;
                uint32x4_t src_sqrt3_32x4;
                uint32x4_t src_sqrt4_32x4;
                uint32x4_t src_sqrt5_32x4;
                uint32x4_t src_sqrt6_32x4;
                uint32x4_t src_sqrt7_32x4;

                for (; x < lWidth - nRadius - 4; x += 4)
                {
                    // lode data

                    // shade mask
                    //lShade_8x8= vld1_u8(pTempShadeMapData + x);
                    //float32x4_t fEps_low_32x4 = vcvtq_f32_s32(vmovl_u16(vget_low_u16(vmovl_u8(lShade_8x8))));
                    //fEps_low_32x4 = vmulq_n_f32(vmulq_n_f32(fEps_low_32x4, FTempEps), 0.015625f);


                    lCur_16x4 = vld1_s16((MInt16*)pDataSrc + x);

                    int16x4_t tempMin_16x4 = vdup_n_s16(128);

                    for (MInt32 i = 0; i < m_lDirection; i++)
                    {
                        MInt32 s_0 = AB_Offset_Src[i][0];
                        MInt32 s_1 = AB_Offset_Src[i][1];
                        MInt32 s_2 = AB_Offset_Src[i][2];
                        MInt32 s_3 = AB_Offset_Src[i][3];

                        lS0_16x4 = vld1_s16((MInt16*)pDataSrc + x + s_0);
                        lS1_16x4 = vld1_s16((MInt16*)pDataSrc + x - s_0);
                        lS2_16x4 = vld1_s16((MInt16*)pDataSrc + x + s_1);
                        lS3_16x4 = vld1_s16((MInt16*)pDataSrc + x - s_1);
                        lS4_16x4 = vld1_s16((MInt16*)pDataSrc + x + s_2);
                        lS5_16x4 = vld1_s16((MInt16*)pDataSrc + x - s_2);
                        lS6_16x4 = vld1_s16((MInt16*)pDataSrc + x + s_3);
                        lS7_16x4 = vld1_s16((MInt16*)pDataSrc + x - s_3);


                        // 和的平方
                        int16x4_t sum_src_16x4 = vadd_s16(lS0_16x4, lS1_16x4);

                        if (m_lRadius >= 2)
                        {
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS2_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS3_16x4);
                        }

                        if (m_lRadius >= 3)
                        {
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS4_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS5_16x4);
                        }

                        if (m_lRadius >= 4)
                        {
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS6_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS7_16x4);
                        }

                        sum_src_16x4 = vadd_s16(sum_src_16x4, lCur_16x4);
#ifdef NEW_CALCU_VAR
                        uint16x4_t diff_16x4;
                        diff_16x4 = vabd_s16(lS0_16x4, lCur_16x4);
                        diff_16x4 = vaba_s16(diff_16x4, lS1_16x4, lCur_16x4);


                        if (m_lRadius >= 2)
                        {
                            diff_16x4 = vaba_s16(diff_16x4, lS2_16x4, lCur_16x4);
                            diff_16x4 = vaba_s16(diff_16x4, lS3_16x4, lCur_16x4);
                        }

                        if (m_lRadius >= 3)
                        {
                            diff_16x4 = vaba_s16(diff_16x4, lS4_16x4, lCur_16x4);
                            diff_16x4 = vaba_s16(diff_16x4, lS5_16x4, lCur_16x4);
                        }

                        if (m_lRadius >= 4)
                        {
                            diff_16x4 = vaba_s16(diff_16x4, lS6_16x4, lCur_16x4);
                            diff_16x4 = vaba_s16(diff_16x4, lS7_16x4, lCur_16x4);
                        }

                        uint32x4_t src_sqrt_sum_low_32x4 = vmull_u16(diff_16x4, diff_16x4);

#else
                        uint32x4_t sum_src_sqrt_low_32x4 = vmull_s16(sum_src_16x4, sum_src_16x4);

                        // 平方的和
                        src_sqrt0_32x4 = vmull_s16(lS0_16x4, lS0_16x4);
                        src_sqrt1_32x4 = vmull_s16(lS1_16x4, lS1_16x4);
                        src_sqrt2_32x4 = vmull_s16(lS2_16x4, lS2_16x4);
                        src_sqrt3_32x4 = vmull_s16(lS3_16x4, lS3_16x4);
                        src_sqrt4_32x4 = vmull_s16(lS4_16x4, lS4_16x4);
                        src_sqrt5_32x4 = vmull_s16(lS5_16x4, lS5_16x4);
                        src_sqrt6_32x4 = vmull_s16(lS6_16x4, lS6_16x4);
                        src_sqrt7_32x4 = vmull_s16(lS7_16x4, lS7_16x4);

                        uint32x4_t src_sqrt_32x4 = vmull_s16(lCur_16x4, lCur_16x4);


                        uint32x4_t src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt0_32x4, src_sqrt1_32x4);
                        src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt2_32x4));
                        src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt3_32x4));
                        src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt4_32x4));
                        src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt5_32x4));
                        src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt6_32x4));
                        src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt7_32x4));
                        src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt_32x4));
                        src_sqrt_sum_low_32x4 = vmulq_n_u32(src_sqrt_sum_low_32x4, lKnlSize);

                        // 计算方差
                        src_sqrt_sum_low_32x4 = vsubq_u32(src_sqrt_sum_low_32x4, sum_src_sqrt_low_32x4);
#endif
                        float32x4_t fVar_low_32x4 = vcvtq_f32_u32(src_sqrt_sum_low_32x4);

                        // 计算AB

                        //float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, fEps_low_32x4);
                        float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, FTempEps_32x4);
                        float32x4_t fInvVar_low_32x4 = vrecpeq_f32(fVarEps_low_32x4);
                        fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4), fInvVar_low_32x4);
                        fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4), fInvVar_low_32x4); // 精确化
                        float32x4_t fCoefA_low_32x4 = vmulq_f32(fInvVar_low_32x4, vmulq_f32(fVar_low_32x4, fRoundValue128));
                        uint16x4_t lCoefA_low_16x4 = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fCoefA_low_32x4, fRoundValue)));

                        tempMin_16x4 = vmin_s16((lCoefA_low_16x4), tempMin_16x4);

                        // store
                        // A
                        //lCoefA_high_16x4 = vdup_n_u16(0);
                        //lCoefA_low_16x4 = vdup_n_u16(0);
                        vst1_s16((Coff_A[i]+y * lWidth + x), vreinterpret_s16_u16(lCoefA_low_16x4));

                        // B
                        float32x4_t fMean_low_32x4 = vcvtq_f32_u32(vmovl_u16((sum_src_16x4)));
                        fMean_low_32x4 = vmulq_f32(fMean_low_32x4, fRoundValue9);
                        uint16x4_t mean_src_16x4 = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fMean_low_32x4, fRoundValue)));
                        vst1_s16((Coff_B[i]+y * lWidth + x), vreinterpret_s16_u16(mean_src_16x4));
                    }
                    vst1_s16((pTempMinA + x), (tempMin_16x4));
                }
            }
#endif
            for (; x < lWidth - nRadius; x++)
            {
                //Feps = FTempEps * ((pTempShadeMapData[x]) / 64.0);

                T lCur = pDataSrc[x];

                // src
                MInt32 src_sum = 0;
                MInt32 src_sum_sqr = 0;
                MInt32 lDiff = 0;
                MInt16 nTempMin = 128;
                MInt32 s[4] = { 0 };
                T a0[4] = { 0 };
                T a1[4] = { 0 };

                for (MInt32 i = 0; i < m_lDirection; i++)
                {
                    src_sum = lCur;
                    src_sum_sqr = lCur * lCur;
                    lDiff = 0;
                    for (MInt32 j = 0; j < m_lRadius; j++)
                    {
                        s[j] = AB_Offset_Src[i][j];
                        a0[j] = pDataSrc[x + s[j]];
                        a1[j] = pDataSrc[x - s[j]];
                        src_sum += (a0[j] + a1[j]);
                        lDiff += (ABS(a0[j] - lCur) + ABS(a1[j] - lCur));
                        src_sum_sqr += (a0[j] * a0[j] + a1[j] * a1[j]);
                    }

#ifdef NEW_CALCU_VAR
                    MInt32 lGVar = lDiff * lDiff;
#else
                    // a = varI / (varI + eps)
                    // b = mean_p * (1 - a)
                    // original: a = CovIP / (varI + eps)
                    // original: b = mean_p - a * mean_I
                    MInt32 lGVar = src_sum_sqr * lKnlSize - src_sum * src_sum;
#endif

                    MInt32 fCof_A = 128.0 * lGVar / (lGVar + FTempEps + 0.001) + 0.5; // 0~1.0

                    src_sum = (src_sum + lKnlSize / 2) / lKnlSize;
                    MInt32 fCof_B = src_sum; // * ( 1 - fCof_A ); // 将1-a放到最后去做

                    nTempMin = MIN(fCof_A, nTempMin);

                    Coff_A[i][y * lWidth + x] = fCof_A; // 0~128
                    Coff_B[i][y * lWidth + x] = fCof_B; // 原图归一化，不再进行定点化

#if 0 // TODO：传入图像本身有填充，内部暂时关闭填充
                    for (MInt32 x = 0; x < nRadius; x++)
                    {
                        Coff_A[i][y * lWidth + x] = Coff_A[i][y * lWidth + nRadius];
                        Coff_B[i][y * lWidth + x] = Coff_B[i][y * lWidth + nRadius];
                    }

                    for (MInt32 x = lWidth - nRadius; x < lWidth; x++)
                    {
                        Coff_A[i][y * lWidth + x] = Coff_A[i][y * lWidth + lWidth - nRadius - 1];
                        Coff_B[i][y * lWidth + x] = Coff_B[i][y * lWidth + lWidth - nRadius - 1];
                    }
#endif
                }

                pTempMinA[x] = nTempMin;

            }
        }

#if 0 // TODO：传入图像本身有填充，内部暂时关闭填充
        if( startRow2 == nRadius )
        {
            for(MInt32 i = 0; i < m_lDirection; i++)
            {
                for(MInt32 y = 0; y < nRadius; y++)
                {
                    MMemCpy(Coff_A[ i ] + y * lWidth, Coff_A[ i ] + nRadius * lWidth, lWidth * sizeof(T1));
                    MMemCpy(Coff_B[ i ] + y * lWidth, Coff_B[ i ] + nRadius * lWidth, lWidth * sizeof(T1));
                }
            }
        }

        if( endRow2 == lHeight - nRadius )
        {
            for(MInt32 i = 0; i < m_lDirection; i++)
            {
                for(MInt32 y = lHeight - nRadius; y < lHeight; y++)
                {
                    MMemCpy(Coff_A[ i ] + y * lWidth, Coff_A[ i ] + ( lHeight - nRadius - 1 ) * lWidth, lWidth * sizeof(T1));
                    MMemCpy(Coff_B[ i ] + y * lWidth, Coff_B[ i ] + ( lHeight - nRadius - 1 ) * lWidth, lWidth * sizeof(T1));
                }
            }
        }
#endif

        //LOGD("GetAB--");
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return MOK;
    }


    template<typename T, typename T0>
    template<typename T1>
    MInt32 AnisotropicGuidedFiltering<T, T0>::GetABThreads(T *pSrc,
                                                           MInt32 lPitchSrc,
                                                           T1 **Coff_A,
                                                           T1 **Coff_B,
                                                           MInt32 lWidth,
                                                           MInt32 lHeight,
                                                           MFloat Feps)
    {
        LOGD("GetABThreads-noguided++");
#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 res = MOK;

        MBool isRunThreads = false;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        isRunThreads = ( lHeight > 64 ) && ( m_mcvParallelMonitor != MNull );
#endif

        if( isRunThreads )
        {
            /// 设置回调函数
            auto func_lamda = [](MVoid *HParam) -> MVoid
            {
                LAni_Guided_Filter_ST filter_sturct = ( LAni_Guided_Filter_ST ) HParam;

                MInt32 startRow = filter_sturct->startRow;
                MInt32 endRow = filter_sturct->endRow;

                T *pSrc = ( T * ) filter_sturct->p1;
                MInt32 lPitchSrc = filter_sturct->lPitchSrc;
                T1 **Coff_A = ( T1 ** ) filter_sturct->p3;
                T1 **Coff_B = ( T1 ** ) filter_sturct->p4;
                MInt32 lWidth = filter_sturct->lWidth;
                MInt32 lHeight = filter_sturct->lHeight;
                MFloat Feps = filter_sturct->Feps;

                AnisotropicGuidedFiltering<T, T0> *obj = ( AnisotropicGuidedFiltering<T, T0> * ) filter_sturct->obj;
                filter_sturct->lRet = obj->GetAB(pSrc,
                                                 lPitchSrc,
                                                 Coff_A,
                                                 Coff_B,
                                                 lWidth,
                                                 lHeight,
                                                 Feps,
                                                 startRow, endRow);
            };
            MVoid (*func)(MVoid *) = func_lamda;



            /// 设置参数
            MInt32 lTaskNum = m_nThreadCount > 0 ? m_nThreadCount : lHeight > 16 ? 16 : 8;
            lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = ( lTaskHeight >> 2 ) << 2;

            Ani_Guided_Filter_ST pParam[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].startRow = lTaskHeight * lnum;
                pParam[ lnum ].endRow = lTaskHeight * ( lnum + 1 );
            }
            pParam[ lTaskNum - 1 ].endRow = lHeight;


            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].obj = this;
                pParam[ lnum ].task_ID = lnum;

                pParam[ lnum ].p1 = ( MVoid * ) pSrc;
                pParam[ lnum ].lPitchSrc = lPitchSrc;
                pParam[ lnum ].p3 = ( MVoid * ) Coff_A;
                pParam[ lnum ].p4 = ( MVoid * ) Coff_B;
                pParam[ lnum ].lWidth = lWidth;
                pParam[ lnum ].lHeight = lHeight;
                pParam[ lnum ].Feps = Feps;
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
            res = GetAB(pSrc,
                        lPitchSrc,
                        Coff_A,
                        Coff_B,
                        lWidth,
                        lHeight,
                        Feps, 0, lHeight);
        }

        exit:
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
    LOGD("GetABThreads-noguided--");
        return res;
    }


    template<typename T, typename T0>
    template<typename T1>
    MInt32 AnisotropicGuidedFiltering<T, T0>::GetABThreads(T *pSrc, MInt32 lPitchSrc,
                                                           T0 *pGuide, MInt32 lGuidePitch,
                                                           T1 **Coff_A, T1 **Coff_B,
                                                           MInt32 lWidth, MInt32 lHeight, MFloat Feps)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        LOGD("GetABThreads-withguided++");
        MInt32 res = MOK;



        MBool isRunThreads = false;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        isRunThreads = ( lHeight > 64 ) && ( m_mcvParallelMonitor != MNull );
#endif

        if( isRunThreads )
        {
            /// 设置回调函数
            auto func_lamda = [](MVoid *HParam) -> MVoid
            {
                LAni_Guided_Filter_ST filter_sturct = ( LAni_Guided_Filter_ST ) HParam;

                MInt32 startRow = filter_sturct->startRow;
                MInt32 endRow = filter_sturct->endRow;

                T *pSrc = ( T * ) filter_sturct->p1;
                T0 *pGuide = ( T0 * ) filter_sturct->p2;
                MInt32 lPitchSrc = filter_sturct->lPitchSrc;
                MInt32 lPitchGuided = filter_sturct->lPitchGuided;
                T1 **Coff_A = ( T1 ** ) filter_sturct->p3;
                T1 **Coff_B = ( T1 ** ) filter_sturct->p4;
                MInt32 lWidth = filter_sturct->lWidth;
                MInt32 lHeight = filter_sturct->lHeight;
                MFloat Feps = filter_sturct->Feps;

                AnisotropicGuidedFiltering<T, T0> *obj = ( AnisotropicGuidedFiltering<T, T0> * ) filter_sturct->obj;
                filter_sturct->lRet = obj->GetAB(pSrc,
                                                 lPitchSrc,
                                                 pGuide,
                                                 lPitchGuided,
                                                 Coff_A,
                                                 Coff_B,
                                                 lWidth,
                                                 lHeight,
                                                 Feps,
                                                 startRow, endRow);
            };
            MVoid (*func)(MVoid *) = func_lamda;



            /// 设置参数
            MInt32 lTaskNum = m_nThreadCount > 0 ? m_nThreadCount : lHeight > 16 ? 16 : 8;
            lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = ( lTaskHeight >> 2 ) << 2;

            Ani_Guided_Filter_ST pParam[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].startRow = lTaskHeight * lnum;
                pParam[ lnum ].endRow = lTaskHeight * ( lnum + 1 );
            }
            pParam[ lTaskNum - 1 ].endRow = lHeight;


            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].obj = this;
                pParam[ lnum ].task_ID = lnum;

                pParam[ lnum ].p1 = ( MVoid * ) pSrc;
                pParam[ lnum ].lPitchSrc = lPitchSrc;
                pParam[ lnum ].p2 = ( MVoid * ) pGuide;
                pParam[ lnum ].lPitchGuided = lGuidePitch;
                pParam[ lnum ].p3 = ( MVoid * ) Coff_A;
                pParam[ lnum ].p4 = ( MVoid * ) Coff_B;
                pParam[ lnum ].lWidth = lWidth;
                pParam[ lnum ].lHeight = lHeight;
                pParam[ lnum ].Feps = Feps;
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
            res = GetAB(pSrc,
                        lPitchSrc,
                        pGuide,
                        lGuidePitch,
                        Coff_A,
                        Coff_B,
                        lWidth,
                        lHeight,
                        Feps, 0, lHeight);
        }

        exit:


#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
    LOGD("GetABThreads-withguided--");
        return res;
    }


    /**
    * @brief            求AB系数,  每个方向半径固定为4
    * @tparam T1        输出AB系数的数据类型, T=>MUInt8时，T1=>MUInt16
    * @param pSrc       [in]
    * @param lPitchSrc  [in]
    * @param Coff_A     [out]   定点化为7bit
    * @param Coff_B     [out]   定点化为7bit
    * @param lWidth     [in]
    * @param lHeight    [in]
    * @param Feps       [in]
    * @return
    */
    template<typename T, typename T0>
    template<typename T1>
    MInt32 AnisotropicGuidedFiltering<T, T0>::GetAB(T *pSrc,
                                                    MInt32 lPitchSrc,
                                                    T0 *pGuide,
                                                    MInt32 lGuidePitch,
                                                    T1 **Coff_A,
                                                    T1 **Coff_B,
                                                    MInt32 lWidth,
                                                    MInt32 lHeight,
                                                    MFloat Feps,
                                                    MInt32 startRow /*= -1*/,
                                                    MInt32 endRow /*= -1*/)
    {
#ifdef USE_NEON_GUIDED
        LOGD("USE_NEON_GUIDED");
#endif
#if CALCULATE_TIME
        BasicTimer time;
#endif
        const MInt32 nRadius = RADIUS; // 这里半径固定为4
        MInt32 lKnlSize = m_lRadius * 2 + 1;
        //Feps = MAX(0.001, Feps);
        Feps = Feps * lKnlSize * lKnlSize;
        const MFloat FTempEps = Feps;

        MInt32 AB_Offset_Src[16][4] = {MNull};

        Compute_AB_Offset_Direction(AB_Offset_Src, lPitchSrc);

        MInt32 startRow2 = MAX(nRadius, startRow);
        MInt32 endRow2 = MIN(lHeight - nRadius, endRow);

        MFloat fDivFactor = 1.0 / lKnlSize;

        for (MInt32 y = startRow2; y < endRow2; y++)
        {
            MInt32 lShif_Num0 = y * lWidth;
            T *pDataSrc = pSrc + y * lPitchSrc;
            T0 *pDataGuide = pGuide + y * lGuidePitch;
            MInt16 *pTempMinA = m_pMinA + lShif_Num0;
            //MUInt8 *pTempShadeMapData = m_pShadeMapData + (y) * m_lShadeMapPitch;


            MInt32 x = nRadius;

#ifdef USE_NEON_GUIDED

            float32x4_t fRoundValue = vdupq_n_f32(0.49999997f);
            float32x4_t fRoundValue9 = vdupq_n_f32(fDivFactor);
            float32x4_t fRoundValue128 = vdupq_n_f32(128.0f);
            float32x4_t FTempEps_32x4 = vdupq_n_f32(FTempEps);

            //////////////////////////////////// MUInt8 ///////////////////////////////////////////////////
            if (sizeof(T) == 1)
            {
                uint8x8_t lShade_8x8;

                uint8x8_t lCur_8x8;

                uint8x8_t lS0_8x8;
                uint8x8_t lS1_8x8;
                uint8x8_t lS2_8x8;
                uint8x8_t lS3_8x8;
                uint8x8_t lS4_8x8;
                uint8x8_t lS5_8x8;
                uint8x8_t lS6_8x8;
                uint8x8_t lS7_8x8;


                for (; x < lWidth - nRadius - 8; x += 8)
                {
                    // lode data
                    lCur_8x8 = vld1_u8((MUInt8 *) pDataSrc + x);

                    for (MInt32 i = 0; i < m_lDirection; i++)
                    {
                        MInt32 s_0 = AB_Offset_Src[i][0];
                        MInt32 s_1 = AB_Offset_Src[i][1];
                        MInt32 s_2 = AB_Offset_Src[i][2];
                        MInt32 s_3 = AB_Offset_Src[i][3];

                        lS0_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_0);
                        lS1_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_0);
                        lS2_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_1);
                        lS3_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_1);
                        lS4_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_2);
                        lS5_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_2);
                        lS6_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_3);
                        lS7_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_3);


                        // 和的平方
                        uint16x8_t sum_src_16x8 = vaddl_u8(lS0_8x8, lS1_8x8);

                        if (m_lRadius >= 2)
                        {
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS2_8x8);
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS3_8x8);
                        }

                        if (m_lRadius >= 3)
                        {
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS4_8x8);
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS5_8x8);
                        }

                        if (m_lRadius >= 4)
                        {
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS6_8x8);
                            sum_src_16x8 = vaddw_u8(sum_src_16x8, lS7_8x8);
                        }

                        sum_src_16x8 = vaddw_u8(sum_src_16x8, lCur_8x8);


                        // store

                        // B
                        float32x4_t fMean_high_32x4 = vcvtq_f32_u32(vmovl_u16(vget_high_u16(sum_src_16x8)));
                        fMean_high_32x4 = vmulq_f32(fMean_high_32x4, fRoundValue9);
                        uint16x4_t mean_src_16x4 = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fMean_high_32x4, fRoundValue)));
                        vst1_s16((Coff_B[i] + lShif_Num0 + x + 4), vreinterpret_s16_u16(mean_src_16x4));
						
                        float32x4_t fMean_low_32x4 = vcvtq_f32_u32(vmovl_u16(vget_low_u16(sum_src_16x8)));
                        fMean_low_32x4 = vmulq_f32(fMean_low_32x4, fRoundValue9);
                        mean_src_16x4 = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fMean_low_32x4, fRoundValue)));
                        vst1_s16((Coff_B[i] + lShif_Num0 + x), vreinterpret_s16_u16(mean_src_16x4));

						//uint16x8_t mean_src_16x8 = vmulq_n_u16(sum_src_16x8, 7);
						//mean_src_16x8 = vrshrq_n_u16(mean_src_16x8, 6);
						//vst1q_s16((Coff_B[i] + y * lWidth + x), vreinterpretq_s16_u16(mean_src_16x8));
                    }
                }
            }

                //////////////////////////////////// MInt16 ///////////////////////////////////////////////////
            else if (sizeof(T) == 2)
            {
                uint8x8_t lShade_8x8;

                int16x4_t lCur_16x4;

                int16x4_t lS0_16x4;
                int16x4_t lS1_16x4;
                int16x4_t lS2_16x4;
                int16x4_t lS3_16x4;
                int16x4_t lS4_16x4;
                int16x4_t lS5_16x4;
                int16x4_t lS6_16x4;
                int16x4_t lS7_16x4;


                for (; x < lWidth - nRadius - 4; x += 4)
                {
                    // lode data

                    lCur_16x4 = vld1_s16((MInt16 *) pDataSrc + x);

                    for (MInt32 i = 0; i < m_lDirection; i++)
                    {
#ifdef USE_INTEGRAL //使用积分图
                        int16x4_t sum_src_16x4;
                        MInt32 s_3 = AB_Offset_Src[i][3];
                        if (i == 0)
                        {
                            MInt16 pSrc_sum[4];

                            pSrc_sum[0] = pColSum[i][y] + pDataSrc[x + s_3];
                            pColSum[i][y] = pSrc_sum[0] - pDataSrc[x - s_3];

                            pSrc_sum[1] = pColSum[i][y] + pDataSrc[x + 1 + s_3];
                            pColSum[i][y] = pSrc_sum[1] - pDataSrc[x + 1 - s_3];

                            pSrc_sum[2] = pColSum[i][y] + pDataSrc[x + 2 + s_3];
                            pColSum[i][y] = pSrc_sum[2] - pDataSrc[x + 2 - s_3];

                            pSrc_sum[3] = pColSum[i][y] + pDataSrc[x + 3 + s_3];
                            pColSum[i][y] = pSrc_sum[3] - pDataSrc[x + 3 - s_3];

                            sum_src_16x4 = vld1_s16(pSrc_sum);

                        }
                        else if (i == 2)
                        {
                            lS3_16x4 = vld1_s16((MInt16 *) pDataSrc + x + s_3);
                            lS7_16x4 = vld1_s16((MInt16 *) pDataSrc + x - s_3);
                            int16x4_t colSum_16x4 = vld1_s16((MInt16 *) pColSum[i] + x);

                            sum_src_16x4 = vadd_s16(colSum_16x4, lS3_16x4);
                            colSum_16x4 = vsub_s16(sum_src_16x4, lS7_16x4);
                            vst1_s16((MInt16 *) pColSum[i] + x, colSum_16x4);

                            //src_sum = pColSum[i][x] + pDataSrc[x + s_3];
                            //pColSum[i][x] = src_sum - pDataSrc[x - s_3];
                        }
                        else
                        {
                            MInt32 s_0 = AB_Offset_Src[i][0];
                            MInt32 s_1 = AB_Offset_Src[i][1];
                            MInt32 s_2 = AB_Offset_Src[i][2];

                            lS0_16x4 = vld1_s16((MInt16 *) pDataSrc + x + s_0);
                            lS1_16x4 = vld1_s16((MInt16 *) pDataSrc + x + s_1);
                            lS2_16x4 = vld1_s16((MInt16 *) pDataSrc + x + s_2);
                            lS3_16x4 = vld1_s16((MInt16 *) pDataSrc + x + s_3);
                            lS4_16x4 = vld1_s16((MInt16 *) pDataSrc + x - s_0);
                            lS5_16x4 = vld1_s16((MInt16 *) pDataSrc + x - s_1);
                            lS6_16x4 = vld1_s16((MInt16 *) pDataSrc + x - s_2);
                            lS7_16x4 = vld1_s16((MInt16 *) pDataSrc + x - s_3);


                            // 和
                            sum_src_16x4 = vadd_s16(lS0_16x4, lS1_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS2_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS3_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS4_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS5_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS6_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS7_16x4);

                            sum_src_16x4 = vadd_s16(sum_src_16x4, lCur_16x4);
                        }

#else
                        MInt32 s_0 = AB_Offset_Src[i][0];
                        MInt32 s_1 = AB_Offset_Src[i][1];
                        MInt32 s_2 = AB_Offset_Src[i][2];
                        MInt32 s_3 = AB_Offset_Src[i][3];

                        lS0_16x4 = vld1_s16((MInt16*)pDataSrc + x + s_0);
                        lS1_16x4 = vld1_s16((MInt16*)pDataSrc + x - s_0);
                        lS2_16x4 = vld1_s16((MInt16*)pDataSrc + x + s_1);
                        lS3_16x4 = vld1_s16((MInt16*)pDataSrc + x - s_1);
                        lS4_16x4 = vld1_s16((MInt16*)pDataSrc + x + s_2);
                        lS5_16x4 = vld1_s16((MInt16*)pDataSrc + x - s_2);
                        lS6_16x4 = vld1_s16((MInt16*)pDataSrc + x + s_3);
                        lS7_16x4 = vld1_s16((MInt16*)pDataSrc + x - s_3);


                        // 和
                        int16x4_t sum_src_16x4 = vadd_s16(lS0_16x4, lS1_16x4);

                        if (m_lRadius >= 2)
                        {
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS2_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS3_16x4);
                        }

                        if (m_lRadius >= 3)
                        {
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS4_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS5_16x4);
                        }

                        if (m_lRadius >= 4)
                        {
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS6_16x4);
                            sum_src_16x4 = vadd_s16(sum_src_16x4, lS7_16x4);
                        }

                        sum_src_16x4 = vadd_s16(sum_src_16x4, lCur_16x4);
#endif

                        // store
#if 1 //不做除法优化
                        // B
                        float32x4_t fMean_low_32x4 = vcvtq_f32_s32(vmovl_s16((sum_src_16x4)));
                        fMean_low_32x4 = vmulq_f32(fMean_low_32x4, fRoundValue9);
                        int16x4_t mean_src_16x4 = vmovn_s32(vcvtq_s32_f32(vaddq_f32(fMean_low_32x4, fRoundValue)));
#else
                        int16x4_t mean_src_16x4;
                        mean_src_16x4 = vrshr_n_s16(sum_src_16x4, 6);
                        mean_src_16x4 = vmul_n_s16(mean_src_16x4, 7);
#endif
                        vst1_s16((Coff_B[i] + lShif_Num0 + x), (mean_src_16x4));

                    }
                }
            }
            ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // 计算引导图的方差
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //////////////////////////////////// MUInt8 ///////////////////////////////////////////////////
            if (sizeof(T0) == 1)
            {
                uint8x8_t lShade_8x8;

                uint8x8_t lCur_8x8;

                uint8x8_t lS0_8x8;
                uint8x8_t lS1_8x8;
                uint8x8_t lS2_8x8;
                uint8x8_t lS3_8x8;
                uint8x8_t lS4_8x8;
                uint8x8_t lS5_8x8;
                uint8x8_t lS6_8x8;
                uint8x8_t lS7_8x8;

                uint16x8_t src_sqrt0_16x8;
                uint16x8_t src_sqrt1_16x8;
                uint16x8_t src_sqrt2_16x8;
                uint16x8_t src_sqrt3_16x8;
                uint16x8_t src_sqrt4_16x8;
                uint16x8_t src_sqrt5_16x8;
                uint16x8_t src_sqrt6_16x8;
                uint16x8_t src_sqrt7_16x8;

                for (x = nRadius; x < lWidth - nRadius - 8; x += 8)
                {
                    // lode data

                    // shade mask
                    //lShade_8x8= vld1_u8(pTempShadeMapData + x);
                    //float32x4_t fEps_high_32x4 = vcvtq_f32_s32(vmovl_u16(vget_high_u16(vmovl_u8(lShade_8x8))));
                    //fEps_high_32x4 = vmulq_n_f32(vmulq_n_f32(fEps_high_32x4, FTempEps), 0.015625f);
                    //float32x4_t fEps_low_32x4 = vcvtq_f32_s32(vmovl_u16(vget_low_u16(vmovl_u8(lShade_8x8))));
                    //fEps_low_32x4 = vmulq_n_f32(vmulq_n_f32(fEps_low_32x4, FTempEps), 0.015625f);

                    lCur_8x8 = vld1_u8((MUInt8*)pDataGuide + x);

                    uint16x8_t tempMin_16x8 = vdupq_n_u16(128);

                    for (MInt32 i = 0; i < m_lDirection; i++)
                    {
                        uint32x4_t sum_src_sqrt_high_32x4;
                        uint32x4_t sum_src_sqrt_low_32x4;
                        uint32x4_t src_sqrt_sum_high_32x4;
                        uint32x4_t src_sqrt_sum_low_32x4;

                        MInt32 s_0 = AB_Offset_Src[i][0];
                        MInt32 s_1 = AB_Offset_Src[i][1];
                        MInt32 s_2 = AB_Offset_Src[i][2];
                        MInt32 s_3 = AB_Offset_Src[i][3];

                        lS0_8x8 = vld1_u8((MUInt8*)pDataGuide + x + s_0);
                        lS1_8x8 = vld1_u8((MUInt8*)pDataGuide + x - s_0);
                        lS2_8x8 = vld1_u8((MUInt8*)pDataGuide + x + s_1);
                        lS3_8x8 = vld1_u8((MUInt8*)pDataGuide + x - s_1);
                        lS4_8x8 = vld1_u8((MUInt8*)pDataGuide + x + s_2);
                        lS5_8x8 = vld1_u8((MUInt8*)pDataGuide + x - s_2);
                        lS6_8x8 = vld1_u8((MUInt8*)pDataGuide + x + s_3);
                        lS7_8x8 = vld1_u8((MUInt8*)pDataGuide + x - s_3);

#ifdef NEW_CALCU_VAR
                        uint16x8_t diff_16x8;
                        diff_16x8 = vabdl_u8(lS0_8x8, lCur_8x8);
                        diff_16x8 = vabal_u8(diff_16x8, lS1_8x8, lCur_8x8);


                        if (m_lRadius >= 2)
                        {
                            diff_16x8 = vabal_u8(diff_16x8, lS2_8x8, lCur_8x8);
                            diff_16x8 = vabal_u8(diff_16x8, lS3_8x8, lCur_8x8);
                        }

                        if (m_lRadius >= 3)
                        {
                            diff_16x8 = vabal_u8(diff_16x8, lS4_8x8, lCur_8x8);
                            diff_16x8 = vabal_u8(diff_16x8, lS5_8x8, lCur_8x8);
                        }

                        if (m_lRadius >= 4)
                        {
                            diff_16x8 = vabal_u8(diff_16x8, lS6_8x8, lCur_8x8);
                            diff_16x8 = vabal_u8(diff_16x8, lS7_8x8, lCur_8x8);
                        }

                        src_sqrt_sum_high_32x4 = vmull_u16(vget_high_u16(diff_16x8), vget_high_u16(diff_16x8));
                        src_sqrt_sum_low_32x4 = vmull_u16(vget_low_u16(diff_16x8), vget_low_u16(diff_16x8));

#else



                        // 和的平方
                        uint16x8_t sum_src_16x8 = vaddl_u8(lS0_8x8, lS1_8x8);
                        sum_src_16x8 = vaddw_u8(sum_src_16x8, lS2_8x8);
                        sum_src_16x8 = vaddw_u8(sum_src_16x8, lS3_8x8);
                        sum_src_16x8 = vaddw_u8(sum_src_16x8, lS4_8x8);
                        sum_src_16x8 = vaddw_u8(sum_src_16x8, lS5_8x8);
                        sum_src_16x8 = vaddw_u8(sum_src_16x8, lS6_8x8);
                        sum_src_16x8 = vaddw_u8(sum_src_16x8, lS7_8x8);

                        sum_src_16x8 = vaddw_u8(sum_src_16x8, lCur_8x8);


                        uint16x4_t sum_src_high_16x4 = vget_high_u16(sum_src_16x8);
                        uint16x4_t sum_src_low_16x4 = vget_low_u16(sum_src_16x8);
                        sum_src_sqrt_high_32x4 = vmull_u16(sum_src_high_16x4, sum_src_high_16x4);
                        sum_src_sqrt_low_32x4 = vmull_u16(sum_src_low_16x4, sum_src_low_16x4);

                        // 平方的和
                        src_sqrt0_16x8 = vmull_u8(lS0_8x8, lS0_8x8);
                        src_sqrt1_16x8 = vmull_u8(lS1_8x8, lS1_8x8);
                        src_sqrt2_16x8 = vmull_u8(lS2_8x8, lS2_8x8);
                        src_sqrt3_16x8 = vmull_u8(lS3_8x8, lS3_8x8);
                        src_sqrt4_16x8 = vmull_u8(lS4_8x8, lS4_8x8);
                        src_sqrt5_16x8 = vmull_u8(lS5_8x8, lS5_8x8);
                        src_sqrt6_16x8 = vmull_u8(lS6_8x8, lS6_8x8);
                        src_sqrt7_16x8 = vmull_u8(lS7_8x8, lS7_8x8);

                        uint16x8_t src_sqrt_16x8 = vmull_u8(lCur_8x8, lCur_8x8);


                        src_sqrt_sum_high_32x4 = vaddl_u16(vget_high_u16(src_sqrt0_16x8), vget_high_u16(src_sqrt1_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt2_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt3_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt4_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt5_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt6_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt7_16x8));
                        src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt_16x8));
                        src_sqrt_sum_high_32x4 = vmulq_n_u32(src_sqrt_sum_high_32x4, lKnlSize);

                        src_sqrt_sum_low_32x4 = vaddl_u16(vget_low_u16(src_sqrt0_16x8), vget_low_u16(src_sqrt1_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt2_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt3_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt4_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt5_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt6_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt7_16x8));
                        src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt_16x8));
                        src_sqrt_sum_low_32x4 = vmulq_n_u32(src_sqrt_sum_low_32x4, lKnlSize);

                        // 计算方差
                        src_sqrt_sum_high_32x4 = vsubq_u32(src_sqrt_sum_high_32x4, sum_src_sqrt_high_32x4);
                        src_sqrt_sum_low_32x4 = vsubq_u32(src_sqrt_sum_low_32x4, sum_src_sqrt_low_32x4);
#endif // NEW_CALCU_VAR
                        float32x4_t fVar_high_32x4 = vcvtq_f32_u32(src_sqrt_sum_high_32x4);
                        float32x4_t fVar_low_32x4 = vcvtq_f32_u32(src_sqrt_sum_low_32x4);

                        // 计算AB


                        //float32x4_t fVarEps_high_32x4 = vaddq_f32(fVar_high_32x4, fEps_high_32x4);
                        float32x4_t fVarEps_high_32x4 = vaddq_f32(fVar_high_32x4, FTempEps_32x4);
                        float32x4_t fInvVar_high_32x4 = vrecpeq_f32(fVarEps_high_32x4); // 倒数
                        fInvVar_high_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_high_32x4, fInvVar_high_32x4),
                            fInvVar_high_32x4);
                        fInvVar_high_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_high_32x4, fInvVar_high_32x4),
                            fInvVar_high_32x4); // 精确化
                        float32x4_t fCoefA_high_32x4 = vmulq_f32(fInvVar_high_32x4,
                            vmulq_f32(fVar_high_32x4, fRoundValue128));
                        uint16x4_t lCoefA_high_16x4 = vmovn_u32(
                            vcvtq_u32_f32(vaddq_f32(fCoefA_high_32x4, fRoundValue)));


                        //float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, fEps_low_32x4);
                        float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, FTempEps_32x4);
                        float32x4_t fInvVar_low_32x4 = vrecpeq_f32(fVarEps_low_32x4);
                        fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4), fInvVar_low_32x4);
                        fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4),
                            fInvVar_low_32x4); // 精确化
                        float32x4_t fCoefA_low_32x4 = vmulq_f32(fInvVar_low_32x4, vmulq_f32(fVar_low_32x4, fRoundValue128));
                        uint16x4_t lCoefA_low_16x4 = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fCoefA_low_32x4, fRoundValue)));

                        // minA
                        tempMin_16x8 = vminq_u16(vcombine_u16(lCoefA_low_16x4, lCoefA_high_16x4), tempMin_16x8);

                        // store
                        // A
//                        lCoefA_high_16x4 = vdup_n_u16(0);
//                        lCoefA_low_16x4 = vdup_n_u16(0);
                        vst1_s16((Coff_A[i] + lShif_Num0 + x), vreinterpret_s16_u16(lCoefA_low_16x4));
                        vst1_s16((Coff_A[i] + lShif_Num0 + x + 4), vreinterpret_s16_u16(lCoefA_high_16x4));
                    }
                    vst1q_s16((pTempMinA + x), vreinterpretq_s16_u16(tempMin_16x8));
                }
            }

			//////////////////////////////////// MInt16 ///////////////////////////////////////////////////
			else if (sizeof(T0) == 2)
			{
				uint8x8_t lShade_8x8;

				int16x4_t lCur_16x4;

				int16x4_t lS0_16x4;
				int16x4_t lS1_16x4;
				int16x4_t lS2_16x4;
				int16x4_t lS3_16x4;
				int16x4_t lS4_16x4;
				int16x4_t lS5_16x4;
				int16x4_t lS6_16x4;
				int16x4_t lS7_16x4;

				uint32x4_t src_sqrt0_32x4;
				uint32x4_t src_sqrt1_32x4;
				uint32x4_t src_sqrt2_32x4;
				uint32x4_t src_sqrt3_32x4;
				uint32x4_t src_sqrt4_32x4;
				uint32x4_t src_sqrt5_32x4;
				uint32x4_t src_sqrt6_32x4;
				uint32x4_t src_sqrt7_32x4;

				for (x = nRadius; x < lWidth - nRadius - 4; x += 4)
				{
					// lode data

					// shade mask
					//lShade_8x8= vld1_u8(pTempShadeMapData + x);
					//float32x4_t fEps_low_32x4 = vcvtq_f32_s32(vmovl_u16(vget_low_u16(vmovl_u8(lShade_8x8))));
					//fEps_low_32x4 = vmulq_n_f32(vmulq_n_f32(fEps_low_32x4, FTempEps), 0.015625f);

					lCur_16x4 = vld1_s16((MInt16*)pDataGuide + x);

					int16x4_t tempMin_16x4 = vdup_n_s16(128);

					for (MInt32 i = 0; i < m_lDirection; i++)
					{
						MInt32 s_0 = AB_Offset_Src[i][0];
						MInt32 s_1 = AB_Offset_Src[i][1];
						MInt32 s_2 = AB_Offset_Src[i][2];
						MInt32 s_3 = AB_Offset_Src[i][3];

						lS0_16x4 = vld1_s16((MInt16*)pDataGuide + x + s_0);
                        lS1_16x4 = vld1_s16((MInt16*)pDataGuide + x - s_0);
						lS2_16x4 = vld1_s16((MInt16*)pDataGuide + x + s_1);
                        lS3_16x4 = vld1_s16((MInt16*)pDataGuide + x - s_1);
						lS4_16x4 = vld1_s16((MInt16*)pDataGuide + x + s_2);
                        lS5_16x4 = vld1_s16((MInt16*)pDataGuide + x - s_2);
						lS6_16x4 = vld1_s16((MInt16*)pDataGuide + x + s_3);
						lS7_16x4 = vld1_s16((MInt16*)pDataGuide + x - s_3);

#ifdef NEW_CALCU_VAR
                        uint16x4_t diff_16x4;
                        diff_16x4 = vabd_s16(lS0_16x4, lCur_16x4);
                        diff_16x4 = vaba_s16(diff_16x4, lS1_16x4, lCur_16x4);


                        if (m_lRadius >= 2)
                        {
                            diff_16x4 = vaba_s16(diff_16x4, lS2_16x4, lCur_16x4);
                            diff_16x4 = vaba_s16(diff_16x4, lS3_16x4, lCur_16x4);
                        }

                        if (m_lRadius >= 3)
                        {
                            diff_16x4 = vaba_s16(diff_16x4, lS4_16x4, lCur_16x4);
                            diff_16x4 = vaba_s16(diff_16x4, lS5_16x4, lCur_16x4);
                }

                        if (m_lRadius >= 4)
                        {
                            diff_16x4 = vaba_s16(diff_16x4, lS6_16x4, lCur_16x4);
                            diff_16x4 = vaba_s16(diff_16x4, lS7_16x4, lCur_16x4);
                        }

                        uint32x4_t src_sqrt_sum_low_32x4 = vmull_u16(diff_16x4, diff_16x4);

#else
						// 和的平方
						int16x4_t sum_src_16x4 = vadd_s16(lS0_16x4, lS1_16x4);
						sum_src_16x4 = vadd_s16(sum_src_16x4, lS2_16x4);
						sum_src_16x4 = vadd_s16(sum_src_16x4, lS3_16x4);
						sum_src_16x4 = vadd_s16(sum_src_16x4, lS4_16x4);
						sum_src_16x4 = vadd_s16(sum_src_16x4, lS5_16x4);
						sum_src_16x4 = vadd_s16(sum_src_16x4, lS6_16x4);
						sum_src_16x4 = vadd_s16(sum_src_16x4, lS7_16x4);

						sum_src_16x4 = vadd_s16(sum_src_16x4, lCur_16x4);

						uint32x4_t sum_src_sqrt_low_32x4 = vmull_s16(sum_src_16x4, sum_src_16x4);

						// 平方的和
						src_sqrt0_32x4 = vmull_s16(lS0_16x4, lS0_16x4);
						src_sqrt1_32x4 = vmull_s16(lS1_16x4, lS1_16x4);
						src_sqrt2_32x4 = vmull_s16(lS2_16x4, lS2_16x4);
						src_sqrt3_32x4 = vmull_s16(lS3_16x4, lS3_16x4);
						src_sqrt4_32x4 = vmull_s16(lS4_16x4, lS4_16x4);
						src_sqrt5_32x4 = vmull_s16(lS5_16x4, lS5_16x4);
						src_sqrt6_32x4 = vmull_s16(lS6_16x4, lS6_16x4);
						src_sqrt7_32x4 = vmull_s16(lS7_16x4, lS7_16x4);

						uint32x4_t src_sqrt_32x4 = vmull_s16(lCur_16x4, lCur_16x4);


						uint32x4_t src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt0_32x4, src_sqrt1_32x4);
						src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt2_32x4));
						src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt3_32x4));
						src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt4_32x4));
						src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt5_32x4));
						src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt6_32x4));
						src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt7_32x4));
						src_sqrt_sum_low_32x4 = vaddq_u32(src_sqrt_sum_low_32x4, (src_sqrt_32x4));
						src_sqrt_sum_low_32x4 = vmulq_n_u32(src_sqrt_sum_low_32x4, lKnlSize);

						// 计算方差
						src_sqrt_sum_low_32x4 = vsubq_u32(src_sqrt_sum_low_32x4, sum_src_sqrt_low_32x4);
#endif
						float32x4_t fVar_low_32x4 = vcvtq_f32_u32(src_sqrt_sum_low_32x4);

						// 计算AB

						float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, FTempEps_32x4);
						float32x4_t fInvVar_low_32x4 = vrecpeq_f32(fVarEps_low_32x4);
						fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4), fInvVar_low_32x4);
						fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4), fInvVar_low_32x4); // 精确化
						float32x4_t fCoefA_low_32x4 = vmulq_f32(fInvVar_low_32x4, vmulq_f32(fVar_low_32x4, fRoundValue128));
						uint16x4_t lCoefA_low_16x4 = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fCoefA_low_32x4, fRoundValue)));

						// minA
						tempMin_16x4 = vmin_s16((lCoefA_low_16x4), tempMin_16x4);

						// store
						// A
						//lCoefA_high_16x4 = vdup_n_u16(0);
						//lCoefA_low_16x4 = vdup_n_u16(0);
						vst1_s16((Coff_A[i] + lShif_Num0 + x), vreinterpret_s16_u16(lCoefA_low_16x4));
					}

					vst1_s16((pTempMinA + x), (tempMin_16x4));
				}
			}
#endif

			for (; x < lWidth - nRadius; x++)
			{
				//Feps = FTempEps * ((pTempShadeMapData[x]) / 64.0);
				T lCur = pDataSrc[x];
				T0 lCur_ = pDataGuide[x];

				MInt32 src_sum = 0;
				MInt32 guide_sum = 0;
				MInt32 guide_sum_sqr = 0;
                MInt32 s[4] = { 0 };
                T a0[4] = { 0 };
                T a1[4] = { 0 };
                T b0[4] = { 0 };
                T b1[4] = { 0 };

				for (MInt32 i = 0; i < m_lDirection; i++)
				{
                    src_sum = lCur;
                    guide_sum = lCur_;
                    guide_sum_sqr = lCur_ * lCur_;
                    MInt32 lDiff = 0;
                    for (MInt32 j = 0; j < m_lRadius; j++)
                    {
                        s[j] = AB_Offset_Src[i][j];

                        a0[j] = pDataSrc[x + s[j]];
                        a1[j] = pDataSrc[x - s[j]];

                        b0[j] = pDataGuide[x + s[j]];
                        b1[j] = pDataGuide[x - s[j]];

                        src_sum += (a0[j] + a1[j]);
    #ifdef NEW_CALCU_VAR
                        lDiff += (ABS(b0[j] - lCur_) + ABS(b1[j] - lCur_));
    #else
                        guide_sum += (b0[j] + b1[j]);
                        guide_sum_sqr += (b0[j] * b0[j] + b1[j] * b1[j]);
    #endif
                    }
          
    #ifdef NEW_CALCU_VAR
                    lDiff = lDiff * lDiff;
    #else
				    
    #endif
                   

					// a = varI / (varI + eps)
					// b = mean_p * (1 - a)
					// original: a = CovIP / (varI + eps)
					// original: b = mean_p - a * mean_I
					//                MFloat lGVar = src_sum_sqr * lKnlSize - src_sum * src_sum;
					//                MFloat fCof_A = lGVar / ( lGVar + Feps ); // 0~1.0
					//                MFloat fCof_B = src_sum * fInv_Scale * ( 1 - fCof_A );
    #ifdef NEW_CALCU_VAR
                    MInt32 lGVar = lDiff;// guide_sum_sqr* lKnlSize - guide_sum * guide_sum;
    #else
                    MInt32 lGVar = guide_sum_sqr* lKnlSize - guide_sum * guide_sum;
    #endif
                    MInt32 fCof_A = 128.0 * lGVar / (lGVar + FTempEps + 0.001) + 0.5;

                    src_sum = (src_sum + lKnlSize / 2) / lKnlSize;
					MInt32 fCof_B = src_sum; // * ( 1 - fCof_A ); // todo:将1-a放到后面去做

					pTempMinA[x] = MIN(fCof_A, pTempMinA[x]);

					Coff_A[i][lShif_Num0 + x] = fCof_A; // 0~128
					Coff_B[i][lShif_Num0 + x] = fCof_B; // 0~(7+sizeof(T))^2


#if 0 // TODO：传入图像本身有填充，内部暂时关闭填充
					for (MInt32 x = 0; x < nRadius; x++)
					{
						Coff_A[i][y * lWidth + x] = Coff_A[i][y * lWidth + nRadius];
						Coff_B[i][y * lWidth + x] = Coff_B[i][y * lWidth + nRadius];
					}

					for (MInt32 x = lWidth - nRadius; x < lWidth; x++)
					{
						Coff_A[i][y * lWidth + x] = Coff_A[i][y * lWidth + lWidth - nRadius - 1];
						Coff_B[i][y * lWidth + x] = Coff_B[i][y * lWidth + lWidth - nRadius - 1];
					}
#endif
				}

            }

#ifdef USE_INTEGRAL008

			//右移操作
			for (MInt32 x = lWidth - 1; x <= 0; x--)
			{
				pColSum[1+2][x] = pColSum[1+2][x - 1];
				pColSum[5+2][x] = pColSum[5+2][x - 1];
				pColSum[9+2][x] = pColSum[9+2][x - 1];
			}

			//左移操作
			for (MInt32 x = 0; x < lWidth; x++)
			{
				pColSum[1][x + lHeight + 1] = pColSum[1][x + lHeight + 1 + 1];
				pColSum[5][x + lHeight + 1] = pColSum[5][x + lHeight + 1 + 1];
				pColSum[9][x + lHeight + 1] = pColSum[9][x + lHeight + 1 + 1];
			}
#endif
		}

#if 0 // TODO：传入图像本身有填充，内部暂时关闭填充
        if( startRow2 == nRadius )
        {
            for(MInt32 i = 0; i < m_lDirection; i++)
            {
                for(MInt32 y = 0; y < nRadius; y++)
                {
                    MMemCpy(Coff_A[ i ] + y * lWidth, Coff_A[ i ] + nRadius * lWidth, lWidth * sizeof(T1));
                    MMemCpy(Coff_B[ i ] + y * lWidth, Coff_B[ i ] + nRadius * lWidth, lWidth * sizeof(T1));
                }
            }
        }

        if( endRow2 == lHeight - nRadius )
        {
            for(MInt32 i = 0; i < m_lDirection; i++)
            {
                for(MInt32 y = lHeight - nRadius; y < lHeight; y++)
                {
                    MMemCpy(Coff_A[ i ] + y * lWidth, Coff_A[ i ] + ( lHeight - nRadius - 1 ) * lWidth, lWidth * sizeof(T1));
                    MMemCpy(Coff_B[ i ] + y * lWidth, Coff_B[ i ] + ( lHeight - nRadius - 1 ) * lWidth, lWidth * sizeof(T1));
                }
            }
        }
#endif

#ifdef USE_INTEGRAL //使用积分图，释放内存

        for (MInt32 i = 0; i < 12; i++)
        {
//            pColSum[i]--;
            SAFE_FREE_ARRAY(MNull, pColSum[i]);
        }

#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return MOK;
    }


    /**
    * @brief  计算 ab 的均值，这里使用4个方向计算
    *       mean_a = Fun_mean(a)
    *       mean_b = Fun_mean(b)
    *
    * @param pCoff_A
    * @param pCoff_B
    * @param pFusCoff_A
    * @param pFusCoff_B
    * @param lWidth
    * @param lHeight
    * @return
    */
    template<typename T, typename T0>
    template<typename T1, typename T2>
    MInt32 AnisotropicGuidedFiltering<T, T0>::MeanAB(T1 **pA,
                                                     T1 **pB,
                                                     T2 *pMeanA,
                                                     T2 *pMeanB,
                                                     MInt32 lWidth,
                                                     MInt32 lHeight,
                                                     MInt32 startRow /*= -1*/,
                                                     MInt32 endRow /*= -1*/)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lOffset = sizeof(T)* sizeof(T)*255;

        MInt32 nRadius = m_lRadius; // 半径
        MInt32 pWeiOffset[16] = {0};

        GetWeiOffset_Direction(pWeiOffset, lWidth, nRadius);

        MInt32 startRow2 = MAX(nRadius, startRow);
        MInt32 endRow2 = MIN(lHeight - nRadius, endRow);

        const int DETAIL_THRESHOLD = 64; //动态方向阈值

        for(MInt32 y = startRow2; y < endRow2; y++)
        {
			MInt32 lShif_Num0 = y * lWidth;

            T2 *pCurFusA = pMeanA + lShif_Num0;
            T2 *pCurFusB = pMeanB + lShif_Num0;
            MInt16 *pTempMinA = m_pMinA + lShif_Num0;

            MInt32 x = nRadius;

#ifdef USE_NEON_GUIDED
			uint16x4_t const1_16x4 = vdup_n_u16(1);

            int32x4_t const128_32x4 = vdupq_n_s32(128);
            int32x4_t const0_32x4 = vdupq_n_s32(0);
            int32x4_t const1_32x4 = vdupq_n_s32(1);
            int32x4_t const32_32x4 = vdupq_n_s32(32);
            int16x4_t const32_16x4 = vdup_n_s16(32);
            int16x4_t const64_16x4 = vdup_n_s16(DETAIL_THRESHOLD);
            float32x4_t fRoundValue = vdupq_n_f32(0.49999997f);

            
            // 输入是MInt16
            for(; x < lWidth - nRadius - 3; x+=4)
            {
				MInt32 lShif_Num = lShif_Num0 + x;

                int32x4_t lSumA_32x4 = const0_32x4;
                int32x4_t lSumB_32x4 = const0_32x4;
                int32x4_t lSumWei_32x4 = const0_32x4;
                int16x4_t nMinA_16x4 = vld1_s16(x + pTempMinA);
                int16x4_t nMask_16x4;
                int16x4_t lCof_A_16x4;
                int16x4_t lCof_B_16x4;
                int32x4_t lWei_32x4;
                MInt32 temp_lCof_A[4];
                MInt32 temp_lWei[4];

                for(MInt32 k = 0; k < m_lDirection; k++)
                {
                    MInt32 weiOffset0 = pWeiOffset[(k << 1)];
                    MInt32 weiOffset1 = pWeiOffset[(k << 1) + 1];

                    // 中心点
                    lCof_A_16x4 = vld1_s16(pA[ k ] + lShif_Num);
                   
#ifdef USING_LWEI_TABLE
                    for (int s = 0; s < 4; s++)
                    {
                        temp_lWei[s] = lWei_Table[*(pA[k] + lShif_Num + s)];
                    }
                    lWei_32x4 = vld1q_s32(temp_lWei);
#else
                    lWei_32x4 = vsubw_s16(const128_32x4, lCof_A_16x4);
					lWei_32x4 = vrshrq_n_s32(vmlaq_s32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                    lWei_32x4 = vaddq_s32(lWei_32x4, const1_32x4);
#endif
                    // 和最小值比较
                    nMask_16x4 = vreinterpret_s16_u16(vand_u16(vcle_s16(vabd_s16(lCof_A_16x4, nMinA_16x4), const64_16x4), const1_16x4));

                    lCof_B_16x4 = vld1_s16(pB[k] + lShif_Num);
                    lWei_32x4 = vmulq_s32(vmovl_s16(nMask_16x4), lWei_32x4);
                    lSumWei_32x4 = vaddq_s32(lSumWei_32x4, lWei_32x4);
                    lSumA_32x4 = vmlaq_s32(lSumA_32x4, vmovl_s16(lCof_A_16x4), lWei_32x4);
                    lSumB_32x4 = vmlaq_s32(lSumB_32x4, vmovl_s16(lCof_B_16x4), lWei_32x4);
                    //lSumA_32x4 = vaddq_s32(lSumA_32x4, vmulq_s32(vmovl_s16(lCof_A_16x4), lWei_32x4));
                    //lSumB_32x4 = vaddq_s32(lSumB_32x4, vmulq_s32(vmovl_s16(lCof_B_16x4), lWei_32x4));

                    // 右边的点
                    lCof_A_16x4 = vld1_s16(pA[ k ] + lShif_Num + weiOffset0);
                    lCof_B_16x4 = vld1_s16(pB[ k ] + lShif_Num + weiOffset0);

#ifdef USING_LWEI_TABLE
                    for (int s = 0; s < 4; s++)
                    {
                        temp_lWei[s] = lWei_Table[*(pA[k] + lShif_Num + weiOffset0 + s)];
                    }
                    lWei_32x4 = vld1q_s32(temp_lWei);
#else
                    lWei_32x4 = vsubw_s16(const128_32x4, lCof_A_16x4);
                    lWei_32x4 = vrshrq_n_s32(vmlaq_s32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                    lWei_32x4 = vaddq_s32(lWei_32x4, const1_32x4);
#endif
                    lWei_32x4 = vmulq_s32(vmovl_s16(nMask_16x4), lWei_32x4);
                    lSumWei_32x4 = vaddq_s32(lSumWei_32x4, lWei_32x4);
                    lSumA_32x4 = vmlaq_s32(lSumA_32x4, vmovl_s16(lCof_A_16x4), lWei_32x4);
                    lSumB_32x4 = vmlaq_s32(lSumB_32x4, vmovl_s16(lCof_B_16x4), lWei_32x4);
                    //lSumA_32x4 = vaddq_s32(lSumA_32x4, vmulq_s32(vmovl_s16(lCof_A_16x4), lWei_32x4));
                    //lSumB_32x4 = vaddq_s32(lSumB_32x4, vmulq_s32(vmovl_s16(lCof_B_16x4), lWei_32x4));

                    // 左边的点
                    lCof_A_16x4 = vld1_s16(pA[ k ] + lShif_Num + weiOffset1);
                    lCof_B_16x4 = vld1_s16(pB[ k ] + lShif_Num + weiOffset1);
               
#ifdef USING_LWEI_TABLE
                    for (int s = 0; s < 4; s++)
                    {
                        temp_lWei[s] = lWei_Table[*(pA[k] + lShif_Num + weiOffset1 + s)];
                    }
                    lWei_32x4 = vld1q_s32(temp_lWei);
#else
                    lWei_32x4 = vsubw_s16(const128_32x4, lCof_A_16x4);
                    lWei_32x4 = vrshrq_n_s32(vmlaq_s32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                    lWei_32x4 = vaddq_s32(lWei_32x4, const1_32x4);
#endif
                    lWei_32x4 = vmulq_s32(vmovl_s16(nMask_16x4), lWei_32x4);
                    lSumWei_32x4 = vaddq_s32(lSumWei_32x4, lWei_32x4);
                    lSumA_32x4 = vmlaq_s32(lSumA_32x4, vmovl_s16(lCof_A_16x4), lWei_32x4);
                    lSumB_32x4 = vmlaq_s32(lSumB_32x4, vmovl_s16(lCof_B_16x4), lWei_32x4);
                    //lSumA_32x4 = vaddq_s32(lSumA_32x4, vmulq_s32(vmovl_s16(lCof_A_16x4), lWei_32x4));
                    //lSumB_32x4 = vaddq_s32(lSumB_32x4, vmulq_s32(vmovl_s16(lCof_B_16x4), lWei_32x4));

                }


                // 动态调整A值
#if defined(DYNAMIC_DENOISE) //是否开启动态降噪，针对噪声不均匀
                nMinA_16x4 = vmax_s16(nMinA_16x4, const32_16x4);
                nMinA_16x4 = vmin_s16(nMinA_16x4, const64_16x4);

                lSumA_32x4   = vmulq_s32(const32_32x4, lSumA_32x4);
                lSumB_32x4   = vmulq_s32(vmovl_s16(nMinA_16x4), lSumB_32x4);
                lSumWei_32x4 = vmulq_s32(vmovl_s16(nMinA_16x4), lSumWei_32x4);
#endif



#if 1 //使用neon中的除法

                // 取权重的倒数
                float32x4_t fSumWei_32x4 = vcvtq_f32_s32(lSumWei_32x4);
				
                float32x4_t fInvSumWei_32x4 = vrecpeq_f32(fSumWei_32x4); // 倒数
                //fInvSumWei_32x4 = vmulq_f32(vrecpsq_f32(fSumWei_32x4, fInvSumWei_32x4), fInvSumWei_32x4);
                fInvSumWei_32x4 = vmulq_f32(vrecpsq_f32(fSumWei_32x4, fInvSumWei_32x4), fInvSumWei_32x4); // 精确化
				
                // 计算A
                int32x4_t lMean_a_32x4 = vcvtq_s32_f32(vmlaq_f32(fRoundValue, vcvtq_f32_s32(lSumA_32x4), fInvSumWei_32x4));
                //int32x4_t lOffset_32x4 = vminq_s32(vceqq_s32 (lSumWei_32x4, vdupq_n_s32(0)), vdupq_n_s32(128));
                //lMean_a_32x4 = vaddq_s32(lMean_a_32x4, lOffset_32x4);
				
                // 计算B
                int32x4_t lMean_b_32x4 = vcvtq_s32_f32(vmlaq_f32(fRoundValue, vcvtq_f32_s32(lSumB_32x4), fInvSumWei_32x4));


                // 取值范围[0, 127]
                // fMean_a 越大 原来像素的值占比越大
                vst1_s16(pCurFusA + x, vmovn_s32(lMean_a_32x4));
                //vst1_s16(pCurFusA + x, vdup_n_s16(0));
                vst1_s16(pCurFusB + x, vmovn_s32(lMean_b_32x4));
#else // TODO:测试发现并未加速
				MInt32 lSumA[4]; 
				MInt32 lSumB[4]; 
				MInt32 lSumWei[4]; 
				
				vst1q_s32(lSumA, lSumA_32x4);
				vst1q_s32(lSumB, lSumB_32x4);
				vst1q_s32(lSumWei, lSumWei_32x4);
				
				pCurFusA[x] = (lSumA[0] + (lSumWei[0] >> 1)) / lSumWei[0];
				pCurFusB[x] = (lSumB[0] + (lSumWei[0] >> 1)) / lSumWei[0];
				
				pCurFusA[x+1] = (lSumA[1] + (lSumWei[1] >> 1)) / lSumWei[1];
				pCurFusB[x+1] = (lSumB[1] + (lSumWei[1] >> 1)) / lSumWei[1];
				
				pCurFusA[x+2] = (lSumA[2] + (lSumWei[2] >> 1)) / lSumWei[2];
				pCurFusB[x+2] = (lSumB[2] + (lSumWei[2] >> 1)) / lSumWei[2];
						  
				pCurFusA[x+3] = (lSumA[3] + (lSumWei[3] >> 1)) / lSumWei[3];
				pCurFusB[x+3] = (lSumB[3] + (lSumWei[3] >> 1)) / lSumWei[3];
				
#endif
            }

#endif // neon end

            for(; x < lWidth - nRadius; x++)
            {
                MInt32 lShif_Num = lShif_Num0 + x;
                MInt32 lSumA = 0;
                MInt32 lSumB = 0;
                MInt32 lSumWei = 0;

#if 1//defined(DYNAMIC_DENOISE) //是否开启动态降噪 // 针对细节
                T1 lCof_A;
                T1 lCof_B;
                MInt32 lWei;
                MInt16 nMask = 0;
                for (MInt32 i = 0; i < m_lDirection; i++)
                {
                    nMask = (pA[i][lShif_Num] - pTempMinA[x] <= DETAIL_THRESHOLD);

                    if (nMask == 0)
                    {
                        continue;
                    }

                    // 动态调节方向数

#if 1 //加或者不加两边的点
                    MInt32 weiOffset0 = pWeiOffset[i * 2];
                    MInt32 weiOffset1 = pWeiOffset[i * 2 + 1];

                    // 加上左边点
                    lCof_A = pA[i][lShif_Num + weiOffset0];
                    lCof_B = pB[i][lShif_Num + weiOffset0];
    #if 1
                    lWei = lWei_Table[lCof_A];
    #else
                    lWei = 128 - lCof_A; // lCof_A 取值[0， 127]
                    lWei = (lWei + lWei * lWei + 8) >> 4; // 从0到127，逐渐降低
                    lWei++;
    #endif
                    lWei *= nMask;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;

                    // 加上右边点
                    lCof_A = pA[i][lShif_Num + weiOffset1];
                    lCof_B = pB[i][lShif_Num + weiOffset1];
    #if 1
                    lWei = lWei_Table[lCof_A];
    #else
                    lWei = 128 - lCof_A; // lCof_A 取值[0， 127]
                    lWei = (lWei + lWei * lWei + 8) >> 4; // 从0到127，逐渐降低
                    lWei++;
    #endif
                    lWei *= nMask;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;
#endif

                    // 加上中心点
                    lCof_A = pA[i][lShif_Num]; 
                    lCof_B = pB[i][lShif_Num];
    #if 1
                    lWei = lWei_Table[lCof_A];
    #else
                    lWei = 128 - lCof_A; // lCof_A 取值[0， 127]
                    lWei = (lWei + lWei * lWei + 8) >> 4; // 从0到127，逐渐降低
                    lWei++;
    #endif
                    lWei *= nMask;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;

                }


#else // 针对平坦区域
                for(MInt32 k = 0; k < m_lDirection; k++)
                {
                    MInt32 weiOffset0 = pWeiOffset[ k * 2 ];
                    MInt32 weiOffset1 = pWeiOffset[ k * 2 + 1 ];

                    T1 lCof_A = pA[ k ][ lShif_Num + weiOffset0 ];
                    T1 lCof_B = pB[ k ][ lShif_Num + weiOffset0 ];
                    MInt32 lWei = 128 - lCof_A; // lCof_A 取值[0， 127]
                    lWei = ( lWei + lWei * lWei + 8 ) >> 4; // 从0到127，逐渐降低
//                    lWei = (lWei*lWei + 64) >> 7;
                    lWei++;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;

                    lCof_A = pA[ k ][ lShif_Num + weiOffset1 ];
                    lCof_B = pB[ k ][ lShif_Num + weiOffset1 ];
                    lWei = 128 - lCof_A;
                    lWei = (lWei + lWei * lWei + 8 ) >> 4;
//                    lWei = (lWei*lWei + 64) >> 7;
                    lWei++;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;

                    lCof_A = pA[ k ][ lShif_Num ]; // 加上中心点
                    lCof_B = pB[ k ][ lShif_Num ];
                    lWei = 128 - lCof_A;
                    lWei = ( lWei + lWei * lWei + 8 ) >> 4;
//                    lWei = (lWei*lWei + 64) >> 7;
                    lWei++;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;
                }
#endif
                // 取值范围[0, 127]
                // fMean_a 越大 原来像素的值占比越大
                MInt32 fMean_a = 128;
                MInt32 fMean_b = lOffset;

                MInt32 lTempSumWei = lSumWei;
#if defined(DYNAMIC_DENOISE) //是否开启动态降噪，针对噪声不均匀
                MInt32 minA = pTempMinA[x];
                CLAMP(minA, 32, 64);

                lSumA = lSumA*32;
                lTempSumWei = lSumWei*minA;
                //fMean_a = fMean_a*32.0/minA+0.5;
#endif

                fMean_a = lSumWei > 0 ? (lSumA + (lTempSumWei >> 1)) / (lTempSumWei) : fMean_a;
                fMean_b = lSumWei > 0 ? (lSumB + (lSumWei >> 1)) / (lSumWei) : fMean_b;



                pCurFusA[ x ] = fMean_a;
                pCurFusB[ x ] = fMean_b;
            }

#if 0 // TODO：传入图像本身有填充，内部暂时关闭填充
            // 填充左右两边边缘
            for(int k = 0; k < nRadius; k++)
            {
                pCurFusA[ k ] = pCurFusA[ nRadius ];
                pCurFusB[ k ] = pCurFusB[ nRadius ];

                pCurFusA[ lWidth - 1 - k ] = pCurFusA[ lWidth - 1 - nRadius ];
                pCurFusB[ lWidth - 1 - k ] = pCurFusB[ lWidth - 1 - nRadius ];
            }
#endif
        }

#if 0 // TODO：传入图像本身有填充，内部暂时关闭填充
        if( startRow2 == nRadius )
        {
            for(int k = 0; k < nRadius; k++)
            {
                MInt32 size = lWidth * sizeof(T2);
                MMemCpy(pMeanA + k * lWidth, pMeanA + nRadius * lWidth, size);
                MMemCpy(pMeanB + k * lWidth, pMeanB + nRadius * lWidth, size);
            }
        }

        if( endRow2 == lHeight - nRadius )
        {
            for(int k = 0; k < nRadius; k++)
            {
                MInt32 size = lWidth * sizeof(T2);
                MMemCpy(pMeanA + ( lHeight - 1 - k ) * lWidth, pMeanA + ( lHeight - 1 - nRadius ) * lWidth, size);
                MMemCpy(pMeanB + ( lHeight - 1 - k ) * lWidth, pMeanB + ( lHeight - 1 - nRadius ) * lWidth, size);
            }
        }
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return MOK;
    }


    template<typename T, typename T0>
    template<typename T1, typename T2>
    MInt32 AnisotropicGuidedFiltering<T, T0>::MeanABThreads(T1 **pA,
                                                            T1 **pB,
                                                            T2 *pMeanA,
                                                            T2 *pMeanB,
                                                            MInt32 lWidth,
                                                            MInt32 lHeight)
    {
        MInt32 res = MOK;

        START_TIME;

        MBool isRunThreads = false;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        isRunThreads = ( lHeight > 64 ) && ( m_mcvParallelMonitor != MNull );
#endif


        if( isRunThreads )
        {
            /// 设置回调函数
            auto func_lamda = [](MVoid *HParam) -> MVoid
            {
                LAni_Guided_Filter_ST filter_sturct = ( LAni_Guided_Filter_ST ) HParam;

                MInt32 startRow = filter_sturct->startRow;
                MInt32 endRow = filter_sturct->endRow;

                T1 **pA = ( T1 ** ) filter_sturct->p1;
                T1 **pB = ( T1 ** ) filter_sturct->p2;
                T2 *pMeanA = ( T2 * ) filter_sturct->p3;
                T2 *pMeanB = ( T2 * ) filter_sturct->p4;
                MInt32 lWidth = filter_sturct->lWidth;
                MInt32 lHeight = filter_sturct->lHeight;

                AnisotropicGuidedFiltering<T, T0> *obj = ( AnisotropicGuidedFiltering<T, T0> * ) filter_sturct->obj;
                filter_sturct->lRet = obj->MeanAB<T1, T2>(pA, pB, pMeanA, pMeanB, lWidth, lHeight, startRow, endRow);
            };
            MVoid (*func)(MVoid *) = func_lamda;

            /// 设置参数
            MInt32 lTaskNum = m_nThreadCount > 0 ? m_nThreadCount : lHeight > 16 ? 16 : 8;
            lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = ( lTaskHeight >> 2 ) << 2;

            Ani_Guided_Filter_ST pParam[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].startRow = lTaskHeight * lnum;
                pParam[ lnum ].endRow = lTaskHeight * ( lnum + 1 );
            }
            pParam[ lTaskNum - 1 ].endRow = lHeight;


            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].obj = this;
                pParam[ lnum ].task_ID = lnum;
                //                pParam[ lnum ].hMemMgr = m_hMemMgr;

                pParam[ lnum ].p1 = ( MVoid * ) pA;
                pParam[ lnum ].p2 = ( MVoid * ) pB;
                pParam[ lnum ].p3 = ( MVoid * ) pMeanA;
                pParam[ lnum ].p4 = ( MVoid * ) pMeanB;
                pParam[ lnum ].lWidth = lWidth;
                pParam[ lnum ].lHeight = lHeight;
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
            res = MeanAB<T1, T2>(pA, pB, pMeanA, pMeanB, lWidth, lHeight, 0, lHeight);
        }

        exit:
        END_TIME;
        return res;
    }



    /// @brief
    /// @param hMemMgr
    /// @param mcvParallelMonitor
    /// @param pSrcImg
    /// @param FusCoff_A
    /// @param FusCoff_B
    /// @param Feps
    /// @return
    template<typename T, typename T0>
    template<typename T1, typename T2>
    MInt32 AnisotropicGuidedFiltering<T, T0>::GetMeanAB(MHandle hMemMgr,
                                                        MHandle mcvParallelMonitor,
                                                        T *pSrc,
                                                        MInt32 lWidth,
                                                        MInt32 lHeight,
                                                        MInt32 lPitch,
                                                        T0 *pGuide,
                                                        MInt32 lGuidePitch,
                                                        T2 *pMeanA,
                                                        T2 *pMeanB,
                                                        MFloat Feps)
    {
        MInt32 lret = MOK;

#ifdef BUILD_OPENCV0
        cv::Mat meanA(lHeight, lWidth, CV_16SC1, pMeanA);   
        cv::Mat meanB(lHeight, lWidth, CV_16SC1, pMeanB);
#endif
        // 根据图像的大小调整半径
#ifdef AUTO_RADIUS
        if (lWidth < 1500)
        {
            m_lRadius = 1;
        }
        else if (lWidth < 2500)
        {
            m_lRadius = 2;
        }
        else if (lWidth < 3000)
        {
            m_lRadius = 3;
        }
#endif

        MBool isNeedGuide = true;
        if(( char * ) pGuide == ( char * ) pSrc && lPitch == lGuidePitch )
        {
            isNeedGuide = false;
        }

        ///  申请存放 ab 值的内存
        T1 *pA[16] = {MNull};
        T1 *pB[16] = {MNull};
        for(MInt32 k = 0; k < m_lDirection; k++)
        {
            pA[ k ] = ( T1 * ) MMemAlloc(m_hMemMgr, lWidth * lHeight * sizeof(T1));
            pB[ k ] = ( T1 * ) MMemAlloc(m_hMemMgr, lWidth * lHeight * sizeof(T1));
            if( !pA[ k ] || !pB[ k ] )
            {
                lret = MERR_NO_MEMORY;
                goto exit;
            }
        }

        {
            //申请最小A存储空间
            m_pMinA = SAFE_MALLOC(m_hMemMgr, MInt16, lWidth * lHeight);
            if (m_pMinA == MNull)
            {
                lret = MERR_NO_MEMORY;
                goto exit;
            }
            MMemSet(m_pMinA, 127, lWidth * lHeight * sizeof(MInt16));
        }

        LOGD("isNeedGuide = %d\n", isNeedGuide);

        // 获取4个方向的 ab 值
        if( isNeedGuide )
        {
            m_lRadius = 4;
            GetABThreads<T1>(pSrc, lPitch, pGuide, lGuidePitch, pA, pB, lWidth, lHeight, Feps);
        }
        else
        {
            GetABThreads<T1>(pSrc, lPitch, pA, pB, lWidth, lHeight, Feps);
        }


        // 取ab均值
        lret = MeanABThreads<T1, T2>(pA, pB, pMeanA, pMeanB, lWidth, lHeight);
#ifdef BUILD_OPENCV0
        cv::boxFilter(meanA, meanA, -1, cv::Size(3, 3));
        cv::boxFilter(meanB, meanB, -1, cv::Size(3, 3));
#endif

#ifdef BUILD_OPENCV
        mat_write255(lHeight, lWidth, CV_16SC1, m_pMinA, "m_pMinA.png", 1.0);
        if (sizeof(T) > 1)
        {
            mat_write255(lHeight, lWidth, CV_16SC1, pSrc, "pSrc812.png", 1.0 / 4);
        }
        else
        {
            mat_write255(lHeight, lWidth, CV_8UC1, pSrc, "pSrc812.png", 1.0);
            mat_write255(lHeight, lPitch, CV_8UC1, pGuide, "pGuide812.png", 1.0);
        }

        for (int i = 0; i < m_lDirection; i++)
        {
            char dstFilename[100] = "";
            sprintf(dstFilename, "pA4[%d].png", i);
            mat_write(lHeight, lWidth, CV_16SC1, pA[i], dstFilename);
            sprintf(dstFilename, "pB4[%d].png", i);
            mat_write255(lHeight, lWidth, CV_16SC1, pB[i], dstFilename, 1.0);
    }
#endif
        exit:
        for(MInt32 k = 0; k < m_lDirection; k++)
        {
            SAFE_FREE_ARRAY(m_hMemMgr, pA[k]);
            SAFE_FREE_ARRAY(m_hMemMgr, pB[k]);
        }
        SAFE_FREE_ARRAY(m_hMemMgr, m_pMinA);

        return lret;
    }


    template<typename T, typename T0>
    template<typename T2>
    MInt32 AnisotropicGuidedFiltering<T, T0>::Mul_A_Plus_B(T *pSrc,
                                                           MInt32 lWidth,
                                                           MInt32 lHeight,
                                                           MInt32 lSrcPitch,
                                                           T *pDst,
                                                           MInt32 lDstPitch,
                                                           T2 *pMeanA,
                                                           T2 *pMeanB,
                                                           MInt32 startRow /*= -1*/,
                                                           MInt32 endRow /*= -1*/)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 max_val = sizeof(T)  > 1 ? 1023 : 255;
        //MInt32 min_val = sizeof(T)  > 1 ? -65534 : 0;

        MInt32 lRadius = MAX(lWidth, lHeight);

        for(MInt32 y = startRow; y < endRow; y++)
        {
            T *pCurSrc = pSrc + y * lSrcPitch;
            T *pCurDst = pDst + y * lDstPitch;
            T2 *pA = pMeanA + y * lWidth;
            T2 *pB = pMeanB + y * lWidth;

            MUInt8 *pTempShadeMapData = m_pShadeMapData + (y) * m_lShadeMapPitch;

            MInt32 x = 0;

#ifdef USE_NEON_GUIDED
            if (sizeof(T) == 1)
            {
				uint16x8_t const1_16x8 = vdupq_n_u16(1);
                uint16x8_t const230_16x8 = vdupq_n_u16(230);
                uint16x8_t const128_16x8 = vdupq_n_u16(128);
                uint16x8_t const13_16x8 = vdupq_n_u16(13);
                uint16x8_t const20_16x8 = vdupq_n_u16(20);
                uint16x8_t const255_16x8 = vdupq_n_u16(255);
				uint16x8_t constContrast_16x8 = vdupq_n_u16(m_lContrast);

                for (; x < lWidth - 8; x += 8)
                {
                    uint8x8_t  val_8x8 = vld1_u8((MUInt8*)pCurSrc + x);
                    uint16x8_t val_16x8 = vmovl_u8(val_8x8);
                    uint16x8_t  A_16x8 = vreinterpretq_u16_s16(vld1q_s16(pA + x));
                    uint16x8_t  B_16x8 = vreinterpretq_u16_s16(vld1q_s16(pB + x));

					uint8x8_t  shade_8x8 = vld1_u8((MUInt8*)pTempShadeMapData + x);

                    //对A进行调整
#if 0
                    //只对最大层加
                    A_16x8 = vmaxq_u16(A_16x8, constContrast_16x8);
					//加入天空mask
					A_16x8 = vrshlq_u16(A_16x8, vsubq_u16(const1_16x8, vshrq_n_u16(vmovl_u8(shade_8x8), 7)));
                    A_16x8 = vrshrq_n_u16(A_16x8, 1);
                    //高光
                    uint16x8_t cmp = vcgtq_u16(val_16x8, const230_16x8);
                    A_16x8 = vaddq_u16(vandq_u16(const128_16x8, cmp), vandq_u16(A_16x8, vmvnq_u16(cmp)));
                    //限制反差大的像素
                    uint16x8_t diff_16x8 = vabdq_u16(val_16x8, B_16x8);
                    cmp = vcgtq_u16(diff_16x8, const13_16x8);
                    A_16x8 = vaddq_u16(vandq_u16(vrshrq_n_u16(vmulq_n_u16(A_16x8, 3), 1), cmp), vandq_u16(A_16x8, vmvnq_u16(cmp)));
                    cmp = vcgtq_u16(diff_16x8, const20_16x8);
                    A_16x8 = vaddq_u16(vandq_u16(vrshrq_n_u16(vmulq_n_u16(A_16x8, 3), 1), cmp), vandq_u16(A_16x8, vmvnq_u16(cmp)));
#endif
                    A_16x8 = vminq_u16(A_16x8, const128_16x8);
                    val_16x8 = vrshrq_n_u16(vaddq_s16(vmulq_u16(val_16x8, A_16x8), vmulq_u16(B_16x8, vsubq_u16(const128_16x8, A_16x8))), 7);
                    val_16x8 = vminq_u16(val_16x8, const255_16x8);

                    vst1_u8((MUInt8*)pCurDst + x, vmovn_u16(val_16x8));
                }
            }
            else if (sizeof(T) == 2)
            {
				int32x4_t const1_32x4 = vdupq_n_s32(1);
                int32x4_t const13_32x4 = vdupq_n_s32(13);
                int32x4_t const20_32x4 = vdupq_n_s32(20);
                int32x4_t const128_32x4 = vdupq_n_s32(128);
                int32x4_t const1023_32x4 = vdupq_n_s32(1023);

                for (; x < lWidth - 4; x += 4)
                {
                    int16x4_t val_16x4 = vld1_s16((MInt16*)pCurSrc + x);
                    int16x4_t  A_16x4 = (vld1_s16(pA + x));
                    int16x4_t  B_16x4 = (vld1_s16(pB + x));

					uint8x8_t  shade_8x8 = vld1_u8((MUInt8*)pTempShadeMapData + x);
					uint16x8_t  shade_16x8 = vmovl_u8(shade_8x8);
					int32x4_t shade_32x4 = vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(shade_16x8)));

                    int32x4_t val_32x4 = vmovl_s16(val_16x4);
                    int32x4_t A_32x4 = vmovl_s16(A_16x4);
                    int32x4_t B_32x4 = vmovl_s16(B_16x4);

                    int32x4_t cmp;
#if 0 // 对于16bit数据先关闭A的调整
                    //调整A
                    //高光
                    cmp = vcgtq_s32(val_32x4, vdupq_n_s32(920));
                    A_32x4 = vaddq_s32(vandq_s32(vdupq_n_s32(128), cmp), vandq_s32(A_32x4, vmvnq_s32(cmp)));
#endif
					// 加入mask调整A
					shade_32x4 = vsubq_s32(const1_32x4, vshrq_n_s32(shade_32x4, 7));
					A_32x4 = vrshlq_s32(A_32x4, shade_32x4);
                    A_32x4 = vrshrq_n_s32(A_32x4, 1);
                    //限制反差大的像素
                    int32x4_t diff_32x4 = vabdq_s32(val_32x4, B_32x4);
                    cmp = vcgtq_s32(diff_32x4, const13_32x4);
                    A_32x4 = vaddq_s32(vandq_s32(vrshrq_n_s32(vmulq_n_s32(A_32x4, 3), 1), cmp), vandq_s32(A_32x4, vmvnq_s32(cmp)));
                    cmp = vcgtq_s32(diff_32x4, const20_32x4);
                    A_32x4 = vaddq_s32(vandq_s32(vrshrq_n_s32(vmulq_n_s32(A_32x4, 3), 1), cmp), vandq_s32(A_32x4, vmvnq_s32(cmp)));

                    A_32x4 = vminq_s32(A_32x4, const128_32x4);
                    val_32x4 = vrshrq_n_s32(vaddq_s32(vmulq_s32(val_32x4, A_32x4), vmulq_s32(B_32x4, vsubq_s32(const128_32x4, A_32x4))), 7);
					val_32x4 = vminq_s32(val_32x4, const1023_32x4);

                    vst1_s16((MInt16 *)pCurDst + x, vmovn_s32(val_32x4));
                }
            }

#endif

            for(; x < lWidth; x++)
            {
                MInt32 val = pCurSrc[ x ];
                MInt32 tempA = MIN(128, pA[ x ]);

#if 0 // 对A进行调整

                tempA = MAX(m_lContrast, tempA);
                tempA = pTempShadeMapData[x] == 0 ? tempA : tempA/2; // 将mask调节权重放到此处，增加线性
                //tempA = tempA / 2;
                tempA = val > 230 ? 128 : tempA; // 加入高光保护

                // 对于前后亮度相差太大的像素，进行调整
                MInt16 diff_val = ABS(val - pB[x]);//*1.0/MAX(1, val);
                MInt16 var_val = 20;//MAX(10, m_nEps*2);
                tempA = diff_val > var_val ? tempA*2 : (diff_val > var_val*0.666 ? tempA*1.5 : tempA);

                // 去除孤立点
                //tempA = diff_val > var_val*2 ? tempA/2 : tempA;

#if 0 // 暗角降噪增强
                MInt32 tempRadius = ABS(x - lWidth/2) + ABS(y - lHeight/2);
                tempA =  tempRadius > lRadius ? tempA*0.66 : (tempRadius > lRadius/2 ? tempA*0.75 : tempA);
#endif

#endif

                //MInt16 offset = val > 0 ? 64 : -64;

                tempA = MIN(128, tempA);
                //tempA = MAX(0, tempA);
                val = ( tempA * val + (128 - tempA) * pB[ x ] + 64) >> 7;
                val = val > max_val ? max_val : val;
                pCurDst[ x ] = val;
            }
        }
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return MOK;
    }


    template<typename T, typename T0>
    template<typename T2>
    MInt32 AnisotropicGuidedFiltering<T, T0>::Mul_A_Plus_B_Threads(T *pSrc,
                                                                   MInt32 lWidth,
                                                                   MInt32 lHeight,
                                                                   MInt32 lSrcPitch,
                                                                   T *pDst,
                                                                   MInt32 lDstPitch,
                                                                   T2 *pMeanA,
                                                                   T2 *pMeanB)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        LOGD("Mul_A_Plus_B_Threads++");
        MInt32 res = MOK;

        //        MInt32 lHeight = pSrcImg->lHeight;
        MBool isRunThreads = false;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        isRunThreads = ( lHeight > 64 ) && ( m_mcvParallelMonitor != MNull );
#endif

        if( isRunThreads )
        {
            /// 设置回调函数
            auto func_lamda = [](MVoid *HParam) -> MVoid
            {
                LAni_Guided_Filter_ST filter_sturct = ( LAni_Guided_Filter_ST ) HParam;

                MInt32 startRow = filter_sturct->startRow;
                MInt32 endRow = filter_sturct->endRow;

                T *pSrc = ( T * ) filter_sturct->p1;
                T *pDst = ( T * ) filter_sturct->p2;
                T2 *pMeanA = ( T2 * ) filter_sturct->p3;
                T2 *pMeanB = ( T2 * ) filter_sturct->p4;

                MInt32 lWidth = filter_sturct->lWidth;
                MInt32 lHeight = filter_sturct->lHeight;
                MInt32 lSrcPitch = filter_sturct->lPitchSrc;
                MInt32 lDstPitch = filter_sturct->lPitchGuided;

                AnisotropicGuidedFiltering<T, T0> *obj = ( AnisotropicGuidedFiltering<T, T0> * ) filter_sturct->obj;
                filter_sturct->lRet = obj->Mul_A_Plus_B(pSrc,
                                                        lWidth,
                                                        lHeight,
                                                        lSrcPitch,
                                                        pDst,
                                                        lDstPitch,
                                                        pMeanA,
                                                        pMeanB,
                                                        startRow, endRow);
            };
            MVoid (*func)(MVoid *) = func_lamda;



            /// 设置参数
            MInt32 lTaskNum = m_nThreadCount > 0 ? m_nThreadCount : lHeight > 16 ? 16 : 8;
            lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = ( lTaskHeight >> 2 ) << 2;

            Ani_Guided_Filter_ST pParam[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].startRow = lTaskHeight * lnum;
                pParam[ lnum ].endRow = lTaskHeight * ( lnum + 1 );
            }
            pParam[ lTaskNum - 1 ].endRow = lHeight;


            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].obj = this;
                pParam[ lnum ].task_ID = lnum;

                pParam[ lnum ].p1 = ( MVoid * ) pSrc;
                pParam[ lnum ].p2 = ( MVoid * ) pDst;
                pParam[ lnum ].p3 = ( MVoid * ) pMeanA;
                pParam[ lnum ].p4 = ( MVoid * ) pMeanB;
                pParam[ lnum ].lWidth = lWidth;
                pParam[ lnum ].lHeight = lHeight;

                pParam[ lnum ].lPitchSrc = lSrcPitch;
                pParam[ lnum ].lPitchGuided = lDstPitch;
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
            res = Mul_A_Plus_B(pSrc,
                               lWidth,
                               lHeight,
                               lSrcPitch,
                               pDst,
                               lDstPitch,
                               pMeanA,
                               pMeanB,
                               0, lHeight);
        }

        exit:
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("Mul_A_Plus_B_Threads--");
        return res;
    }



    template<typename T, typename T0>
    template<typename T1>
    MVoid AnisotropicGuidedFiltering<T, T0>::FillExpandPixels(T1 *pSrc,
                                                              MInt32 lWidth,
                                                              MInt32 lHeight,
                                                              MInt32 lPitch,
                                                              MInt32 lExpandSize)
    {

        for(MInt32 y = lExpandSize; y < lHeight - lExpandSize; y++)
        {
            T1 *pData = pSrc + y * lPitch;
            for(MInt32 k = 0; k < lExpandSize; k++)
            {
                pData[ k ] = pData[ lExpandSize ];
                pData[ lWidth - lExpandSize + k ] = pData[ lWidth - lExpandSize - 1 ];
            }
        }

        for(MInt32 y = 0; y < lExpandSize; y++)
        {
            MMemCpy(pSrc + y * lPitch,
                    pSrc + lExpandSize * lPitch, lPitch * sizeof(T1));
        }

        for(MInt32 y = lHeight - lExpandSize; y < lHeight; y++)
        {
            MMemCpy(pSrc + y * lPitch,
                    pSrc + ( y - lExpandSize - 1 ) * lPitch, lPitch * sizeof(T1));
        }
    }


    template<typename T, typename T0>
    MInt32 AnisotropicGuidedFiltering<T, T0>::Run(MHandle hMemMgr,
                                                  MHandle mcvParallelMonitor,
                                                  T *pSrc,
                                                  MInt32 lWidth,
                                                  MInt32 lHeight,
                                                  MInt32 lSrcPitch,
                                                  MFloat Feps,
                                                  MInt32 lScale)
    {
        return Run(hMemMgr, mcvParallelMonitor,
                   pSrc, ( T0 * ) pSrc,
                   lWidth, lHeight, lSrcPitch, lSrcPitch,
                   pSrc, lSrcPitch,
                   Feps, lScale);
    }

    /**
    * @brief
    * @param hMemMgr
    * @param mcvParallelMonitor
    * @param pSrcImg
    * @param pDstImg
    * @param Feps
    * @param lScale
    * @return
    */
    template<typename T, typename T0>
    MInt32 AnisotropicGuidedFiltering<T, T0>::Run(MHandle hMemMgr,
                                                  MHandle mcvParallelMonitor,
                                                  T *pSrc,
                                                  MInt32 lWidth,
                                                  MInt32 lHeight,
                                                  MInt32 lSrcPitch,
                                                  T *pDst,
                                                  MInt32 lDstPitch,
                                                  MFloat Feps,
                                                  MInt32 lScale)
    {
        return Run(hMemMgr, mcvParallelMonitor,
                   pSrc, ( T0 * ) pSrc,
                   lWidth, lHeight, lSrcPitch, lSrcPitch,
                   pDst, lDstPitch,
                   Feps, lScale);
    }


    /**
    * @brief
    * @param hMemMgr
    * @param mcvParallelMonitor
    * @param pSrcImg
    * @param pDstImg
    * @param Feps
    * @param lScale
    * @return
    */
    template<typename T, typename T0>
    MInt32 AnisotropicGuidedFiltering<T, T0>::Run(MHandle hMemMgr,
                                                  MHandle mcvParallelMonitor,
                                                  T *pSrc,
                                                  T0 *pGuide,
                                                  MInt32 lWidth,
                                                  MInt32 lHeight,
                                                  MInt32 lSrcPitch,
                                                  MInt32 lGuidePitch,
                                                  T *pDst,
                                                  MInt32 lDstPitch,
                                                  MFloat Feps,
                                                  MInt32 lScale)
    {
        START_TIME;
        MInt32 lret = MOK;
        if (Feps <= 0.0)
        {
            if (pSrc == pDst)
            {
                return 0;
            }
            else
            {
                CopyImageToImage<T>(pSrc, lWidth, lHeight, lSrcPitch, pDst, lDstPitch);
                return 0;
            }
        }

        m_lScale = lScale;
        m_lWidth = lWidth;
        m_lHeight = lHeight;
        m_nEps = (MInt16) Feps;

        LOGD("m_lWidth = %d", m_lWidth);
        LOGD("m_lScale = %d", m_lScale);

        MBool isNeedGuide = true;
        if ((char *) pSrc == (char *) pGuide && lSrcPitch == lGuidePitch)
        {
            isNeedGuide = false;
        }

        if (lScale != 2 && lScale != 4 && lScale != 1)
        {
            return MERR_UNSUPPORTED;
        }

        MInt32 lExpandSize = 8;
        MInt32 lSmWidth = lWidth / lScale;
        MInt32 lSmHeight = lHeight / lScale;
        MInt32 lSmWidthPad = lSmWidth + lExpandSize * 2;
        MInt32 lSmHeightPad = lSmHeight + lExpandSize * 2;
        MInt32 lSmPitchPad = ((lSmWidthPad + 7) >> 3) << 3;

        MInt16 *pSmallMeanAPad = {MNull};
        MInt16 *pSmallMeanBPad = {MNull};

        T *pSmallSrcImg = {MNull};
        T *pSmallSrcImgPad = {MNull};
        T0 *pSmallGuideImg = {MNull};
        T0 *pSmallGuideImgPad = {MNull};

        if (m_pMeanA == MNull || m_pMeanB == MNull) // 外部传入的话，就不需要计算该值了
        {
            // ==================================================
            // 申请内存
            // ==================================================
            /// Mean A B
            m_pMeanA = SAFE_MALLOC(hMemMgr, MInt16, lWidth * lHeight);
            m_pMeanB = SAFE_MALLOC(hMemMgr, MInt16, lWidth * lHeight);
            if (m_pMeanA == MNull || m_pMeanB == MNull) {
                lret = MERR_NO_MEMORY;
                LOGE("m_pMeanA == MNull || m_pMeanB == MNull!!");
                goto exit;
            }
        }

        if (m_AB_Type > 0)
        {
            /// small Mean A B
            pSmallMeanAPad = (MInt16 *) MMemAlloc(hMemMgr, lSmWidthPad * lSmHeightPad * sizeof(MInt16));
            pSmallMeanBPad = (MInt16 *) MMemAlloc(hMemMgr, lSmWidthPad * lSmHeightPad * sizeof(MInt16));
            m_pSmallMeanA = pSmallMeanAPad + lExpandSize * lSmWidthPad + lExpandSize;
            m_pSmallMeanB = pSmallMeanBPad + lExpandSize * lSmWidthPad + lExpandSize;
            if (!pSmallMeanAPad || !pSmallMeanBPad)
            {
                lret = MERR_NO_MEMORY;
                goto exit;
            }

            /// small src
            pSmallSrcImgPad = (T*)MMemAlloc(hMemMgr, lSmPitchPad * lSmHeightPad * sizeof(T));
            pSmallSrcImg = pSmallSrcImgPad + lExpandSize * lSmPitchPad + lExpandSize;
            if (pSmallSrcImgPad == MNull)
            {
                lret = MERR_NO_MEMORY;
                goto exit;
            }


            /// small Guide
            if (isNeedGuide)
            {
                pSmallGuideImgPad = (T0 *) MMemAlloc(hMemMgr, lSmPitchPad * lSmHeightPad * sizeof(T0));
                pSmallGuideImg = pSmallGuideImgPad + lExpandSize * lSmPitchPad + lExpandSize;
                if (pSmallGuideImgPad == MNull)
                {
                    lret = MERR_NO_MEMORY;
                    goto exit;
                }
            }


            // ==================================================
            // 缩放
            // ==================================================
            if (lScale == 1) // 原尺寸扩展
            {
                for (MInt32 y = 0; y < lHeight; y++)
                {
                    MMemCpy(pSmallSrcImg + (y) * lSmPitchPad, pSrc + y * lSrcPitch, lSrcPitch * sizeof(T));
                }

                if (isNeedGuide)
                {
                    for (MInt32 y = 0; y < lHeight; y++)
                    {
                        MMemCpy(pSmallGuideImg + (y) * lSmPitchPad, pGuide + y * lGuidePitch,
                                lGuidePitch * sizeof(T0));
                    }
                }
            }
            else
            {
                /// down src
                // MUInt8
                ScaleType_t type;
                if (lScale == 2)
                {
                    ImageInfo<MUInt8> srcImage((T*)pSrc, lWidth, lHeight, lSrcPitch);
                    ImageInfo<MUInt8> dstImage((T*)pSmallSrcImg, lSmWidth, lSmHeight, lSmPitchPad);
                    Guass3x3Down2Threads(hMemMgr, &srcImage, &dstImage);
                    if (isNeedGuide)
                    {
                        ImageInfo<MUInt8> srcImage1((T*)pGuide, lWidth, lHeight, lGuidePitch);
                        ImageInfo<MUInt8> dstImage1((T*)pSmallGuideImg, lSmWidth, lSmHeight, lSmPitchPad);
                        Guass3x3Down2Threads(hMemMgr, &srcImage1, &dstImage1);
                    }
                }
                else
                {
                    type = kMeanDown4;
                    Up_Down_Scale_Mean2x2_4x4<T>(hMemMgr, mcvParallelMonitor, m_nThreadCount).
                        Run((T*)pSrc, lWidth, lHeight, lSrcPitch,
                            (T*)pSmallSrcImg, lSmWidth, lSmHeight, lSmPitchPad,
                            1, type);

                    if (isNeedGuide)
                    {
                        Up_Down_Scale_Mean2x2_4x4<T0>(hMemMgr, mcvParallelMonitor, m_nThreadCount).
                            Run((T0*)pGuide, lWidth, lHeight, lGuidePitch,
                                (T0*)pSmallGuideImg, lSmWidth, lSmHeight, lSmPitchPad,
                                1, type);
                    }
                }  
            }

#ifdef BUILD_OPENCV
        if (sizeof(T) > 1)
        {
            mat_write255(lSmHeightPad, lSmPitchPad, CV_16SC1, pSmallSrcImgPad, "scale_src.png", 1.0 /4);
        }
        else
        {
            mat_write255(lSmHeightPad, lSmPitchPad, CV_8UC1, pSmallSrcImgPad, "scale_src.png", 1.0);
        }
#endif

            // ==================================================
            // 扩展
            // ==================================================
            FillExpandPixels<T>((T *) pSmallSrcImgPad,
                                lSmWidthPad,
                                lSmHeightPad,
                                lSmPitchPad,
                                lExpandSize);

            if (isNeedGuide)
            {
                FillExpandPixels<T0>((T0 *) pSmallGuideImgPad,
                                     lSmWidthPad,
                                     lSmHeightPad,
                                     lSmPitchPad,
                                     lExpandSize);
#ifdef BUILD_OPENCV
            if (sizeof(T0) > 1)
            {
                mat_write255(lSmHeightPad, lSmPitchPad, CV_16SC1, pSmallGuideImgPad, "scale_guidedFill.png", 1.0 / 4);
            }
            else
            {
                mat_write255(lSmHeightPad, lSmPitchPad, CV_8UC1, pSmallGuideImgPad, "scale_guidedFill.png", 1.0);
            }
#endif
            }

#ifdef BUILD_OPENCV
        if (sizeof(T) > 1)
        {
            mat_write255(lSmHeightPad, lSmPitchPad, CV_16SC1, pSmallSrcImgPad, "scale_srcFill.png", 1.0 * 16);
        }
        else
        {
            mat_write255(lSmHeightPad, lSmPitchPad, CV_8UC1, pSmallSrcImgPad, "scale_srcFill.png", 1.0);
        }
#endif
            // ==================================================
            // 计算 AB
            // ==================================================
            /// GetMeanAB
            if (isNeedGuide)
            {
                GetMeanAB<MInt16, MInt16>(hMemMgr,
                                          mcvParallelMonitor,
                                          (T *) pSmallSrcImgPad,
                                          lSmWidthPad,
                                          lSmHeightPad,
                                          lSmPitchPad,
                                          (T0 *) pSmallGuideImgPad,
                                          lSmPitchPad,
                                          pSmallMeanAPad,
                                          pSmallMeanBPad,
                                          Feps);
            }
            else
            {
                GetMeanAB<MInt16, MInt16>(hMemMgr,
                                          mcvParallelMonitor,
                                          (T *) pSmallSrcImgPad,
                                          lSmWidthPad,
                                          lSmHeightPad,
                                          lSmPitchPad,
                                          (T0 *) pSmallSrcImgPad,
                                          lSmPitchPad,
                                          pSmallMeanAPad,
                                          pSmallMeanBPad,
                                          Feps);
            }


#ifdef BUILD_OPENCV
        {
            char name[255];
            sprintf(name, "scale_A_%d.png", 0);
            mat_write255(lSmHeightPad, lSmWidthPad, CV_16SC1, pSmallMeanAPad, name, 1.0);
            if (sizeof(T) > 1)
            {
                sprintf(name, "scale_B_%d.png", 0);
                mat_write255(lSmHeightPad, lSmWidthPad, CV_16SC1, pSmallMeanBPad, name, 1.0*16);
            }
            else
            {
                sprintf(name, "scale_B_%d.png", 0);
                mat_write255(lSmHeightPad, lSmWidthPad, CV_16SC1, pSmallMeanBPad, name, 1.0);
            }

        }
#endif


            // ==================================================
            // A, B up
            // ==================================================
            if (lScale == 1)
            {
                for (MInt32 y = 0; y < lHeight; y++)
                {
                    MMemCpy(m_pMeanA + y * lWidth, m_pSmallMeanA + (y) * lSmWidthPad, lWidth * sizeof(MInt16));
                    MMemCpy(m_pMeanB + y * lWidth, m_pSmallMeanB + (y) * lSmWidthPad, lWidth * sizeof(MInt16));
                }
            }
            else
            {
                ScaleType_t type16;
                if (lScale == 2)
                {           
                    ImageInfo<MInt16> srcImage(m_pSmallMeanA, lSmWidth, lSmHeight, lSmWidthPad);
                    ImageInfo<MInt16> dstImage(m_pMeanA, lWidth, lHeight, lWidth);
                    Guass3x3Up2Threads<MInt16>(&srcImage, &dstImage);
                    
                    ImageInfo<MInt16> srcImage1(m_pSmallMeanB, lSmWidth, lSmHeight, lSmWidthPad);
                    ImageInfo<MInt16> dstImage1(m_pMeanB, lWidth, lHeight, lWidth);
                    Guass3x3Up2Threads<MInt16>(&srcImage1, &dstImage1);
                }
                else
                {
                    type16 = kBilinearUp4;
                    Up_Down_Scale_Mean2x2_4x4<MInt16>(hMemMgr, mcvParallelMonitor, m_nThreadCount).
                        Run(m_pSmallMeanA, lSmWidth, lSmHeight, lSmWidthPad,
                            m_pMeanA, lWidth, lHeight, lWidth,
                            1, type16);

                    Up_Down_Scale_Mean2x2_4x4<MInt16>(hMemMgr, mcvParallelMonitor, m_nThreadCount).
                        Run(m_pSmallMeanB, lSmWidth, lSmHeight, lSmWidthPad,
                            m_pMeanB, lWidth, lHeight, lWidth,
                            1, type16);
                }

            }
#ifdef BUILD_OPENCV
            mat_write255(lHeight, lWidth, CV_16SC1, m_pMeanA, "pMeanA1scale.jpg", 1.0);
            if (sizeof(T) > 1)
            {
                mat_write255(lHeight, lWidth, CV_16SC1, m_pMeanB, "pMeanB1scale.jpg", 1.0*16);
            }
            else
            {
                mat_write255(lHeight, lWidth, CV_16SC1, m_pMeanB, "pMeanB1scale.jpg", 1.0);
            }
#endif

        }
        // ==================================================
        // A PLUS B
        // ==================================================
        Mul_A_Plus_B_Threads(pSrc,
                             lWidth,
                             lHeight,
                             lSrcPitch,
                             pDst,
                             lDstPitch,
                             m_pMeanA,
                             m_pMeanB);

        exit:
        SAFE_FREE_ARRAY(hMemMgr, pSmallSrcImgPad);
        if (isNeedGuide)
        {
            SAFE_FREE_ARRAY(hMemMgr, pSmallGuideImgPad);
        }
        SAFE_FREE_ARRAY(hMemMgr, pSmallMeanAPad);
        SAFE_FREE_ARRAY(hMemMgr, pSmallMeanBPad);

        END_TIME;
        return lret;
    }


    template<typename T, typename T0>
    MVoid AnisotropicGuidedFiltering<T, T0>::CopyTo(MInt16 *pMeanA, MInt16 *pMeanB, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch)
    {
        if (lWidth == m_lWidth && lHeight == m_lHeight)
        {
            LOGI("lWidth == m_lWidth && lHeight == m_lHeight");
            MMemCpy(pMeanA, m_pMeanA, lWidth*lHeight*sizeof(MInt16));
            MMemCpy(pMeanB, m_pMeanB, lWidth*lHeight*sizeof(MInt16));
        }
        else
        {
            LOGI("lWidth != m_lWidth");
            LOGD("lWidth = %d", lWidth);
            LOGD("m_lWidth = %d", m_lWidth);
            for (MInt32 y = 0; y < lHeight; y++)
            {
                MMemCpy(pMeanA + y*lPitch, m_pMeanA + y*lWidth, lWidth*sizeof(MInt16));
                MMemCpy(pMeanB + y*lPitch, m_pMeanB + y*lWidth, lWidth*sizeof(MInt16));
            }
        }

    }

NS_SINFLE_IMAGE_ENHANCEMENT_END

