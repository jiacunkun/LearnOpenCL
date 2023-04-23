#include <merror.h>
#include <ammem.h>
#include <thread>
#include <mobilecv.h>

#if defined(USE_NEON) || defined(__ARM_NEON__)
#define USE_NEON_PYRAMID //todo：后续完善neon
#endif

#ifdef USE_NEON_PYRAMID
#if defined(ANDROID) || defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN


/// @brief
/// @param FullLine
/// @param pHalfLine
/// @param lFullWidth
/// @param lHalfWidth
/// @return

    template <typename T>
    static MVoid Odd_Line_UpScaele_C1(T *FullLine, T *pHalfLine, MInt32 lFullWidth, MInt32 lHalfWidth)
    {
        MInt32 Max = 255 * sizeof(T) * sizeof(T);
        FullLine[ 0 ] = pHalfLine[ 0 ];
        MInt32 k = 1;
        for(MInt32 x = 1; x < lHalfWidth; x++, k += 2)
        {
            FullLine[ k ] = ( pHalfLine[ x ] + pHalfLine[ x - 1 ] + 1 ) >> 1;
            FullLine[ k + 1 ] = pHalfLine[ x ];
            CLAMP(FullLine[ k ], -128, Max);
            CLAMP(FullLine[ k + 1 ], -128, Max);
        }
        for(; k < lFullWidth; k++)
        {
            FullLine[ k ] = FullLine[ k - 1 ];
            CLAMP(FullLine[ k ], -128, Max);
        }
    }

