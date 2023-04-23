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
#include "LaplacianSharpen.h"
#include <type_traits>
#include "ammem.h"
#include "merror.h"

#include "imagebase.h"
#include "mobilecv.h"
#include "ArcsoftLog.h"


#if defined(USE_NEON) || defined(__ARM_NEON__)
#define USE_NEON_LAPLACIAN_SHARPEN
#endif

#ifdef USE_NEON_LAPLACIAN_SHARPEN
#if defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif


//#define SHARPEN_THRESHOLD 16
//#define SHARPEN_MAP
//static MInt16 pMap_Sharpen[16] = {102, 128, 153, 179, 204, 230, 253, 253, 253, 253, 230, 230, 204, 192, 179, 153};
static MInt16 pMap_Sharpen[16] = {256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256};


NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template<typename T>
    LaplacianSharpen<T>::LaplacianSharpen()
    {

    }


    template<typename T>
    LaplacianSharpen<T>::~LaplacianSharpen()
    {

    }


    // indicates T is signed or unsigned
    template<typename T>
    struct TFSigned
    {
        enum
        {
            fSigned = T(-1) < 0
        };
    };

    // represents the bit length of T
    template<typename T>
    struct TBitCount
    {
        enum
        {
            cBits = sizeof(T) * 8
        };
    };

    template<typename T, bool fSigned>
    struct TMinMaxHelper
    {

    };

    template<typename T>
    struct TMinMaxHelper<T, true/*fSigned*/ >
    {
        static const T min = static_cast<T>( static_cast<T>(1) << ( TBitCount<T>::cBits - 1 ));
        static const T max = static_cast<T>( ~( static_cast<T>(1) << ( TBitCount<T>::cBits - 1 )));
    };

    template<typename T>
    struct TMinMaxHelper<T, false/*fSigned*/ >
    {
        static const T min = static_cast<T>( 0 );
        static const T max = static_cast<T>(-1);
    };

    template<typename T>
    struct TMinValue
    {
        static const T v = TMinMaxHelper<T, TFSigned<T>::fSigned>::min;
    };

    template<typename T>
    struct TMaxValue
    {
        static const T v = TMinMaxHelper<T, TFSigned<T>::fSigned>::max;
    };

    /**
    * @brief
    * @tparam T             只支持 u8 和 u6
    * @param pDataBuf       [in,out]
    * @param pTmpBuf        [in,out]    存放每一行像素值的临时内存,必须左右各扩展2个像素
    * @param width          [in]
    * @param height         [in]
    * @param pitch          [in]        以字节为单位
    * @param channels       [in]        通道数，1表示 y 通道，2表示 uv 通道
    * @param sharpScale     [in]        锐化程度，取值范围[0, 255]
    * @param startRow       [in]
    * @param endRow         [in]
    * @return
    */
    template<typename T>
    MVoid LaplacianSharpen<T>::processRow(T *pDataBuf,
                                          T *pTmpBuf,
                                          MInt32 width,
                                          MInt32 height,
                                          MInt32 pitch,
                                          MInt32 channels,
                                          MInt32 sharpScale,
                                          MInt32 startRow,
                                          MInt32 endRow)
    {


        bool is_uint8 = std::is_same<T, MUInt8>::value;
        bool is_int16 = std::is_same<T, MInt16>::value;

        MInt32 max_t = TMaxValue<T>::v;
        if( is_uint8 )
        {
            max_t = 255;
        }
        if( is_int16 )
        {
            max_t = 1020;
        }

        sharpScale = sharpScale << 6;

#ifdef USE_NEON_LAPLACIAN_SHARPEN
        int16x8_t vScale, vConst_0, vConst_1020;
        
        if (is_uint8) {
            vScale = vdupq_n_s16(sharpScale);
        }
        
        if (is_int16) {
             vScale = vdupq_n_s16(sharpScale);
             vConst_0 = vdupq_n_s16(0);
             vConst_1020 = vdupq_n_s16(1020);
        }

#endif

        if( channels == 1 )
        {
            T *pInputRow = pTmpBuf + 2;
            for(MInt32 y = startRow; y < endRow; y++)
            {
                T *pOutputRow = pDataBuf + y * pitch;
                MMemCpy(pInputRow, pOutputRow, width * sizeof(T));

                //ant的neon代码左边缘处理
                pInputRow[ -1 ] = pInputRow[ 1 ];
                pInputRow[ -2 ] = pInputRow[ 2 ];
                //neon代码没有右边缘处理,先镜像处理
                pInputRow[ width ] = pInputRow[ width - 2 ];
                pInputRow[ width + 1 ] = pInputRow[ width - 3 ];

                MInt32 x = 0;
#ifdef USE_NEON_LAPLACIAN_SHARPEN
                if (is_uint8)
                {
                uint8x8_t vsrc0, vsrc1;
                vsrc1 = vld1_u8((MUInt8*)pInputRow + x - 2);

                for (x = 0; x < width - 7; x += 8)
                {
                    uint8x8_t vdata0, vdata1, vdata2, vdata3, vdata4;
                    int16x8_t vTmp0, vTmp1;

                    vsrc0 = vsrc1;     // -2 -1 0 1 2 3 4 5
                    vsrc1 = vld1_u8((MUInt8*)pInputRow + x - 2 + 8); // 6 7 8 9 10 11 12 13

                    vdata0 = vsrc0;                    // -2 -1 0 1 2 3 4 5
                    vdata1 = vext_u8(vsrc0, vsrc1, 1); // -1 0 1 2 3 4 5 6
                    vdata2 = vext_u8(vsrc0, vsrc1, 2); // 0 1 2 3 4 5 6 7
                    vdata3 = vext_u8(vsrc0, vsrc1, 3); // 1 2 3 4 5 6 7 8
                    vdata4 = vext_u8(vsrc0, vsrc1, 4); // 2 3 4 5 6 7 8 9

                    vTmp0 = vreinterpretq_s16_u16(vaddl_u8(vdata0, vdata1));
                    vTmp1 = vreinterpretq_s16_u16(vaddl_u8(vdata3, vdata4));
                    vTmp0 = vaddq_s16(vTmp0, vTmp1);
                    vTmp1 = vreinterpretq_s16_u16(vshll_n_u8(vdata2, 2));

                    vTmp0 = vqrdmulhq_s16(vsubq_s16(vTmp1, vTmp0), vScale);
                    vTmp0 = vaddq_s16(vTmp0, vreinterpretq_s16_u16(vmovl_u8(vdata2)));

                    vst1_u8((MUInt8*)pOutputRow + x, vqmovun_s16(vTmp0));

                }
            }
                
                if (is_int16)
                {
                    int16x8_t vsrc0, vsrc1;
                    vsrc1 = vld1q_s16((MInt16*)pInputRow + x - 2);

                    for (x = 0; x < width - 7; x += 8)
                    {
                        int16x8_t vdata0, vdata1, vdata2, vdata3, vdata4;
                        int16x8_t vTmp0, vTmp1;

                        vsrc0 = vsrc1;     // -2 -1 0 1 2 3 4 5
                        vsrc1 = vld1q_s16((MInt16*)pInputRow + x - 2 + 8); // 6 7 8 9 10 11 12 13

                        vdata0 = vsrc0;                      // -2 -1 0 1 2 3 4 5
                        vdata1 = vextq_s16(vsrc0, vsrc1, 1); // -1 0 1 2 3 4 5 6
                        vdata2 = vextq_s16(vsrc0, vsrc1, 2); // 0 1 2 3 4 5 6 7
                        vdata3 = vextq_s16(vsrc0, vsrc1, 3); // 1 2 3 4 5 6 7 8
                        vdata4 = vextq_s16(vsrc0, vsrc1, 4); // 2 3 4 5 6 7 8 9

                        vTmp0 = vaddq_s16(vdata0, vdata1);
                        vTmp1 = vaddq_s16(vdata3, vdata4);
                        vTmp0 = vaddq_s16(vTmp0, vTmp1);
                        vTmp1 = vshlq_n_s16(vdata2, 2);

                        vTmp0 = vqrdmulhq_s16(vsubq_s16(vTmp1, vTmp0), vScale);
                        vTmp0 = vaddq_s16(vTmp0, vdata2);
                        vTmp0 = vminq_s16(vmaxq_s16(vTmp0, vConst_0), vConst_1020);

                        vst1q_s16((MInt16*)pOutputRow + x, vTmp0);

                    }
                }
#endif
                for(; x < width; x++)
                {
#ifdef SHARPEN_THRESHOLD
                    MInt16 value = ( pInputRow[ x ] << 2 ) - ( pInputRow[ x - 1 ] + pInputRow[ x - 2 ] + pInputRow[ x + 1 ] + pInputRow[ x + 2 ] );
#ifdef SHARPEN_MAP
                    MInt16 lWeight = pMap_Sharpen[ABS(value) >> 6];
                    value = (value*sharpScale + shiftVal) >> 15;
                    value = value * lWeight >> 8;
                    value = pInputRow[x] + value;
                    pOutputRow[x] = value < 0 ? 0 : value > max_t ? max_t : value;
#else
                    if( ABS(value) > SHARPEN_THRESHOLD )
                    {
                        value = ( value * sharpScale + ( 1 << 14 )) >> 15;
                        value = pInputRow[x] + value;
                        pOutputRow[x] = value < 0 ? 0 : value > max_t ? max_t : value;
                    }
#endif

#else
                    MInt16 val = ((( MInt32 ) ( pInputRow[ x ] << 2 ) -
                                   ( pInputRow[ x - 1 ] + pInputRow[ x - 2 ] + pInputRow[ x + 1 ] + pInputRow[ x + 2 ] )) * sharpScale + ( 1 << 14 ))
                            >> 15;
                    T value = pInputRow[ x ] + val;
                    pOutputRow[ x ] = value < 0 ? 0 : value > max_t ? max_t : value;
#endif
                }
            }
        }
        else
        {
            T *pInputRow = pTmpBuf + 4;
            for(MInt32 y = startRow; y < endRow; y++)
            {
                T *pOutputRow = pDataBuf + y * pitch;
                MMemCpy(pInputRow, pOutputRow, width * sizeof(T));

                //按照ant代码做左边缘的处理
                pInputRow[ -1 ] = pInputRow[ 1 ];
                pInputRow[ -2 ] = pInputRow[ 2 ];
                pInputRow[ -3 ] = pInputRow[ 3 ];
                pInputRow[ -4 ] = pInputRow[ 4 ];

                pInputRow[ width ] = pInputRow[ width - 4 ];
                pInputRow[ width + 1 ] = pInputRow[ width - 3 ];
                pInputRow[ width + 2 ] = pInputRow[ width - 6 ];
                pInputRow[ width + 3 ] = pInputRow[ width - 5 ];

                MInt32 x = 0;
#ifdef USE_NEON_LAPLACIAN_SHARPEN
                if (is_uint8)
                {
                    
                
                uint8x8x2_t vsrc0, vsrc1;
                vsrc1 = vld2_u8((MUInt8*)pInputRow + x - 4);

                for (x = 0; x < width * channels - 15; x += 16)
                {
                    vsrc0 = vsrc1;
                    vsrc1 = vld2_u8((MUInt8*)pInputRow + x - 4 + 16);
                    // vsrc0.val[0]: -4 -2 0 2 4 6 8 10
                    // vsrc0.val[1]: -3 -1 1 3 5 7 9 11

                    // vsrc1.val[0]: 12 14 16 18 20 22 24 26
                    // vsrc1.val[1]: 13 15 17 19 21 23 25 27

                    uint8x8_t vdata0, vdata1, vdata2, vdata3, vdata4;
                    int16x8_t vTmp0, vTmp1;
                    uint8x8x2_t vRes;

                    // U
                    vdata0 = vsrc0.val[0];
                    vdata1 = vext_u8(vsrc0.val[0], vsrc1.val[0], 1);
                    vdata2 = vext_u8(vsrc0.val[0], vsrc1.val[0], 2);
                    vdata3 = vext_u8(vsrc0.val[0], vsrc1.val[0], 3);
                    vdata4 = vext_u8(vsrc0.val[0], vsrc1.val[0], 4);

                    vTmp0 = vreinterpretq_s16_u16(vaddl_u8(vdata0, vdata1));
                    vTmp1 = vreinterpretq_s16_u16(vaddl_u8(vdata3, vdata4));
                    vTmp0 = vaddq_s16(vTmp0, vTmp1);
                    vTmp1 = vreinterpretq_s16_u16(vshll_n_u8(vdata2, 2));

                    vTmp0 = vqrdmulhq_s16(vsubq_s16(vTmp1, vTmp0), vScale);
                    vTmp0 = vaddq_s16(vTmp0, vreinterpretq_s16_u16(vmovl_u8(vdata2)));

                    vRes.val[0] = vqmovun_s16(vTmp0);

                    // V
                    vdata0 = vsrc0.val[1];
                    vdata1 = vext_u8(vsrc0.val[1], vsrc1.val[1], 1);
                    vdata2 = vext_u8(vsrc0.val[1], vsrc1.val[1], 2);
                    vdata3 = vext_u8(vsrc0.val[1], vsrc1.val[1], 3);
                    vdata4 = vext_u8(vsrc0.val[1], vsrc1.val[1], 4);

                    vTmp0 = vreinterpretq_s16_u16(vaddl_u8(vdata0, vdata1));
                    vTmp1 = vreinterpretq_s16_u16(vaddl_u8(vdata3, vdata4));
                    vTmp0 = vaddq_s16(vTmp0, vTmp1);
                    vTmp1 = vreinterpretq_s16_u16(vshll_n_u8(vdata2, 2));

                    vTmp0 = vqrdmulhq_s16(vsubq_s16(vTmp1, vTmp0), vScale);
                    vTmp0 = vaddq_s16(vTmp0, vreinterpretq_s16_u16(vmovl_u8(vdata2)));

                    vRes.val[1] = vqmovun_s16(vTmp0);

                    vst2_u8((MUInt8*)pOutputRow + x, vRes);

                }
                }
                
                if (is_int16)
                {
                    int16x8x2_t vsrc0, vsrc1;
                    vsrc1 = vld2q_s16((MInt16*)pInputRow + x - 4);
                    
                    for (x = 0; x < width * channels - 15; x += 16)
                    {
                        vsrc0 = vsrc1;
                        vsrc1 = vld2q_s16((MInt16*)pInputRow + x - 4 + 16);
                        // vsrc0.val[0]: -4 -2 0 2 4 6 8 10
                        // vsrc0.val[1]: -3 -1 1 3 5 7 9 11

                        // vsrc1.val[0]: 12 14 16 18 20 22 24 26
                        // vsrc1.val[1]: 13 15 17 19 21 23 25 27

                        int16x8_t vdata0, vdata1, vdata2, vdata3, vdata4;
                        int16x8_t vTmp0, vTmp1;
                        int16x8x2_t vRes;

                        // U
                        vdata0 = vsrc0.val[0];
                        vdata1 = vextq_s16(vsrc0.val[0], vsrc1.val[0], 1);
                        vdata2 = vextq_s16(vsrc0.val[0], vsrc1.val[0], 2);
                        vdata3 = vextq_s16(vsrc0.val[0], vsrc1.val[0], 3);
                        vdata4 = vextq_s16(vsrc0.val[0], vsrc1.val[0], 4);

                        vTmp0 = vaddq_s16(vdata0, vdata1);
                        vTmp1 = vaddq_s16(vdata3, vdata4);
                        vTmp0 = vaddq_s16(vTmp0, vTmp1);
                        vTmp1 = vshlq_n_s16(vdata2, 2);

                        vTmp0 = vqrdmulhq_s16(vsubq_s16(vTmp1, vTmp0), vScale);
                        vTmp0 = vaddq_s16(vTmp0, vdata2);
                        vRes.val[0] = vminq_s16(vmaxq_s16(vTmp0, vConst_0), vConst_1020);


                        // V
                        vdata0 = vsrc0.val[1];
                        vdata1 = vextq_s16(vsrc0.val[1], vsrc1.val[1], 1);
                        vdata2 = vextq_s16(vsrc0.val[1], vsrc1.val[1], 2);
                        vdata3 = vextq_s16(vsrc0.val[1], vsrc1.val[1], 3);
                        vdata4 = vextq_s16(vsrc0.val[1], vsrc1.val[1], 4);

                        vTmp0 = vaddq_s16(vdata0, vdata1);
                        vTmp1 = vaddq_s16(vdata3, vdata4);
                        vTmp0 = vaddq_s16(vTmp0, vTmp1);
                        vTmp1 = vshlq_n_s16(vdata2, 2);

                        vTmp0 = vqrdmulhq_s16(vsubq_s16(vTmp1, vTmp0), vScale);
                        vTmp0 = vaddq_s16(vTmp0, vdata2);
                        vRes.val[1] = vminq_s16(vmaxq_s16(vTmp0, vConst_0), vConst_1020);

                        vst2q_s16((MInt16*)pOutputRow + x, vRes);
                    }
                }
#endif
                for(; x < width * channels; x++)
                {

#ifdef SHARPEN_THRESHOLD
                    MInt16 value = ( pInputRow[ x ] << 2 ) - ( pInputRow[ x - 2 ] + pInputRow[ x - 4 ] + pInputRow[ x + 2 ] + pInputRow[ x + 4 ] );
#ifdef SHARPEN_MAP
                    MInt16 lWeight = pMap_Sharpen[ABS(value) >> 6];
                    value = (value*sharpScale + shiftVal) >> 15;
                    value = value * lWeight >> 8;
                    value = pInputRow[x] + value;
                    pOutputRow[x] = value < 0 ? 0 : value > max_t ? max_t : value;
#else
                    if( ABS(value) > SHARPEN_THRESHOLD )
                    {
                        value = ( value * sharpScale + ( 1 << 14 )) >> 15;
                        value = pInputRow[x] + value;
                        pOutputRow[x] = value < 0 ? 0 : value > max_t ? max_t : value;
                    }
#endif
#else
                    MInt16 val = (((( MInt32 ) pInputRow[ x ] << 2 ) -
                                   ( pInputRow[ x - 2 ] + pInputRow[ x - 4 ] + pInputRow[ x + 2 ] + pInputRow[ x + 4 ] )) * sharpScale + ( 1 << 14 ))
                            >> 15;
                    T value = pInputRow[ x ] + val;
                    pOutputRow[ x ] = value < 0 ? 0 : ( value > max_t ? max_t : value );
#endif
                }
            }
        }
    }


    typedef struct _tag_SHARP8DATA
    {
        MVoid *obj;
        MInt32 task_ID;
        MHandle hMemMgr;
        MInt32 lRet;

        MVoid *pSrcBuf;
        MVoid *pDstBuf;
        MVoid *pTmpBuf;

        MInt32 lWidth;
        MInt32 lHeight;
        MInt32 lPitch;
        MInt32 channels;
        MInt32 sharpScale;
        MInt32 startRow;
        MInt32 endRow;
    } Sharp8Data;

    template<typename T>
    MInt32 LaplacianSharpen<T>::processRowThreads(MHandle hMemMgr,
                                                  MHandle mcvParallelMonitor,
                                                  T *pDataBuf,
                                                  MInt32 width,
                                                  MInt32 height,
                                                  MInt32 pitch,
                                                  MInt32 channels,
                                                  MInt32 sharpScale,
                                                  MInt32 nThreadCount)
    {
        MInt32 res = MOK;
        MInt32 lTaskNum = nThreadCount > 0 ? nThreadCount : 8;
        lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;

        T *pTmpBuf[16] = {MNull};
        pTmpBuf[ 0 ] = ( T * ) MMemAlloc(hMemMgr, ( width + 8 ) * sizeof(T) * lTaskNum);
        if( pTmpBuf[ 0 ] == MNull )
        {
            res = MERR_NO_MEMORY;
            goto exit;
        }

        for(MInt32 i = 1; i < lTaskNum; i++)
        {
            pTmpBuf[ i ] = pTmpBuf[ 0 ] + i * ( width + 8 );
        }

        if( height > 64 && mcvParallelMonitor )
        {
            /// 设置回调函数
            auto func_lamda = [](MVoid *HParam) -> MVoid
            {
                Sharp8Data *param = ( Sharp8Data * ) HParam;
                LaplacianSharpen<T> *obj = ( LaplacianSharpen<T> * ) param->obj;
                obj->processRow(( T * ) param->pSrcBuf,
                                ( T * ) param->pTmpBuf,
                                param->lWidth,
                                param->lHeight,
                                param->lPitch,
                                param->channels,
                                param->sharpScale,
                                param->startRow,
                                param->endRow);
            };
            MVoid (*func)(MVoid *) = func_lamda;



            /// 设置参数
            MInt32 lTaskHeight = height / lTaskNum;
            lTaskHeight = ( lTaskHeight >> 2 ) << 2;

            Sharp8Data pParam[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].startRow = lTaskHeight * lnum;
                pParam[ lnum ].endRow = lTaskHeight * ( lnum + 1 );
            }
            pParam[ lTaskNum - 1 ].endRow = height;

            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].obj = this;
                //pParam[ lnum ].hMemMgr = m_hMemMgr;
                pParam[ lnum ].pSrcBuf = ( void * ) pDataBuf;
                pParam[ lnum ].pTmpBuf = ( void * ) pTmpBuf[ lnum ];
                pParam[ lnum ].lWidth = width;
                pParam[ lnum ].lHeight = height;
                pParam[ lnum ].lPitch = pitch;
                pParam[ lnum ].channels = channels;
                pParam[ lnum ].sharpScale = sharpScale;
            }



            /// 创建线程
            MInt32 lTaskID[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                lTaskID[ lnum ] = mcvAddTask(mcvParallelMonitor, func, ( MVoid * ) &pParam[ lnum ]);
                if( lTaskID[ lnum ] < 0 )
                {
                    res = MERR_BAD_STATE;
                    goto exit;
                }
            }

            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                mcvWaitTask(mcvParallelMonitor, lTaskID[ lnum ]);
            }
        }
        else
        {
            processRow(pDataBuf, pTmpBuf[ 0 ], width, height, pitch, channels, sharpScale, 0, height);
        }

        exit:
        if( pTmpBuf[ 0 ] )
            MMemFree(hMemMgr, pTmpBuf[ 0 ]);

        return res;
    }

    template<typename T>
    MVoid LaplacianSharpen<T>::processCol(T *pSrcBuf,
                                          T *pDstBuf,
                                          MInt32 width,
                                          MInt32 height,
                                          MInt32 pitch,
                                          MInt32 sharpScale,
                                          MInt32 startRow,
                                          MInt32 endRow)
    {
        bool is_uint8 = std::is_same<T, MUInt8>::value;
        bool is_int16 = std::is_same<T, MInt16>::value;

        MInt32 max_t = TMaxValue<T>::v;
        if( is_uint8 )
        {
            max_t = 255;
        }
        if( is_int16 )
        {
            max_t = 1020;
        }

        sharpScale = sharpScale << 6;
#ifdef USE_NEON_LAPLACIAN_SHARPEN
        int16x8_t vScale, vConst_0, vConst_1020;
        
        if (is_uint8) {
            vScale = vdupq_n_s16(sharpScale);
        }
        
        if (is_int16) {
             vScale = vdupq_n_s16(sharpScale);
             vConst_0 = vdupq_n_s16(0);
             vConst_1020 = vdupq_n_s16(1020);
        }

#endif

        //做中间行,当前行y的结果存在y-2行的buffer上
        for(MInt32 y = startRow; y < endRow; y++)
        {
            MInt32 y0 = ( y - 2 ) < 0 ? ( 2 - y ) : ( y - 2 );
            MInt32 y1 = ( y - 1 ) < 0 ? ( 2 - y ) : ( y - 1 );
            MInt32 y2 = y;
            MInt32 y3 = ( y + 1 ) >= height ? ( 2 * height - y - 3 ) : ( y + 1 );
            MInt32 y4 = ( y + 2 ) >= height ? ( 2 * height - y - 4 ) : ( y + 2 );
            T *pInputRow0 = pSrcBuf + y0 * pitch;
            T *pInputRow1 = pSrcBuf + y1 * pitch;
            T *pInputRow2 = pSrcBuf + y2 * pitch;
            T *pInputRow3 = pSrcBuf + y3 * pitch;
            T *pInputRow4 = pSrcBuf + y4 * pitch;

            T *pOutputRow = pDstBuf + y2 * pitch;

            MInt32 x = 0;
#ifdef USE_NEON_LAPLACIAN_SHARPEN
            if (is_uint8)
            {
                for (x = 0; x < width - 7; x += 8)
                {
                    uint8x8_t vdata0, vdata1, vdata2, vdata3, vdata4;
                    int16x8_t vTmp0, vTmp1;
                    vdata0 = vld1_u8((MUInt8*)pInputRow0 + x);
                    vdata1 = vld1_u8((MUInt8*)pInputRow1 + x);
                    vdata2 = vld1_u8((MUInt8*)pInputRow2 + x);
                    vdata3 = vld1_u8((MUInt8*)pInputRow3 + x);
                    vdata4 = vld1_u8((MUInt8*)pInputRow4 + x);

                    vTmp0 = vreinterpretq_s16_u16(vaddl_u8(vdata0, vdata1));
                    vTmp1 = vreinterpretq_s16_u16(vaddl_u8(vdata3, vdata4));
                    vTmp0 = vaddq_s16(vTmp0, vTmp1);
                    vTmp1 = vreinterpretq_s16_u16(vshll_n_u8(vdata2, 2));

                    vTmp0 = vqrdmulhq_s16(vsubq_s16(vTmp1, vTmp0), vScale);
                    vTmp0 = vaddq_s16(vTmp0, vreinterpretq_s16_u16(vmovl_u8(vdata2)));

                    vst1_u8((MUInt8*)pOutputRow + x, vqmovun_s16(vTmp0));

                }
            }
            
            if (is_int16)
            {
                for (x = 0; x < width - 7; x += 8)
                {
                    int16x8_t vdata0, vdata1, vdata2, vdata3, vdata4;
                    int16x8_t vTmp0, vTmp1;
                    vdata0 = vld1q_s16((MInt16*)pInputRow0 + x);
                    vdata1 = vld1q_s16((MInt16*)pInputRow1 + x);
                    vdata2 = vld1q_s16((MInt16*)pInputRow2 + x);
                    vdata3 = vld1q_s16((MInt16*)pInputRow3 + x);
                    vdata4 = vld1q_s16((MInt16*)pInputRow4 + x);

                    vTmp0 = vaddq_s16(vdata0, vdata1);
                    vTmp1 = vaddq_s16(vdata3, vdata4);
                    vTmp0 = vaddq_s16(vTmp0, vTmp1);
                    vTmp1 = vshlq_n_s16(vdata2, 2);

                    vTmp0 = vqrdmulhq_s16(vsubq_s16(vTmp1, vTmp0), vScale);
                    vTmp0 = vaddq_s16(vTmp0, vdata2);

                    vTmp0 = vminq_s16(vmaxq_s16(vTmp0, vConst_0), vConst_1020);
                    vst1q_s16((MInt16*)pOutputRow + x, vTmp0);
                }
            }

#endif

            for(; x < width; x++)
            {
#ifdef SHARPEN_THRESHOLD
                MInt16 value = ( pInputRow2[ x ] << 2 ) - ( pInputRow0[ x ] + pInputRow1[ x ] + pInputRow3[ x ] + pInputRow4[ x ] );
#ifdef SHARPEN_MAP
                MInt16 lWeight = pMap_Sharpen[ABS(value) >> 6];
                value = (value*sharpScale + (1 << 14)) >> 15;
                value = value * lWeight >> 8;
                value = pInputRow2[x] + value;
                pOutputRow[x] = value < 0 ? 0 : value > max_t ? max_t : value;
#else
                if( ABS(value) > SHARPEN_THRESHOLD )
                {
                    value = ( value * sharpScale + ( 1 << 14 )) >> 15;
                    value = pInputRow2[x] + value;
                    pOutputRow[x] = value < 0 ? 0 : value > max_t ? max_t : value;
                }
#endif
#else
                MInt16 val = ((( MInt32 ) ( pInputRow2[ x ] << 2 ) - ( pInputRow0[ x ] + pInputRow1[ x ] + pInputRow3[ x ] + pInputRow4[ x ] )) *
                              sharpScale + ( 1 << 14 )) >> 15;
                T value = pInputRow2[ x ] + val;
                pOutputRow[ x ] = value < 0 ? 0 : value > max_t ? max_t : value;
#endif
            }
        }
    }

    template<typename T>
    MInt32 LaplacianSharpen<T>::processColThreads(MHandle hMemMgr,
                                                  MHandle mcvParallelMonitor,
                                                  T *pSrcBuf,
                                                  T *pDstBuf,
                                                  MInt32 width,
                                                  MInt32 height,
                                                  MInt32 pitch,
                                                  MInt32 sharpScale,
                                                  MInt32 nThreadCount)
    {
        MInt32 i, lSize, res = MOK;
        MInt32 lTaskNum = nThreadCount > 0 ? nThreadCount : 8;
        lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;

        if( height > 64 && mcvParallelMonitor )
        {
            /// 设置回调函数
            auto func_lamda = [](MVoid *HParam) -> MVoid
            {
                Sharp8Data *param = ( Sharp8Data * ) HParam;
                LaplacianSharpen<T> *obj = ( LaplacianSharpen<T> * ) param->obj;

                obj->processCol(( T * ) param->pSrcBuf,
                                ( T * ) param->pDstBuf,
                                param->lWidth,
                                param->lHeight,
                                param->lPitch,
                                param->sharpScale,
                                param->startRow,
                                param->endRow);
            };
            MVoid (*func)(MVoid *) = func_lamda;



            /// 设置参数
            MInt32 lTaskHeight = height / lTaskNum;
            lTaskHeight = ( lTaskHeight >> 2 ) << 2;

            Sharp8Data pParam[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].startRow = lTaskHeight * lnum;
                pParam[ lnum ].endRow = lTaskHeight * ( lnum + 1 );
            }
            pParam[ lTaskNum - 1 ].endRow = height;

            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[ lnum ].obj = this;
                pParam[ lnum ].pSrcBuf = ( void * ) pSrcBuf;
                pParam[ lnum ].pDstBuf = ( void * ) pDstBuf;
                pParam[ lnum ].lWidth = width;
                pParam[ lnum ].lHeight = height;
                pParam[ lnum ].lPitch = pitch;
                pParam[ lnum ].sharpScale = sharpScale;
            }



            /// 创建线程
            MInt32 lTaskID[16] = {MNull};
            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                lTaskID[ lnum ] = mcvAddTask(mcvParallelMonitor, func, ( MVoid * ) &pParam[ lnum ]);
            }

            for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                mcvWaitTask(mcvParallelMonitor, lTaskID[ lnum ]);
            }
        }
        else
        {
            processCol(pSrcBuf, pDstBuf, width, height, pitch, sharpScale, 0, height);
        }

        return res;
    }


    template<typename T>
    MInt32 LaplacianSharpen<T>::sharpen(MHandle hMemMgr,
                                        MHandle mcvParallelMonitor,
                                        T *pData,
                                        T *pBufSharpen,
                                        MInt32 width,
                                        MInt32 height,
                                        MInt32 pitch,
                                        MInt32 channels,
                                        MInt32 sharpScale,
                                        MInt32 nThreadCount)
    {
        MInt32 lret = MOK;
        T *pDataBuf = ( T * ) ( pData );
        T *pTmpBuf = ( T * ) ( pBufSharpen );

        lret = processRowThreads(hMemMgr, mcvParallelMonitor, pDataBuf, width, height, pitch, channels, sharpScale, nThreadCount);
        if( lret != MOK )
        {
            return lret;
        }

        MMemCpy(pTmpBuf, pDataBuf, height * pitch * sizeof(T));

        if( height > 4 )
        {
            lret = processColThreads(hMemMgr, mcvParallelMonitor, pTmpBuf, pDataBuf, width, height, pitch, sharpScale, nThreadCount);
            if( lret != MOK )
            {
                return lret;
            }
        }

        return lret;
    }


    ///  目前只支持这两种格式
    template
    class LaplacianSharpen<MUInt8>;

    template
    class LaplacianSharpen<MInt16>;


NS_SINFLE_IMAGE_ENHANCEMENT_END


