#include "PyramidLayer.h"
#include <thread>  // for c++11 threads
#include <mobilecv.h>
#include "imagebase.h" // for ABS, MAX...
//#include "NEON_2_SSE.h"
#include "single_image_enhancement_define.h"

#if defined(USE_NEON) || defined(__ARM_NEON__)
#define USE_NEON_PYRAMID
#endif

#ifdef USE_NEON_PYRAMID
#if defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif


template<class T>
PyramidLayer<T>::PyramidLayer(MHandle hMemMgr) : m_hMemMgr(hMemMgr), m_pImage(MNull), m_lWidth(0), m_lHeight(0), m_lPitch(0), m_lElementSize(0),
                                              m_bIsRefImage(false)
{
}

template<class T>
PyramidLayer<T>::~PyramidLayer()
{
    if( m_pImage )
    {
        FreePyramidLevel();
    }
}

#pragma mark - get
template<class T>
MVoid PyramidLayer<T>::SetMemMgr(MHandle hMemMgr)
{
    m_hMemMgr = hMemMgr;
}

template<class T>
MHandle PyramidLayer<T>::GetMemMgr()
{
    return m_hMemMgr;
}

template<class T>
MByte *PyramidLayer<T>::GetImage()
{
    return m_pImage;
}

template<class T>
MInt32 PyramidLayer<T>::GetWidth()
{
    return m_lWidth;
}

template<class T>
MInt32 PyramidLayer<T>::GetHeight()
{
    return m_lHeight;
}

template<class T>
MInt32 PyramidLayer<T>::GetPitch()
{
    return m_lPitch;
}

template<class T>
MInt32 PyramidLayer<T>::GetElementSize()
{
    return m_lElementSize;
}


/// @brief 创建金字塔层，申请实际内存地址
/// @param lWidth       [in]
/// @param lHeight      [in]
/// @param lPitch       [in]
/// @return

template<class T>
MInt32 PyramidLayer<T>::NewPyramidLevel(MInt32 lWidth,
                                         MInt32 lHeight,
                                         MInt32 lPitch,
                                         MInt32 lElementSize)
{
    FreePyramidLevel();

    m_lWidth = lWidth;
    m_lHeight = lHeight;
    m_lPitch = lPitch;
    m_lElementSize = lElementSize;
    m_pImage = ( MByte * ) MMemAlloc(m_hMemMgr, lHeight * lPitch * sizeof(MByte) * lElementSize);
    if( !m_pImage )
    {
        return MERR_NO_MEMORY;
    }
    m_bIsRefImage = false;
    return MOK;
}


template<class T>
MInt32 PyramidLayer<T>::NewPyramidLevel(PyramidLayer *res)
{
    return NewPyramidLevel(res->m_lWidth, res->m_lHeight, res->m_lPitch);
}

template<class T>
MInt32 PyramidLayer<T>::NewPyramidLevel(MByte *pImage,
                                     MInt32 lWidth,
                                     MInt32 lHeight,
                                     MInt32 lPitch,
                                     MInt32 lElementSize)
{
    FreePyramidLevel();

    m_lWidth = lWidth;
    m_lHeight = lHeight;
    m_lPitch = lPitch;
    m_lElementSize = lElementSize;
    m_pImage = pImage;
    m_bIsRefImage = true;
    return MOK;
}


/// @brief 释放金字塔内存
/// @return

template<class T>
MVoid PyramidLayer<T>::FreePyramidLevel()
{
    if( m_pImage && m_bIsRefImage == false )
    {
        MMemFree(m_hMemMgr, m_pImage);
    }

    m_pImage = MNull;
    m_lWidth = 0;
    m_lHeight = 0;
    m_lPitch = 0;
}

#pragma mark - 金字塔加减操作
template<class T>
MVoid PyramidLayer<T>::PySubC1(PyramidLayer *pSubImg,
                            MInt32 lTopLine,
                            MInt32 lBotLine)
{
    MByte *pSrcDstData = m_pImage;
    MByte *pSubData = pSubImg->m_pImage;
    MInt32 lWidth = m_lWidth;
    MInt32 lHeight = m_lHeight;
    MInt32 lPitch = m_lPitch;
    MInt32 x, y;

#ifdef USE_NEON_PYRAMID
    uint8x16_t srcdata, subdata;
    uint16x8_t tmpdata;
    uint8x8_t resdata;
#endif
    for(y = lTopLine; y < lBotLine; y++)
    {
        MByte *tmpSrcDst = pSrcDstData + y * lPitch;
        MByte *tmpSub = pSubData + y * lPitch;

        x = 0;
#ifdef USE_NEON_PYRAMID
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
        for(; x < lWidth; x++)
        {
            MInt32 lVal = tmpSrcDst[ x ] - tmpSub[ x ] + 128;
            tmpSrcDst[ x ] = TRIMBYTE(lVal);
        }
#else
        for(x = 0; x < lWidth; x++)
        {
            MInt32 lVal = tmpSrcDst[ x ] - tmpSub[ x ] + 128;
            tmpSrcDst[ x ] = TRIMBYTE(lVal);
        }
#endif
    }
    return;
}


template<class T>
MVoid PyramidLayer<T>::PySubC1(MByte *pSubImg,
                            MInt32 lTopLine,
                            MInt32 lBotLine)
{
    MByte *pSrcDstData = m_pImage;
    MByte *pSubData = pSubImg;
    MInt32 lWidth = m_lWidth;
    MInt32 lHeight = m_lHeight;
    MInt32 lPitch = m_lPitch;
    MInt32 x, y;

#ifdef USE_NEON_PYRAMID
    uint8x16_t srcdata, subdata;
    uint16x8_t tmpdata;
    uint8x8_t resdata;
#endif
    for(y = lTopLine; y < lBotLine; y++)
    {
        MByte *tmpSrcDst = pSrcDstData + y * lPitch;
        MByte *tmpSub = pSubData + y * lPitch;

        x = 0;
#ifdef USE_NEON_PYRAMID
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
        for(; x < lWidth; x++)
        {
            MInt32 lVal = tmpSrcDst[ x ] - tmpSub[ x ] + 128;
            tmpSrcDst[ x ] = TRIMBYTE(lVal);
        }
#else
        for(x = 0; x < lWidth; x++)
        {
            MInt32 lVal = tmpSrcDst[ x ] - tmpSub[ x ] + 128;
            tmpSrcDst[ x ] = TRIMBYTE(lVal);
        }
#endif
    }
    return;
}


template<class T>
MVoid PyramidLayer<T>::PyAddC1(PyramidLayer *pAddImg,
                            MInt32 lTopLine,
                            MInt32 lBotLine)
{
    MByte *pSrcDstData = m_pImage;
    MByte *pAddData = pAddImg->m_pImage;
    MInt32 lWidth = m_lWidth;
    MInt32 lHeight = m_lHeight;
    MInt32 lPitch = m_lPitch;
    MInt32 x, y;
    for(y = lTopLine; y < lBotLine; y++)
    {
        MByte *tmpSrcDst = pSrcDstData + y * lPitch;
        MByte *tmpAdd = pAddData + y * lPitch;
        for(x = 0; x < lWidth; x++)
        {
            MInt32 lVal = tmpSrcDst[ x ] + tmpAdd[ x ] - 128;
            tmpSrcDst[ x ] = TRIMBYTE(lVal);
        }
    }
    return;
}

template<class T>
MVoid PyramidLayer<T>::PyAddC1(MByte *pAddImg,
                            MInt32 lTopLine,
                            MInt32 lBotLine)
{
    MByte *pSrcDstData = m_pImage;
    MByte *pAddData = pAddImg;
    MInt32 lWidth = m_lWidth;
    MInt32 lHeight = m_lHeight;
    MInt32 lPitch = m_lPitch;
    MInt32 x, y;
    for(y = lTopLine; y < lBotLine; y++)
    {
        MByte *tmpSrcDst = pSrcDstData + y * lPitch;
        MByte *tmpAdd = pAddData + y * lPitch;
        for(x = 0; x < lWidth; x++)
        {
            MInt32 lVal = tmpSrcDst[ x ] + tmpAdd[ x ] - 128;
            tmpSrcDst[ x ] = TRIMBYTE(lVal);
        }
    }
    return;
}

template<class T>
MVoid PyramidLayer<T>::PyAddC2(PyramidLayer *pAddImg01,
                            PyramidLayer *pAddImg02,
                            MInt32 lTopLine,
                            MInt32 lBotLine)
{
    MByte *pSrcDstData = m_pImage;
    MByte *pAddData01 = pAddImg01->m_pImage;
    MByte *pAddData02 = pAddImg02->m_pImage;
    MInt32 lWidth = m_lWidth;
    MInt32 lHeight = m_lHeight;
    MInt32 lPitchSrc = m_lPitch;
    MInt32 lPitchAdd = pAddImg01->m_lPitch;
    MInt32 x, y;
#ifdef USE_NEON_PYRAMID
    uint8x8x2_t srcdata, resdata;
    uint8x8_t adddata00, adddata01;
    uint16x8_t tmpdata;
    uint16x8_t vconst_128 = vdupq_n_u16(128);
#endif
    for(y = lTopLine; y < lBotLine; y++)
    {
        MByte *tmpSrcDst = pSrcDstData + y * lPitchSrc;
        MByte *tmpAdd01 = pAddData01 + y * lPitchAdd;
        MByte *tmpAdd02 = pAddData02 + y * lPitchAdd;
        MInt32 k;

        x = 0;
        k = 0;
#ifdef USE_NEON_PYRAMID
        for(; x < lWidth - 7; x += 8, k += 16)
        {
            adddata00 = vld1_u8(tmpAdd01 + x);
            adddata01 = vld1_u8(tmpAdd02 + x);
            srcdata = vld2_u8(tmpSrcDst + k);
            tmpdata = vaddl_u8(srcdata.val[ 0 ], adddata00);
            tmpdata = vqsubq_u16(tmpdata, vconst_128);
            resdata.val[ 0 ] = vqmovn_u16(tmpdata);
            tmpdata = vaddl_u8(srcdata.val[ 1 ], adddata01);
            tmpdata = vqsubq_u16(tmpdata, vconst_128);
            resdata.val[ 1 ] = vqmovn_u16(tmpdata);
            vst2_u8(tmpSrcDst + k, resdata);
        }
        for(; x < lWidth; x++, k += 2)
        {
            MInt32 lVal01 = tmpSrcDst[ k ] + tmpAdd01[ x ] - 128;
            MInt32 lVal02 = tmpSrcDst[ k + 1 ] + tmpAdd02[ x ] - 128;
            tmpSrcDst[ k ] = TRIMBYTE(lVal01);
            tmpSrcDst[ k + 1 ] = TRIMBYTE(lVal02);
        }
#else
        for(x = 0, k = 0; x < lWidth; x++, k += 2)
        {
            MInt32 lVal01 = tmpSrcDst[ k ] + tmpAdd01[ x ] - 128;
            MInt32 lVal02 = tmpSrcDst[ k + 1 ] + tmpAdd02[ x ] - 128;
            tmpSrcDst[ k ] = TRIMBYTE(lVal01);
            tmpSrcDst[ k + 1 ] = TRIMBYTE(lVal02);
        }
#endif
    }
    return;
}