/// @brief
/// @param CurFullLine
/// @param NexFullLine
/// @param PreFullLine
/// @param pHalfLine
/// @param lFullWidth
/// @param lHalfWidth
/// @return

    template <typename T>
    static MVoid Even_Odd_Line_UpScaele_C1(T *CurFullLine, T *NexFullLine, T *PreFullLine, T *pHalfLine, MInt32 lFullWidth, MInt32 lHalfWidth)
    {
        MInt32 Max = 255 * sizeof(T) * sizeof(T);
        MInt32 x = 0;
        MInt32 k = 0;

#ifdef USE_NEON_PYRAMID //todo:neon需要兼容
        if (sizeof(T) ==  1)
        {
            uint8x16_t src01_u8x16;
            uint8x16_t src02_u8x16;
            uint8x16x2_t pre_u8x16x2;
            uint8x16x2_t dst01_u8x16x2;
            uint8x16x2_t dst02_u8x16x2;
            //uint16x8_t sum01_u16x8, sum02_u16x8;
            for (x = 0; x < lHalfWidth - 16; x += 16, k += 32)
            {
                src01_u8x16 = vld1q_u8((MUInt8*)pHalfLine + x);
                src02_u8x16 = vld1q_u8((MUInt8*)pHalfLine + x + 1);
                //sum01_u16x8 = vaddl_u8(vget_low_u8(src01_u8x16), vget_low_u8(src02_u8x16));
                //sum02_u16x8 = vaddl_u8(vget_high_u8(src01_u8x16), vget_high_u8(src02_u8x16));
                //dst01_u8x16x2.val[0] = src01_u8x16;
                //dst01_u8x16x2.val[1] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));
                dst01_u8x16x2.val[0] = src01_u8x16;
                dst01_u8x16x2.val[1] = vrhaddq_u8(src01_u8x16, src02_u8x16);
                pre_u8x16x2 = vld2q_u8((MUInt8*)PreFullLine + k);
                vst2q_u8((MUInt8*)NexFullLine + k, dst01_u8x16x2);

                //sum01_u16x8 = vaddl_u8(vget_low_u8(pre_u8x16x2.val[0]), vget_low_u8(dst01_u8x16x2.val[0]));
                //sum02_u16x8 = vaddl_u8(vget_high_u8(pre_u8x16x2.val[0]), vget_high_u8(dst01_u8x16x2.val[0]));
                //dst02_u8x16x2.val[0] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));

                //sum01_u16x8 = vaddl_u8(vget_low_u8(pre_u8x16x2.val[1]), vget_low_u8(dst01_u8x16x2.val[1]));
                //sum02_u16x8 = vaddl_u8(vget_high_u8(pre_u8x16x2.val[1]), vget_high_u8(dst01_u8x16x2.val[1]));
                //dst02_u8x16x2.val[1] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));

                dst02_u8x16x2.val[0] = vrhaddq_u8(pre_u8x16x2.val[0], dst01_u8x16x2.val[0]);
                dst02_u8x16x2.val[1] = vrhaddq_u8(pre_u8x16x2.val[1], dst01_u8x16x2.val[1]);
                vst2q_u8((MUInt8*)CurFullLine + k, dst02_u8x16x2);
            }
        }
        else if (sizeof(T) == 2)
        {
            int16x8_t src01_s16x8;
            int16x8_t src02_s16x8;
            int16x8x2_t pre_s16x8x2;
            int16x8x2_t dst01_s16x8x2;
            int16x8x2_t dst02_s16x8x2;
            //uint16x8_t sum01_u16x8, sum02_u16x8;
            for (x = 0; x < lHalfWidth - 8; x += 8, k += 16)
            {
                src01_s16x8 = vld1q_s16((MInt16*)pHalfLine + x);
                src02_s16x8 = vld1q_s16((MInt16*)pHalfLine + x + 1);
                //sum01_u16x8 = vaddl_u8(vget_low_u8(src01_u8x16), vget_low_u8(src02_u8x16));
                //sum02_u16x8 = vaddl_u8(vget_high_u8(src01_u8x16), vget_high_u8(src02_u8x16));
                //dst01_u8x16x2.val[0] = src01_u8x16;
                //dst01_u8x16x2.val[1] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));
                dst01_s16x8x2.val[0] = src01_s16x8;
                dst01_s16x8x2.val[1] = vrhaddq_s16(src01_s16x8, src02_s16x8);
                pre_s16x8x2 = vld2q_s16((MInt16*)PreFullLine + k);
                vst2q_s16((MInt16*)NexFullLine + k, dst01_s16x8x2);

                //sum01_u16x8 = vaddl_u8(vget_low_u8(pre_u8x16x2.val[0]), vget_low_u8(dst01_u8x16x2.val[0]));
                //sum02_u16x8 = vaddl_u8(vget_high_u8(pre_u8x16x2.val[0]), vget_high_u8(dst01_u8x16x2.val[0]));
                //dst02_u8x16x2.val[0] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));

                //sum01_u16x8 = vaddl_u8(vget_low_u8(pre_u8x16x2.val[1]), vget_low_u8(dst01_u8x16x2.val[1]));
                //sum02_u16x8 = vaddl_u8(vget_high_u8(pre_u8x16x2.val[1]), vget_high_u8(dst01_u8x16x2.val[1]));
                //dst02_u8x16x2.val[1] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));

                dst02_s16x8x2.val[0] = vrhaddq_s16(pre_s16x8x2.val[0], dst01_s16x8x2.val[0]);
                dst02_s16x8x2.val[1] = vrhaddq_s16(pre_s16x8x2.val[1], dst01_s16x8x2.val[1]);
                vst2q_s16((MInt16*)CurFullLine + k, dst02_s16x8x2);
            }
        }
#endif

        NexFullLine[ k ] = pHalfLine[ x ];
        CurFullLine[ k ] = ( NexFullLine[ k ] + PreFullLine[ k ] + 1 ) >> 1;
        k++;
        x++;
        for(; x < lHalfWidth; x++, k += 2)//x = 1
        {
            NexFullLine[ k ] = ( pHalfLine[ x ] + pHalfLine[ x - 1 ] + 1 ) >> 1;
            NexFullLine[ k + 1 ] = pHalfLine[ x ];

            CurFullLine[ k ] = ( NexFullLine[ k ] + PreFullLine[ k ] + 1 ) >> 1;
            CurFullLine[ k + 1 ] = ( NexFullLine[ k + 1 ] + PreFullLine[ k + 1 ] + 1 ) >> 1;
        }
        for(; k < lFullWidth; k++)
        {
            NexFullLine[ k ] = NexFullLine[ k - 1 ];
            CurFullLine[ k ] = CurFullLine[ k - 1 ];
        }
    }


    template <typename T>
    MInt32 Guass3x3Up2(ImageInfo<T> *pHalfImg, ImageInfo<T> *pFullImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt32 lret = MOK;
        T *pFullData = pFullImg->pData;
        MInt32 lFullWidth = pFullImg->lWidth;
        MInt32 lFullHeight = pFullImg->lHeight;
        MInt32 lFullPitch = pFullImg->lStride;

        T *pHalfData   = pHalfImg->pData;
        MInt32 lHalfWidth  = pHalfImg->lWidth;
        MInt32 lHalfHeight = pHalfImg->lHeight;
        MInt32 lHalfPitch  = pHalfImg->lStride;

        MInt32 y, k;
        T *pPreFullImg;
        T *pCurFullImg = pFullData, *pNexFullImg = pFullData;
        T *tmppHalfImg = pHalfData;

        y = lTopLine;
        if( 0 == lTopLine )
        {
            tmppHalfImg = pHalfData;
            Odd_Line_UpScaele_C1(pCurFullImg, tmppHalfImg, lFullWidth, lHalfWidth);
            k = 1;
            y = 1;
        }
        else
        {
            pCurFullImg = pFullData + lFullPitch * ( y * 2 - 2 );
            tmppHalfImg = pHalfData + ( y - 1 ) * lHalfPitch;
            Odd_Line_UpScaele_C1(pCurFullImg, tmppHalfImg, lFullWidth, lHalfWidth);
            k = y * 2 - 1;
        }

        for(; y < lBotLine; y++, k += 2)//y = 1
        {
            pPreFullImg = pFullData + ( k - 1 ) * lFullPitch;
            pCurFullImg = pPreFullImg + lFullPitch;
            pNexFullImg = pCurFullImg + lFullPitch;
            tmppHalfImg = pHalfData + y * lHalfPitch;
            Even_Odd_Line_UpScaele_C1(pCurFullImg, pNexFullImg, pPreFullImg, tmppHalfImg, lFullWidth, lHalfWidth);
        }

        if( lBotLine == lHalfHeight )
        {
            for(; k < lFullHeight; k++)
            {
                pPreFullImg = pFullData + ( k - 1 ) * lFullPitch;
                pCurFullImg = pPreFullImg + lFullPitch;
                MMemCpy(pCurFullImg, pPreFullImg, lFullWidth* sizeof(T));
            }
        }
        return lret;
    }


    template <typename T>
    MInt32 Guass3x3Up2Threads(ImageInfo<T> *pHalfImg, ImageInfo<T> *pFullImg, MInt32 nThreadCount)
    {
        LOGD("Guass3x3Up2Threads++");

        MInt32 lret = MOK;
        MInt32 nHeight = pHalfImg->lHeight;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        {
            int threadCount = nThreadCount > 0 ? nThreadCount : nHeight >= 1024 ? 16 : 8;
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

                Guass3x3Up2(pHalfImg, pFullImg, startHeight, endHeight);
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
        }

#else
        lret = Guass3x3Up2(pHalfImg, pFullImg, 0, nHeight);
#endif
        LOGD("Guass3x3Up2Threads--");
        return lret;
    }

    template <typename T>
    struct IMG_SG_UPSCALE
    {
        ImageInfo<T>* pHalfImg;
        ImageInfo<T>* pFullImg;
        MInt32 lTopLine;
        MInt32 lBotLine;
    };

    template <typename T>
    MInt32 Guass3x3Up2Threads(MHandle mcvParallelMonitor, ImageInfo<T>* pHalfImg, ImageInfo<T>* pFullImg, MInt32 nThreadCount)
    {
        START_TIME;

        MInt32 lret = MOK;
        MInt32 lHeight = pHalfImg->lHeight;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        {
            MInt32 lTaskNum = nThreadCount > 0 ? nThreadCount : lHeight > 1024 ? 16 : 8;
            /// 设置回调函数
            auto func_lamda = [](MVoid* HParam) -> MVoid
            {
                auto SG_NLM_sturct = (IMG_SG_UPSCALE<T>*)HParam;

                Guass3x3Up2(SG_NLM_sturct->pHalfImg, 
                    SG_NLM_sturct->pFullImg,
                    SG_NLM_sturct->lTopLine,
                    SG_NLM_sturct->lBotLine);

            };
            MVoid(*func)(MVoid*) = func_lamda;



            /// 设置参数
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = (lTaskHeight >> 2) << 2;

            IMG_SG_UPSCALE<T> pParam[16] = { MNull };
            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[lnum].lTopLine = lTaskHeight * lnum;
                pParam[lnum].lBotLine = lTaskHeight * (lnum + 1);
            }
            pParam[0].lTopLine = 0;
            pParam[lTaskNum - 1].lBotLine = lHeight;


            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[lnum].pHalfImg = pHalfImg;
                pParam[lnum].pFullImg = pFullImg;
            }

            /// 创建线程     
            MInt32 lTaskID[16] = { MNull };
            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
                if (lTaskID[lnum] < 0)
                {
                    lret = MERR_BAD_STATE;
                }
            }

            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
            }
        }

