#include <thread>
#include <ammem.h>
#include "ArcsoftLog.h"
#include "DefineForDebug.h"
//#include "AnisotropicGuidedFiltering.h"
#include "arcsoft_guided_filter.h"
#include "Arcsoft_Up_Down_Scale_Handle.h"
#include "BasicTimer.h"
//#include "RemoveBlockNoise_Pyramid.h"
#include <merror.h>
#include "CopyImageToImage.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template <typename T>
    RemoveBlockNoise_Pyramid<T>::RemoveBlockNoise_Pyramid(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lDstPitch)
    {
        LOGD("RemoveBlockNoise_Pyramid(T) = %d", sizeof(T));
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;

        m_lLayer = LAYER;
        m_lWidth = lWidth;
        m_lHeight = lHeight;
        m_lPitch = lPitch;
        m_lDstPitch = lDstPitch;
    }

    template <typename T>
    RemoveBlockNoise_Pyramid<T>::~RemoveBlockNoise_Pyramid()
    {
        release();
    }

    template <typename T>
    MInt32 RemoveBlockNoise_Pyramid<T>::init()
    {
        MInt32 lRet = 0;

        m_SrcPyrImage0.pData = MNull; // 0层为传入图像
        m_SrcPyrImage0.lWidth = m_lWidth;
        m_SrcPyrImage0.lHeight = m_lHeight;
        m_SrcPyrImage0.lStride = m_lPitch;

        m_DstPyrImage0.pData = MNull; // 0层为传入图像
        m_DstPyrImage0.lWidth = m_lWidth;
        m_DstPyrImage0.lHeight = m_lHeight;
        m_DstPyrImage0.lStride = m_lDstPitch;

        m_TempBuffer0.lWidth = m_lWidth;
        m_TempBuffer0.lHeight = m_lHeight;
        m_TempBuffer0.lStride = m_lPitch;
        m_TempBuffer0.pData = MNull;
        m_TempBuffer0.pData = (MUInt8*)MMemAlloc(m_hMemMgr, m_TempBuffer0.lStride * m_TempBuffer0.lHeight * sizeof(MUInt8));
        if (m_TempBuffer0.pData == MNull)
        {
            release();
            return MERR_NO_MEMORY;
        }

        for (MInt32 i = 0; i < LAYER; i++)
        {
            m_SrcPyrImage[i].lWidth = m_lWidth >> (i + 1);
            m_SrcPyrImage[i].lHeight = m_lHeight >> (i + 1);
            m_SrcPyrImage[i].lStride = m_lWidth >> (i + 1);
            m_SrcPyrImage[i].pData = (T*)MMemAlloc(m_hMemMgr, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(T));
            if (m_SrcPyrImage[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }

            m_DstPyrImage[i].lWidth = m_lWidth >> (i + 1);
            m_DstPyrImage[i].lHeight = m_lHeight >> (i + 1);
            m_DstPyrImage[i].lStride = m_lWidth >> (i + 1);
            m_DstPyrImage[i].pData = (T*)MMemAlloc(m_hMemMgr, m_DstPyrImage[i].lHeight * m_DstPyrImage[i].lStride * sizeof(T));
            if (m_DstPyrImage[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }

            m_TempBuffer[i].lWidth = m_lWidth >> (i + 1);
            m_TempBuffer[i].lHeight = m_lHeight >> (i + 1);
            m_TempBuffer[i].lStride = m_lWidth >> (i + 1);
            m_TempBuffer[i].pData = (T*)MMemAlloc(m_hMemMgr, m_TempBuffer[i].lHeight * m_TempBuffer[i].lStride * sizeof(T));
            if (m_TempBuffer[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }
        }
        return lRet;
    }

    template <typename T>
    MVoid  RemoveBlockNoise_Pyramid<T>::release()
    {
        for (MInt32 i = 0; i < LAYER; i++)
        {
            if (m_SrcPyrImage[i].pData)
            {
                MMemFree(m_hMemMgr, m_SrcPyrImage[i].pData);
                m_SrcPyrImage[i].pData = MNull;
            }

            if (m_DstPyrImage[i].pData)
            {
                MMemFree(m_hMemMgr, m_DstPyrImage[i].pData);
                m_DstPyrImage[i].pData = MNull;
            }

            if (m_TempBuffer[i].pData)
            {
                MMemFree(m_hMemMgr, m_TempBuffer[i].pData);
                m_TempBuffer[i].pData = MNull;
            }
        }

        if (m_TempBuffer0.pData)
        {
            MMemFree(m_hMemMgr, m_TempBuffer0.pData);
            m_TempBuffer0.pData = MNull;
        }
    }

    template <typename T>
    MInt32 RemoveBlockNoise_Pyramid<T>::run(MUInt8 *pSrc, MUInt8 *pDst, MInt32 lWidth,
                                            MInt32 lHeight, MInt32 lPitch, MFloat feps, MInt32 lLayer)
    {
        LOGD("RemoveBlockNoise_Pyramid++");
        MInt32 lret = 0;

#ifdef  BUILD_OPENCV
        char filename[255];
#endif
        MBool bIsCopy = false;
        if (0)//(pDst == pSrc) //用不到最大层，不需要拷贝了
        {
            bIsCopy = true;
            pSrc = (MUInt8*)MMemAlloc(m_hMemMgr, lHeight*lPitch);
            MMemCpy(pSrc, pDst, lHeight*lPitch);
        }


        m_SrcPyrImage0.pData = pSrc;
        m_DstPyrImage0.pData = pDst;

        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor);
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 获取图像金字塔
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (MInt32 i = 0; i < LAYER; i++)
        {
            if (i == 0)
            {
                up_down_scale.downScale2(m_SrcPyrImage0, m_SrcPyrImage[i], gaussian3x3);
            }
            else
            {
                up_down_scale.downScale2(m_SrcPyrImage[i - 1], m_SrcPyrImage[i], gaussian3x3);
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 对最小图做滤波
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        {
#if 1
#if 0 //方向引导滤波
            MFloat fEps = feps;
                fEps = sizeof(T) > 1 ? fEps*16:fEps;
                auto obj0 = AnisotropicGuidedFiltering<T, T>(8);
                lret = obj0.Run(hMemMgr, mcvParallelMonitor,
                                m_SrcPyrImage[LAYER-1].pData,  m_SrcPyrImage[LAYER-1].lWidth,  m_SrcPyrImage[LAYER-1].lHeight,  m_SrcPyrImage[LAYER-1].lStride,
                                m_DstPyrImage[LAYER-1].pData,  m_DstPyrImage[LAYER-1].lStride, fEps, 1);
#else //引导滤波

#if 0
            //todo:10bit的引导滤波暂时没有实现，先用方向引导滤波替代
                MFloat fEps = feps;
                fEps = sizeof(T) > 1 ? fEps*16:fEps;
                auto obj0 = AnisotropicGuidedFiltering<T, T>(8);
                lret = obj0.Run(m_hMemMgr, m_mcvParallelMonitor,
                                m_SrcPyrImage[LAYER-1].pData,  m_SrcPyrImage[LAYER-1].lWidth,  m_SrcPyrImage[LAYER-1].lHeight,  m_SrcPyrImage[LAYER-1].lStride,
                                m_DstPyrImage[LAYER-1].pData,  m_DstPyrImage[LAYER-1].lStride, fEps, 1);
#endif

            ARCGF_PARAM pParam;
            ARC_GuidedFilter_GetDefaultParam(&pParam);
            pParam.gfRadius = 3;
            pParam.gfEpsilon = feps * 0.4064025; // * 0.0001f * 255 * 255/16;
            pParam.gfScale = 2;

            MHandle algorithmEngine = MNull;
            MInt32 gfMode = ARCGF_FAST_GUIDED_FILTER;

            lret = ARC_GuidedFilter_Init(m_hMemMgr, &algorithmEngine, gfMode);
            CheckFuncStatus("ARC_GuidedFilter_Init", lret);

            ASVLOFFSCREEN srcRGB;
            Arcsoft_Up_Down_Scale_Handle::ImageInfo2ASVLOFFSCREEN(m_SrcPyrImage[LAYER - 1], srcRGB);

            lret = ARC_GuidedFilter_Create(m_mcvParallelMonitor, algorithmEngine, &srcRGB, &pParam, 1);
            CheckFuncStatus("ARC_GuidedFilter_Create", lret);

            ASVLOFFSCREEN dstRGB;
            Arcsoft_Up_Down_Scale_Handle::ImageInfo2ASVLOFFSCREEN(m_DstPyrImage[LAYER - 1], dstRGB);

            lret = ARC_GuidedFilter_Filter(m_mcvParallelMonitor, algorithmEngine, &srcRGB, &dstRGB);
            CheckFuncStatus("ARC_GuidedFilter_Filter", lret);

            lret = ARC_GuidedFilter_Uninit(m_hMemMgr, &algorithmEngine);
            CheckFuncStatus("ARC_GuidedFilter_Uninit", lret);

#endif

#else // 小图不做处理，检查金字塔构建是否有问题
            MMemCpy(m_DstPyrImage[LAYER-1].pData, m_SrcPyrImage[LAYER-1].pData,
                    m_SrcPyrImage[LAYER-1].lHeight* m_SrcPyrImage[LAYER-1].lStride*sizeof(T));
#endif

#ifdef  BUILD_OPENCV
            MInt32 i = LAYER - 1;
            sprintf(filename, "m_SrcPyrImage3[%d].jpg", i);
            if (sizeof(T) > 1)
            {
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_16SC1, m_SrcPyrImage[i].pData, filename, 1.0/4);
                sprintf(filename, "m_DstPyrImage3[%d].jpg", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0/4);
            }
            else
            {
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "m_DstPyrImage3[%d].jpg", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
            }
#endif
        }

        // 得到拉普拉斯结果图
        ImageSubImage(&m_DstPyrImage[LAYER-1], &m_SrcPyrImage[LAYER-1]);
        up_down_scale.upScale2(m_DstPyrImage[LAYER-1], m_TempBuffer[LAYER-2], gaussian3x3);
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 重构金字塔
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (MInt32 i = LAYER - 3; i >= 0; i--) // 16bit
        {
#if 1 // 在小图做减法，上采样

            up_down_scale.upScale2(m_TempBuffer[i + 1], m_TempBuffer[i], gaussian3x3);

#else
            //up_down_scale.upScale2(m_DstPyrImage[i + 1], m_DstPyrImage[i], gaussian5x5);
            //up_down_scale.upScale2(m_SrcPyrImage[i + 1], m_TempBuffer[i], gaussian5x5);


#ifdef  BUILD_OPENCV
            sprintf(filename, "m_DstPyrImage3Before[%d].jpg", i);
            if (sizeof(T) > 1)
            {
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0/4);
            }
            else
            {
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
            }
#endif

            // 重建上层
            ReconstructPyramid<T>(m_SrcPyrImage[i], m_TempBuffer[i], m_DstPyrImage[i]);

#ifdef  BUILD_OPENCV
            if (sizeof(T) > 1)
            {
                sprintf(filename, "m_SrcPyrImage3[%d].jpg", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_16SC1, m_SrcPyrImage[i].pData, filename, 1.0/4);
                sprintf(filename, "m_DstPyrImage3[%d].jpg", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0/4);
                sprintf(filename, "TempBuffer[%d].jpg", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_16SC1, m_TempBuffer[i].pData, filename, 1.0/4);

            }
            else
            {
                sprintf(filename, "m_SrcPyrImage3[%d].jpg", i);
                mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "m_DstPyrImage3[%d].jpg", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
                sprintf(filename, "TempBuffer[%d].jpg", i);
                mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lWidth, CV_8UC1, m_TempBuffer[i].pData, filename, 1.0);

            }
#endif
#endif
        }
        CopyImageToImage<T>(&m_SrcPyrImage0, &m_DstPyrImage0);
        up_down_scale.upScale2(m_TempBuffer[0], m_TempBuffer0, gaussian3x3);
        ImageAddImage(&m_DstPyrImage0, &m_TempBuffer0);

#ifdef  BUILD_OPENCV
        sprintf(filename, "m_SrcPyrImage0.jpg");
        mat_write255(m_SrcPyrImage0.lHeight, m_SrcPyrImage0.lWidth, CV_8UC1, m_SrcPyrImage0.pData, filename, 1.0);
        sprintf(filename, "m_DstPyrImage0.jpg");
        mat_write255(m_DstPyrImage0.lHeight, m_DstPyrImage0.lStride, CV_8UC1, m_DstPyrImage0.pData, filename, 1.0);
        sprintf(filename, "TempBuffer.jpg");
        mat_write255(m_TempBuffer0.lHeight, m_TempBuffer0.lWidth, CV_8UC1, m_TempBuffer0.pData, filename, 1.0);
#endif

#if 0
        {
            // 最大图
            up_down_scale.upScale2(m_DstPyrImage[0], m_DstPyrImage0, gaussian5x5);
            up_down_scale.upScale2(m_SrcPyrImage[0], m_TempBuffer0, gaussian5x5);

#ifdef  BUILD_OPENCV
            sprintf(filename, "m_DstPyrImage0Before.jpg");
            mat_write255(m_DstPyrImage0.lHeight, m_DstPyrImage0.lWidth, CV_8UC1, m_DstPyrImage0.pData, filename, 1.0);
#endif

            // 重建上层
            ReconstructPyramid<MUInt8>(m_SrcPyrImage0, m_TempBuffer0, m_DstPyrImage0);

#ifdef  BUILD_OPENCV
            sprintf(filename, "m_SrcPyrImage0.jpg");
            mat_write255(m_SrcPyrImage0.lHeight, m_SrcPyrImage0.lWidth, CV_8UC1, m_SrcPyrImage0.pData, filename, 1.0);
            sprintf(filename, "m_DstPyrImage0.jpg");
            mat_write255(m_DstPyrImage0.lHeight, m_DstPyrImage0.lWidth, CV_8UC1, m_DstPyrImage0.pData, filename, 1.0);
            sprintf(filename, "TempBuffer.jpg");
            mat_write255(m_DstPyrImage0.lHeight, m_DstPyrImage0.lWidth, CV_8UC1, m_TempBuffer0.pData, filename, 1.0);
#endif
        }
#endif

        if (bIsCopy)
        {
            MMemFree(m_hMemMgr, pSrc);
            pSrc = pDst;
        }

        LOGD("RemoveBlockNoise_Pyramid--");
        return lret;
    }

    template <typename T>
    MInt32 RemoveBlockNoise_Pyramid<T>::run(LPASVLOFFSCREEN pSrc,LPASVLOFFSCREEN pDst, MFloat feps, MInt32 lLayer)
    {
        return run(pSrc->ppu8Plane[0],  pDst->ppu8Plane[0],  pSrc->i32Width,
                   pSrc->i32Height,  pSrc->pi32Pitch[0],  feps,  lLayer);
    }


    template <typename T>
    template <typename T0>
    MVoid RemoveBlockNoise_Pyramid<T>::ReconstructPyramid(ImageInfo<T0> srcImage, ImageInfo<T0> tempImage, ImageInfo<T0> dstImage)
    {
        MInt32 srcWidth = srcImage.lWidth;
        MInt32 srcHeight = srcImage.lHeight;
        MInt32 srcPitch = srcImage.lStride;

        MInt16 min = 0;
        MInt16 max = sizeof(T0) > 1 ? 1020:255;

        T0 *pTempSrc = srcImage.pData;
        T0 *pTempDst = dstImage.pData;
        T0 *pTempBuffer = tempImage.pData;
        for (MInt32 y = 0; y < srcHeight; y++)
        {
            for (MInt32 x = 0; x < srcWidth; x++)
            {
                MInt32 temp = 1.0*pTempDst[x] + 1.0*pTempSrc[x] - 1.0*pTempBuffer[x];
                temp = temp < min ? min:temp;
                temp = temp > max ? max:temp;
                pTempDst[x] = temp;
            }

            pTempSrc     += srcPitch;
            pTempDst     += srcPitch;
            pTempBuffer  += srcPitch;
        }
    }

    template<class T>
    MVoid RemoveBlockNoise_Pyramid<T>::ImageSubImage(ImageInfo<T>* pImage, ImageInfo<T>* pSubImg)
    {
        LOGD("ImageSubImage++");
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 nHeight = pImage->lHeight;
#if 0//defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
       
        int threadCount =  nHeight >= 1024 ? 16 : 8;
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

            ImageSubImage(pImage, pSubImg, startHeight, endHeight);
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
            ImageSubImage(pImage,  pSubImg, 0, nHeight);
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("ImageSubImage--");
    }

    template<class T>
    MVoid RemoveBlockNoise_Pyramid<T>::ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt32 lOffset = sizeof(T) > 1 ? 512 : 128;

        T *pSrcDstData = m_pImage->pData;
        T *pSubData = pSubImg->pData;
        MInt32 lWidth = m_pImage->lWidth;
        MInt32 lHeight = m_pImage->lHeight;
        MInt32 lPitch = m_pImage->lStride;
        MInt32 lSubPitch = pSubImg->lStride;
        MInt32 x, y;

#ifdef USE_NEON00
        uint8x16_t srcdata, subdata;
        uint16x8_t tmpdata;
        uint8x8_t resdata;
#endif
        for(y = lTopLine; y < lBotLine; y++)
        {
            T *tmpSrcDst = pSrcDstData + y * lPitch;
            T *tmpSub = pSubData + y * lSubPitch;

            x = 0;
#ifdef USE_NEON00
            if (sizeof(T) == 1)
            {
                for (; x < lWidth - 15; x += 16)
                {
                    srcdata = vld1q_u8((MUInt8 *) tmpSrcDst + x);
                    subdata = vld1q_u8((MUInt8 *) tmpSub + x);

                    tmpdata = vaddl_u8(vget_low_u8(srcdata), vdup_n_u8(128));
                    tmpdata = vsubw_u8(tmpdata, vget_low_u8(subdata));
                    resdata = vmovn_u16(tmpdata);
                    vst1_u8((MUInt8 *) tmpSrcDst + x, resdata);

                    tmpdata = vaddl_u8(vget_high_u8(srcdata), vdup_n_u8(128));
                    tmpdata = vsubw_u8(tmpdata, vget_high_u8(subdata));
                    resdata = vmovn_u16(tmpdata);
                    vst1_u8((MUInt8 *) tmpSrcDst + x + 8, resdata);
                }
            }
#endif
            for(; x < lWidth; x++)
            {
                MInt32 lVal = (MInt16)tmpSrcDst[ x ] - (MInt16)tmpSub[ x ] + lOffset;
                CLAMP(lVal, 0, lOffset*2-1);
                tmpSrcDst[ x ] = lVal;
            }

        }

    }

    template<class T>
    MVoid RemoveBlockNoise_Pyramid<T>::ImageAddImage(ImageInfo<T>* pImage, ImageInfo<T>* pAddImg)
    {
        LOGD("ImageAddImage++");
#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 nHeight = pImage->lHeight;
#if 0//defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        MInt32 nHeight = pImage->lHeight;
        int threadCount = nHeight >= 1024 ? 16 : 8;
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

            ImageAddImage(pImage, pAddImg, startHeight, endHeight);
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
            ImageAddImage(pImage,  pAddImg, 0, nHeight);
#endif

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("ImageAddImage--");
    }

    template<class T>
    MVoid RemoveBlockNoise_Pyramid<T>::ImageAddImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pAddImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt16 lOffset = sizeof(T) > 1 ? 512 : 128;

        T *pSrcDstData = m_pImage->pData;
        T *pAddData = pAddImg->pData;
        MInt32 lWidth = m_pImage->lWidth;
        MInt32 lHeight = m_pImage->lHeight;
        MInt32 lPitch = m_pImage->lStride;
        MInt32 lAddPitch = pAddImg->lStride;
        MInt32 x, y;

        for(y = lTopLine; y < lBotLine; y++)
        {
            T *tmpSrcDst = pSrcDstData + y * lPitch;
            T *tmpAdd = pAddData + y * lAddPitch;
            x = 0;

            //clock_t t = clock(); //采用系统时间很耗时，不采纳

#ifdef USE_NEON00
            if (sizeof(T) == 1)
            {
                uint8x8_t srcDst_8x8;
                uint8x8_t add_8x8;
                for(; x < lWidth - 8; x+=8)
                {
                    srcDst_8x8 = vld1_u8((MUInt8*)tmpSrcDst+x);
                    add_8x8 = vld1_u8((MUInt8*)tmpAdd+x);
                    uint16x8_t val_16x8 = vaddl_u8(srcDst_8x8, add_8x8);
                    val_16x8 = vqsubq_u16(val_16x8, vdupq_n_u16(128));
                    val_16x8 = vminq_u16(val_16x8, vdupq_n_u16(255));

                    vst1_u8((MUInt8*)tmpSrcDst+x, vmovn_u16(val_16x8));
                }
            }
#endif

            for(; x < lWidth; x++)
            {
                MInt16 val = tmpSrcDst[x];
                MInt16 temp = (tmpAdd[x] - lOffset);

#if 1
                MInt16 r = val % 2 + 0;
                temp = temp > 0 ? temp - r : temp;
                temp = temp < 0 ? temp + r : temp;

#endif
                MInt16 lVal = val + temp;
                CLAMP(lVal, 0, lOffset*2-1);
                tmpSrcDst[ x ] = lVal;
            }
        }
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END