template<class T>
MVoid PyramidLayer<T>::PySubC1Threads(PyramidLayer *pSubImg, MInt32 nThreadCount)
{
    MInt32 nHeight = m_lHeight;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)

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

        PySubC1(pSubImg, startHeight, endHeight);
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

    PySubC1(pSubImg, 0, nHeight);
#endif
}


template<class T>
MVoid PyramidLayer<T>::PyAddC1Threads(PyramidLayer *pAddImg, MInt32 nThreadCount)
{
    MInt32 nHeight = m_lHeight;
 #if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)

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

        PyAddC1(pAddImg, startHeight, endHeight);
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

    PyAddC1(pAddImg, 0, nHeight);
#endif
}


template<class T>
MVoid PyramidLayer<T>::PyAddC2Threads(PyramidLayer *pAddImg01,
                                   PyramidLayer *pAddImg02,
                                   MInt32 nThreadCount)
{
    MInt32 nHeight = m_lHeight;
    #if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)


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

        PyAddC2(pAddImg01, pAddImg02, startHeight, endHeight);
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

    PyAddC2(pAddImg01, pAddImg02, 0, nHeight);
    #endif
}


#pragma mark - 金字塔上采样

/// @brief 2倍上采样
/// @param pFullImg     [out]
/// @return

template<class T>
MInt32 PyramidLayer<T>::PyUpScale2Mean(PyramidLayer *pFullImg)
{
    MInt32 lret = MOK;
    MByte *pFullData = pFullImg->m_pImage;
    MInt32 lFullWidth = pFullImg->m_lWidth;
    MInt32 lFullHeight = pFullImg->m_lHeight;
    MInt32 lFullPitch = pFullImg->m_lPitch;

    MByte *pHalfData = m_pImage;
    MInt32 lHalfWidth = m_lWidth;
    MInt32 lHalfHeight = m_lHeight;
    MInt32 lHalfPitch = m_lPitch;


    for(MInt32 y = 0; y < lHalfHeight - 1; y++)
    {
        MByte *cur_ful_data00 = pFullData + y * 2 * lFullPitch;
        MByte *cur_ful_data01 = cur_ful_data00 + lFullPitch;
        MByte *cur_half_data = pHalfData + y * lHalfPitch;

        MInt32 k = 0;
        for(MInt32 x = 0; x < lHalfWidth - 1; x++, k += 2)
        {
            cur_ful_data00[ k ] = cur_ful_data00[ k + 1 ] = cur_half_data[ x ];
            cur_ful_data01[ k ] = cur_ful_data01[ k + 1 ] = cur_half_data[ x ];
        }
        for(; k < lFullWidth; k++)
        {
            cur_ful_data00[ k ] = cur_half_data[ lHalfWidth - 1 ];
            cur_ful_data01[ k ] = cur_half_data[ lHalfWidth - 1 ];
        }
    }


    for(MInt32 y = 2 * ( lHalfHeight - 2 ); y < lFullHeight; y++)
    {
        MByte *cur_ful_data = pFullData + y * lFullPitch;
        MByte *cur_half_data = pHalfData + ( lHalfHeight - 1 ) * lHalfPitch;
        MInt32 k = 0;
        for(MInt32 x = 0; x < lHalfWidth - 1; x++, k += 2)
        {
            cur_ful_data[ k ] = cur_ful_data[ k + 1 ] = cur_half_data[ x ];
        }
        for(; k < lFullWidth; k++)
        {
            cur_ful_data[ k ] = cur_half_data[ lHalfWidth - 1 ];
        }
    }

    return lret;
}


/// @brief
/// @param FullLine
/// @param pHalfLine
/// @param lFullWidth
/// @param lHalfWidth
/// @return

