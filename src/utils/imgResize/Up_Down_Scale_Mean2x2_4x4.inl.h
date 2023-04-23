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
#include <type_traits>
#include <thread>
#include "ammem.h"
#include "merror.h"
#include "imagebase.h"
#include "mobilecv.h"
#include "ArcsoftLog.h"
#include "BasicTimer.h"
#include "Arcsoft_Copy_To_FilledImage.h"

#if defined(USE_NEON) || defined(__ARM_NEON__)
#define USE_NEON_SCALE
#endif

#ifdef USE_NEON_SCALE
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif


NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN


    typedef struct _tag_Image_Resize_ST
    {
        MVoid *obj;
        MInt32 task_ID;
        MHandle hMemMgr;
        MInt32 lRet;

        MInt32 startRow;
        MInt32 endRow;

        MVoid *pSrcImg;
        MInt32 lWidthSrc;
        MInt32 lHeigthSrc;
        MInt32 lPitchSrc;

        MVoid *pDstImg;
        MInt32 lWidthDst;
        MInt32 lHeightDst;
        MInt32 lPitchDst;

        //    _tag_Image_Resize_ST()
        //    {
        //        obj = MNull;
        //         task_ID = 0;
        //         hMemMgr = MNull;
        //         lRet = 0;
        //
        //         startRow = 0;
        //         endRow = 0;
        //
        //        pSrcImg = MNull;
        //         lWidthSrc = 0;
        //         lHeigthSrc = 0;
        //         lPitchSrc = 0;
        //
        //         pDstImg = MNull;
        //         lWidthDst = 0;
        //         lHeightDst = 0;
        //         lPitchDst = 0;
        //    }

    } Image_Resize_ST, *LImage_Resize_ST;


    template<typename T>
    Up_Down_Scale_Mean2x2_4x4<T>::Up_Down_Scale_Mean2x2_4x4(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 nThreadCount)
    {
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_nThreadCount = nThreadCount;
    }

    template<typename T>
    Up_Down_Scale_Mean2x2_4x4<T>::~Up_Down_Scale_Mean2x2_4x4()
    {

    }

    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Run(T *pSrc,
                                             MInt32 lWidthSrc,
                                             MInt32 lHeightSrc,
                                             MInt32 lPitchSrc,
                                             T *pDst,
                                             MInt32 lWidthDst,
                                             MInt32 lHeightDst,
                                             MInt32 lPitchDst,
                                             MInt32 cn,
                                             ScaleType_t type)
    {
        MInt32 ret = MOK;


        switch( type )
        {
            case kMeanDown2:
            {
                if( cn == 1 )
                {
                    ret = Mean_Down2_C1(pSrc, lWidthSrc, lHeightSrc, lPitchSrc, pDst, lWidthDst, lHeightDst, lPitchDst);
                }
                else
                {
                    ret = Mean_Down2_C2(pSrc, lWidthSrc, lHeightSrc, lPitchSrc, pDst, lWidthDst, lHeightDst, lPitchDst, 0, lHeightDst);
                }
                break;
            }
            case kMeanDown4:
            {
                if( cn == 1 )
                {
                    ret = Mean_Down4_C1(pSrc, lWidthSrc, lHeightSrc, lPitchSrc, pDst, lWidthDst, lHeightDst, lPitchDst);
                }
                else
                {
                    ret = Mean_Down4_C2(pSrc, lWidthSrc, lHeightSrc, lPitchSrc, pDst, lWidthDst, lHeightDst, lPitchDst, 0, lHeightDst);
                }
                break;
            }
            case kBilinearUp2:
            {
                if( cn == 1 )
                {
                    ret = Bilinear_Up2_C1_Threads(pSrc, lWidthSrc, lHeightSrc, lPitchSrc, pDst, lWidthDst, lHeightDst, lPitchDst);
                }
                break;
            }
            case kBilinearUp4:
            {
                if( cn == 1 )
                {
                    ret = Bilinear_Up4_C1_Threads(pSrc, lWidthSrc, lHeightSrc, lPitchSrc, pDst, lWidthDst, lHeightDst, lPitchDst);
                }
                break;
            }
            default:
                ret = -1;
                break;
        }

        return ret;
    }

    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Mean_Down2_C1(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                                                       T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst)
    {
        MInt32 nHeight = lDstHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)

		int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight > 16 ? 16 : 8;
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

            Mean_Down2_C1(pSrc, lSrcWidth, lSrcHeight, lPitchSrc,
                          pDst, lDstWidth, lDstHeight, lPitchDst, startHeight, endHeight);
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
        Mean_Down2_C1(pSrc, lSrcWidth, lSrcHeight, lPitchSrc,
                      pDst, lDstWidth, lDstHeight, lPitchDst, 0, nHeight);
