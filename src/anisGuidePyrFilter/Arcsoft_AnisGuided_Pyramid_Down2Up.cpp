#include "Arcsoft_AnisGuided_Pyramid_Down2Up.h"
#include <ammem.h>
#include <ArcsoftSharpen.h>
#include "up16_fix.h"
#include "Arcsoft_Up_Down_Scale_Handle.h"
#include "ArcsoftLog.h"
#include "DefineForDebug.h"
#include "AnisotropicGuidedFiltering.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    Arcsoft_AnisGuided_Pyramid_Down2Up::Arcsoft_AnisGuided_Pyramid_Down2Up(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 layer, MInt32 nThreadCount)
    {
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_nThreadCount = nThreadCount;
        m_lDirection = 4;
        m_lLayer = layer;
        m_lWidth = lWidth;
        m_lHeight = lHeight;
        m_lPitch = lPitch;

        m_SrcPyrImage0.pData = MNull; // 0层为传入图像
        m_SrcPyrImage0.lWidth = lWidth;
        m_SrcPyrImage0.lHeight = lHeight;
        m_SrcPyrImage0.lStride = lPitch;

        m_DstPyrImage0.pData = MNull; // 0层为传入图像
        m_DstPyrImage0.lWidth = lWidth;
        m_DstPyrImage0.lHeight = lHeight;
        m_DstPyrImage0.lStride = lPitch;

        m_GuidedImage[0].lWidth = lWidth;
        m_GuidedImage[0].lHeight = lHeight;
        m_GuidedImage[0].lStride = lWidth;
        m_GuidedImage[0].pData = MNull;

        m_dnShade[0].lWidth =  lWidth ;
        m_dnShade[0].lHeight = lHeight;
        m_dnShade[0].lStride =  lWidth ;
        m_dnShade[0].pData = MNull;

        for (MInt32 i = 0; i < m_lLayer - 1; i++)
        {
            m_SrcPyrImage[i].lWidth  = lWidth >>  (i + 1);
            m_SrcPyrImage[i].lHeight = lHeight >> (i + 1);
            m_SrcPyrImage[i].lStride  = lWidth >> (i + 1);
            m_SrcPyrImage[i].pData = (MInt16*)MMemAlloc(hMemMgr, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(MInt16));

            m_DstPyrImage[i].lWidth  = lWidth >>  (i + 1);
            m_DstPyrImage[i].lHeight = lHeight >> (i + 1);
            m_DstPyrImage[i].lStride  = lWidth >> (i + 1);
            m_DstPyrImage[i].pData = (MInt16*)MMemAlloc(hMemMgr, m_DstPyrImage[i].lHeight * m_DstPyrImage[i].lStride * sizeof(MInt16));
        }

        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            m_GuidedImage[i].lWidth   = lWidth >> i;
            m_GuidedImage[i].lHeight  = lHeight >> i;
            m_GuidedImage[i].lStride   = lWidth >> i;
            m_GuidedImage[i].pData = (MUInt8*)MMemAlloc(hMemMgr, m_GuidedImage[i].lHeight * m_GuidedImage[i].lStride * sizeof(MUInt8));

            m_dnShade[i].lWidth  = lWidth >>  (i);
            m_dnShade[i].lHeight = lHeight >> (i);
            m_dnShade[i].lStride  = lWidth >> (i);
            m_dnShade[i].pData = (MUInt8*)MMemAlloc(hMemMgr, m_dnShade[i].lHeight * m_dnShade[i].lStride * sizeof(MUInt8));
        }


    }
    Arcsoft_AnisGuided_Pyramid_Down2Up::~Arcsoft_AnisGuided_Pyramid_Down2Up()
    {
        for (MInt32 i = 0; i < m_lLayer - 1; i++)
        {
            SAFE_FREE_ARRAY(m_hMemMgr, m_SrcPyrImage[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_DstPyrImage[i].pData);
        }

        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            SAFE_FREE_ARRAY(m_hMemMgr, m_GuidedImage[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_dnShade[i].pData);
        }


    }


    MInt32 Arcsoft_AnisGuided_Pyramid_Down2Up::run(MUInt8 *pSrc, MUInt8 *pGuided, MUInt8 *pDst, LPASVLOFFSCREEN pShade,
                                                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 lGuidedPitch,
                                                   MFloat* pEps, MInt32* pSharpenIntensity, MInt32 lScale)

    {
        LOGD("Arcsoft_AnisGuided_Pyramid_Down2Up++");

#if CALCULATE_TIME
        BasicTimer time;
#endif

        MInt32 lret = 0;
        if (pSrc == MNull || pGuided == MNull || pDst == MNull || pEps == MNull || pSharpenIntensity == MNull)
        {
            LOGD("pSrc == MNull || pGuided == MNull || pDst == MNull || pEps == MNull || pSharpenIntensity == MNull");
            return MERR_INVALID_PARAM;
        }

        m_SrcPyrImage0.pData = pSrc;
        m_SrcPyrImage0.lStride = lPitch;
        m_DstPyrImage0.pData = pDst;
        m_DstPyrImage0.lStride = lPitch;
        m_GuidedImage[0].pData = pGuided;
        m_GuidedImage[0].lStride = lGuidedPitch;

        MBool isNewShade = false;
        if (pShade == MNull)
        {
            LOGD("pShade == MNull");
            isNewShade = true;
            pShade = new ASVLOFFSCREEN();
            pShade->ppu8Plane[0] = (MUInt8*)MMemAlloc(m_hMemMgr, lPitch*lHeight);
            MMemSet(pShade->ppu8Plane[0], 0, lWidth*lHeight);
            pShade->pi32Pitch[0] = lWidth;
        }
        ImageInfo<>::ASVLOFFSCREEN2ImageInfo(*pShade, m_dnShade[0]);

#ifdef  BUILD_OPENCV
        char filename[255];
#endif
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 逐层获取图像金字塔，并进行操作
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor);


        // 第0层
        {
#if CALCULATE_TIME
            BasicTimer time2;
#endif

#ifdef  BUILD_OPENCV
            sprintf(filename, "m_SrcPyrImage0.bmp");
            mat_write255(m_SrcPyrImage0.lHeight, m_SrcPyrImage0.lStride, CV_8UC1, m_SrcPyrImage0.pData, filename, 1.0);
#endif

            if (pEps[0] > 0)
            {
                {
                    //加中值
                    //mcvFilterMedian3x3u8(m_SrcPyrImage0.pData, m_DstPyrImage0.pData, m_SrcPyrImage0.lWidth, m_SrcPyrImage0.lHeight);

                }
                if (pSrc == pGuided)
                {
                    LOGD("pSrc == pGuided!");
                    auto obj = AnisotropicGuidedFiltering<MUInt8, MUInt8>(m_hMemMgr, m_mcvParallelMonitor, m_lDirection,
                                                                          m_nThreadCount, pShade);
                    lret = obj.Run(m_hMemMgr,
                                   m_mcvParallelMonitor,
                                   m_SrcPyrImage0.pData,
                                   m_SrcPyrImage0.lWidth,
                                   m_SrcPyrImage0.lHeight,
                                   m_SrcPyrImage0.lStride,
                                   m_DstPyrImage0.pData,
                                   m_DstPyrImage0.lStride,
                                   pEps[0],
                                   lScale);
                }
                else
                {
                    auto obj = AnisotropicGuidedFiltering<MUInt8, MUInt8>(m_hMemMgr, m_mcvParallelMonitor, m_lDirection,
                                                                          m_nThreadCount, pShade);
                    lret = obj.Run(m_hMemMgr,
                                   m_mcvParallelMonitor,
                                   m_SrcPyrImage0.pData,
                                   m_GuidedImage[0].pData,
                                   m_SrcPyrImage0.lWidth,
                                   m_SrcPyrImage0.lHeight,
                                   m_SrcPyrImage0.lStride,
                                   m_GuidedImage[0].lStride,
                                   m_DstPyrImage0.pData,
                                   m_DstPyrImage0.lStride,
                                   pEps[0],
                                   lScale);
                }
            }
            else
            {
                if (pSrc != pDst)
                {
                    MMemCpy(m_DstPyrImage0.pData, m_SrcPyrImage0.pData, m_DstPyrImage0.lHeight * m_DstPyrImage0.lStride *
                                                                        sizeof(MUInt8));
                }
            }
#ifdef  BUILD_OPENCV
            sprintf(filename, "m_DstPyrImage0.jpg");
            mat_write255(m_DstPyrImage0.lHeight, m_DstPyrImage0.lStride, CV_8UC1, m_DstPyrImage0.pData, filename, 1.0);
            sprintf(filename, "m_GuidedImage[0].jpg");
            mat_write255(m_GuidedImage[0].lHeight, m_GuidedImage[0].lWidth, CV_8UC1, m_GuidedImage[0].pData, filename, 1.0);
#endif
            if (m_lLayer > 1)
            {
                up_down_scale.downScale2(m_DstPyrImage0, m_SrcPyrImage[0], mean2x2);
            }
#if CALCULATE_TIME
            LOGD("The 0 layer pyramid is finished timer count = %fms!\n", time2.UpdateAndGetDelta());
#endif

        }

        // 其他层逐层处理后，下采样
        for (MInt32 i = 0; i < m_lLayer - 1; i++)
        {
#if CALCULATE_TIME
            BasicTimer time2;
#endif
            if (pEps[i + 1] > 0)
            {
                up_down_scale.downScale2(m_dnShade[i], m_dnShade[1 + i], mean2x2);
                ASVLOFFSCREEN ShadeTemp;
                ImageInfo<>::ImageInfo2ASVLOFFSCREEN(&m_dnShade[1 + i], ShadeTemp);

                if (pSrc == pGuided)
                {
                    LOGD("pSrc == pGuided!");
                    auto obj = AnisotropicGuidedFiltering<MInt16, MInt16>(m_hMemMgr, m_mcvParallelMonitor, m_lDirection, m_nThreadCount, &ShadeTemp);
                    lret = obj.Run(m_hMemMgr,
                                   m_mcvParallelMonitor,
                                   m_SrcPyrImage[i].pData,
                                   m_SrcPyrImage[i].lWidth,
                                   m_SrcPyrImage[i].lHeight,
                                   m_SrcPyrImage[i].lStride,
                                   m_DstPyrImage[i].pData,
                                   m_DstPyrImage[i].lStride,
                                   pEps[ i + 1 ]*16,
                                   1);
                }
                else
                {
                    up_down_scale.downScale2(m_GuidedImage[i], m_GuidedImage[1 + i], mean2x2);

                    auto obj = AnisotropicGuidedFiltering<MInt16, MUInt8>(m_hMemMgr, m_mcvParallelMonitor, m_lDirection, m_nThreadCount, &ShadeTemp);
                    lret = obj.Run(m_hMemMgr,
                                   m_mcvParallelMonitor,
                                   m_SrcPyrImage[i].pData,
                                   m_GuidedImage[i + 1].pData,
                                   m_SrcPyrImage[i].lWidth,
                                   m_SrcPyrImage[i].lHeight,
                                   m_SrcPyrImage[i].lStride,
                                   m_GuidedImage[i + 1].lStride,
                                   m_DstPyrImage[i].pData,
                                   m_DstPyrImage[i].lStride,
                                   pEps[ i + 1],
                                   1);
                }
            }
            else
            {

                MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride *
                                                                        sizeof(MInt16));

            }
#ifdef  BUILD_OPENCV
            sprintf(filename, "m_SrcPyrImage[%d].jpg", i);
            mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_16SC1, m_SrcPyrImage[i].pData, filename, 1.0/4);
            sprintf(filename, "m_DstPyrImage[%d].jpg", i);
            mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0/4);
            sprintf(filename, "m_GuidedImage[%d].jpg", i+1);
            mat_write255(m_GuidedImage[i+1].lHeight, m_GuidedImage[i+1].lWidth, CV_8UC1, m_GuidedImage[i+1].pData, filename, 1.0/4);