#else
        lret = Guass3x3Up2(pHalfImg, pFullImg, 0, lHeight);
#endif
        END_TIME;
        return lret;
    }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 下采样
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
* 滤波核
* 1  2  1
* 2  4  2
* 1  2  1
*/

/// @brief 垂直方向滤波, 核[1, 2, 1]
/// @param tmpSrc00
/// @param tmpSrc01
/// @param tmpSrc02
/// @param pSmoothBuf
/// @param lWidth
/// @return


    template <typename T>
    static MVoid VerSmooth(T *tmpSrc00, T *tmpSrc01, T *tmpSrc02, MShort *pSmoothBuf, MInt32 lWidth)
    {
		MInt32 x = 0;

#ifdef USE_NEON
		if (sizeof(T) == 1)
		{
			uint8x8_t tmpSrc00_8x8;
			uint8x8_t tmpSrc01_8x8;
			uint8x8_t tmpSrc02_8x8;

			int16x8_t smoothBuf_16x8;

			for (; x < lWidth-7; x+=8)
			{
				tmpSrc00_8x8 = vld1_u8((MUInt8*)tmpSrc00 + x);
				tmpSrc01_8x8 = vld1_u8((MUInt8*)tmpSrc01 + x);
				tmpSrc02_8x8 = vld1_u8((MUInt8*)tmpSrc02 + x);

				smoothBuf_16x8 = vreinterpretq_s16_u16(vaddq_u16(vaddl_u8(tmpSrc00_8x8, tmpSrc02_8x8), vshll_n_u8(tmpSrc01_8x8, 1)));

				vst1q_s16(pSmoothBuf + x, smoothBuf_16x8);
			}
		}
		else if (sizeof(T) == 2)
		{
			int16x8_t tmpSrc00_16x8;
			int16x8_t tmpSrc01_16x8;
			int16x8_t tmpSrc02_16x8;

			int16x8_t smoothBuf_16x8;

			for (; x < lWidth - 7; x += 8)
			{
				tmpSrc00_16x8 = vld1q_s16((MInt16*)tmpSrc00 + x);
				tmpSrc01_16x8 = vld1q_s16((MInt16*)tmpSrc01 + x);
				tmpSrc02_16x8 = vld1q_s16((MInt16*)tmpSrc02 + x);

				smoothBuf_16x8 = (vaddq_s16(vaddq_s16(tmpSrc00_16x8, tmpSrc02_16x8), vshlq_n_s16(tmpSrc01_16x8, 1)));

				vst1q_s16((MInt16*)pSmoothBuf + x, smoothBuf_16x8);
			}
		}

#endif 

        for(; x < lWidth; x++)
        {
            pSmoothBuf[ x ] = tmpSrc00[ x ] + tmpSrc02[ x ] + ( tmpSrc01[ x ] << 1 );
        }
    }