#endif
        return 0;
    }

    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Mean_Down2_C1(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                                                       T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst,
                                                       MInt32 startRow,
                                                       MInt32 endRow)
    {
        for(MInt32 y = startRow; y < endRow; y++)
        {
            T *pSrc00 = pSrc + y * 2 * lPitchSrc;
            T *pSrc01 = pSrc00 + lPitchSrc;
            if(( y * 2 + 1 ) >= lSrcHeight )
            {
                pSrc01 = pSrc00;
            }

            T *pTmpDst = pDst + y * lPitchDst;

            MInt32 x = 0;
            MInt32 xSrc = 0;
#ifdef USE_NEON_SCALE

            if (sizeof(T) == 1)
            {
                for (;  x < lDstWidth - 7; x+=8, xSrc += 16)
                {
                    uint8x16_t vsrc0 = vld1q_u8((MUInt8*)pSrc00 + xSrc);
                    uint8x16_t vsrc1 = vld1q_u8((MUInt8*)pSrc01 + xSrc);

                    uint16x8_t vsum0 = vpaddlq_u8(vsrc0);
                    uint16x8_t vsum1 = vpaddlq_u8(vsrc1);

                    vsum0 = vaddq_u16(vsum0, vsum1);
                    vst1_u8((MUInt8*)pTmpDst + x, vrshrn_n_u16(vsum0, 2));
                }
            }
            else if (sizeof(T) == 2)
            {
                for (;  x < lDstWidth - 7; x+=8, xSrc += 16)
                {
                    int16x8x2_t vsrc0 = vld2q_s16((MInt16*)pSrc00 + xSrc);
                    int16x8x2_t vsrc1 = vld2q_s16((MInt16*)pSrc01 + xSrc);

                    int16x8_t vsum0 = vaddq_s16(vsrc0.val[0], vsrc0.val[1]);
                    int16x8_t vsum1 = vaddq_s16(vsrc1.val[0], vsrc1.val[1]);

                    vsum0 = vaddq_s16(vsum0, vsum1);
                    vst1q_s16((MInt16*)pTmpDst + x, vrshrq_n_s16(vsum0, 2));
                }
            }
#endif

            for(; x < lDstWidth; x++, xSrc += 2)
            {
                pTmpDst[ x ] = (( MFloat ) pSrc00[ xSrc ] + pSrc00[ xSrc + 1 ] + pSrc01[ xSrc ] + pSrc01[ xSrc + 1 ] + 2 ) / 4;
            }
        }
        return 0;
    }

    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Mean_Down2_C2(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                                                       T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst,
                                                       MInt32 startRow,
                                                       MInt32 endRow)
    {
        for(MInt32 y = startRow; y < endRow; y++)
        {
            T *pSrc00 = pSrc + y * 2 * lPitchSrc;
            T *pSrc01 = pSrc00 + lPitchSrc;
            if(( y * 2 + 1 ) >= lSrcHeight )
            {
                pSrc01 = pSrc00;
            }
            T *pTmpDst = pDst + y * lPitchDst;

            MInt32 x = 0;
            MInt32 xSrc = 0;

#ifdef USE_NEON_SCALE
            if (std::is_same<T, MUInt8>::value)
            {
                for (; x < lDstWidth * 2 - 15; x += 16, xSrc += 32)
                {
                    uint8x16x2_t vsrc0 = vld2q_u8((MUInt8*)pSrc00 + xSrc);
                    uint8x16x2_t vsrc1 = vld2q_u8((MUInt8*)pSrc01 + xSrc);

                    uint16x8_t vsumu0 = vpaddlq_u8(vsrc0.val[0]);
                    uint16x8_t vsumv0 = vpaddlq_u8(vsrc0.val[1]);
                    uint16x8_t vsumu1 = vpaddlq_u8(vsrc1.val[0]);
                    uint16x8_t vsumv1 = vpaddlq_u8(vsrc1.val[1]);

                    vsumu0 = vaddq_u16(vsumu0, vsumu1);
                    vsumv0 = vaddq_u16(vsumv0, vsumv1);

                    uint8x8x2_t vres;
                    vres.val[0] = vrshrn_n_u16(vsumu0, 2);
                    vres.val[1] = vrshrn_n_u16(vsumv0, 2);

                    vst2_u8((MUInt8*)pTmpDst + x, vres);
                }
            }
#endif
            for(; x < lDstWidth * 2; x += 2, xSrc += 4)
            {
                pTmpDst[ x ] = (( MFloat ) pSrc00[ xSrc + 0 ] + pSrc00[ xSrc + 2 ] + pSrc01[ xSrc + 0 ] + pSrc01[ xSrc + 2 ] + 2 ) / 4;
                pTmpDst[ x + 1 ] = (( MFloat ) pSrc00[ xSrc + 1 ] + pSrc00[ xSrc + 3 ] + pSrc01[ xSrc + 1 ] + pSrc01[ xSrc + 3 ] + 2 ) / 4;
            }
        }
        return 0;
    }

    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Mean_Down4_C1(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                                                       T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 nHeight = lDstHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)

        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : nHeight > 16 ? 16 : 8;
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

            Mean_Down4_C1(pSrc, lSrcWidth, lSrcHeight, lPitchSrc,
                          pDst, lDstWidth, lDstHeight, lPitchDst, startHeight, endHeight);
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
        Mean_Down4_C1(pSrc, lSrcWidth, lSrcHeight, lPitchSrc,
                      pDst, lDstWidth, lDstHeight, lPitchDst, 0, nHeight);
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return 0;
    }

    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Mean_Down4_C1(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                                                       T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst,
                                                       MInt32 startRow,
                                                       MInt32 endRow)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

        for(MInt32 y = startRow; y < endRow; y++)
        {
            T *pSrc00 = pSrc + y * 4 * lPitchSrc;
            T *pSrc01 = pSrc00 + lPitchSrc;
            T *pSrc02 = pSrc01 + lPitchSrc;
            T *pSrc03 = pSrc02 + lPitchSrc;
            T *pTmpDst = pDst + y * lPitchDst;

            MInt32 x = 0;
            MInt32 xSrc = 0;
#ifdef USE_NEON_SCALE
            if (sizeof(T) == 1)
            {
                for (; x < lDstWidth - 7; x += 8, xSrc += 32)
                {
                    uint8x8x4_t vData0, vData1, vData2, vData3;
                    uint16x8x4_t vTmp16;
                    uint16x8_t vRes;
                    vData0 = vld4_u8((MUInt8*)pSrc00 + xSrc);
                    vData1 = vld4_u8((MUInt8*)pSrc01 + xSrc);
                    vData2 = vld4_u8((MUInt8*)pSrc02 + xSrc);
                    vData3 = vld4_u8((MUInt8*)pSrc03 + xSrc);

                    vTmp16.val[0] = vaddl_u8(vData0.val[0], vData1.val[0]);
                    vTmp16.val[0] = vaddw_u8(vTmp16.val[0], vData2.val[0]);
                    vTmp16.val[0] = vaddw_u8(vTmp16.val[0], vData3.val[0]);

                    vTmp16.val[1] = vaddl_u8(vData0.val[1], vData1.val[1]);
                    vTmp16.val[1] = vaddw_u8(vTmp16.val[1], vData2.val[1]);
                    vTmp16.val[1] = vaddw_u8(vTmp16.val[1], vData3.val[1]);

                    vTmp16.val[2] = vaddl_u8(vData0.val[2], vData1.val[2]);
                    vTmp16.val[2] = vaddw_u8(vTmp16.val[2], vData2.val[2]);
                    vTmp16.val[2] = vaddw_u8(vTmp16.val[2], vData3.val[2]);

                    vTmp16.val[3] = vaddl_u8(vData0.val[3], vData1.val[3]);
                    vTmp16.val[3] = vaddw_u8(vTmp16.val[3], vData2.val[3]);
                    vTmp16.val[3] = vaddw_u8(vTmp16.val[3], vData3.val[3]);

                    vRes = vaddq_u16(vTmp16.val[0], vTmp16.val[1]);
                    vRes = vaddq_u16(vRes, vTmp16.val[2]);
                    vRes = vaddq_u16(vRes, vTmp16.val[3]);

                    vst1_u8((MUInt8*)pTmpDst + x, vqrshrn_n_u16(vRes, 4));
                }
            }
            else if (sizeof(T) == 2)
            {
				for (; x < lDstWidth - 7; x += 8, xSrc += 32)
				{
					int16x8x4_t vData0, vData1, vData2, vData3;
					int16x8x4_t vTmp16;
					int16x8_t vRes;
					vData0 = vld4q_s16((MInt16*)pSrc00 + xSrc);
					vData1 = vld4q_s16((MInt16*)pSrc01 + xSrc);
					vData2 = vld4q_s16((MInt16*)pSrc02 + xSrc);
					vData3 = vld4q_s16((MInt16*)pSrc03 + xSrc);

					vTmp16.val[0] = vaddq_s16(vData0.val[0], vData1.val[0]);
					vTmp16.val[0] = vaddq_s16(vTmp16.val[0], vData2.val[0]);
					vTmp16.val[0] = vaddq_s16(vTmp16.val[0], vData3.val[0]);
									
					vTmp16.val[1] = vaddq_s16(vData0.val[1], vData1.val[1]);
					vTmp16.val[1] = vaddq_s16(vTmp16.val[1], vData2.val[1]);
					vTmp16.val[1] = vaddq_s16(vTmp16.val[1], vData3.val[1]);
										
					vTmp16.val[2] = vaddq_s16(vData0.val[2], vData1.val[2]);
					vTmp16.val[2] = vaddq_s16(vTmp16.val[2], vData2.val[2]);
					vTmp16.val[2] = vaddq_s16(vTmp16.val[2], vData3.val[2]);
										
					vTmp16.val[3] = vaddq_s16(vData0.val[3], vData1.val[3]);
					vTmp16.val[3] = vaddq_s16(vTmp16.val[3], vData2.val[3]);
					vTmp16.val[3] = vaddq_s16(vTmp16.val[3], vData3.val[3]);

					vRes = vaddq_s16(vTmp16.val[0], vTmp16.val[1]);
					vRes = vaddq_s16(vRes, vTmp16.val[2]);
					vRes = vaddq_s16(vRes, vTmp16.val[3]);

					vst1q_s16((MInt16*)pTmpDst + x, vrshrq_n_s16(vRes, 4));
				}
            }
#endif

            for(; x < lDstWidth; x++, xSrc += 4)
            {
                MInt32 lVal = pSrc00[ xSrc ] + pSrc00[ xSrc + 1 ] + pSrc00[ xSrc + 2 ] + pSrc00[ xSrc + 3 ];
                lVal += pSrc01[ xSrc ] + pSrc01[ xSrc + 1 ] + pSrc01[ xSrc + 2 ] + pSrc01[ xSrc + 3 ];
                lVal += pSrc02[ xSrc ] + pSrc02[ xSrc + 1 ] + pSrc02[ xSrc + 2 ] + pSrc02[ xSrc + 3 ];
                lVal += pSrc03[ xSrc ] + pSrc03[ xSrc + 1 ] + pSrc03[ xSrc + 2 ] + pSrc03[ xSrc + 3 ];

                pTmpDst[ x ] = ( lVal + 8 ) >> 4;
            }
        }
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return 0;
    }


    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Mean_Down4_C2(T *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lPitchSrc,
                                                       T *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lPitchDst,
                                                       MInt32 startRow,
                                                       MInt32 endRow)
    {
        for(MInt32 y = startRow; y < endRow; y++)
        {
            T *pSrc00 = pSrc + y * 4 * lPitchSrc;
            T *pSrc01 = pSrc00 + lPitchSrc;
            T *pSrc02 = pSrc01 + lPitchSrc;
            T *pSrc03 = pSrc02 + lPitchSrc;
            T *pTmpDst = pDst + y * lPitchDst;
            MInt32 x = 0;
            MInt32 xSrc = 0;
#ifdef USE_NEON_SCALE
            if (std::is_same<T, MUInt8>::value)
            {
                for (; x < lDstWidth * 2 - 7; x += 8, xSrc += 32)
                {
                    uint8x16x2_t vsrc0_u8x16x2, vsrc1_u8x16x2, vsrc2_u8x16x2, vsrc3_u8x16x2;
                    uint16x8_t vsum2_u16x8, vtmp_u16x8;
                    uint16x4_t vsum4u_u16x4, vsum4v_u16x4;
                    uint16x4x2_t vsum4uv_u16x4x2;
                    vsrc0_u8x16x2 = vld2q_u8((MUInt8*)pSrc00 + xSrc);
                    vsrc1_u8x16x2 = vld2q_u8((MUInt8*)pSrc01 + xSrc);
                    vsrc2_u8x16x2 = vld2q_u8((MUInt8*)pSrc02 + xSrc);
                    vsrc3_u8x16x2 = vld2q_u8((MUInt8*)pSrc03 + xSrc);

                    // add up 4 rows
                    vsum2_u16x8 = vpaddlq_u8(vsrc0_u8x16x2.val[0]); // u0+u1 u2+u3 u4+u5 u6+u7.. u14+u15
                    vtmp_u16x8 = vpaddlq_u8(vsrc1_u8x16x2.val[0]);
                    vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
                    vtmp_u16x8 = vpaddlq_u8(vsrc2_u8x16x2.val[0]);
                    vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
                    vtmp_u16x8 = vpaddlq_u8(vsrc3_u8x16x2.val[0]);
                    vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
                    vsum4u_u16x4 = vpadd_u16(vget_low_u16(vsum2_u16x8), vget_high_u16(vsum2_u16x8)); // u0+u1+u2+u3 u4+u5+u6+u7 u8+u9+u10+u11 u12+u13+u14+u15

                    vsum2_u16x8 = vpaddlq_u8(vsrc0_u8x16x2.val[1]); // v0+v1 v2+v3 v4+v5 v6+v7.. v14+v15
                    vtmp_u16x8 = vpaddlq_u8(vsrc1_u8x16x2.val[1]);
                    vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
                    vtmp_u16x8 = vpaddlq_u8(vsrc2_u8x16x2.val[1]);
                    vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
                    vtmp_u16x8 = vpaddlq_u8(vsrc3_u8x16x2.val[1]);
                    vsum2_u16x8 = vaddq_u16(vsum2_u16x8, vtmp_u16x8);
                    vsum4v_u16x4 = vpadd_u16(vget_low_u16(vsum2_u16x8), vget_high_u16(vsum2_u16x8)); // v0+v1+v2+v3 v4+v5+v6+v7 v8+v9+v10+v11 v12+v13+v14+v15

                    vsum4uv_u16x4x2 = vzip_u16(vsum4u_u16x4, vsum4v_u16x4); // u v u v
                    vtmp_u16x8 = vcombine_u16(vsum4uv_u16x4x2.val[0], vsum4uv_u16x4x2.val[1]);

                    vst1_u8((MUInt8*)pTmpDst + x, vqrshrn_n_u16(vtmp_u16x8, 4));
                }
            }
#endif
            for(; x < lDstWidth * 2; x += 2, xSrc += 8)
            {
                MFloat sumU = 0, sumV = 0;
                sumU += ( MFloat ) pSrc00[ xSrc + 0 ] + pSrc00[ xSrc + 2 ] + pSrc00[ xSrc + 4 ] + pSrc00[ xSrc + 6 ];
                sumU += ( MFloat ) pSrc01[ xSrc + 0 ] + pSrc01[ xSrc + 2 ] + pSrc01[ xSrc + 4 ] + pSrc01[ xSrc + 6 ];
                sumU += ( MFloat ) pSrc02[ xSrc + 0 ] + pSrc02[ xSrc + 2 ] + pSrc02[ xSrc + 4 ] + pSrc02[ xSrc + 6 ];
                sumU += ( MFloat ) pSrc03[ xSrc + 0 ] + pSrc03[ xSrc + 2 ] + pSrc03[ xSrc + 4 ] + pSrc03[ xSrc + 6 ];

                sumV += ( MFloat ) pSrc00[ xSrc + 1 ] + pSrc00[ xSrc + 3 ] + pSrc00[ xSrc + 5 ] + pSrc00[ xSrc + 7 ];
                sumV += ( MFloat ) pSrc01[ xSrc + 1 ] + pSrc01[ xSrc + 3 ] + pSrc01[ xSrc + 5 ] + pSrc01[ xSrc + 7 ];
                sumV += ( MFloat ) pSrc02[ xSrc + 1 ] + pSrc02[ xSrc + 3 ] + pSrc02[ xSrc + 5 ] + pSrc02[ xSrc + 7 ];
                sumV += ( MFloat ) pSrc03[ xSrc + 1 ] + pSrc03[ xSrc + 3 ] + pSrc03[ xSrc + 5 ] + pSrc03[ xSrc + 7 ];

                pTmpDst[ x ] = ( sumU + 8 ) / 16;
                pTmpDst[ x + 1 ] = ( sumV + 8 ) / 16;
            }
        }
        return 0;
    }

    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Bilinear_Up2_C1(T *pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
                                                         T *pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst,
                                                         MInt32 startRow,
                                                         MInt32 endRow)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 newStartRow = MAX(0, startRow);
        MInt32 newEndRow = MIN(lHeightDst, endRow);
        