template<class T>
MVoid PyramidLayer<T>::Odd_Line_UpScaele_C1(MByte *FullLine,
                                         MByte *pHalfLine,
                                         MInt32 lFullWidth,
                                         MInt32 lHalfWidth)
{
    FullLine[ 0 ] = pHalfLine[ 0 ];
    MInt32 k = 1;
    for(MInt32 x = 1; x < lHalfWidth; x++, k += 2)
    {
        FullLine[ k ] = ( pHalfLine[ x ] + pHalfLine[ x - 1 ] + 1 ) >> 1;
        FullLine[ k + 1 ] = pHalfLine[ x ];
    }
    for(; k < lFullWidth; k++)
    {
        FullLine[ k ] = FullLine[ k - 1 ];
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

template<class T>
MVoid PyramidLayer<T>::Even_Odd_Line_UpScaele_C1(MByte *CurFullLine,
                                              MByte *NexFullLine,
                                              MByte *PreFullLine,
                                              MByte *pHalfLine,
                                              MInt32 lFullWidth,
                                              MInt32 lHalfWidth)
{
    MInt32 x = 0;
    MInt32 k = 0;

#ifdef USE_NEON_PYRAMID
    uint8x16_t src01_u8x16;
    uint8x16_t src02_u8x16;
    uint8x16x2_t pre_u8x16x2;
    uint8x16x2_t dst01_u8x16x2;
    uint8x16x2_t dst02_u8x16x2;
    //uint16x8_t sum01_u16x8, sum02_u16x8;
    for(x = 0; x < lHalfWidth - 16; x += 16, k += 32)
    {
        src01_u8x16 = vld1q_u8(pHalfLine + x);
        src02_u8x16 = vld1q_u8(pHalfLine + x + 1);
        //sum01_u16x8 = vaddl_u8(vget_low_u8(src01_u8x16), vget_low_u8(src02_u8x16));
        //sum02_u16x8 = vaddl_u8(vget_high_u8(src01_u8x16), vget_high_u8(src02_u8x16));
        //dst01_u8x16x2.val[0] = src01_u8x16;
        //dst01_u8x16x2.val[1] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));
        dst01_u8x16x2.val[ 0 ] = src01_u8x16;
        dst01_u8x16x2.val[ 1 ] = vrhaddq_u8(src01_u8x16, src02_u8x16);
        pre_u8x16x2 = vld2q_u8(PreFullLine + k);
        vst2q_u8(NexFullLine + k, dst01_u8x16x2);

        //sum01_u16x8 = vaddl_u8(vget_low_u8(pre_u8x16x2.val[0]), vget_low_u8(dst01_u8x16x2.val[0]));
        //sum02_u16x8 = vaddl_u8(vget_high_u8(pre_u8x16x2.val[0]), vget_high_u8(dst01_u8x16x2.val[0]));
        //dst02_u8x16x2.val[0] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));

        //sum01_u16x8 = vaddl_u8(vget_low_u8(pre_u8x16x2.val[1]), vget_low_u8(dst01_u8x16x2.val[1]));
        //sum02_u16x8 = vaddl_u8(vget_high_u8(pre_u8x16x2.val[1]), vget_high_u8(dst01_u8x16x2.val[1]));
        //dst02_u8x16x2.val[1] = vcombine_u8(vqrshrn_n_u16(sum01_u16x8, 1), vqrshrn_n_u16(sum02_u16x8, 1));

        dst02_u8x16x2.val[ 0 ] = vrhaddq_u8(pre_u8x16x2.val[ 0 ], dst01_u8x16x2.val[ 0 ]);
        dst02_u8x16x2.val[ 1 ] = vrhaddq_u8(pre_u8x16x2.val[ 1 ], dst01_u8x16x2.val[ 1 ]);
        vst2q_u8(CurFullLine + k, dst02_u8x16x2);
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


template<class T>
MInt32 PyramidLayer<T>::PyUpScale2(PyramidLayer *pFullImg,
                                MInt32 lTopLine,
                                MInt32 lBotLine)
{
    MInt32 lret = MOK;
    MByte *pFullData = pFullImg->m_pImage;
    MInt32 lFullWidth = pFullImg->m_lWidth;
    MInt32 lFullHeight = pFullImg->m_lHeight;
    MInt32 lFullPitch = pFullImg->m_lPitch;

    MByte *pHalfData = m_pImage;
    MInt32 lHalfWidth = m_lWidth;
    MInt32 lHalfHeight = m_lHeight;
    MInt32 lHalfPitch = m_lPitch;

    MInt32 y, k;
    MByte *pPreFullImg;
    MByte *pCurFullImg = pFullData, *pNexFullImg = pFullData;
    MByte *tmppHalfImg = pHalfData;

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
            MMemCpy(pCurFullImg, pPreFullImg, lFullWidth);
        }
    }
    return lret;
}


/// @brief
/// @param pFullImg         [out]
/// @param nThreadCount     [in]
/// @return

template<class T>
MInt32 PyramidLayer<T>::PyUpScale2Threads(PyramidLayer *pFullImg,
                                       MInt32 nThreadCount)
{
    MInt32 lret = MOK;
    MInt32 nHeight = m_lHeight;

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

            PyUpScale2(pFullImg, startHeight, endHeight);
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
    lret = PyUpScale2(pFullImg, 0, nHeight);
#endif

    return lret;
}


//#pragma mark - 金字塔下采样

/// @brief 均值下采样, for y 通道
/// @param dst
/// @return

template<class T>
MInt32 PyramidLayer<T>::PyMeanPooling4Y(PyramidLayer *dst)
{
    MByte *pSrcImg = m_pImage;
    MInt32 lSrcWidth = m_lWidth;
    MInt32 lSrcHeight = m_lHeight;
    MInt32 lSrcPitch = m_lPitch;

    MByte *pDstImg = dst->m_pImage;
    MInt32 lDstWidth = dst->m_lWidth;
    MInt32 lDstHeight = dst->m_lHeight;
    MInt32 lDstPitch = dst->m_lPitch;

    PyMeanPooling4Y(pSrcImg, lSrcWidth, lSrcHeight, lSrcPitch,
                    pDstImg, lDstWidth, lDstHeight, lDstPitch);

    return MOK;
}

/// @brief 均值下采样, for y 通道
/// @param hMemMgr
/// @param pSrcImg
/// @param lSrcWidth
/// @param lSrcHeight
/// @param lSrcPitch
/// @param pDstImg
/// @param lDstWidth
/// @param lDstHeight
/// @param lDstPitch
/// @return

template<class T>
MInt32 PyramidLayer<T>::PyMeanPooling4Y(MByte *pSrcImg,
                                     MInt32 lSrcWidth,
                                     MInt32 lSrcHeight,
                                     MInt32 lSrcPitch,
                                     MByte *pDstImg,
                                     MInt32 lDstWidth,
                                     MInt32 lDstHeight,
                                     MInt32 lDstPitch)
{
    MInt32 x;
    for(MInt32 y = 0; y < lDstHeight; y++)
    {
        MByte *tmpSrc00 = pSrcImg + y * 2 * lSrcPitch;
        MByte *tmpSrc01 = ( y * 2 == lSrcHeight - 1 ) ? tmpSrc00 : tmpSrc00 + lSrcPitch;
        MByte *tmpDst = pDstImg + y * lDstPitch;

        MInt32 k = 0;
        for(x = 0; x < lDstWidth - 1; x++, k += 2)
        {
            tmpDst[ x ] = ( tmpSrc00[ k ] + tmpSrc00[ k + 1 ] + tmpSrc01[ k ] + tmpSrc01[ k + 1 ] + 2 ) >> 2;
        }
        if( lSrcWidth == ( lSrcWidth >> 1 ) << 1 )
        {
            tmpDst[ x ] = (( tmpSrc00[ k ] + tmpSrc00[ k + 1 ] + tmpSrc01[ k ] + tmpSrc01[ k + 1 ] + 2 )) >> 2;
        }
        else
        {
            tmpDst[ x ] = ( tmpSrc00[ k ] + tmpSrc01[ k ] + 1 ) >> 1;
        }
    }

    return MOK;
}


/// @brief
/// @param dst1
/// @param dst2
/// @return

template<class T>
MInt32 PyramidLayer<T>::PyMeanPooling4UV(PyramidLayer *dst1, PyramidLayer *dst2)
{
    MByte *pSrcImg = m_pImage;
    MInt32 lSrcWidth = m_lWidth;
    MInt32 lSrcHeight = m_lHeight;
    MInt32 lSrcPitch = m_lPitch;

    MByte *pDstImgC1 = dst1->m_pImage;
    MByte *pDstImgC2 = dst2->m_pImage;
    MInt32 lDstWidth = dst1->m_lWidth;
    MInt32 lDstHeight = dst1->m_lHeight;
    MInt32 lDstPitch = dst1->m_lPitch;

    return PyMeanPooling4UV(pSrcImg,
                            lSrcWidth,
                            lSrcHeight,
                            lSrcPitch,
                            pDstImgC1,
                            pDstImgC2,
                            lDstWidth,
                            lDstHeight,
                            lDstPitch);
}


/// @brief
/// @param pSrcImg
/// @param lSrcWidth
/// @param lSrcHeight
/// @param lSrcPitch
/// @param pDstImgC1
/// @param pDstImgC2
/// @param lDstWidth
/// @param lDstHeight
/// @param lDstPitch
/// @return

template<class T>
MInt32 PyramidLayer<T>::PyMeanPooling4UV(MByte *pSrcImg,
                                      MInt32 lSrcWidth,
                                      MInt32 lSrcHeight,
                                      MInt32 lSrcPitch,
                                      MByte *pDstImgC1,
                                      MByte *pDstImgC2,
                                      MInt32 lDstWidth,
                                      MInt32 lDstHeight,
                                      MInt32 lDstPitch)
{
    MInt32 x;
    for(MInt32 y = 0; y < lDstHeight; y++)
    {
        MByte *tmpSrc00 = pSrcImg + y * 2 * lSrcPitch;
        MByte *tmpSrc01 = ( y * 2 == lSrcHeight - 1 ) ? tmpSrc00 : tmpSrc00 + lSrcPitch;
        MByte *tmpDstC1 = pDstImgC1 + y * lDstPitch;
        MByte *tmpDstC2 = pDstImgC2 + y * lDstPitch;

        MInt32 k = 0;
        for(x = 0; x < lDstWidth - 1; x++, k += 4)
        {
            tmpDstC1[ x ] = ( tmpSrc00[ k ] + tmpSrc00[ k + 2 ] + tmpSrc01[ k ] + tmpSrc01[ k + 2 ] + 2 ) >> 2;
            tmpDstC2[ x ] = ( tmpSrc00[ k + 1 ] + tmpSrc00[ k + 3 ] + tmpSrc01[ k + 1 ] + tmpSrc01[ k + 3 ] + 2 ) >> 2;
        }
        if( lSrcWidth == ( lSrcWidth >> 1 ) << 1 )
        {
            tmpDstC1[ x ] = ( tmpSrc00[ k ] + tmpSrc00[ k + 2 ] + tmpSrc01[ k ] + tmpSrc01[ k + 2 ] + 2 ) >> 2;
            tmpDstC2[ x ] = ( tmpSrc00[ k + 1 ] + tmpSrc00[ k + 3 ] + tmpSrc01[ k + 1 ] + tmpSrc01[ k + 3 ] + 2 ) >> 2;
        }
        else
        {
            tmpDstC1[ x ] = ( tmpSrc00[ k ] + tmpSrc01[ k ] + 1 ) >> 1;
            tmpDstC2[ x ] = ( tmpSrc00[ k + 1 ] + tmpSrc01[ k + 1 ] + 1 ) >> 1;
        }
    }

    return MOK;
}


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

template<class T>
MVoid PyramidLayer<T>::VerSmooth(MByte *tmpSrc00, MByte *tmpSrc01, MByte *tmpSrc02, MShort *pSmoothBuf, MInt32 lWidth)
{
    for(MInt32 x = 0; x < lWidth; x++)
    {
        pSmoothBuf[ x ] = tmpSrc00[ x ] + tmpSrc02[ x ] + ( tmpSrc01[ x ] << 1 );
    }
}

/// @brief 水平方向滤波, 核[1, 2, 1]
/// @param pSmoothBuf
/// @param tmpDst
/// @param lWidth
/// @return

template<class T>
MVoid PyramidLayer<T>::HorSmooth(MShort *pSmoothBuf, MByte *tmpDst, MInt32 lWidth)
{
    for(MInt32 x = 0; x < lWidth; x++)
    {
        MInt32 lval = (( pSmoothBuf[ x ] << 1 ) + pSmoothBuf[ x - 1 ] + pSmoothBuf[ x + 1 ] + 8 ) >> 4;
        tmpDst[ x ] = lval;
    }
}

/// @brief 水平方向滤波, 核[1, 2, 1], 下采样倍数为2, for y通道
/// @param pSmoothBuf
/// @param tmpDst
/// @param lWidth
/// @return

template<class T>
MVoid PyramidLayer<T>::HorSmoothDown2ForY(MShort *SmoothBuf, MByte *tmpDst, MInt32 lSrcWidth, MInt32 lDstWidth)
{
    MInt32 k = 0;
    for(MInt32 x = 0; x < lDstWidth; x++, k += 2)
    {
        MInt32 lval = (( SmoothBuf[ k ] << 1 ) + SmoothBuf[ k - 1 ] + SmoothBuf[ k + 1 ] + 8 ) >> 4;
        tmpDst[ x ] = lval;
    }
}

/// @brief 水平方向滤波, 核[1, 2, 1], 下采样倍数为2, for uv 通道
/// @param pSmoothBuf
/// @param tmpDst
/// @param lWidth
/// @return

template<class T>
MVoid PyramidLayer<T>::HorSmoothDown2ForUV(MShort *SmoothBuf, MByte *tmpDstC1, MByte *tmpDstC2, MInt32 lSrcWidth, MInt32 lDstWidth)
{
    MInt32 k = 0;
    for(MInt32 x = 0; x < lDstWidth; x++, k += 4)
    {
        MInt32 lval = (( SmoothBuf[ k ] << 1 ) + SmoothBuf[ k - 2 ] + SmoothBuf[ k + 2 ] + 8 ) >> 4;
        tmpDstC1[ x ] = lval;

        lval = (( SmoothBuf[ k + 1 ] << 1 ) + SmoothBuf[ k - 1 ] + SmoothBuf[ k + 3 ] + 8 ) >> 4;
        tmpDstC2[ x ] = lval;
    }
}


/// @brief
/// @param hMemMgr
/// @param pSrcImg
/// @param lSrcWidth
/// @param lSrcHeight
/// @param lSrcPitch
/// @param pDstImg
/// @param lDstWidth
/// @param lDstHeight
/// @param lDstPitch
/// @param lTopLine
/// @param lBotLine
/// @param SmoothBuf
/// @return

template<class T>
MInt32 PyramidLayer<T>::Guass3x3Down2ForY(MHandle hMemMgr,
                                       MByte *pSrcImg,
                                       MInt32 lSrcWidth,
                                       MInt32 lSrcHeight,
                                       MInt32 lSrcPitch,
                                       MByte *pDstImg,
                                       MInt32 lDstWidth,
                                       MInt32 lDstHeight,
                                       MInt32 lDstPitch,
                                       MInt32 lTopLine,
                                       MInt32 lBotLine,
                                       MShort *SmoothBuf)
{
    for(MInt32 y = lTopLine; y < lBotLine; y++)
    {
        MByte *tmpSrc01 = pSrcImg + y * 2 * lSrcPitch;
        MByte *tmpSrc00 = ( 0 == y ) ? tmpSrc01 : tmpSrc01 - lSrcPitch;
        MByte *tmpSrc02 = ( y * 2 == lSrcHeight - 1 ) ? tmpSrc01 : tmpSrc01 + lSrcPitch;
        MByte *tmpDst = pDstImg + y * lDstPitch;

        VerSmooth(tmpSrc00, tmpSrc01, tmpSrc02, SmoothBuf + 1, lSrcWidth);
        SmoothBuf[ 0 ] = SmoothBuf[ 1 ];
        SmoothBuf[ lSrcWidth + 1 ] = SmoothBuf[ lSrcWidth ];

        HorSmoothDown2ForY(SmoothBuf + 1, tmpDst, lSrcWidth, lDstWidth);
    }

    return MOK;
}

template<class T>
MInt32 PyramidLayer<T>::Guass3x3Down2ForUV(MHandle hMemMgr,
                                        MByte *pSrcImg,
                                        MInt32 lSrcWidth,
                                        MInt32 lSrcHeight,
                                        MInt32 lSrcPitch,
                                        MByte *pDstImgC1,
                                        MByte *pDstImgC2,
                                        MInt32 lDstWidth,
                                        MInt32 lDstHeight,
                                        MInt32 lDstPitch,
                                        MInt32 lTopLine,
                                        MInt32 lBotLine,
                                        MShort *SmoothBuf)
{
    for(MInt32 y = lTopLine; y < lBotLine; y++)
    {
        MByte *tmpSrc01 = pSrcImg + y * 2 * lSrcPitch;
        MByte *tmpSrc00 = ( 0 == y ) ? tmpSrc01 : tmpSrc01 - lSrcPitch;
        MByte *tmpSrc02 = ( y * 2 == lSrcHeight - 1 ) ? tmpSrc01 : tmpSrc01 + lSrcPitch;
        MByte *tmpDstC1 = pDstImgC1 + y * lDstPitch;
        MByte *tmpDstC2 = pDstImgC2 + y * lDstPitch;

        VerSmooth(tmpSrc00, tmpSrc01, tmpSrc02, SmoothBuf + 2, lSrcWidth * 2);

        SmoothBuf[ 0 ] = SmoothBuf[ 2 ];
        SmoothBuf[ 1 ] = SmoothBuf[ 3 ];
        SmoothBuf[ lSrcWidth * 2 + 2 ] = SmoothBuf[ lSrcWidth * 2 ];
        SmoothBuf[ lSrcWidth * 2 + 3 ] = SmoothBuf[ lSrcWidth * 2 + 1 ];
        HorSmoothDown2ForUV(SmoothBuf + 2, tmpDstC1, tmpDstC2, lSrcWidth, lDstWidth);
    }
    return MOK;
}


/// @brief
/// @param hMemMgr
/// @param pSrcImg
/// @param lSrcWidth
/// @param lSrcHeight
/// @param lSrcPitch
/// @param pDstImg
/// @param lDstWidth
/// @param lDstHeight
/// @param lDstPitch
/// @return

template<class T>
MInt32 PyramidLayer<T>::Guass3x3Down2ForYThreads(MHandle hMemMgr,
                                              MByte *pSrcImg,
                                              MInt32 lSrcWidth,
                                              MInt32 lSrcHeight,
                                              MInt32 lSrcPitch,
                                              MByte *pDstImg,
                                              MInt32 lDstWidth,
                                              MInt32 lDstHeight,
                                              MInt32 lDstPitch,
                                              MInt32 nThreadCount)
{
    MInt32 lret = MOK;
    MInt32 nHeight = lDstHeight;

    int threadCount = nThreadCount > 0 ? nThreadCount : nHeight >= 1024 ? 16 : 8;
    #if !( defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD))
    threadCount = 1;
    #endif

    MShort *SmoothBuf[16] = {MNull};
    for(MInt32 lnum = 0; lnum < threadCount; lnum++)
    {
        SmoothBuf[ lnum ] = ( MShort * ) MMemAlloc(hMemMgr, ( lSrcWidth + 4 ) * sizeof(MShort));
        if( MNull == SmoothBuf[ lnum ] )
        {
            lret = MERR_NO_MEMORY;
            goto exit;
        }
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

            Guass3x3Down2ForY(hMemMgr,
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
                              ( MShort * ) SmoothBuf[ currentThreadId ]);
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

    lret =    Guass3x3Down2ForY(hMemMgr,
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
    for(MInt32 lnum = 0; lnum < threadCount; lnum++)
    {
        if( SmoothBuf[ lnum ] )
        {
            MMemFree(hMemMgr, SmoothBuf[ lnum ]);
            SmoothBuf[ lnum ] = MNull;
        }
    }
    return lret;
}

/// @brief
/// @param hMemMgr
/// @param dst
/// @return

template<class T>
MInt32 PyramidLayer<T>::Guass3x3Down2ForYThreads(MHandle hMemMgr,
                                              PyramidLayer *src,
                                              MInt32 nThreadCount)
{
    MByte *pSrcImg = src->m_pImage;
    MInt32 lSrcWidth = src->m_lWidth;
    MInt32 lSrcHeight = src->m_lHeight;
    MInt32 lSrcPitch = src->m_lPitch;

    MByte *pDstImg = m_pImage;
    MInt32 lDstWidth = m_lWidth;
    MInt32 lDstHeight = m_lHeight;
    MInt32 lDstPitch = m_lPitch;
    return Guass3x3Down2ForYThreads(hMemMgr,
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


/// @brief
/// @param hMemMgr
/// @param pSrcImg
/// @param lSrcWidth
/// @param lSrcHeight
/// @param lSrcPitch
/// @param pDstImgC1
/// @param pDstImgC2
/// @param lDstWidth
/// @param lDstHeight
/// @param lDstPitch
/// @param nThreadCount
/// @return

template<class T>
MInt32 PyramidLayer<T>::Guass3x3Down2ForUVThreads(MHandle hMemMgr,
                                               MByte *pSrcImg,
                                               MInt32 lSrcWidth,
                                               MInt32 lSrcHeight,
                                               MInt32 lSrcPitch,
                                               MByte *pDstImgC1,
                                               MByte *pDstImgC2,
                                               MInt32 lDstWidth,
                                               MInt32 lDstHeight,
                                               MInt32 lDstPitch,
                                               MInt32 nThreadCount /*= -1*/)
{
    MInt32 lret = MOK;
    MInt32 nHeight = lDstHeight;

    int threadCount = nThreadCount > 0 ? nThreadCount : nHeight >= 1024 ? 16 : 8;
    threadCount = threadCount > 16 ? 16 : threadCount;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    threadCount = 1;
#endif

    MShort *SmoothBuf[16] = {MNull};
    for(MInt32 lnum = 0; lnum < threadCount; lnum++)
    {
        SmoothBuf[ lnum ] = ( MShort * ) MMemAlloc(hMemMgr, ( lSrcWidth + 2 ) * 2 * sizeof(MShort));
        if( MNull == SmoothBuf[ lnum ] )
        {
            lret = MERR_NO_MEMORY;
            goto exit;
        }
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

            Guass3x3Down2ForUV(hMemMgr,
                               pSrcImg,
                               lSrcWidth,
                               lSrcHeight,
                               lSrcPitch,
                               pDstImgC1,
                               pDstImgC2,
                               lDstWidth,
                               lDstHeight,
                               lDstPitch,
                               startHeight,
                               endHeight,
                               ( MShort * ) SmoothBuf[ currentThreadId ]);
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
    lret = Guass3x3Down2ForUV(hMemMgr,
            pSrcImg,
            lSrcWidth,
            lSrcHeight,
            lSrcPitch,
            pDstImgC1,
            pDstImgC2,
            lDstWidth,
            lDstHeight,
            lDstPitch,
            0,
            lSrcHeight,
            (MShort*)SmoothBuf[0]);
#endif


    exit:
    for(MInt32 lnum = 0; lnum < threadCount; lnum++)
    {
        if( SmoothBuf[ lnum ] )
        {
            MMemFree(hMemMgr, SmoothBuf[ lnum ]);
            SmoothBuf[ lnum ] = MNull;
        }
    }
    return lret;
}

/// @brief
/// @param hMemMgr
/// @param dst1
/// @param dst12
/// @param nThreadCount
/// @return

template<class T>
MInt32 PyramidLayer<T>::Guass3x3Down2ForUVThreads(MHandle hMemMgr,
                                               PyramidLayer *dst1,
                                               PyramidLayer *dst2,
                                               MInt32 nThreadCount /*= -1*/)
{
    MByte *pSrcImg = m_pImage;
    MInt32 lSrcWidth = m_lWidth;
    MInt32 lSrcHeight = m_lHeight;
    MInt32 lSrcPitch = m_lPitch;

    MByte *pDstImgC1 = dst1->m_pImage;
    MByte *pDstImgC2 = dst2->m_pImage;
    MInt32 lDstWidth = dst1->m_lWidth;
    MInt32 lDstHeight = dst1->m_lHeight;
    MInt32 lDstPitch = dst1->m_lPitch;

    return Guass3x3Down2ForUVThreads(hMemMgr,
                                     pSrcImg,
                                     lSrcWidth,
                                     lSrcHeight,
                                     lSrcPitch,
                                     pDstImgC1,
                                     pDstImgC2,
                                     lDstWidth,
                                     lDstHeight,
                                     lDstPitch,
                                     nThreadCount);

}


#pragma mark - down

typedef struct _tag_PYRAMID_DOWN_MEAN
{
    MVoid *obj;
    MInt32 task_ID;
    MHandle hMemMgr;
    MInt32 lRet;

    MVoid *pSrc;
    MInt32 lSrcWidth;
    MInt32 lSrcHeight;
    MInt32 lSrcStep;

    MVoid *pDst;
    MInt32 lDstWidth;
    MInt32 lDstHeight;
    MInt32 lDstStep;
    MInt32 lStartLine;
    MInt32 lEndLine;
} PYRAMID_DOWN_MEAN_t;


/**
* @brief 均值下采样, 壳大小为2x2, 输入是MUInt8, 输出是 MInt16格式
*       注意：结果值没有除以4
* @param pSrc       [in]    MUInt8
* @param lSrcStep
* @param pDst
* @param lDstWidth
* @param lDstStep
* @param lStartLine
* @param lEndLine
* @return
*/

template<class T>
MInt32 PyramidLayer<T>::down_mean2x2_cn1(MUInt8 *pSrc, MInt32 lSrcStep, MUInt16 *pDst, MInt32 lDstWidth, MInt32 lDstStep,
                                      MInt32 lStartLine, MInt32 lEndLine)
{
    for(MInt32 y = lStartLine; y < lEndLine; ++y)
    {
        MUInt8 *pSrcRow0 = pSrc + 2 * y * lSrcStep;
        MUInt8 *pSrcRow1 = pSrcRow0 + lSrcStep;
        MUInt16 *pDstRow = pDst + y * lDstStep;
        MInt32 x = 0;


#ifdef USE_NEON_PYRAMID
        for(x = 0; x < lDstWidth - 7; x += 8)
        {
            MInt32 xx = 2 * x;
            uint8x8x2_t vSrc0_u8x8x2 = vld2_u8(pSrcRow0 + xx);
            uint8x8x2_t vSrc1_u8x8x2 = vld2_u8(pSrcRow1 + xx);
            uint16x8_t vSum0 = vaddl_u8(vSrc0_u8x8x2.val[ 0 ], vSrc0_u8x8x2.val[ 1 ]);
            uint16x8_t vSum1 = vaddl_u8(vSrc1_u8x8x2.val[ 0 ], vSrc1_u8x8x2.val[ 1 ]);
            vst1q_u16(pDstRow + x, vaddq_u16(vSum0, vSum1));
        }
#endif

        for(; x < lDstWidth; ++x)
        {
            MInt32 xx = 2 * x;
            MInt32 val = pSrcRow0[ xx ] + pSrcRow0[ xx + 1 ] + pSrcRow1[ xx ] + pSrcRow1[ xx + 1 ];
            pDstRow[ x ] = val;
        }
    }

    return 0;
}


template<class T>
MInt32 PyramidLayer<T>::down_mean2x2_cn1_threads(MHandle mcvParallelMonitor,
                                              MUInt8 *pSrc, MInt32 lSrcStep,
                                              MUInt16 *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstStep,
                                              MInt32 nThreadCount)
{
    MInt32 lRet = MOK;
    if( mcvParallelMonitor )
    {
        /// 设置回调函数
        auto func_lamda = [](MVoid *HParam) -> MVoid
        {
            PYRAMID_DOWN_MEAN_t *pParam = ( PYRAMID_DOWN_MEAN_t * ) HParam;
            PyramidLayer *obj = ( PyramidLayer * ) pParam->obj;
            obj->down_mean2x2_cn1(( MUInt8 * ) pParam->pSrc, pParam->lSrcStep, ( MUInt16 * ) pParam->pDst, pParam->lDstWidth, pParam->lDstStep,
                                  pParam->lStartLine, pParam->lEndLine);
        };
        MVoid (*func)(MVoid *) = func_lamda;


        /// 设置参数
        MInt32 lTaskNum = nThreadCount > 0 ? nThreadCount : lDstHeight >= 1024 ? 16 : 8;
        if( lTaskNum > 16 ) lTaskNum = 16;
        MInt32 lTaskHeight = lDstHeight / lTaskNum;
        lTaskHeight = ( lTaskHeight >> 2 ) << 2;

        PYRAMID_DOWN_MEAN_t pParam[16] = {MNull};
        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[ lnum ].lStartLine = lTaskHeight * lnum;
            pParam[ lnum ].lEndLine = lTaskHeight * ( lnum + 1 );
        }
        pParam[ lTaskNum - 1 ].lEndLine = lDstHeight;

        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[ lnum ].obj = this;
            //pParam[ lnum ].hMemMgr = m_hMemMgr;
            pParam[ lnum ].pSrc = pSrc;
            pParam[ lnum ].lSrcStep = lSrcStep;
            pParam[ lnum ].pDst = pDst;
            pParam[ lnum ].lDstWidth = lDstWidth;
            pParam[ lnum ].lDstHeight = lDstHeight;
            pParam[ lnum ].lDstStep = lDstStep;
        }

        /// 创建线程
        MInt32 lTaskID[16] = {MNull};
        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[ lnum ] = mcvAddTask(mcvParallelMonitor, func, ( MVoid * ) &pParam[ lnum ]);
            if( lTaskID[ lnum ] < 0 )
            {
                lRet = MERR_BAD_STATE;
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
        lRet = down_mean2x2_cn1(pSrc, lSrcStep, pDst, lDstWidth, lDstStep, 0, lDstHeight);
    }
    exit:
    return lRet;
}

template<class T>
MInt32 PyramidLayer<T>::down_mean2x2_cn2(MUInt8 *pSrc, MInt32 lSrcStep, MUInt16 *pDst, MInt32 lDstWidth, MInt32 lDstStep,
                                      MInt32 lStartLine, MInt32 lEndLine)
{
    for(MInt32 y = lStartLine; y < lEndLine; ++y)
    {
        MUInt8 *pSrcRow0 = pSrc + 2 * y * lSrcStep;
        MUInt8 *pSrcRow1 = pSrcRow0 + lSrcStep;
        MUInt16 *pDstRow = pDst + y * lDstStep;

        MInt32 x = 0;
#ifdef USE_NEON_PYRAMID
        for(; x < lDstWidth * 2 - 15; x += 16)
        {
            MInt32 xx = 2 * x;
            uint8x8x4_t vSrc0 = vld4_u8(pSrcRow0 + xx);
            uint8x8x4_t vSrc1 = vld4_u8(pSrcRow1 + xx);
            uint16x8x2_t vRes;
            uint16x8_t vTemp0 = vaddl_u8(vSrc0.val[ 0 ], vSrc0.val[ 2 ]);
            uint16x8_t vTemp1 = vaddl_u8(vSrc1.val[ 0 ], vSrc1.val[ 2 ]);
            vRes.val[ 0 ] = vaddq_u16(vTemp0, vTemp1);

            vTemp0 = vaddl_u8(vSrc0.val[ 1 ], vSrc0.val[ 3 ]);
            vTemp1 = vaddl_u8(vSrc1.val[ 1 ], vSrc1.val[ 3 ]);
            vRes.val[ 1 ] = vaddq_u16(vTemp0, vTemp1);

            vst2q_u16(pDstRow + x, vRes);
        }
#endif

        for(; x < lDstWidth * 2; x += 2)
        {
            MInt32 xx = 2 * x;
            MInt32 val0 = pSrcRow0[ xx ] + pSrcRow0[ xx + 2 ] + pSrcRow1[ xx ] + pSrcRow1[ xx + 2 ];
            MInt32 val1 = pSrcRow0[ xx + 1 ] + pSrcRow0[ xx + 3 ] + pSrcRow1[ xx + 1 ] + pSrcRow1[ xx + 3 ];
            pDstRow[ x ] = val0;
            pDstRow[ x + 1 ] = val1;
        }
    }

    return 0;
}


template<class T>
MInt32 PyramidLayer<T>::down_mean2x2_cn2_threads(MHandle mcvParallelMonitor,
                                              MUInt8 *pSrc, MInt32 lSrcStep,
                                              MUInt16 *pDst, MInt32 lDstWidth, MInt32 lDstHeight,
                                              MInt32 lDstStep,
                                              MInt32 nThreadCount)
{
    MInt32 lRet = MOK;
    if( mcvParallelMonitor )
    {
        /// 设置回调函数
        auto func_lamda = [](MVoid *HParam) -> MVoid
        {
            PYRAMID_DOWN_MEAN_t *pParam = ( PYRAMID_DOWN_MEAN_t * ) HParam;
            PyramidLayer *obj = ( PyramidLayer * ) pParam->obj;
            obj->down_mean2x2_cn2(( MUInt8 * ) pParam->pSrc,
                                  pParam->lSrcStep,
                                  ( MUInt16 * ) pParam->pDst,
                                  pParam->lDstWidth,
                                  pParam->lDstStep,
                                  pParam->lStartLine,
                                  pParam->lEndLine);
        };
        MVoid (*func)(MVoid *) = func_lamda;


        /// 设置参数
        MInt32 lTaskNum = nThreadCount > 0 ? nThreadCount : lDstHeight >= 1024 ? 16 : 8;
        if( lTaskNum > 16 ) lTaskNum = 16;
        MInt32 lTaskHeight = lDstHeight / lTaskNum;
        lTaskHeight = ( lTaskHeight >> 2 ) << 2;

        PYRAMID_DOWN_MEAN_t pParam[16] = {MNull};
        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[ lnum ].lStartLine = lTaskHeight * lnum;
            pParam[ lnum ].lEndLine = lTaskHeight * ( lnum + 1 );
        }
        pParam[ lTaskNum - 1 ].lEndLine = lDstHeight;

        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[ lnum ].obj = this;
            //pParam[ lnum ].hMemMgr = m_hMemMgr;
            pParam[ lnum ].pSrc = pSrc;
            pParam[ lnum ].lSrcStep = lSrcStep;
            pParam[ lnum ].pDst = pDst;
            pParam[ lnum ].lDstWidth = lDstWidth;
            pParam[ lnum ].lDstHeight = lDstHeight;
            pParam[ lnum ].lDstStep = lDstStep;
        }

        /// 创建线程
        MInt32 lTaskID[16] = {MNull};
        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[ lnum ] = mcvAddTask(mcvParallelMonitor, func, ( MVoid * ) &pParam[ lnum ]);
            if( lTaskID[ lnum ] < 0 )
            {
                lRet = MERR_BAD_STATE;
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
        lRet = down_mean2x2_cn2(pSrc, lSrcStep, pDst, lDstWidth, lDstStep, 0, lDstHeight);
    }
    exit:
    return lRet;
}

template<class T>
MInt32 PyramidLayer<T>::down_mean2x2_threads(MHandle mcvParallelMonitor,
                                          MUInt8 *pSrc, MUInt16 *pDst,
                                          MInt32 lSrcWidth, MInt32 lSrcHeight,
                                          MInt32 lSrcStep,
                                          MInt32 lDstStep,
                                          MInt32 cn, MInt32 nThreadCount)
{
    MInt32 lret = 0;

    MInt32 lDstWidth = lSrcWidth >> 1;
    MInt32 lDstHeight = lSrcHeight >> 1;

    if( cn == 1 )
    {
        lret = down_mean2x2_cn1_threads(mcvParallelMonitor, pSrc, lSrcStep, pDst, lDstWidth, lDstHeight, lDstStep, nThreadCount);
    }
    else
    {
        lret = down_mean2x2_cn2_threads(mcvParallelMonitor, pSrc, lSrcStep, pDst, lDstWidth, lDstHeight, lDstStep, nThreadCount);
    }

    return lret;
}


template<class T>
MInt32 PyramidLayer<T>::down_mean2x2_cn1(MInt16 *pSrc, MInt32 lSrcStep, MInt16 *pDst, MInt32 lDstWidth, MInt32 lDstStep,
                                      MInt32 lStartLine, MInt32 lEndLine)
{
    MInt32 lret = 0;
    for(MInt32 y = lStartLine; y < lEndLine; ++y)
    {
        MInt16 *pSrcRow0 = pSrc + 2 * y * lSrcStep;
        MInt16 *pSrcRow1 = pSrcRow0 + lSrcStep;
        MInt16 *pDstRow = pDst + y * lDstStep;

        MInt32 x = 0;
#ifdef USE_NEON_PYRAMID
        for(; x < lDstWidth - 7; x += 8)
        {
            MInt32 xx = 2 * x;
            int16x8x2_t vSrc0_s16x8x2 = vld2q_s16(pSrcRow0 + xx);
            int16x8x2_t vSrc1_s16x8x2 = vld2q_s16(pSrcRow1 + xx);
            int16x8_t vSum0 = vaddq_s16(vSrc0_s16x8x2.val[ 0 ], vSrc0_s16x8x2.val[ 1 ]);
            int16x8_t vSum1 = vaddq_s16(vSrc1_s16x8x2.val[ 0 ], vSrc1_s16x8x2.val[ 1 ]);
            vSum0 = vaddq_s16(vSum0, vSum1);
            vst1q_s16(pDstRow + x, vrshrq_n_s16(vSum0, 2));
        }
#endif
        for(; x < lDstWidth; ++x)
        {
            MInt32 xx = 2 * x;
            MInt32 val = ( pSrcRow0[ xx ] + pSrcRow0[ xx + 1 ] + pSrcRow1[ xx ] + pSrcRow1[ xx + 1 ] + 2 ) >> 2;
            pDstRow[ x ] = val;
        }
    }

    return 0;
}


template<class T>
MInt32 PyramidLayer<T>::down_mean2x2_cn1_threads(MHandle mcvParallelMonitor,
                                              MInt16 *pSrc, MInt32 lSrcStep,
                                              MInt16 *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstStep,
                                              MInt32 nThreadCount)
{
    MInt32 lRet = MOK;
    if( mcvParallelMonitor )
    {
        /// 设置回调函数
        auto func_lamda = [](MVoid *HParam) -> MVoid
        {
            PYRAMID_DOWN_MEAN_t *pParam = ( PYRAMID_DOWN_MEAN_t * ) HParam;
            PyramidLayer *obj = ( PyramidLayer * ) pParam->obj;
            obj->down_mean2x2_cn1(( MInt16 * ) pParam->pSrc,
                                  pParam->lSrcStep,
                                  ( MInt16 * ) pParam->pDst,
                                  pParam->lDstWidth,
                                  pParam->lDstStep,
                                  pParam->lStartLine,
                                  pParam->lEndLine);
        };
        MVoid (*func)(MVoid *) = func_lamda;


        /// 设置参数
        MInt32 lTaskNum = nThreadCount > 0 ? nThreadCount : lDstHeight >= 1024 ? 16 : 8;
        if( lTaskNum > 16 ) lTaskNum = 16;
        MInt32 lTaskHeight = lDstHeight / lTaskNum;
        lTaskHeight = ( lTaskHeight >> 2 ) << 2;

        PYRAMID_DOWN_MEAN_t pParam[16] = {MNull};
        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[ lnum ].lStartLine = lTaskHeight * lnum;
            pParam[ lnum ].lEndLine = lTaskHeight * ( lnum + 1 );
        }
        pParam[ lTaskNum - 1 ].lEndLine = lDstHeight;

        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[ lnum ].obj = this;
            //pParam[ lnum ].hMemMgr = m_hMemMgr;
            pParam[ lnum ].pSrc = pSrc;
            pParam[ lnum ].lSrcStep = lSrcStep;
            pParam[ lnum ].pDst = pDst;
            pParam[ lnum ].lDstWidth = lDstWidth;
            pParam[ lnum ].lDstHeight = lDstHeight;
            pParam[ lnum ].lDstStep = lDstStep;
        }

        /// 创建线程
        MInt32 lTaskID[16] = {MNull};
        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[ lnum ] = mcvAddTask(mcvParallelMonitor, func, ( MVoid * ) &pParam[ lnum ]);
            if( lTaskID[ lnum ] < 0 )
            {
                lRet = MERR_BAD_STATE;
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
        down_mean2x2_cn1(( MInt16 * ) pSrc, lSrcStep, ( MInt16 * ) pDst, lDstWidth, lDstStep, 0, lDstHeight);
    }
    exit:
    return lRet;
}


template<class T>
MInt32 PyramidLayer<T>::down_mean2x2_cn2(MInt16 *pSrc, MInt32 lSrcStep, MInt16 *pDst, MInt32 lDstWidth, MInt32 lDstStep,
                                      MInt32 lStartLine, MInt32 lEndLine)
{
    for(MInt32 y = lStartLine; y < lEndLine; ++y)
    {
        MInt16 *pSrcRow0 = pSrc + 2 * y * lSrcStep;
        MInt16 *pSrcRow1 = pSrcRow0 + lSrcStep;
        MInt16 *pDstRow = pDst + y * lDstStep;
        MInt32 x = 0;
#ifdef USE_NEON_PYRAMID
        for(x = 0; x < lDstWidth * 2 - 15; x += 16)
        {
            MInt32 xx = 2 * x;
            int16x8x4_t vSrc0 = vld4q_s16(pSrcRow0 + xx);
            int16x8x4_t vSrc1 = vld4q_s16(pSrcRow1 + xx);
            int16x8x2_t vRes;
            int16x8_t vTemp0, vTemp1;
            vTemp0 = vaddq_s16(vSrc0.val[ 0 ], vSrc0.val[ 2 ]);
            vTemp1 = vaddq_s16(vSrc1.val[ 0 ], vSrc1.val[ 2 ]);
            vRes.val[ 0 ] = vrshrq_n_s16(vaddq_s16(vTemp0, vTemp1), 2);

            vTemp0 = vaddq_s16(vSrc0.val[ 1 ], vSrc0.val[ 3 ]);
            vTemp1 = vaddq_s16(vSrc1.val[ 1 ], vSrc1.val[ 3 ]);
            vRes.val[ 1 ] = vrshrq_n_s16(vaddq_s16(vTemp0, vTemp1), 2);

            vst2q_s16(pDstRow + x, vRes);
        }
#endif

        for(; x < lDstWidth * 2; x += 2)
        {
            MInt32 xx = 2 * x;
            MInt32 val0 = ( pSrcRow0[ xx ] + pSrcRow0[ xx + 2 ] + pSrcRow1[ xx ] + pSrcRow1[ xx + 2 ] + 2 ) >> 2;
            MInt32 val1 = ( pSrcRow0[ xx + 1 ] + pSrcRow0[ xx + 3 ] + pSrcRow1[ xx + 1 ] + pSrcRow1[ xx + 3 ] + 2 ) >> 2;

            pDstRow[ x ] = val0;
            pDstRow[ x + 1 ] = val1;
        }
    }

    return 0;
}


template<class T>
MInt32 PyramidLayer<T>::down_mean2x2_cn2_threads(MHandle mcvParallelMonitor,
                                              MInt16 *pSrc, MInt32 lSrcStep,
                                              MInt16 *pDst, MInt32 lDstWidth, MInt32 lDstHeight,
                                              MInt32 lDstStep,
                                              MInt32 nThreadCount)
{
    MInt32 lRet = MOK;
    if( mcvParallelMonitor )
    {
        /// 设置回调函数
        auto func_lamda = [](MVoid *HParam) -> MVoid
        {
            PYRAMID_DOWN_MEAN_t *pParam = ( PYRAMID_DOWN_MEAN_t * ) HParam;
            PyramidLayer *obj = ( PyramidLayer * ) pParam->obj;
            obj->down_mean2x2_cn2(( MInt16 * ) pParam->pSrc,
                                  pParam->lSrcStep,
                                  ( MInt16 * ) pParam->pDst,
                                  pParam->lDstWidth,
                                  pParam->lDstStep,
                                  pParam->lStartLine,
                                  pParam->lEndLine);
        };
        MVoid (*func)(MVoid *) = func_lamda;


        /// 设置参数
        MInt32 lTaskNum = nThreadCount > 0 ? nThreadCount : lDstHeight >= 1024 ? 16 : 8;
        if( lTaskNum > 16 ) lTaskNum = 16;
        MInt32 lTaskHeight = lDstHeight / lTaskNum;
        lTaskHeight = ( lTaskHeight >> 2 ) << 2;

        PYRAMID_DOWN_MEAN_t pParam[16] = {MNull};
        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[ lnum ].lStartLine = lTaskHeight * lnum;
            pParam[ lnum ].lEndLine = lTaskHeight * ( lnum + 1 );
        }
        pParam[ lTaskNum - 1 ].lEndLine = lDstHeight;

        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[ lnum ].obj = this;
            //pParam[ lnum ].hMemMgr = m_hMemMgr;
            pParam[ lnum ].pSrc = pSrc;
            pParam[ lnum ].lSrcStep = lSrcStep;
            pParam[ lnum ].pDst = pDst;
            pParam[ lnum ].lDstWidth = lDstWidth;
            pParam[ lnum ].lDstHeight = lDstHeight;
            pParam[ lnum ].lDstStep = lDstStep;
        }

        /// 创建线程
        MInt32 lTaskID[16] = {MNull};
        for(MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[ lnum ] = mcvAddTask(mcvParallelMonitor, func, ( MVoid * ) &pParam[ lnum ]);
            if( lTaskID[ lnum ] < 0 )
            {
                lRet = MERR_BAD_STATE;
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
        down_mean2x2_cn2(( MInt16 * ) pSrc, lSrcStep, ( MInt16 * ) pDst, lDstWidth, lDstStep, 0, lDstHeight);
    }
    exit:
    return lRet;
}

template<class T>
MInt32 PyramidLayer<T>::down_mean2x2_threads(MHandle mcvParallelMonitor,
                                          MInt16 *pSrc, MInt16 *pDst,
                                          MInt32 lSrcWidth, MInt32 lSrcHeight,
                                          MInt32 lSrcStep,
                                          MInt32 lDstStep,
                                          MInt32 cn, MInt32 nThreadCount)
{
    long lret = 0;

    MInt32 lDstWidth = lSrcWidth >> 1;
    MInt32 lDstHeight = lSrcHeight >> 1;

    if( cn == 1 )
    {
        lret = down_mean2x2_cn1_threads(mcvParallelMonitor, ( MInt16 * ) pSrc, lSrcStep, ( MInt16 * ) pDst, lDstWidth, lDstHeight, lDstStep,
                                        nThreadCount);
    }
    else
    {
        lret = down_mean2x2_cn2_threads(mcvParallelMonitor, ( MInt16 * ) pSrc, lSrcStep, ( MInt16 * ) pDst, lDstWidth, lDstHeight, lDstStep,
                                        nThreadCount);
    }

    return lret;
}


//#pragma mark - up
//
//typedef struct _tag_UP8
//{
//    MInt32 task_ID;
//
//    MUInt16 *pSrc;
//    MUInt16 *pSmooth;
//    MUInt8 *pDst;
//    MInt32 lDstWidth;
//    MInt32 lDstHeight;
//    MInt32 lSrcStep;
//    MInt32 lDstStep;
//
//    MInt32 lStartLine;
//    MInt32 lEndLine;
//} PARAM_UP8;
//
//
//MVoid up8_cn1_stripe(MUInt16 *pSrc, MUInt16 *pSmooth, MInt32 lSrcStep, MUInt8 *pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstStep,
//                     MInt32 rowStart, MInt32 rowEnd)
//{
//#ifdef USE_NEON_PYRAMID
//    MInt32 lWidthSm = lDstWidth >> 1;
//    MInt32 lHeightSm = lDstHeight >> 1;
//
//    rowStart = MAX(rowStart, 1);
//    rowEnd = MIN(rowEnd, lHeightSm - 2);
//
//    MInt16 arrCubic[4] = {-2304, 28416, 7424, -768};//{ 0xF700, 0x6F00, 0x1D00, 0xFD00 };//{-9,111,29,-3}
//    int16x4_t vcubic = vld1_s16(arrCubic);
//
//    MInt32 yl, ys, xl, xs;
//    for(ys = rowStart; ys < rowEnd; ys++)
//    {
//        yl = ( ys << 1 ) + 1;
//
//        MUInt16 *pSrc0 = pSrc + ( ys - 1 ) * lSrcStep;
//        MUInt16 *pSrc1 = pSrc + ( ys + 0 ) * lSrcStep;
//        MUInt16 *pSrc2 = pSrc + ( ys + 1 ) * lSrcStep;
//        MUInt16 *pSrc3 = pSrc + ( ys + 2 ) * lSrcStep;
//
//        MUInt16 *pSmooth0 = pSmooth + ( ys - 1 ) * lSrcStep;
//        MUInt16 *pSmooth1 = pSmooth + ( ys + 0 ) * lSrcStep;
//        MUInt16 *pSmooth2 = pSmooth + ( ys + 1 ) * lSrcStep;
//        MUInt16 *pSmooth3 = pSmooth + ( ys + 2 ) * lSrcStep;
//
//        MUInt8 *pDst0 = pDst + ( yl + 0 ) * lDstStep;
//        MUInt8 *pDst1 = pDst + ( yl + 1 ) * lDstStep;
//
//        int16x8_t vdif00, vdif10, vdif20, vdif30; // 前8个
//        int16x8_t vdif01, vdif11, vdif21, vdif31; // 后8个
//
//        vdif00 = vsubq_s16(vld1q_s16(( MInt16 * ) pSmooth0), vld1q_s16(( MInt16 * ) pSrc0));
//        vdif10 = vsubq_s16(vld1q_s16(( MInt16 * ) pSmooth1), vld1q_s16(( MInt16 * ) pSrc1));
//        vdif20 = vsubq_s16(vld1q_s16(( MInt16 * ) pSmooth2), vld1q_s16(( MInt16 * ) pSrc2));
//        vdif30 = vsubq_s16(vld1q_s16(( MInt16 * ) pSmooth3), vld1q_s16(( MInt16 * ) pSrc3));
//
//        int16x8_t valPos0, valRev0, valPos1, valRev1;
//        valPos0 = vaddq_s16(
//                vaddq_s16(
//                        vaddq_s16(
//                                vqrdmulhq_lane_s16(vdif00, vcubic, 0),
//                                vqrdmulhq_lane_s16(vdif10, vcubic, 1)),
//                        vqrdmulhq_lane_s16(vdif20, vcubic, 2)),
//                vqrdmulhq_lane_s16(vdif30, vcubic, 3));
//
//        valRev0 = vaddq_s16(
//                vaddq_s16(
//                        vaddq_s16(
//                                vqrdmulhq_lane_s16(vdif00, vcubic, 3),
//                                vqrdmulhq_lane_s16(vdif10, vcubic, 2)),
//                        vqrdmulhq_lane_s16(vdif20, vcubic, 1)),
//                vqrdmulhq_lane_s16(vdif30, vcubic, 0));
//
//        for(xs = 8, xl = 3; xl < lDstWidth - 15; xs += 8, xl += 16)
//        {
//            uint8x8x2_t vDst0 = vld2_u8(pDst0 + xl);
//            uint8x8x2_t vDst1 = vld2_u8(pDst1 + xl);
//
//            vdif01 = vsubq_s16(vld1q_s16(( MInt16 * ) pSmooth0 + xs), vld1q_s16(( MInt16 * ) pSrc0 + xs));
//            vdif11 = vsubq_s16(vld1q_s16(( MInt16 * ) pSmooth1 + xs), vld1q_s16(( MInt16 * ) pSrc1 + xs));
//            vdif21 = vsubq_s16(vld1q_s16(( MInt16 * ) pSmooth2 + xs), vld1q_s16(( MInt16 * ) pSrc2 + xs));
//            vdif31 = vsubq_s16(vld1q_s16(( MInt16 * ) pSmooth3 + xs), vld1q_s16(( MInt16 * ) pSrc3 + xs));
//
//            valPos1 = vaddq_s16(
//                    vaddq_s16(
//                            vaddq_s16(
//                                    vqrdmulhq_lane_s16(vdif01, vcubic, 0),
//                                    vqrdmulhq_lane_s16(vdif11, vcubic, 1)),
//                            vqrdmulhq_lane_s16(vdif21, vcubic, 2)),
//                    vqrdmulhq_lane_s16(vdif31, vcubic, 3));
//
//            valRev1 = vaddq_s16(
//                    vaddq_s16(
//                            vaddq_s16(
//                                    vqrdmulhq_lane_s16(vdif01, vcubic, 3),
//                                    vqrdmulhq_lane_s16(vdif11, vcubic, 2)),
//                            vqrdmulhq_lane_s16(vdif21, vcubic, 1)),
//                    vqrdmulhq_lane_s16(vdif31, vcubic, 0));
//
//            int16x8_t vHori0, vHori1, vHori2, vHori3;
//            int16x8_t vDifPosPos, vDifPosRev, vDifRevPos, vDifRevRev;
//
//            vHori0 = valPos0;
//            vHori1 = vextq_s16(valPos0, valPos1, 1);
//            vHori2 = vextq_s16(valPos0, valPos1, 2);
//            vHori3 = vextq_s16(valPos0, valPos1, 3);
//
//            vDifPosPos = vaddq_s16(
//                    vaddq_s16(
//                            vaddq_s16(
//                                    vqrdmulhq_lane_s16(vHori0, vcubic, 0),
//                                    vqrdmulhq_lane_s16(vHori1, vcubic, 1)),
//                            vqrdmulhq_lane_s16(vHori2, vcubic, 2)),
//                    vqrdmulhq_lane_s16(vHori3, vcubic, 3));
//
//            vDifPosRev = vaddq_s16(
//                    vaddq_s16(
//                            vaddq_s16(
//                                    vqrdmulhq_lane_s16(vHori0, vcubic, 3),
//                                    vqrdmulhq_lane_s16(vHori1, vcubic, 2)),
//                            vqrdmulhq_lane_s16(vHori2, vcubic, 1)),
//                    vqrdmulhq_lane_s16(vHori3, vcubic, 0));
//
//            vDst0.val[ 0 ] = vqrshrun_n_s16(vaddq_s16(vDifPosPos, vshll_n_u8(vDst0.val[ 0 ], 2)), 2);
//            vDst0.val[ 1 ] = vqrshrun_n_s16(vaddq_s16(vDifPosRev, vshll_n_u8(vDst0.val[ 1 ], 2)), 2);
//            vst2_u8(pDst0 + xl, vDst0);
//
//            vHori0 = valRev0;
//            vHori1 = vextq_s16(valRev0, valRev1, 1);
//            vHori2 = vextq_s16(valRev0, valRev1, 2);
//            vHori3 = vextq_s16(valRev0, valRev1, 3);
//
//            vDifRevPos = vaddq_s16(
//                    vaddq_s16(
//                            vaddq_s16(
//                                    vqrdmulhq_lane_s16(vHori0, vcubic, 0),
//                                    vqrdmulhq_lane_s16(vHori1, vcubic, 1)),
//                            vqrdmulhq_lane_s16(vHori2, vcubic, 2)),
//                    vqrdmulhq_lane_s16(vHori3, vcubic, 3));
//
//            vDifRevRev = vaddq_s16(
//                    vaddq_s16(
//                            vaddq_s16(
//                                    vqrdmulhq_lane_s16(vHori0, vcubic, 3),
//                                    vqrdmulhq_lane_s16(vHori1, vcubic, 2)),
//                            vqrdmulhq_lane_s16(vHori2, vcubic, 1)),
//                    vqrdmulhq_lane_s16(vHori3, vcubic, 0));
//
//            vDst1.val[ 0 ] = vqrshrun_n_s16(vaddq_s16(vDifRevPos, vshll_n_u8(vDst1.val[ 0 ], 2)), 2);
//            vDst1.val[ 1 ] = vqrshrun_n_s16(vaddq_s16(vDifRevRev, vshll_n_u8(vDst1.val[ 1 ], 2)), 2);
//            vst2_u8(pDst1 + xl, vDst1);
//
//
//            vdif00 = vdif01;
//            vdif10 = vdif11;
//            vdif20 = vdif21;
//            vdif30 = vdif31;
//            valPos0 = valPos1;
//            valRev0 = valRev1;
//        }
//    }
//#endif
//}

//
//
//MVoid thread_up8_cn1(MVoid* HParam)
//{
//    PARAM_UP8* pParam = (PARAM_UP8*)HParam;
//    MInt32 lret = MOK;
//
//    up8_cn1_stripe(pParam->pSrc, pParam->pSmooth, pParam->pDst, pParam->lDstWidth, pParam->lDstHeight, pParam->lSrcStep, pParam->lDstStep,
//                   pParam->lStartLine, pParam->lEndLine);
//    return;
//}
//
//
//MVoid up8_cn1(MHandle hMemMgr, MHandle mcvParallelMonitor, MUInt16* pSrc, MUInt16* pSmooth, MUInt8* pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep)
//{
//    MInt32 lWidthSm = lDstWidth >> 1;
//    MInt32 lHeightSm = lDstHeight >> 1;
//    MInt32 rowStart = 0, rowEnd = lHeightSm;
//
//    if (mcvParallelMonitor)
//    {
//        MInt32 lTaskNum = TASK_NUM;
//        MInt32 lTaskHeight = lHeightSm / lTaskNum;
//        MInt32 lTaskID[TASK_NUM] = {MNull };
//        PARAM_UP8 pParam[TASK_NUM] = {MNull };
//        MInt32 lnum = 0;
//
//        lTaskHeight = lTaskHeight >> 2 << 2;
//        for (lnum = 0; lnum < lTaskNum; lnum++)
//        {
//            pParam[lnum].lStartLine = lTaskHeight * lnum;
//            pParam[lnum].lEndLine = lTaskHeight * (lnum + 1);
//        }
//        pParam[lTaskNum - 1].lEndLine = lHeightSm;
//
//        for (lnum = 0; lnum < lTaskNum; lnum++)
//        {
//            pParam[lnum].pSrc = pSrc;
//            pParam[lnum].pDst = pDst;
//            pParam[lnum].pSmooth = pSmooth;
//            pParam[lnum].lDstWidth = lDstWidth;
//            pParam[lnum].lDstHeight = lDstHeight;
//            pParam[lnum].lSrcStep = lSrcStep;
//            pParam[lnum].lDstStep = lDstStep;
//
//        }
//
//        for (lnum = 0; lnum < lTaskNum; lnum++)
//        {
//            lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, thread_up8_cn1, (MVoid*)&pParam[lnum]);
//        }
//
//        for (lnum = 0; lnum < lTaskNum; lnum++)
//        {
//            mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
//        }
//    }
//    else
//    {
//        up8_cn1_stripe(pSrc, pSmooth, pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep, 0, lHeightSm);
//    }
//}
//
//static MVoid conv8To16(MUInt8*pSrc, MInt32 lSrcStep, MInt16 *pDst, MInt32 lDstStep, MInt32 lWidth, MInt32 lHeight, MInt32 cn)
//{
//    MInt32 x, y;
//    for (y = 0; y < lHeight; ++y)
//    {
//        MUInt8*pSrcRow = pSrc + y * lSrcStep;
//        MInt16 *pDstRow = pDst + y * lDstStep;
//        x = 0;
//#ifdef __ARM_NEON__
//        for (; x < lWidth * cn - 7; x += 8)
//		{
//			uint8x8_t vSrc = vld1_u8(pSrcRow + x);
//			vst1q_s16(pDstRow + x, vreinterpretq_s16_u16(vshll_n_u8(vSrc, 2)));
//		}
//#endif
//
//
//        for (x = 0; x < lWidth * cn; ++x)
//        {
//            pDstRow[x] = (MInt16)pSrcRow[x] << 2;
//        }
//    }
//}
//
//static MVoid conv16To8(MInt16 *pSrc, MInt32 lSrcStep, MUInt8*pDst, MInt32 lDstStep, MInt32 lWidth, MInt32 lHeight, MInt32 cn)
//{
//    MInt32 x, y;
//    for (y = 0; y < lHeight; ++y)
//    {
//        MInt16 *pSrcRow = pSrc + y * lSrcStep;
//        MUInt8*pDstRow = pDst + y * lDstStep;
//        x = 0;
//#ifdef __ARM_NEON__
//        for (x = 0; x < lWidth * cn - 7; x += 8)
//		{
//			int16x8_t vSrc = vld1q_s16(pSrcRow + x);
//			vst1_u8(pDstRow + x, vqrshrun_n_s16(vSrc, 2));
//		}
//#endif
//        for (; x < lWidth * cn; ++x)
//        {
//            MInt16 res = (pSrcRow[x] + 2) >> 2;
//            pDstRow[x] = (MUInt8)TRIMBYTE(res);
//        }
//    }
//}
//
//MInt32 up8(MHandle hMemMgr, MHandle mcvParallelMonitor, MVoid* pSrc, MVoid* pSmooth, MVoid* pDst, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lSrcStep, MInt32 lDstStep, MInt32 cn)
//{
//    MInt32 lret = 0;
//    if (cn == 1)
//    {
//        up8_cn1(hMemMgr, mcvParallelMonitor, (MUInt16*)pSrc, (MUInt16*)pSmooth, (MUInt8*)pDst, lDstWidth, lDstHeight, lSrcStep, lDstStep);
//    }
//    else
//    {
//        MInt16 *pDstBuf = (MInt16 *)MMemAlloc(hMemMgr, cn * lDstWidth * lDstHeight * sizeof(MInt16));
//        if (!pDstBuf)
//        {
//            return MERR_NO_MEMORY;
//        }
//
//        conv8To16((MUInt8*)pDst, lDstStep, pDstBuf, cn * lDstWidth, lDstWidth, lDstHeight, cn);
//
//        lret = up16(hMemMgr, mcvParallelMonitor, pSrc, pSmooth, pDstBuf, lDstWidth, lDstHeight, lSrcStep, cn * lDstWidth, cn);
//
//        conv16To8(pDstBuf, cn * lDstWidth, (MUInt8*)pDst, lDstStep, lDstWidth, lDstHeight, cn);
//
//        if (pDstBuf)
//        {
//            MMemFree(hMemMgr, pDstBuf);
//            pDstBuf = NULL;
//        }
//    }
//    return lret;
//}




///  目前只支持这两种格式
template class PyramidLayer<MUInt8>;
template class PyramidLayer<MUInt16>;