/// @brief 水平方向滤波, 核[1, 2, 1], 下采样倍数为2, for y通道
/// @param pSmoothBuf
/// @param tmpDst
/// @param lWidth
/// @return

    template <typename T>
    static MVoid HorSmoothDown2ForY(MShort *SmoothBuf, T *tmpDst, MInt32 lSrcWidth, MInt32 lDstWidth)
    {
        MInt32 k = 0;
		MInt32 x = 0;
        MInt32 max = sizeof(T) > 1 ? 1020 : 255;

#ifdef USE_NEON00 //TODO：优化暂时有问题
		if (sizeof(T) == 1)
		{
			int16x8x3_t SmoothBuf_16x8x3;

			for (; x < lDstWidth - 7; x+=8, k += 16)
			{
				SmoothBuf_16x8x3 = vld3q_s16(SmoothBuf + k);

				int16x8_t lval_16x8 = vrshrq_n_s16(vaddq_s16(vaddq_s16(SmoothBuf_16x8x3.val[0], SmoothBuf_16x8x3.val[2]), vshlq_n_s16(SmoothBuf_16x8x3.val[1], 1)), 4);

				lval_16x8 = vminq_s16(lval_16x8, vdupq_n_s16(max));

				vst1_u8((MUInt8*)tmpDst + x, (vqmovun_s16(lval_16x8)));
			
			}
		}
		else
		{
			int16x8x3_t SmoothBuf_16x8x3;

			for (; x < lDstWidth - 7; x += 8, k += 16)
			{
				SmoothBuf_16x8x3 = vld3q_s16(SmoothBuf + k);
				int16x8_t lval_16x8 = vrshrq_n_s16(vaddq_s16(vaddq_s16(SmoothBuf_16x8x3.val[0], SmoothBuf_16x8x3.val[2]), vshlq_n_s16(SmoothBuf_16x8x3.val[1], 1)), 4);

				lval_16x8 = vminq_s16(lval_16x8, vdupq_n_s16(max));

				vst1q_s16((MInt16*)tmpDst + x, lval_16x8);
			}
		}

#endif 

        for(; x < lDstWidth; x++, k += 2)
        {
            MInt32 lval = (( SmoothBuf[ k ] << 1 ) + SmoothBuf[ k - 1 ] + SmoothBuf[ k + 1 ] + 8 ) >> 4;
            tmpDst[ x ] = MIN(max, lval);
        }
    }

    template <typename T>
    MInt32 Guass3x3Down2(MHandle hMemMgr,
                         T *pSrcImg,
                         MInt32 lSrcWidth,
                         MInt32 lSrcHeight,
                         MInt32 lSrcPitch,
                         T *pDstImg,
                         MInt32 lDstWidth,
                         MInt32 lDstHeight,
                         MInt32 lDstPitch,
                         MInt32 lTopLine,
                         MInt32 lBotLine,
                         MShort *SmoothBuf)
    {
        for(MInt32 y = lTopLine; y < lBotLine; y++)
        {
            MInt32 tempY = y << 1;
            T *tmpSrc01 = pSrcImg + tempY * lSrcPitch;
            T *tmpSrc00 = ( 0 == y ) ? tmpSrc01 : tmpSrc01 - lSrcPitch;
            T *tmpSrc02 = ( tempY == lSrcHeight - 1 ) ? tmpSrc01 : tmpSrc01 + lSrcPitch;
            T *tmpDst = pDstImg + y * lDstPitch;

            VerSmooth(tmpSrc00, tmpSrc01, tmpSrc02, SmoothBuf + 1, lSrcWidth);
            SmoothBuf[ 0 ] = SmoothBuf[ 1 ];
            SmoothBuf[ lSrcWidth + 1 ] = SmoothBuf[ lSrcWidth ];

            HorSmoothDown2ForY(SmoothBuf + 1, tmpDst, lSrcWidth, lDstWidth);
        }

        return MOK;
    }

    template <typename T>
    MInt32 Guass3x3Down2Threads(MHandle hMemMgr,
                                T *pSrcImg,
                                MInt32 lSrcWidth,
                                MInt32 lSrcHeight,
                                MInt32 lSrcPitch,
                                T *pDstImg,
                                MInt32 lDstWidth,
                                MInt32 lDstHeight,
                                MInt32 lDstPitch,
                                MInt32 nThreadCount)
    {
        LOGD("Guass3x3Down2Threads++");
        MInt32 lret = MOK;
        MInt32 nHeight = lDstHeight;

        int threadCount = nThreadCount > 0 ? nThreadCount : nHeight >= 1024 ? 16 : 8;
#if !( defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD))
        threadCount = 1;
#endif

        MShort *SmoothBuf[16] = {MNull};
        MShort* pMemory = MNull;
        pMemory = (MShort*)MMemAlloc(hMemMgr, (lSrcWidth + 4) * sizeof(MShort) * threadCount);
        if (MNull == pMemory)
        {
            lret = MERR_NO_MEMORY;
            goto exit;
        }
        for(MInt32 lnum = 0; lnum < threadCount; lnum++)
        {
            SmoothBuf[ lnum ] = pMemory + ( lSrcWidth + 4 ) * lnum;
        }

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        {
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

                Guass3x3Down2<T>(hMemMgr,
                              pSrcImg,
                              lSrcWidth,
                              lSrcHeight,
                              lSrcPitch,
                              pDstImg,
                              lDstWidth,
                              lDstHeight,
                              lDstPitch,
                              startHeight,
                              endHeight,
                              (MShort *) SmoothBuf[currentThreadId]);
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
        }
#else

        lret = Guass3x3Down2<T>(hMemMgr,
                            pSrcImg,
                            lSrcWidth,
                            lSrcHeight,
                            lSrcPitch,
                            pDstImg,
                            lDstWidth,
                            lDstHeight,
                            lDstPitch,
                            0,
                            lDstHeight,
                            (MShort*)SmoothBuf[0]);
#endif

        exit:
        if(pMemory)
        {
            MMemFree(hMemMgr, pMemory);
            pMemory = MNull;
        }
        
        LOGD("Guass3x3Down2Threads--");
        return lret;
    }

    template <typename T>
    struct IMG_SG_RESIZE
    {
        MHandle hMemMgr;
        T* pSrcImg;
        MInt32 lSrcWidth;
        MInt32 lSrcHeight;
        MInt32 lSrcPitch;
        T* pDstImg;
        MInt32 lDstWidth;
        MInt32 lDstHeight;
        MInt32 lDstPitch;
        MInt32 startRow;
        MInt32 endRow;
        MShort* pSmoothBuf;
    };

    template <typename T>
    MInt32 Guass3x3Down2Threads(MHandle hMemMgr,
        MHandle mcvParallelMonitor,
        T* pSrcImg,
        MInt32 lSrcWidth,
        MInt32 lSrcHeight,
        MInt32 lSrcPitch,
        T* pDstImg,
        MInt32 lDstWidth,
        MInt32 lDstHeight,
        MInt32 lDstPitch,
        MInt32 nThreadCount)
    {
        LOGD("Guass3x3Down2Threads++");
        MInt32 lret = MOK;
        MInt32 lHeight = lDstHeight;

        MInt32 lTaskNum = nThreadCount > 0 ? nThreadCount : lHeight > 1024 ? 16 : 8;
        MShort* SmoothBuf[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            SmoothBuf[lnum] = (MShort*)MMemAlloc(hMemMgr, (lSrcWidth + 4) * sizeof(MShort));
            if (MNull == SmoothBuf[lnum])
            {
                lret = MERR_NO_MEMORY;
                goto exit;
            }
        }

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        {
            /// 设置回调函数
            auto func_lamda = [](MVoid* HParam) -> MVoid
            {
                auto SG_NLM_sturct = (IMG_SG_RESIZE<T>*)HParam;

                Guass3x3Down2<T>(SG_NLM_sturct->hMemMgr,
                    SG_NLM_sturct->pSrcImg,
                    SG_NLM_sturct->lSrcWidth,
                    SG_NLM_sturct->lSrcHeight,
                    SG_NLM_sturct->lSrcPitch,
                    SG_NLM_sturct->pDstImg,
                    SG_NLM_sturct->lDstWidth,
                    SG_NLM_sturct->lDstHeight,
                    SG_NLM_sturct->lDstPitch,
                    SG_NLM_sturct->startRow,
                    SG_NLM_sturct->endRow,
                    SG_NLM_sturct->pSmoothBuf);
                
            };
            MVoid(*func)(MVoid*) = func_lamda;



            /// 设置参数
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = (lTaskHeight >> 2) << 2;

            IMG_SG_RESIZE<T> pParam[16] = { MNull };
            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[lnum].startRow = lTaskHeight * lnum;
                pParam[lnum].endRow = lTaskHeight * (lnum + 1);
            }
            pParam[0].startRow = 0;
            pParam[lTaskNum - 1].endRow = lHeight;


            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[lnum].hMemMgr = hMemMgr;
                pParam[lnum].pSrcImg = pSrcImg;
                pParam[lnum].lSrcWidth = lSrcWidth;
                pParam[lnum].lSrcHeight = lSrcHeight;
                pParam[lnum].lSrcPitch = lSrcPitch;
                pParam[lnum].pDstImg = pDstImg;
                pParam[lnum].lDstWidth = lDstWidth;
                pParam[lnum].lDstHeight = lDstHeight;
                pParam[lnum].lDstPitch = lDstPitch;
                pParam[lnum].pSmoothBuf = SmoothBuf[lnum];
            }                
                
            /// 创建线程     
            MInt32 lTaskID[16] = { MNull };
            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
                if (lTaskID[lnum] < 0)
                {
                    lret = MERR_BAD_STATE;
                    goto exit;
                }
            }

            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
            }
        }