#if 1 // 采用定点，并优化流程
        newStartRow = startRow;//MAX(1, startRow);
        newEndRow = MIN(lHeightDst - 1, endRow);

        MInt16 *pTempDst = (MInt16*) MMemAlloc(MNull, (newEndRow - newStartRow + 1)*lPitchSrc* sizeof(MInt16));
        //MMemSet(pTempDst, 0, (newEndRow - newStartRow + 1)*lPitchSrc* sizeof(MInt16));

        //MBool isOdd = (newStartRow % 2); // 判断是否从奇数开始

        //if (isOdd) //从奇数开始 // 多线程分割处，强制从奇数开始
        {
            //先做列
            for(MInt32 y = newStartRow; y < newEndRow; y+=2)
            {
                auto *pTempTempDst = pTempDst + (y - newStartRow)*lPitchSrc;
                auto *pTempSrc = pSrcImg + (y)/2*lPitchSrc;
                MInt32 x = 0;

#ifdef USE_NEON_SCALE
                if (sizeof(T) == 2)
                {
					int16x4_t const3 = vdup_n_s16(3);

                    int16x4_t tempSrc0_16x4;
                    int16x4_t tempSrc1_16x4;

                    for (; x < lWidthSrc - 4; x += 4)
                    {
                        tempSrc0_16x4 = vld1_s16((MInt16*)pTempSrc + x);
                        tempSrc1_16x4 = vld1_s16((MInt16*)pTempSrc + x + lPitchSrc);

                        int16x4_t val_16x4 = (vadd_s16(vmul_s16((tempSrc0_16x4), const3), (tempSrc1_16x4)));
                        vst1_s16(pTempTempDst + x, val_16x4);

                        val_16x4           = (vadd_s16(vmul_s16((tempSrc1_16x4), const3), (tempSrc0_16x4)));
                        vst1_s16(pTempTempDst + x + lPitchSrc, val_16x4);
                    }
                }
#endif

                for(; x < lWidthSrc; x++)
                {
                    pTempTempDst[x]             = (MInt16)(pTempSrc[x])*3 + (MInt16)(pTempSrc[x + lPitchSrc])*1;
                    pTempTempDst[x + lPitchSrc] = (MInt16)(pTempSrc[x])*1 + (MInt16)(pTempSrc[x + lPitchSrc])*3;
                }
            }
        }
