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

@brief  金字塔封装

@version: 1.0

@author:

@date:

@change:

@note:

@todo: 1、同类功能合并
*******************************************************************************/
#ifndef _H_PYRAMID_LAYER_H_
#define _H_PYRAMID_LAYER_H_

#include <stdio.h>
#include <math.h>
#include <string.h>
// mpbase
#include "merror.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "amcomdef.h"
//#include "arc_image.h"

template<class T = MUInt8>
class PyramidLayer //: public ACS_NS::ARCIMAGE
{
public:
    PyramidLayer(MHandle hMemMgr = MNull);

    ~PyramidLayer();

public:
    MVoid SetMemMgr(MHandle hMemMgr);

    MHandle GetMemMgr();

    MByte *GetImage();

    MInt32 GetWidth();

    MInt32 GetHeight();

    MInt32 GetPitch();

    MInt32 GetElementSize();

public:
    /**********************************************************
    *  内存
    **********************************************************/
    /// @brief 创建金字塔层，申请实际内存地址
    /// @param lWidth       [in]
    /// @param lHeight      [in]
    /// @param lPitch       [in]
    /// @return
    MInt32 NewPyramidLevel(MInt32 lWidth,
                           MInt32 lHeight,
                           MInt32 lPitch,
                           MInt32 lElementSize = 1);

    MInt32 NewPyramidLevel(PyramidLayer *res);


    /// @brief 创建金字塔层，引用外部指针，内部没有实际申请内存
    /// @param lWidth
    /// @param lHeight
    /// @param lPitch
    /// @param pImage       [in]
    /// @return
    MInt32 NewPyramidLevel(MByte *pImage,
                           MInt32 lWidth,
                           MInt32 lHeight,
                           MInt32 lPitch,
                           MInt32 lElementSize = 1);


    /// @brief 释放金字塔内存
    /// @return
    MVoid FreePyramidLevel();


public:
    /**********************************************************
    *  金字塔加减操作
    **********************************************************/

    /// @brief
    /// @param pSubImg
    /// @param lTopLine
    /// @param lBotLine
    /// @return
    MVoid PySubC1(PyramidLayer *pSubImg,
                  MInt32 lTopLine,
                  MInt32 lBotLine);

    MVoid PySubC1(MByte *pSubImg,
                  MInt32 lTopLine,
                  MInt32 lBotLine);

    MVoid PySubC1Threads(PyramidLayer *pSubImg,
                         MInt32 nThreadCount = -1);

    /// @brief
    /// @param pAddImg
    /// @param lTopLine
    /// @param lBotLine
    /// @return
    MVoid PyAddC1(PyramidLayer *pAddImg,
                  MInt32 lTopLine,
                  MInt32 lBotLine);

    MVoid PyAddC1(MByte *pAddImg,
                  MInt32 lTopLine,
                  MInt32 lBotLine);

    MVoid PyAddC1Threads(PyramidLayer *pAddImg,
                         MInt32 nThreadCount = -1);


    /// @brief
    /// @param pAddImg01
    /// @param pAddImg02
    /// @param lTopLine
    /// @param lBotLine
    /// @return
    MVoid PyAddC2(PyramidLayer *pAddImg01,
                  PyramidLayer *pAddImg02,
                  MInt32 lTopLine,
                  MInt32 lBotLine);

    MVoid PyAddC2Threads(PyramidLayer *pAddImg01,
                         PyramidLayer *pAddImg02,
                         MInt32 nThreadCount = -1);

    /**********************************************************
    *  金字塔上采样
    **********************************************************/
    /// @brief 2倍上采样
    /// @param pFullImg     [out]
    /// @return
    MInt32 PyUpScale2Mean(PyramidLayer *pFullImg);

    /// @brief
    /// @param FullLine
    /// @param pHalfLine
    /// @param lFullWidth
    /// @param lHalfWidth
    /// @return
    MVoid Odd_Line_UpScaele_C1(MByte *FullLine,
                               MByte *pHalfLine,
                               MInt32 lFullWidth,
                               MInt32 lHalfWidth);


    /// @brief
    /// @param CurFullLine
    /// @param NexFullLine
    /// @param PreFullLine
    /// @param pHalfLine
    /// @param lFullWidth
    /// @param lHalfWidth
    /// @return
    MVoid Even_Odd_Line_UpScaele_C1(MByte *CurFullLine,
                                    MByte *NexFullLine,
                                    MByte *PreFullLine,
                                    MByte *pHalfLine,
                                    MInt32 lFullWidth,
                                    MInt32 lHalfWidth);