#else

        lret = Guass3x3Down2<T>(hMemMgr,
            pSrcImg,
            lSrcWidth,
            lSrcHeight,
            lSrcPitch,
            pDstImg,
            lDstWidth,
            lDstHeight,
            lDstPitch,
            0,
            lDstHeight,
            (MShort*)SmoothBuf[0]);
#endif

    exit:
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            if (SmoothBuf[lnum])
            {
                MMemFree(hMemMgr, SmoothBuf[lnum]);
                SmoothBuf[lnum] = MNull;
            }
        }
        LOGD("Guass3x3Down2Threads--");
        return lret;
    }

    template <typename T>
    MInt32 Guass3x3Down2Threads(MHandle hMemMgr, ImageInfo<T> *src, ImageInfo<T> *dst, MInt32 nThreadCount)
    {
        T *pSrcImg = src->pData;
        MInt32 lSrcWidth = src->lWidth;
        MInt32 lSrcHeight = src->lHeight;
        MInt32 lSrcPitch = src->lStride;

        T *pDstImg    = dst->pData;
        MInt32 lDstWidth  = dst->lWidth;
        MInt32 lDstHeight = dst->lHeight;
        MInt32 lDstPitch  = dst->lStride;
        return Guass3x3Down2Threads<T>(hMemMgr,
                                    pSrcImg,
                                    lSrcWidth,
                                    lSrcHeight,
                                    lSrcPitch,
                                    pDstImg,
                                    lDstWidth,
                                    lDstHeight,
                                    lDstPitch,
                                    nThreadCount);
    }

    template <typename T>
    MInt32 Guass3x3Down2Threads(MHandle hMemMgr, MHandle mcvParallelMonitor, ImageInfo<T>* src, ImageInfo<T>* dst, MInt32 nThreadCount)
    {
        START_TIME;
        T* pSrcImg = src->pData;
        MInt32 lSrcWidth = src->lWidth;
        MInt32 lSrcHeight = src->lHeight;
        MInt32 lSrcPitch = src->lStride;

        T* pDstImg = dst->pData;
        MInt32 lDstWidth = dst->lWidth;
        MInt32 lDstHeight = dst->lHeight;
        MInt32 lDstPitch = dst->lStride;
        MInt32 lRet = Guass3x3Down2Threads<T>(hMemMgr,
            mcvParallelMonitor,
            pSrcImg,
            lSrcWidth,
            lSrcHeight,
            lSrcPitch,
            pDstImg,
            lDstWidth,
            lDstHeight,
            lDstPitch,
            nThreadCount);
        END_TIME;
        return lRet;
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END