//        else //从偶数开始的行
//        {
//            //先做列
//            for(MInt32 y = newStartRow; y < newEndRow; y+=2)
//            {
//                auto *pTempTempDst = pTempDst + (y - newStartRow)*lPitchSrc;
//                auto *pTempSrc = pSrcImg + (y)/2*lPitchSrc;
//                MInt32 x = 0;
//
//#ifdef USE_NEON_SCALE
//                if (sizeof(T) == 2)
//                {
//                    int16x4_t tempSrc0_16x4;
//                    int16x4_t tempSrc1_16x4;
//                    int16x4_t tempSrc2_16x4;
//
//                    for (; x < lWidthSrc - 4; x += 4)
//                    {
//                        tempSrc0_16x4 = vld1_s16((MInt16*)pTempSrc + x);
//                        tempSrc1_16x4 = vld1_s16((MInt16*)pTempSrc + x - lPitchSrc);
//                        tempSrc2_16x4 = vld1_s16((MInt16*)pTempSrc + x + lPitchSrc);
//
//                        int16x4_t val_16x4 = (vadd_s16(vmul_n_s16((tempSrc0_16x4), 3), (tempSrc1_16x4)));
//                        vst1_s16(pTempTempDst + x, val_16x4);
//
//                        val_16x4           = (vadd_s16(vmul_n_s16((tempSrc0_16x4), 3), (tempSrc2_16x4)));
//                        vst1_s16(pTempTempDst + x + lPitchSrc, val_16x4);
//                    }
//                }
//#endif
//                for(; x < lWidthSrc; x++)
//                {
//                    pTempTempDst[x]             = (MInt16)(pTempSrc[x])*3 + (MInt16)(pTempSrc[x - lPitchSrc])*1;
//                    pTempTempDst[x + lPitchSrc] = (MInt16)(pTempSrc[x])*3 + (MInt16)(pTempSrc[x + lPitchSrc])*1;
//                }
//            }
//        }

        //后做行
        for(MInt32 y = newStartRow; y < newEndRow; y++)
        {
            auto *pTempTempDst = pTempDst + (y - newStartRow)*lPitchSrc;
            auto *pNewDstImg = pDstImg + (y)*lPitchDst;

            MInt32 x = 1;

#ifdef USE_NEON_SCALE

            if (sizeof(T) == 2) //10bit
            {
				int16x4_t const3 = vdup_n_s16(3);

                int16x4_t tempDst0_16x4;
                int16x4_t tempDst1_16x4;

                for(; x < lWidthDst - 1 - 8; x+=8)
                {
                    tempDst0_16x4 = vld1_s16(pTempTempDst + (x>>1));
                    tempDst1_16x4 = vld1_s16(pTempTempDst + (x>>1) + 1);
                    int16x4x2_t val_16x4x2;

                    int16x4_t val_16x4 = vadd_s16(vmul_s16(tempDst0_16x4, const3), tempDst1_16x4);
                    val_16x4 = vrshr_n_s16(val_16x4, 4);
                    val_16x4x2.val[0] = val_16x4;

                    val_16x4 = vadd_s16(tempDst0_16x4, vmul_s16(tempDst1_16x4, const3));
                    val_16x4 = vrshr_n_s16(val_16x4, 4);
                    val_16x4x2.val[1] = val_16x4;


                    vst2_s16((MInt16*)pNewDstImg + x, val_16x4x2);
                }
            }

#endif
            for(; x < lWidthDst - 1; x+=2)
            {
                MInt16 val = ((MInt16)pTempTempDst[x/2])*3 + ((MInt16)pTempTempDst[x/2+1])*1;
                val = (val + 8) >> 4;
                pNewDstImg[x] = val;

                val = ((MInt16)pTempTempDst[x/2])*1 + ((MInt16)pTempTempDst[x/2+1])*3;
                val = (val + 8) >> 4;
                pNewDstImg[x+1] = val;
            }

            pNewDstImg[0] = pNewDstImg[1];
            pNewDstImg[lWidthDst - 1] = pNewDstImg[lWidthDst - 2];
        }

        if( startRow == 1 )
        {
            MMemCpy(pDstImg, pDstImg + lPitchDst, lWidthDst * sizeof(T));
        }

        if( endRow == lHeightDst - 1)
        {
            MMemCpy(pDstImg + (lHeightDst-1)*lPitchDst, pDstImg + (lHeightDst-2)*lPitchDst, lWidthDst * sizeof(T));
        }


        SAFE_FREE_ARRAY(MNull, pTempDst);