#endif

            if (i != m_lLayer - 2)
            {
                up_down_scale.downScale2(m_DstPyrImage[i], m_SrcPyrImage[i + 1], mean2x2);
            }
#if CALCULATE_TIME
            LOGD("The %d layer is finished timer count = %fms!\n", i+1, time2.UpdateAndGetDelta());
#endif
        }

        // 逐层锐化和上采样
        for (MInt32 i = m_lLayer - 2; i >= 1; i--)
        {
            if( pSharpenIntensity[i+1] > 0 )
            {
                ArcsoftSharpen<MInt16>().Run(m_hMemMgr, m_mcvParallelMonitor,
                                             m_DstPyrImage[i].pData,
                                             m_DstPyrImage[i].lWidth,
                                             m_DstPyrImage[i].lHeight,
                                             m_DstPyrImage[i].lStride,
                                             pSharpenIntensity[i+1]);
            }

            up16(m_hMemMgr, m_mcvParallelMonitor,
                 ( MVoid * ) m_SrcPyrImage[i].pData,
                 ( MVoid * ) m_DstPyrImage[i].pData,
                 ( MVoid * ) m_DstPyrImage[i - 1].pData,
                 m_DstPyrImage[i - 1].lWidth,
                 m_DstPyrImage[i - 1].lHeight,
                 m_DstPyrImage[ i ].lStride,
                 m_DstPyrImage[i - 1].lStride,
                 1);

#ifdef  BUILD_OPENCV
//            sprintf(filename, "m_SrcPyrImage_Restore[%d].jpg", i-1);
//            mat_write255(m_SrcPyrImage[i-1].lHeight, m_SrcPyrImage[i-1].lStride, CV_16SC1, m_SrcPyrImage[i-1].pData, filename, 1.0/4);
            sprintf(filename, "m_DstPyrImage_Restore[%d].jpg", i-1);
            mat_write255(m_DstPyrImage[i-1].lHeight, m_DstPyrImage[i-1].lStride, CV_16SC1, m_DstPyrImage[i-1].pData, filename, 1.0/4);
#endif
        }

        // 上采样到0层，锐化
        {
            if (m_lLayer > 1)
            {

                MInt32 i = 0;
                if( pSharpenIntensity[i+1] > 0 )
                {
                    ArcsoftSharpen<MInt16>().Run(m_hMemMgr, m_mcvParallelMonitor,
                                                 m_DstPyrImage[i].pData,
                                                 m_DstPyrImage[i].lWidth,
                                                 m_DstPyrImage[i].lHeight,
                                                 m_DstPyrImage[i].lStride,
                                                 pSharpenIntensity[i+1],
                                                 5);
                }

                up8(m_hMemMgr, m_mcvParallelMonitor,
                    ( MVoid * ) m_SrcPyrImage[0].pData,
                    ( MVoid * ) m_DstPyrImage[0].pData,
                    ( MVoid * ) m_DstPyrImage0.pData,
                    m_DstPyrImage0.lWidth,
                    m_DstPyrImage0.lHeight,
                    m_DstPyrImage[0].lStride,
                    m_DstPyrImage0.lStride,
                    1);
            }


            if(pSharpenIntensity[0] > 0)
            {
                ArcsoftSharpen<MUInt8>().Run(m_hMemMgr, m_mcvParallelMonitor,
                                             m_DstPyrImage0.pData,
                                             m_DstPyrImage0.lWidth,
                                             m_DstPyrImage0.lHeight,
                                             m_DstPyrImage0.lStride,
                                             pSharpenIntensity[0],
                                             10);
            }
#ifdef  BUILD_OPENCV
            MInt32 i = 0;
//            sprintf(filename, "m_SrcPyrImage0.jpg", i);
//            mat_write255(m_SrcPyrImage0.lHeight, m_SrcPyrImage0.lStride, CV_8UC1, m_SrcPyrImage0.pData, filename, 1.0);
            sprintf(filename, "m_DstPyrImage0_Restore.bmp", i);
            mat_write255(m_DstPyrImage0.lHeight, m_DstPyrImage0.lStride, CV_8UC1, m_DstPyrImage0.pData, filename, 1.0);
#endif
        }

        if (isNewShade)
        {
            SAFE_FREE_ARRAY(m_hMemMgr, pShade->ppu8Plane[0]);
            SAFE_DELETE(pShade);
        }

#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        LOGD("Arcsoft_AnisGuided_Pyramid_Down2Up--");
        return lret;
    }


    MInt32 Arcsoft_AnisGuided_Pyramid_Down2Up::run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pGuided, LPASVLOFFSCREEN pDst, MFloat* pEps, MInt32* pSharpenIntensity, LPASVLOFFSCREEN pShade, MInt32 lScale)
    {
        MInt32 lret = 0;

        lret = run(pSrc->ppu8Plane[0], pGuided->ppu8Plane[0], pDst->ppu8Plane[0], pShade,
                   pSrc->i32Width, pSrc->i32Height, pSrc->pi32Pitch[0], pGuided->pi32Pitch[0],
                   pEps, pSharpenIntensity, lScale);

        return lret;

    }


NS_SINFLE_IMAGE_ENHANCEMENT_END