    /// @brief
    /// @param pFullImg
    /// @param lTopLine
    /// @param lBotLine
    /// @return
    MInt32 PyUpScale2(PyramidLayer *pFullImg,
                      MInt32 lTopLine,
                      MInt32 lBotLine);


    /// @brief
    /// @param pFullImg         [out]
    /// @param nThreadCount     [in]
    /// @return
    MInt32 PyUpScale2Threads(PyramidLayer *pFullImg,
                             MInt32 nThreadCount = -1);


private:
    /**********************************************************
    *  金字塔下采样
    **********************************************************/

    /// @brief 垂直方向滤波, 核[1, 2, 1]
    /// @param tmpSrc00
    /// @param tmpSrc01
    /// @param tmpSrc02
    /// @param pSmoothBuf
    /// @param lWidth
    /// @return
    MVoid VerSmooth(MByte *tmpSrc00, MByte *tmpSrc01, MByte *tmpSrc02, MShort *pSmoothBuf, MInt32 lWidth);

    /// @brief 水平方向滤波, 核[1, 2, 1]
    /// @param pSmoothBuf
    /// @param tmpDst
    /// @param lWidth
    /// @return
    MVoid HorSmooth(MShort *pSmoothBuf, MByte *tmpDst, MInt32 lWidth);

    /// @brief 水平方向滤波, 核[1, 2, 1], 下采样倍数为2, for y通道
    /// @param pSmoothBuf
    /// @param tmpDst
    /// @param lWidth
    /// @return
    MVoid HorSmoothDown2ForY(MShort *SmoothBuf, MByte *tmpDst, MInt32 lSrcWidth, MInt32 lDstWidth);

    /// @brief 水平方向滤波, 核[1, 2, 1], 下采样倍数为2, for uv 通道
    /// @param pSmoothBuf
    /// @param tmpDst
    /// @param lWidth
    /// @return
    MVoid HorSmoothDown2ForUV(MShort *SmoothBuf, MByte *tmpDstC1, MByte *tmpDstC2, MInt32 lSrcWidth, MInt32 lDstWidth);


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
    MInt32 Guass3x3Down2ForY(MHandle hMemMgr,
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
                             MShort *SmoothBuf);

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
    /// @param lTopLine
    /// @param lBotLine
    /// @param SmoothBuf
    /// @return
    MInt32 Guass3x3Down2ForUV(MHandle hMemMgr,
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
                              MShort *SmoothBuf);

public:

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
    MInt32 Guass3x3Down2ForYThreads(MHandle hMemMgr,
                                    MByte *pSrcImg,
                                    MInt32 lSrcWidth,
                                    MInt32 lSrcHeight,
                                    MInt32 lSrcPitch,
                                    MByte *pDstImg,
                                    MInt32 lDstWidth,
                                    MInt32 lDstHeight,
                                    MInt32 lDstPitch,
                                    MInt32 nThreadCount = -1);

    MInt32 Guass3x3Down2ForYThreads(MHandle hMemMgr,
                                    PyramidLayer *src,
                                    MInt32 nThreadCount = -1);

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
    MInt32 Guass3x3Down2ForUVThreads(MHandle hMemMgr,
                                     MByte *pSrcImg,
                                     MInt32 lSrcWidth,
                                     MInt32 lSrcHeight,
                                     MInt32 lSrcPitch,
                                     MByte *pDstImgC1,
                                     MByte *pDstImgC2,
                                     MInt32 lDstWidth,
                                     MInt32 lDstHeight,
                                     MInt32 lDstPitch,
                                     MInt32 nThreadCount = -1);

    MInt32 Guass3x3Down2ForUVThreads(MHandle hMemMgr,
                                     PyramidLayer *dst1,
                                     PyramidLayer *dst2,
                                     MInt32 nThreadCount = -1);


    /// @brief 均值下采样, for y 通道
    /// @param pSrcImg
    /// @param lSrcWidth
    /// @param lSrcHeight
    /// @param lSrcPitch
    /// @param pDstImg
    /// @param lDstWidth
    /// @param lDstHeight
    /// @param lDstPitch
    /// @return
    MInt32 PyMeanPooling4Y(MByte *pSrcImg,
                           MInt32 lSrcWidth,
                           MInt32 lSrcHeight,
                           MInt32 lSrcPitch,
                           MByte *pDstImg,
                           MInt32 lDstWidth,
                           MInt32 lDstHeight,
                           MInt32 lDstPitch);