#else // 之前的实现，采用浮点数
        for(MInt32 y = startRow; y < endRow; y++)
        {
            MFloat interRow = -0.25f + y * 0.5f;
            MInt32 row0, row1;
            MFloat wy;
            if( interRow < 0 )
            {
                row0 = row1 = 0;
                wy = 0;
            }
            else if( interRow > lHeigthSrc - 1 )
            {
                row0 = row1 = lHeigthSrc - 1;
                wy = 0;
            }
            else
            {
                row0 = MInt32(interRow);
                row1 = row0 + 1;
                wy = interRow - row0;
            }

            T *pCurDst = pDstImg + y * lPitchDst;
            T *pCurSrc0 = pSrcImg + row0 * lPitchSrc;
            T *pCurSrc1 = pSrcImg + row1 * lPitchSrc;

            for(MInt32 x = 0; x < lWidthDst; x++)
            {
                MFloat interCol = -0.25f + x * 0.5f;
                MInt32 col0, col1;
                MFloat wx;
                if( interCol < 0 )
                {
                    col0 = col1 = 0;
                    wx = 0;
                }
                else if( interCol > lWidthSrc - 1 )
                {
                    col0 = col1 = lWidthSrc - 1;
                    wx = 0;
                }
                else
                {
                    col0 = MInt32(interCol);
                    col1 = col0 + 1;
                    wx = interCol - col0;
                }

                MFloat val0, val1, val2, val3;
                val0 = pCurSrc0[ col0 ];
                val1 = pCurSrc0[ col1 ];
                val2 = pCurSrc1[ col0 ];
                val3 = pCurSrc1[ col1 ];

                val0 = val0 * ( 1 - wx ) + val1 * wx;
                val2 = val2 * ( 1 - wx ) + val3 * wx;

                pCurDst[ x ] = ( val0 * ( 1 - wy ) + val2 * wy );
            }
        }
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return 0;
    }

    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Bilinear_Up2_C1_Threads(T *pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
                                                                 T *pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst)
    {
#ifdef USE_NEON_SCALE
        LOGD("USE_NEON_SCALE!");
#endif

#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 res = MOK;
        MInt32 lHeight = lHeightDst;

        MBool isRunThreads = false;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        isRunThreads = ( lHeight > 64 ) && ( m_mcvParallelMonitor != MNull );
#endif

        if( isRunThreads )
        {
            /// 设置回调函数
            auto func_lamda = [](MVoid *HParam) -> MVoid
            {
                LImage_Resize_ST filter_sturct = ( LImage_Resize_ST ) HParam;

                MInt32 startRow = filter_sturct->startRow;
                MInt32 endRow = filter_sturct->endRow;

                T *pSrcImg = ( T * ) filter_sturct->pSrcImg;
                MInt32 lWidthSrc = filter_sturct->lWidthSrc;
                MInt32 lHeigthSrc = filter_sturct->lHeigthSrc;
                MInt32 lPitchSrc = filter_sturct->lPitchSrc;
                T *pDstImg = ( T * ) filter_sturct->pDstImg;
                MInt32 lWidthDst = filter_sturct->lWidthDst;
                MInt32 lHeightDst = filter_sturct->lHeightDst;
                MInt32 lPitchDst = filter_sturct->lPitchDst;

                Up_Down_Scale_Mean2x2_4x4<T> *obj = ( Up_Down_Scale_Mean2x2_4x4<T> * ) filter_sturct->obj;
                filter_sturct->lRet = obj->Bilinear_Up2_C1(pSrcImg, lWidthSrc, lHeigthSrc, lPitchSrc,
                                                           pDstImg, lWidthDst, lHeightDst, lPitchDst, startRow, endRow);
            };
            MVoid (*func)(MVoid *) = func_lamda;



            /// 设置参数
            MInt32 lTaskNum = m_nThreadCount > 0 ? m_nThreadCount : lHeight > 16 ? 16 : 8;
            lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = (( lTaskHeight >> 2 ) << 2);

            Image_Resize_ST pParam[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].startRow = lTaskHeight * lnum + 1;
                pParam[ lnum ].endRow = lTaskHeight * ( lnum + 1 ) + 1;
            }
            pParam[ lTaskNum - 1 ].endRow = lHeight;


            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].obj = this;
                pParam[ lnum ].task_ID = lnum;
                //                pParam[ lnum ].hMemMgr = m_hMemMgr;


                pParam[ lnum ].pSrcImg = ( MVoid * ) pSrcImg;
                pParam[ lnum ].lWidthSrc = lWidthSrc;
                pParam[ lnum ].lHeigthSrc = lHeigthSrc;
                pParam[ lnum ].lPitchSrc = lPitchSrc;

                pParam[ lnum ].pDstImg = ( MVoid * ) pDstImg;
                pParam[ lnum ].lWidthDst = lWidthDst;
                pParam[ lnum ].lHeightDst = lHeightDst;
                pParam[ lnum ].lPitchDst = lPitchDst;
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
            res = Bilinear_Up2_C1(pSrcImg, lWidthSrc, lHeigthSrc, lPitchSrc,
                                  pDstImg, lWidthDst, lHeightDst, lPitchDst, 0,lHeightDst );
        }

        exit:
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return res;
    }


	///////////////////////////////////////////////////////
	// 均值上采样4倍点的分布如下
	// 。。 。。| 。。 。。
	// 。。 。。| 。。 。。
	//	   *    |    *    
	// 。。 。。| 。。 。。
	// 。。 。。| 。。 。。
	//////////////////////////////////////////////////////
#if defined(__ANDROID__) || defined(ANDROID)
    template<typename T>
    inline MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Bilinear_Up4_C1(T *pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
                                                                T *pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst,
                                                                MInt32 startRow, MInt32 endRow)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 newStartRow = startRow;
        MInt32 newEndRow =   endRow;
#ifdef USE_NEON_SCALE
        const int offset = 512;
        uint16x4_t v7531,v1357,v3,v5,v7,v32;
        int16x4_t v512;
#if 1
        v7531[0] = v1357[3] = 7;
        v7531[1] = v1357[2] = 5;
        v7531[2] = v1357[1] = 3;
        v7531[3] = v1357[0] = 1;
#else
        unsigned short t1357[] = {1,3,5,7};
        unsigned short t7531[] = {7,5,3,1};
        v1357 = vld1_u16(t1357);
        v7531 = vld1_u16(t7531);
#endif
        v3 = vdup_n_u16(3);
        v5 = vdup_n_u16(5);
        v7 = vdup_n_u16(7);
        v32 = vdup_n_u16(32);
        v512 = vdup_n_s16(offset);
#endif
            for(MInt32 y = newStartRow; y < newEndRow; y+=4)
            {
                int yd4 = y>>2;
                int yd4_1 = yd4 + 1 >= lHeigthSrc?lHeigthSrc-1:yd4 + 1;
                auto *pTempSrc0 = pSrcImg + yd4*lPitchSrc;
                auto *pTempSrc1 = pSrcImg + yd4_1*lPitchSrc;
                auto *pTempDst = pDstImg + y*lPitchDst;
                MInt32 x,sx,sx1;
                x = sx = 0;
                x = 2;
#ifdef USE_NEON_SCALE
                for(; x < lWidthDst - 3  ; x+=4 ,sx++)
                {
                    sx1 = sx + 1 >= lWidthSrc?lWidthSrc-1:sx + 1;
                    uint16x4_t vs00,vs01,vs10,vs11,vr0,vr1;
                    uint16x4_t vd0,vd1,vd2,vd3;
                    vs00 = vdup_n_u16(pTempSrc0[sx]+offset);
                    vs01 = vdup_n_u16(pTempSrc0[sx1]+offset);
                    vs10 = vdup_n_u16(pTempSrc1[sx]+offset);
                    vs11 = vdup_n_u16(pTempSrc1[sx1]+offset);

                    vr0 = vmul_u16(vs00,v7531);
                    vr0 = vmla_u16(vr0,vs01,v1357);
                    vr1 = vmul_u16(vs10,v7531);
                    vr1 = vmla_u16(vr1,vs11,v1357);

                    vd0 = vadd_u16(vr1,vmul_u16(vr0,v7));
                    vd1 = vadd_u16(vmul_u16(vr1,v3),vmul_u16(vr0,v5));
                    vd2 = vadd_u16(vmul_u16(vr1,v5),vmul_u16(vr0,v3));
                    vd3 = vadd_u16(vmul_u16(vr1,v7),vr0);

                    vd0 = vshr_n_u16(vadd_u16(vd0,v32),6);
                    vd1 = vshr_n_u16(vadd_u16(vd1,v32),6);
                    vd2 = vshr_n_u16(vadd_u16(vd2,v32),6);
                    vd3 = vshr_n_u16(vadd_u16(vd3,v32),6);

                    vst1_s16((MInt16*)pTempDst +x              ,vsub_s16((int16x4_t)vd0,v512));
                    vst1_s16((MInt16*)pTempDst +x + lPitchDst  ,vsub_s16((int16x4_t)vd1,v512));
                    vst1_s16((MInt16*)pTempDst +x + 2*lPitchDst,vsub_s16((int16x4_t)vd2,v512));
                    vst1_s16((MInt16*)pTempDst +x + 3*lPitchDst,vsub_s16((int16x4_t)vd3,v512));
                }
#endif
                for(; x < lWidthDst; x+=4 ,sx++)
                {
                    sx1 = sx + 1 >= lWidthSrc?lWidthSrc-1:sx + 1;
                    // row
                    int vr0_0 = pTempSrc0[sx]*7 + pTempSrc0[sx1] ;
                    int vr0_1 = pTempSrc0[sx]*5 + pTempSrc0[sx1]*3;
                    int vr0_2 = pTempSrc0[sx]*3 + pTempSrc0[sx1]*5;
                    int vr0_3 = pTempSrc0[sx]   + pTempSrc0[sx1]*7;

                    int vr1_0 = pTempSrc1[sx]*7 + pTempSrc1[sx1] ;
                    int vr1_1 = pTempSrc1[sx]*5 + pTempSrc1[sx1]*3;
                    int vr1_2 = pTempSrc1[sx]*3 + pTempSrc1[sx1]*5;
                    int vr1_3 = pTempSrc1[sx]   + pTempSrc1[sx1]*7;

                    // col
                    int ret[16];
                    int wei[4] = {7,5,3,1};
                    for(int k = 0 ; k < 4 ;k++)
                    {
                        ret[k*4 + 0] = vr0_0*wei[k] + vr1_0*wei[3-k];
                        ret[k*4 + 1] = vr0_1*wei[k] + vr1_1*wei[3-k];
                        ret[k*4 + 2] = vr0_2*wei[k] + vr1_2*wei[3-k];
                        ret[k*4 + 3] = vr0_3*wei[k] + vr1_3*wei[3-k];
                    }

                    // mean
                    for(int i = 0 ; i < 16;i++)
                        ret[i] = (ret[i]+32)>>6;

                    // st
                    for(int k = 0 ; k < 4 ;k++)
                    {
                        auto *pdst = pTempDst + k*lPitchDst;
                        for(int i = 0 ; i < 4 ;i++)
                            pdst[x+i] = ret[i+k*4];
                    }
                }

                int offsetX[4] = {0,lPitchDst,2*lPitchDst,3*lPitchDst};
                pTempDst[offsetX[0]] =  pTempDst[offsetX[0] + 1] = pTempDst[offsetX[0] + 2];
                pTempDst[offsetX[1]] =  pTempDst[offsetX[1] + 1] = pTempDst[offsetX[1] + 2];
                pTempDst[offsetX[2]] =  pTempDst[offsetX[2] + 1] = pTempDst[offsetX[2] + 2];
                pTempDst[offsetX[3]] =  pTempDst[offsetX[3] + 1] = pTempDst[offsetX[3] + 2];
        }
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return 0;
    }

