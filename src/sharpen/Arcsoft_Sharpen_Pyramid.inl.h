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
#include <ammem.h>
#include "ArcsoftLog.h"
#include "DefineForDebug.h"
#include "Arcsoft_Up_Down_Scale_Handle.h"
#include "Up_Down_Scale_Gaussian3x3.h"
#include "ArcsoftSharpen.h"
//#include "Arcsoft_Sharpen_Pyramid.h"
#include "AnisotropicGuidedFiltering.h"
#include "anis_filtering_process8.h"

//#define GAUSS5X5 //是否用5X5高斯上下采样

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template <typename T>
    Arcsoft_Sharpen_Pyramid<T>::Arcsoft_Sharpen_Pyramid(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 layer, MInt32 lThreadCount)
    {
        LOGD("Arcsoft_Sharpen_Pyramid(T) = %d", sizeof(T));
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_lThreadCount = lThreadCount;
        m_lLayer = layer;
        m_lWidth = lWidth;
        m_lHeight = lHeight;
        m_lPitch = lPitch;

        m_SrcPyrImage[0].pData = MNull; // 0层为传入图像
        m_SrcPyrImage[0].lWidth = lWidth;
        m_SrcPyrImage[0].lHeight = lHeight;
        m_SrcPyrImage[0].lStride = lPitch / sizeof(T);
    }

    template <typename T>
    Arcsoft_Sharpen_Pyramid<T>::~Arcsoft_Sharpen_Pyramid()
    {
        release();
    }

    template <typename T>
    MVoid Arcsoft_Sharpen_Pyramid<T>::release()
    {
        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            if (m_SrcPyrImage[i].pData)
            {
                MMemFree(m_hMemMgr, m_SrcPyrImage[i].pData);
                m_SrcPyrImage[i].pData = MNull;
            }
        }

        for (MInt32 i = 0; i < m_lLayer; i++)
        {
            if (m_DetailImage[i].pData)
            {
                MMemFree(m_hMemMgr, m_DetailImage[i].pData);
                m_DetailImage[i].pData = MNull;

                SAFE_FREE_ARRAY(m_hMemMgr, m_pMeanA[i]);
                SAFE_FREE_ARRAY(m_hMemMgr, m_pMeanB[i]);
            }

            if (m_TempBuffer[i].pData)
            {
                MMemFree(m_hMemMgr, m_TempBuffer[i].pData);
                m_TempBuffer[i].pData = MNull;
            }
            SAFE_FREE_ARRAY(m_hMemMgr, m_DstPyrImage[i].pData);
        }
    }

    template <typename T>
    MInt32 Arcsoft_Sharpen_Pyramid<T>::init()
    {
        MInt32 lRet = 0;
        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            m_SrcPyrImage[i].lWidth = m_lWidth >> i;
            m_SrcPyrImage[i].lHeight = m_lHeight >> i;
            m_SrcPyrImage[i].lStride = m_lWidth >> i;
            m_SrcPyrImage[i].pData = (T*)MMemAlloc(m_hMemMgr, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(T));
            if (m_SrcPyrImage[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }
        }

        for (MInt32 i = 0; i < m_lLayer; i++)
        {
            m_DstPyrImage[i].lWidth = m_lWidth >> i;
            m_DstPyrImage[i].lHeight = m_lHeight >> i;
            m_DstPyrImage[i].lStride = m_lWidth >> i;
            m_DstPyrImage[i].pData = (T*)MMemAlloc(m_hMemMgr, m_DstPyrImage[i].lHeight * m_DstPyrImage[i].lStride * sizeof(T));
            if (m_DstPyrImage[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }

            m_DetailImage[i].lWidth = m_lWidth >> i;
            m_DetailImage[i].lHeight = m_lHeight >> i;
            m_DetailImage[i].lStride = m_lWidth >> i;
#ifdef DETAIL_U8
            m_DetailImage[i].pData = (MUInt8*)MMemAlloc(m_hMemMgr, m_DetailImage[i].lHeight * m_DetailImage[i].lStride * sizeof(MUInt8));
#else
            m_DetailImage[i].pData = (MInt16*)MMemAlloc(m_hMemMgr, m_DetailImage[i].lHeight * m_DetailImage[i].lStride * sizeof(MInt16));
#endif
            //MMemSet(m_DetailImage[i].pData, 0, m_DetailImage[i].lHeight * m_DetailImage[i].lStride * sizeof(MInt16));
            if (m_DetailImage[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }

            m_TempBuffer[i].lWidth = m_lWidth >> i;
            m_TempBuffer[i].lHeight = m_lHeight >> i;
            m_TempBuffer[i].lStride = m_lWidth >> i;
#ifdef DETAIL_U8
            m_TempBuffer[i].pData = (MUInt8*)MMemAlloc(m_hMemMgr, m_TempBuffer[i].lHeight * m_TempBuffer[i].lStride * sizeof(MUInt8));
#else
            m_TempBuffer[i].pData = (MInt16*)MMemAlloc(m_hMemMgr, m_TempBuffer[i].lHeight * m_TempBuffer[i].lStride * sizeof(MInt16));
#endif
            if (m_TempBuffer[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }

            m_pMeanA[i] = (MInt16*)MMemAlloc(m_hMemMgr, m_DetailImage[i].lHeight * m_DetailImage[i].lStride * sizeof(MInt16));
            m_pMeanB[i] = (MInt16*)MMemAlloc(m_hMemMgr, m_DetailImage[i].lHeight * m_DetailImage[i].lStride * sizeof(MInt16));
            if (m_pMeanA[i] == MNull || m_pMeanB[i] == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }
        }
        return lRet;
    }

    template <typename T>
    MInt32 Arcsoft_Sharpen_Pyramid<T>::run(T *pSrcDst,
                                           MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                                           MInt32 pIntensity[], MInt32 pRange[], MInt32 lFilterVal)
    {
        START_TIME;
        MInt32 lret = 0;

        lPitch = lPitch / sizeof(T); //数据强转后，字节数变成字数，需要除以2

        m_SrcPyrImage[0].pData = pSrcDst;

#ifdef  BUILD_OPENCV
        char filename[255];
#endif
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 获取图像金字塔
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor, m_lThreadCount);

        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            up_down_scale.downScale2(m_SrcPyrImage[i - 1], m_SrcPyrImage[i], gaussian3x3);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 每层金字塔进行操作
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ArcsoftSharpen<T> sharpenObj(m_lThreadCount);
        MFloat fEps = lFilterVal*0.2;
        MInt32 lScale0 = 2;
        MInt32 lScale1 = 2;
        if (lFilterVal > 70)
        {
            lScale1 = 2;
        }
        MFloat pEps[4] = { fEps, fEps, fEps, fEps };
        MInt32 pScale[4] = { lScale0, lScale1, 1, 1 };

        MInt32 pAB_type[4] = { 1, 1, 1, 1 };

        if (lFilterVal == 0)
        {
            // 得到边缘mask
            sharpenObj.GetMask(m_hMemMgr, m_mcvParallelMonitor, m_SrcPyrImage[1].pData,
                m_SrcPyrImage[1].lWidth, m_SrcPyrImage[1].lHeight, m_SrcPyrImage[1].lStride, 200,
                (MUInt8*)m_pMeanA[1]);
            ImageInfo<MUInt8> LargeMeanA((MUInt8*)m_pMeanA[0], m_lWidth, m_lHeight, m_lWidth);
            ImageInfo<MUInt8> MeanA((MUInt8*)m_pMeanA[1], m_lWidth / 2, m_lHeight / 2, m_lWidth / 2);
            up_down_scale.upScale2(MeanA, LargeMeanA, gaussian5x5);
        }

        // 提取细节
        for (MInt32 i = m_lLayer - 1; i >= 0; i--)
        {
            if (pIntensity[i] <= 0)
            {
                continue;
            }
#if 0 // 尝试后，效果差别即大，不然后做降噪效果好，暂停
            auto obj = AnisotropicGuidedFiltering<T, T>(m_hMemMgr, m_mcvParallelMonitor, m_pMeanA[i], m_pMeanB[i], pAB_type[i], 4, 8, MNull);
            lret = obj.Run(m_hMemMgr,
                m_mcvParallelMonitor,
                m_SrcPyrImage[i].pData,
                m_SrcPyrImage[i].lWidth,
                m_SrcPyrImage[i].lHeight,
                m_SrcPyrImage[i].lStride,
                m_DstPyrImage[i].pData,
                m_DstPyrImage[i].lStride,
                pEps[i],
                pScale[i]);
#endif
#if 0 // 尝试先对原图做降噪，再提取细节，关闭
#if 0
            MInt32 defaultWeiRange[] = { 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2 };

            MInt32 defaultDifScale[] = { 16384, 16384, 16384, 16384 };

            lret = anis_filtering_process8(m_mcvParallelMonitor, m_SrcPyrImage[i].pData, m_DstPyrImage[i].pData,
                m_SrcPyrImage[i].lWidth, m_SrcPyrImage[i].lHeight, m_DstPyrImage[i].lStride, 1,
                pEps[i], defaultDifScale[0],
                defaultWeiRange,
                MNull, 0);
#else
            MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData, m_SrcPyrImage[i].lWidth*m_SrcPyrImage[i].lHeight);
#endif
#endif

            lret = sharpenObj.GetDetailImage_U8(m_hMemMgr, m_mcvParallelMonitor, m_SrcPyrImage[i].pData,
                                                m_SrcPyrImage[i].lWidth, m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, m_DetailImage[i].pData);
    
    #ifdef  BUILD_OPENCV
            sprintf(filename, "detailImage[%d].png", i);
            mat_write255(m_DetailImage[i].lHeight, m_DetailImage[i].lWidth, CV_8UC1, m_DetailImage[i].pData, filename, 1.0);
            sprintf(filename, "srcImage[%d].png", i);
            mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lWidth, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
    #endif


            if (lFilterVal > 0)
            {
                lret = sharpenObj.GetAnisotropicDetailImage_U8(m_hMemMgr, m_mcvParallelMonitor, m_DetailImage[i].pData,
                    m_SrcPyrImage[i].pData, m_DetailImage[i].lStride,
                    m_DetailImage[i].lWidth, m_DetailImage[i].lHeight,
                    m_SrcPyrImage[i].lStride,
                    m_DetailImage[i].pData, m_DetailImage[i].lStride, pEps[i], pScale[i],
                    m_pMeanA[i], m_pMeanB[i], pAB_type[i]);
            #ifdef  BUILD_OPENCV
                sprintf(filename, "filter_detailImage[%d].png", i);
                mat_write255(m_DetailImage[i].lHeight, m_DetailImage[i].lWidth, CV_8UC1, m_DetailImage[i].pData, filename, 1.0);
            #endif

                if (i == 1 && pAB_type[0] == 0)
                {
                    ImageInfo<MInt16> LargeMeanA(m_pMeanA[i - 1], m_lWidth, m_lHeight, m_lWidth);
                    ImageInfo<MInt16> LargeMeanB(m_pMeanB[i - 1], m_lWidth, m_lHeight, m_lWidth);
                    ImageInfo<MInt16> MeanA(m_pMeanA[i], m_lWidth / 2, m_lHeight / 2, m_lWidth / 2);
                    ImageInfo<MInt16> MeanB(m_pMeanB[i], m_lWidth / 2, m_lHeight / 2, m_lWidth / 2);

                    up_down_scale.upScale2(MeanA, LargeMeanA, gaussian5x5);
                    up_down_scale.upScale2(MeanB, LargeMeanB, gaussian5x5);
                }
    #ifdef  BUILD_OPENCV
                sprintf(filename, "m_pMeanA[%d].png", i);
                mat_write255(m_DetailImage[i].lHeight, m_DetailImage[i].lWidth, CV_16SC1, m_pMeanA[i], filename, 1.0);
                sprintf(filename, "m_pMeanB[%d].png", i);
                mat_write255(m_DetailImage[i].lHeight, m_DetailImage[i].lWidth, CV_16SC1, m_pMeanB[i], filename, 1.0);
    #endif
            }
        }

        // 叠加细节
        for (MInt32 i = 0; i < m_lLayer; i++)
        {
            if (pIntensity[i] <= 0)
            {
                continue;
            }

            if (i != 0)
            {
                // 将上层细节上采样和当前层叠加
                up_down_scale.upScale2(m_DetailImage[i], m_TempBuffer[i - 1], gaussian3x3);
    #ifdef  BUILD_OPENCV
                sprintf(filename, "upScale_detailImage[%d].png", i);
                mat_write255(m_TempBuffer[i-1].lHeight, m_TempBuffer[i-1].lWidth, CV_8UC1, m_TempBuffer[i-1].pData, filename, 1.0);
    #endif
                if (lFilterVal > 0)
                {
                    sharpenObj.AddDetail_U8(m_hMemMgr, m_mcvParallelMonitor, pSrcDst, m_TempBuffer[i - 1].pData, lWidth, lHeight, lPitch, pIntensity[i], pRange[i]); // 粗纹理
                }
                else
                {
                    sharpenObj.AddDetail_U8(m_hMemMgr, m_mcvParallelMonitor, pSrcDst, m_TempBuffer[i - 1].pData, (MUInt8*)m_pMeanA[0], lWidth, lHeight, lPitch, pIntensity[i], pRange[i], 50); // 粗纹理
                }
            }
            else
            {
                if (lFilterVal > 0)
                {
                    sharpenObj.AddDetail_U8(m_hMemMgr, m_mcvParallelMonitor, pSrcDst, m_DetailImage[0].pData, lWidth, lHeight, lPitch, pIntensity[0], pRange[0]); // 细纹理
                }
                else
                {
                    sharpenObj.AddDetail_U8(m_hMemMgr, m_mcvParallelMonitor, pSrcDst, m_DetailImage[0].pData, (MUInt8*)m_pMeanA[0], lWidth, lHeight, lPitch, pIntensity[0], pRange[0], 50); // 细纹理
                }
            }
        }

        END_TIME;

        return lret;
    }

    template<class T>
    template<class T1>
    MVoid Arcsoft_Sharpen_Pyramid<T>::ImageAddImage(ImageInfo<T1>* m_pImage, ImageInfo<T1>* pAddImg, MInt32 lIntensity, MInt32 lRange, MInt32 lTopLine, MInt32 lBotLine)
    {
        LOGD("ImageAddImage++");
        LOGD("lIntensity = %d", lIntensity);
        LOGD("lRange = %d", lRange);

        T1 *pSrcDstData = m_pImage->pData;
        T1 *pAddData = pAddImg->pData;
        MInt32 lWidth = m_pImage->lWidth;
        MInt32 lHeight = m_pImage->lHeight;
        MInt32 lPitch = m_pImage->lStride;
        MInt32 lAddPitch = pAddImg->lStride;
        MInt32 x, y;

        for(y = lTopLine; y < lBotLine; y++)
        {
            T1 *tmpSrcDst = pSrcDstData + y * lPitch;
            T1 *tmpAdd = pAddData + y * lAddPitch;
            for(x = 0; x < lWidth; x++)
            {
                MInt32 lDetail = tmpAdd[ x ];
                CLAMP(lDetail, -lRange, lRange);  //截断黑白边
                MInt32 lVal = tmpSrcDst[ x ]*16 + lIntensity * lDetail;
                lVal = lVal > 0 ? (lVal + 8) / 16 : (lVal - 8) / 16;
                tmpSrcDst[ x ] = lVal;
            }
        }
        LOGD("ImageAddImage--");
    }


NS_SINFLE_IMAGE_ENHANCEMENT_END