    /// @brief 均值下采样, for y 通道
    /// @param dst
    /// @return
    MInt32 PyMeanPooling4Y(PyramidLayer *dst);

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
    MInt32 PyMeanPooling4UV(MByte *pSrcImg,
                            MInt32 lSrcWidth,
                            MInt32 lSrcHeight,
                            MInt32 lSrcPitch,
                            MByte *pDstImgC1,
                            MByte *pDstImgC2,
                            MInt32 lDstWidth,
                            MInt32 lDstHeight,
                            MInt32 lDstPitch);

    /// @brief
    /// @param dst1
    /// @param dst2
    /// @return
    MInt32 PyMeanPooling4UV(PyramidLayer *dst1, PyramidLayer *dst2);


    #pragma mark - down

private:

    MInt32 down_mean2x2_cn1(MUInt8 *pSrc, MInt32 lSrcStep, MUInt16 *pDst, MInt32 lDstWidth, MInt32 lDstStep, MInt32 lStartLine, MInt32 lEndLine);
    MInt32 down_mean2x2_cn1(MInt16 *pSrc, MInt32 lSrcStep, MInt16 *pDst, MInt32 lDstWidth, MInt32 lDstStep, MInt32 lStartLine, MInt32 lEndLine);
    MInt32 down_mean2x2_cn2(MUInt8 *pSrc, MInt32 lSrcStep, MUInt16 *pDst, MInt32 lDstWidth, MInt32 lDstStep, MInt32 lStartLine, MInt32 lEndLine);
    MInt32 down_mean2x2_cn2(MInt16 *pSrc, MInt32 lSrcStep, MInt16 *pDst, MInt32 lDstWidth, MInt32 lDstStep, MInt32 lStartLine, MInt32 lEndLine);



    MInt32 down_mean2x2_cn1_threads(MHandle mcvParallelMonitor, MUInt8 *pSrc, MInt32 lSrcStep, MUInt16 *pDst, MInt32 lDstWidth, MInt32 lDstHeight,
                                    MInt32 lDstStep, MInt32 nThreadCount = -1);
    MInt32 down_mean2x2_cn1_threads(MHandle mcvParallelMonitor, MInt16 *pSrc, MInt32 lSrcStep, MInt16 *pDst, MInt32 lDstWidth, MInt32 lDstHeight,
                                    MInt32 lDstStep, MInt32 nThreadCount = -1);
    MInt32 down_mean2x2_cn2_threads(MHandle mcvParallelMonitor, MUInt8 *pSrc, MInt32 lSrcStep, MUInt16 *pDst, MInt32 lDstWidth, MInt32 lDstHeight,
                                    MInt32 lDstStep, MInt32 nThreadCount = -1);
    MInt32 down_mean2x2_cn2_threads(MHandle mcvParallelMonitor, MInt16 *pSrc, MInt32 lSrcStep, MInt16 *pDst, MInt32 lDstWidth, MInt32 lDstHeight,
                                    MInt32 lDstStep, MInt32 nThreadCount = -1);


public:
    MInt32 down_mean2x2_threads(MHandle mcvParallelMonitor, MUInt8 *pSrc, MUInt16 *pDst, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcStep,
                                MInt32 lDstStep, MInt32 cn, MInt32 nThreadCount = -1);
    MInt32 down_mean2x2_threads(MHandle mcvParallelMonitor, MInt16 *pSrc, MInt16 *pDst, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcStep,
                                MInt32 lDstStep, MInt32 cn, MInt32 nThreadCount = -1);



    #pragma mark - up


//    /**
// * @brief 8bit图像上采样4倍
// * @param hMemMgr
// * @param mcvParallelMonitor
// * @param srcSmall
// * @param dstSmall
// * @param dstLarge
// * @param widthLarge
// * @param heightLarge
// * @param pitchSmall
// * @param pitchLarge
// * @param cn
// * @return
// */
//    MInt32 up8(MHandle hMemMgr, MHandle mcvParallelMonitor, MVoid* srcSmall, MVoid* dstSmall, MVoid* dstLarge, MInt32 widthLarge, MInt32 heightLarge,
//               MInt32 pitchSmall, MInt32 pitchLarge, MInt32 cn);

private:
    MByte *m_pImage;
    MHandle m_hMemMgr;
    bool m_bIsRefImage;

    MInt32 m_lWidth;
    MInt32 m_lHeight;
    MInt32 m_lPitch;
    MInt32 m_lElementSize; // 每数据占用的字节数
};

typedef PyramidLayer<MUInt8> PyramidLayerU8;
typedef PyramidLayer<MUInt16> PyramidLayerU16;

#endif