#else

    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Bilinear_Up4_C1(T *pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
                                                         T *pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst,
                                                         MInt32 startRow, MInt32 endRow)
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif

#if 1 // 采用定点，并优化流程
        MInt32 newStartRow = startRow;//MAX(2, startRow);
        MInt32 newEndRow = MIN(lHeightDst - 2, endRow);

        MInt32 *pTempDst = (MInt32*)MMemAlloc(MNull, (newEndRow - newStartRow + 3)*lPitchSrc* sizeof(MInt32));

        //MInt16 nStarLine = (newStartRow % 4); // 判断是否从奇数开始

        // 都是4的倍数，在多线程分割时，强制开始值都是2开始
        //if (nStarLine == 2)
        {
            //先做列
            for(MInt32 y = newStartRow; y < newEndRow; y+=4)
            {
                auto *pTempTempDst = pTempDst + (y - newStartRow)*lPitchSrc;
                auto *pTempSrc = pSrcImg + (y>>2)*lPitchSrc;
                MInt32 x = 0;

#ifdef USE_NEON_SCALE
               if (sizeof(T) == 2)
                {
				   int16x4_t const7 = vdup_n_s16(7);
				   int16x4_t const5 = vdup_n_s16(5);
				   int16x4_t const3 = vdup_n_s16(3);

                    int16x4_t tempSrc0_16x4;
                    int16x4_t tempSrc1_16x4;

                    int32x4_t val_32x4;

                    for (; x < lWidthSrc - 4; x += 4)
                    {
                        tempSrc0_16x4 = vld1_s16((MInt16*)pTempSrc + x);
                        tempSrc1_16x4 = vld1_s16((MInt16*)pTempSrc + x + lPitchSrc);

                        val_32x4 = vaddl_s16(vmul_s16((tempSrc0_16x4), const7), (tempSrc1_16x4));
                        vst1q_s32(pTempTempDst + x, val_32x4);

                        val_32x4 = vaddl_s16(vmul_s16(tempSrc0_16x4, const5), vmul_s16(tempSrc1_16x4, const3));
                        vst1q_s32(pTempTempDst + x + 1*lPitchSrc, val_32x4);

                        val_32x4 = vaddl_s16(vmul_s16(tempSrc0_16x4, const3), vmul_s16(tempSrc1_16x4, const5));
                        vst1q_s32(pTempTempDst + x + 2*lPitchSrc, val_32x4);

                        val_32x4 = vaddl_s16(tempSrc0_16x4, vmul_s16(tempSrc1_16x4, const7));
                        vst1q_s32(pTempTempDst + x + 3*lPitchSrc, val_32x4);
                    }
                }
#endif
                for(; x < lWidthSrc; x++)
                {
                    pTempTempDst[x + 0*lPitchSrc] = (MInt32)(pTempSrc[x])*7 + (MInt32)(pTempSrc[x+lPitchSrc])*1;
                    pTempTempDst[x + 1*lPitchSrc] = (MInt32)(pTempSrc[x])*5 + (MInt32)(pTempSrc[x+lPitchSrc])*3;
                    pTempTempDst[x + 2*lPitchSrc] = (MInt32)(pTempSrc[x])*3 + (MInt32)(pTempSrc[x+lPitchSrc])*5;
                    pTempTempDst[x + 3*lPitchSrc] = (MInt32)(pTempSrc[x])*1 + (MInt32)(pTempSrc[x+lPitchSrc])*7;
                }
            }
        }
//        else if (nStarLine == 3)
//        {
//            //先做列
//            for(MInt32 y = newStartRow; y < newEndRow; y+=4)
//            {
//                auto *pTempTempDst = pTempDst + (y - newStartRow)*lPitchSrc;
//                auto *pTempSrc = pSrcImg + (y>>2)*lPitchSrc;
//                MInt32 x = 0;
//
//#ifdef USE_NEON_SCALE
//                if (sizeof(T) == 2)
//                {
//                    int16x4_t tempSrc0_16x4;
//                    int16x4_t tempSrc1_16x4;
//
//                    int32x4_t val_32x4;
//
//                    for (; x < lWidthSrc - 4; x += 4)
//                    {
//                        tempSrc0_16x4 = vld1_s16((MInt16*)pTempSrc + x);
//                        tempSrc1_16x4 = vld1_s16((MInt16*)pTempSrc + x + lPitchSrc);
//
//                        val_32x4 = (vaddl_s16(vmul_n_s16((tempSrc0_16x4), 7), (tempSrc1_16x4)));
//                        vst1q_s32(pTempTempDst + x + 3*lPitchSrc, val_32x4);
//
//                        val_32x4 = vaddl_s16(vmul_n_s16(tempSrc0_16x4, 5), vmul_n_s16(tempSrc1_16x4, 3));
//                        vst1q_s32(pTempTempDst + x, val_32x4);
//
//                        val_32x4 = vaddl_s16(vmul_n_s16(tempSrc0_16x4, 3), vmul_n_s16(tempSrc1_16x4, 5));
//                        vst1q_s32(pTempTempDst + x + 1*lPitchSrc, val_32x4);
//
//                        val_32x4 = vaddl_s16(tempSrc0_16x4, vmul_n_s16(tempSrc1_16x4, 7));
//                        vst1q_s32(pTempTempDst + x + 2*lPitchSrc, val_32x4);
//                    }
//                }
//#endif
//                for(; x < lWidthSrc; x++)
//                {
//                    pTempTempDst[x + 3*lPitchSrc] = (MInt32)(pTempSrc[x])*7 + (MInt32)(pTempSrc[x+lPitchSrc])*1;
//                    pTempTempDst[x + 0*lPitchSrc] = (MInt32)(pTempSrc[x])*5 + (MInt32)(pTempSrc[x+lPitchSrc])*3;
//                    pTempTempDst[x + 1*lPitchSrc] = (MInt32)(pTempSrc[x])*3 + (MInt32)(pTempSrc[x+lPitchSrc])*5;
//                    pTempTempDst[x + 2*lPitchSrc] = (MInt32)(pTempSrc[x])*1 + (MInt32)(pTempSrc[x+lPitchSrc])*7;
//                }
//            }
//        }
//        else if (nStarLine == 0)
//        {
//            //先做列
//            for(MInt32 y = newStartRow; y < newEndRow; y+=4)
//            {
//                auto *pTempTempDst = pTempDst + (y - newStartRow)*lPitchSrc;
//                auto *pTempSrc = pSrcImg + (y>>2)*lPitchSrc;
//                MInt32 x = 0;
//
//#ifdef USE_NEON_SCALE
//                if (sizeof(T) == 2)
//                {
//                    int16x4_t tempSrc0_16x4;
//                    int16x4_t tempSrc1_16x4;
//                    int16x4_t tempSrc2_16x4;
//
//                    int32x4_t val_32x4;
//
//                    for (; x < lWidthSrc - 4; x += 4)
//                    {
//                        tempSrc0_16x4 = vld1_s16((MInt16*)pTempSrc + x);
//                        tempSrc1_16x4 = vld1_s16((MInt16*)pTempSrc + x + lPitchSrc);
//                        tempSrc2_16x4 = vld1_s16((MInt16*)pTempSrc + x - lPitchSrc);
//
//                        val_32x4 = (vaddl_s16(vmul_n_s16((tempSrc0_16x4), 7), (tempSrc1_16x4)));
//                        vst1q_s32(pTempTempDst + x + 2*lPitchSrc, val_32x4);
//
//                        val_32x4 = vaddl_s16(vmul_n_s16(tempSrc0_16x4, 5), vmul_n_s16(tempSrc1_16x4, 3));
//                        vst1q_s32(pTempTempDst + x + 3*lPitchSrc, val_32x4);
//
//                        val_32x4 = vaddl_s16(vmul_n_s16(tempSrc0_16x4, 5), vmul_n_s16(tempSrc2_16x4, 3));
//                        vst1q_s32(pTempTempDst + x, val_32x4);
//
//                        val_32x4 = vaddl_s16(vmul_n_s16((tempSrc0_16x4), 7), tempSrc2_16x4);
//                        vst1q_s32(pTempTempDst + x  + 1*lPitchSrc, val_32x4);
//                    }
//                }
//#endif
//                for(; x < lWidthSrc; x++)
//                {
//                    pTempTempDst[x + 2*lPitchSrc] = (MInt32)(pTempSrc[x])*7 + (MInt32)(pTempSrc[x+lPitchSrc])*1;
//                    pTempTempDst[x + 3*lPitchSrc] = (MInt32)(pTempSrc[x])*5 + (MInt32)(pTempSrc[x+lPitchSrc])*3;
//                    pTempTempDst[x + 0*lPitchSrc] = (MInt32)(pTempSrc[x])*5 + (MInt32)(pTempSrc[x-lPitchSrc])*3;
//                    pTempTempDst[x + 1*lPitchSrc] = (MInt32)(pTempSrc[x])*7 + (MInt32)(pTempSrc[x-lPitchSrc])*1;
//                }
//            }
//        }
//        else if (nStarLine == 1)
//        {
//            //先做列
//            for(MInt32 y = newStartRow; y < newEndRow; y+=4)
//            {
//                auto *pTempTempDst = pTempDst + (y - newStartRow)*lPitchSrc;
//                auto *pTempSrc = pSrcImg + (y>>2)*lPitchSrc;
//                MInt32 x = 0;
//
//#ifdef USE_NEON_SCALE
//                if (sizeof(T) == 2)
//                {
//                    int16x4_t tempSrc0_16x4;
//                    int16x4_t tempSrc1_16x4;
//                    int16x4_t tempSrc2_16x4;
//
//                    int32x4_t val_32x4;
//
//                    for (; x < lWidthSrc - 4; x += 4)
//                    {
//                        tempSrc0_16x4 = vld1_s16((MInt16*)pTempSrc + x);
//                        tempSrc1_16x4 = vld1_s16((MInt16*)pTempSrc + x + lPitchSrc);
//                        tempSrc2_16x4 = vld1_s16((MInt16*)pTempSrc + x - lPitchSrc);
//
//                        val_32x4 = (vaddl_s16(vmul_n_s16((tempSrc0_16x4), 7), (tempSrc1_16x4)));
//                        vst1q_s32(pTempTempDst + x + 1*lPitchSrc, val_32x4);
//
//                        val_32x4 = vaddl_s16(vmul_n_s16(tempSrc0_16x4, 5), vmul_n_s16(tempSrc1_16x4, 3));
//                        vst1q_s32(pTempTempDst + x + 2*lPitchSrc, val_32x4);
//
//                        val_32x4 = vaddl_s16(vmul_n_s16(tempSrc0_16x4, 3), vmul_n_s16(tempSrc1_16x4, 5));
//                        vst1q_s32(pTempTempDst + x + 3*lPitchSrc, val_32x4);
//
//                        val_32x4 = vaddl_s16(vmul_n_s16((tempSrc0_16x4), 7), tempSrc2_16x4);
//                        vst1q_s32(pTempTempDst + x, val_32x4);
//                    }
//                }
//#endif
//                for(; x < lWidthSrc; x++)
//                {
//                    pTempTempDst[x + 1*lPitchSrc] = (MInt32)(pTempSrc[x])*7 + (MInt32)(pTempSrc[x+lPitchSrc])*1;
//                    pTempTempDst[x + 2*lPitchSrc] = (MInt32)(pTempSrc[x])*5 + (MInt32)(pTempSrc[x+lPitchSrc])*3;
//                    pTempTempDst[x + 3*lPitchSrc] = (MInt32)(pTempSrc[x])*3 + (MInt32)(pTempSrc[x+lPitchSrc])*5;
//                    pTempTempDst[x + 0*lPitchSrc] = (MInt32)(pTempSrc[x])*7 + (MInt32)(pTempSrc[x-lPitchSrc])*1;
//                }
//            }
//        }


        //后做行
        for(MInt32 y = newStartRow; y < newEndRow; y++)
        {
            auto *pTempTempDst = pTempDst + (y - newStartRow)*lPitchSrc;
            auto *pNewDstImg = pDstImg + (y)*lPitchDst;

            MInt32 x = 2;

#ifdef USE_NEON_SCALE
            if (sizeof(T) == 2) //10bit
            {
				int32x4_t const7 = vdupq_n_s32(7);
				int32x4_t const5 = vdupq_n_s32(5);
				int32x4_t const3 = vdupq_n_s32(3);

                int32x4_t tempDst0_32x4;
                int32x4_t tempDst1_32x4;

                for(; x < lWidthDst - 1 - 16; x+=16)
                {
                    tempDst0_32x4 = vld1q_s32(pTempTempDst + (x>>2));
                    tempDst1_32x4 = vld1q_s32(pTempTempDst + (x>>2) + 1);

                    int16x4x4_t val_16x4x4;

                    int16x4_t val_16x4 = vrshrn_n_s32(vaddq_s32(vmulq_s32(tempDst0_32x4, const7), tempDst1_32x4), 6);
                    val_16x4x4.val[0] = val_16x4;

                    val_16x4 =  vrshrn_n_s32(vaddq_s32(vmulq_s32(tempDst0_32x4, const5), vmulq_s32(tempDst1_32x4, const3)), 6);
                    val_16x4x4.val[1] = val_16x4;

                    val_16x4 =  vrshrn_n_s32(vaddq_s32(vmulq_s32(tempDst0_32x4, const3), vmulq_s32(tempDst1_32x4, const5)), 6);
                    val_16x4x4.val[2] = val_16x4;

                    val_16x4 =  vrshrn_n_s32(vaddq_s32(tempDst0_32x4, vmulq_s32(tempDst1_32x4, const7)), 6);
                    val_16x4x4.val[3] = val_16x4;

                    vst4_s16((MInt16*)pNewDstImg + x, val_16x4x4);
                }
            }
#endif
            for(; x < lWidthDst - 2; x+=4)
            {
                MInt16 val = ((MInt32)pTempTempDst[x/4])*7 + ((MInt32)pTempTempDst[x/4+1])*1;
                pNewDstImg[x] = (val + 32) >> 6;
                val        = ((MInt32)pTempTempDst[x/4])*5 + ((MInt32)pTempTempDst[x/4+1])*3;
                pNewDstImg[x+1] = (val + 32) >> 6;
                val        = ((MInt32)pTempTempDst[x/4])*3 + ((MInt32)pTempTempDst[x/4+1])*5;
                pNewDstImg[x+2] = (val + 32) >> 6;
                val        = ((MInt32)pTempTempDst[x/4])*1 + ((MInt32)pTempTempDst[x/4+1])*7;
                pNewDstImg[x+3] = (val + 32) >> 6;
            }

            pNewDstImg[0] = pNewDstImg[2];
            pNewDstImg[1] = pNewDstImg[2];
            pNewDstImg[lWidthDst - 1] = pNewDstImg[lWidthDst - 3];
            pNewDstImg[lWidthDst - 2] = pNewDstImg[lWidthDst - 3];
        }

        if(newStartRow == 2)
        {
            MMemCpy(pDstImg, pDstImg + 2*lPitchDst, lWidthDst * sizeof(T));
            MMemCpy(pDstImg + lPitchDst, pDstImg + 2*lPitchDst, lWidthDst * sizeof(T));
        }

        if(newEndRow == lHeightDst-2)
        {
            MMemCpy(pDstImg + (lHeightDst-1)*lPitchDst, pDstImg + (lHeightDst-3)*lPitchDst, lWidthDst * sizeof(T));
            MMemCpy(pDstImg + (lHeightDst-2)*lPitchDst, pDstImg + (lHeightDst-3)*lPitchDst, lWidthDst * sizeof(T));
        }


        SAFE_FREE_ARRAY(MNull, pTempDst);

#else // 之前浮点数，有误差，没有进行四舍五入
        startRow = MAX(0, startRow);
        endRow = MIN(lHeightDst, endRow);

        for(MInt32 y = startRow; y < endRow; y++)
        {
            MFloat interRow = ( -1.5f + y ) * 0.25f;
            MInt32 row0, row1;
            MFloat wy;
            if( interRow < 0 )
            {
                row0 = row1 = 0;
                wy = 0;
            }
            else if( interRow > lHeigthSrc - 1 )
            {
                row0 = row1 = lHeigthSrc - 1;
                wy = 0;
            }
            else
            {
                row0 = MInt32(interRow);
                row1 = row0 + 1;
                wy = interRow - row0;
            }

            T *pCurDst = pDstImg + y * lPitchDst;
            T *pCurSrc0 = pSrcImg + row0 * lPitchSrc;
            T *pCurSrc1 = pSrcImg + row1 * lPitchSrc;

            for(MInt32 x = 0; x < lWidthDst; x++)
            {
                MFloat interCol = ( -1.5f + x ) * 0.25f;
                MInt32 col0, col1;
                MFloat wx;
                if( interCol < 0 )
                {
                    col0 = col1 = 0;
                    wx = 0;
                }
                else if( interCol > lWidthSrc - 1 )
                {
                    col0 = col1 = lWidthSrc - 1;
                    wx = 0;
                }
                else
                {
                    col0 = MInt32(interCol);
                    col1 = col0 + 1;
                    wx = interCol - col0;
                }

                MFloat val0, val1, val2, val3;
                val0 = pCurSrc0[ col0 ];
                val1 = pCurSrc0[ col1 ];
                val2 = pCurSrc1[ col0 ];
                val3 = pCurSrc1[ col1 ];

                val0 = val0 * ( 1 - wx ) + val1 * wx;
                val2 = val2 * ( 1 - wx ) + val3 * wx;

                pCurDst[ x ] = ( val0 * ( 1 - wy ) + val2 * wy );
            }
        }
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return 0;
    }
#endif

    template<typename T>
    MInt32 Up_Down_Scale_Mean2x2_4x4<T>::Bilinear_Up4_C1_Threads(T *pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
                                                                 T *pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst)
    {
#ifdef USE_NEON_SCALE
        LOGD("USE_NEON_SCALE!");
#endif

#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 res = MOK;
        MInt32 lHeight = lHeightDst;

        MBool isRunThreads = false;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        isRunThreads = ( lHeight > 64 ) && ( m_mcvParallelMonitor != MNull );
#endif

        if( isRunThreads )
        {
            /// 设置回调函数
            auto func_lamda = [](MVoid *HParam) -> MVoid
            {
                LImage_Resize_ST filter_sturct = ( LImage_Resize_ST ) HParam;

                MInt32 startRow = filter_sturct->startRow;
                MInt32 endRow = filter_sturct->endRow;

                T *pSrcImg = ( T * ) filter_sturct->pSrcImg;
                MInt32 lWidthSrc = filter_sturct->lWidthSrc;
                MInt32 lHeigthSrc = filter_sturct->lHeigthSrc;
                MInt32 lPitchSrc = filter_sturct->lPitchSrc;
                T *pDstImg = ( T * ) filter_sturct->pDstImg;
                MInt32 lWidthDst = filter_sturct->lWidthDst;
                MInt32 lHeightDst = filter_sturct->lHeightDst;
                MInt32 lPitchDst = filter_sturct->lPitchDst;

                Up_Down_Scale_Mean2x2_4x4 <T> *obj = ( Up_Down_Scale_Mean2x2_4x4 <T> * ) filter_sturct->obj;
                filter_sturct->lRet = obj->Bilinear_Up4_C1(pSrcImg, lWidthSrc, lHeigthSrc, lPitchSrc,
                                                           pDstImg, lWidthDst, lHeightDst, lPitchDst, startRow, endRow);
            };
            MVoid (*func)(MVoid *) = func_lamda;



            /// 设置参数
            MInt32 lTaskNum = m_nThreadCount > 0 ? m_nThreadCount : lHeight > 16 ? 16 : 8;
            lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = (( lTaskHeight >> 2 ) << 2);

            Image_Resize_ST pParam[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
#if defined(__ANDROID__) || defined(ANDROID)
                pParam[ lnum ].startRow = lTaskHeight * lnum;
                pParam[ lnum ].endRow = lTaskHeight * ( lnum + 1 );
#else
                pParam[ lnum ].startRow = lTaskHeight * lnum + 2; // 强制初始化从2开始
                pParam[ lnum ].endRow = lTaskHeight * ( lnum + 1 ) + 2;
#endif
            }
            pParam[ lTaskNum - 1 ].endRow = lHeight;


            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].obj = this;
                pParam[ lnum ].task_ID = lnum;

                pParam[ lnum ].pSrcImg = ( MVoid * ) pSrcImg;
                pParam[ lnum ].lWidthSrc = lWidthSrc;
                pParam[ lnum ].lHeigthSrc = lHeigthSrc;
                pParam[ lnum ].lPitchSrc = lPitchSrc;

                pParam[ lnum ].pDstImg = ( MVoid * ) pDstImg;
                pParam[ lnum ].lWidthDst = lWidthDst;
                pParam[ lnum ].lHeightDst = lHeightDst;
                pParam[ lnum ].lPitchDst = lPitchDst;
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
            res = Bilinear_Up4_C1(pSrcImg, lWidthSrc, lHeigthSrc, lPitchSrc,
                                  pDstImg, lWidthDst, lHeightDst, lPitchDst, 0, lHeightDst);
        }

        exit:
#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return res;
    }


    /////  目前只支持这两种格式
    //template class Up_Down_Scale_Mean2x2_4x4<MUInt8>;
    //
    //template class Up_Down_Scale_Mean2x2_4x4<MUInt16>;


NS_SINFLE_IMAGE_ENHANCEMENT_END
