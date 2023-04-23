#include "Arcsoft_Anisotropic_Pyramid_Down2Up.h"
#include <ammem.h>
#include <ArcsoftSharpen.h>
#include <up16_fix.h>
#include "Arcsoft_Up_Down_Scale_Handle.h"
#include "ArcsoftLog.h"
#include "DefineForDebug.h"
#include "anis_filtering_process8.h"
#include "anis_filtering_process16.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    Arcsoft_Anisotropic_Pyramid_Down2Up::Arcsoft_Anisotropic_Pyramid_Down2Up(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 layer, MInt32 nThreadCount)
    {
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_nThreadCount = nThreadCount;
        m_lDirection = 8;
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



    }
    Arcsoft_Anisotropic_Pyramid_Down2Up::~Arcsoft_Anisotropic_Pyramid_Down2Up()
    {
        for (MInt32 i = 0; i < m_lLayer - 1; i++)
        {
            SAFE_FREE_ARRAY(m_hMemMgr, m_SrcPyrImage[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_DstPyrImage[i].pData);
        }

    }


    MInt32 Arcsoft_Anisotropic_Pyramid_Down2Up::run(MUInt8 *pSrc, MUInt8 *pDst, LPASVLOFFSCREEN pShade,
                                                   MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                                                   MFloat* pEps, MInt32* pSharpenIntensity, MInt32 *absDifScale, MInt32 *weiEachRange)

    {
        LOGD("Arcsoft_NLM_Pyramid++");
        MInt32 lret = 0;

        /********************************参数设置***************************************************/
        MInt32 defaultWeiRange[] = {3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2};
        if( !weiEachRange )
        {
            weiEachRange = defaultWeiRange;
        }

        MInt32 defaultDifScale[] = {16384, 16384, 16384, 16384};
        if( !absDifScale )
        {
            absDifScale = defaultDifScale;
        }

        MBool isNewDst = false;
        if (pDst == pSrc)
        {
            isNewDst = true;
            pSrc = (MUInt8*)MMemAlloc(m_hMemMgr, lHeight*lPitch*sizeof(MUInt8));
            MMemCpy(pSrc, pDst, lHeight*lPitch*sizeof(MUInt8));
        }

        m_SrcPyrImage0.pData = pSrc;
        m_SrcPyrImage0.lStride = lPitch;
        m_DstPyrImage0.pData = pDst;
        m_DstPyrImage0.lStride = lPitch;

        MUInt8 *pTempShadeImg = MNull;
        MInt32 lTempShadePitch = 0;
        if (pShade)
        {
            pTempShadeImg = pShade->ppu8Plane[0];
            lTempShadePitch = pShade->pi32Pitch[0];
        }

#ifdef  BUILD_OPENCV
        char filename[255];
#endif
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 逐层获取图像金字塔，并进行操作
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor);


        // 第0层
        {
#ifdef  BUILD_OPENCV

            sprintf(filename, "m_SrcPyrImage0.jpg");
            mat_write255(m_SrcPyrImage0.lHeight, m_SrcPyrImage0.lStride, CV_8UC1, m_SrcPyrImage0.pData, filename, 1.0);

#endif

            if (pEps[0] > 0)
            {

                    anis_filtering_process8(m_mcvParallelMonitor, m_SrcPyrImage0.pData, m_DstPyrImage0.pData,
                                            m_DstPyrImage0.lWidth, m_DstPyrImage0.lHeight, m_DstPyrImage0.lStride, 1,
                                            pEps[0], absDifScale[0], weiEachRange, pTempShadeImg, lTempShadePitch);

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
#endif
            if (m_lLayer > 1)
            {
                up_down_scale.downScale2(m_DstPyrImage0, m_SrcPyrImage[0], mean2x2);
            }

        }

        // 逐层处理后，下采样
        for (MInt32 i = 0; i < m_lLayer - 1; i++)
        {
            if (pEps[i + 1] > 0)
            {
                    anis_filtering_process16(m_mcvParallelMonitor,
                                             m_SrcPyrImage[i].pData,
                                             m_DstPyrImage[i].pData,
                                             m_SrcPyrImage[i].lWidth,
                                             m_SrcPyrImage[i].lHeight,
                                             m_SrcPyrImage[i].lStride,
                                             1,
                                             4 * pEps[i + 1],
                                             absDifScale[i + 1],
                                             weiEachRange + 3 * (i + 1),
                                             pTempShadeImg, lTempShadePitch, i + 1);

            }
            else
            {

                MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData,
                        m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(MInt16));

            }
#ifdef  BUILD_OPENCV
            sprintf(filename, "m_SrcPyrImage86[%d].jpg", i);
            mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_16SC1, m_SrcPyrImage[i].pData, filename, 1.0/4);
            sprintf(filename, "m_DstPyrImage86[%d].jpg", i);
            mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_16SC1, m_DstPyrImage[i].pData, filename, 1.0/4);
#endif

            if (i != m_lLayer - 2)
            {
                up_down_scale.downScale2(m_DstPyrImage[i], m_SrcPyrImage[i + 1], mean2x2);
            }
        }

        // 逐层锐化和上采样
        for (MInt32 i = m_lLayer - 2; i >= 1; i--)
        {
            if( pSharpenIntensity[i] > 0 )
            {
                ArcsoftSharpen<MInt16>().Run(m_hMemMgr, m_mcvParallelMonitor,
                                             m_DstPyrImage[i].pData,
                                             m_DstPyrImage[i].lWidth,
                                             m_DstPyrImage[i].lHeight,
                                             m_DstPyrImage[i].lStride,
                                             pSharpenIntensity[i]);
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
            sprintf(filename, "m_SrcPyrImage87[%d].jpg", (i-1));
            mat_write255(m_SrcPyrImage[i-1].lHeight, m_SrcPyrImage[i-1].lStride, CV_16SC1, m_SrcPyrImage[i-1].pData, filename, 1.0/4);
            sprintf(filename, "m_DstPyrImage87[%d].jpg", (i-1));
            mat_write255(m_DstPyrImage[i-1].lHeight, m_DstPyrImage[i-1].lStride, CV_16SC1, m_DstPyrImage[i-1].pData, filename, 1.0/4);
#endif
        }

        // 上采样到0层，锐化
        {
            if (m_lLayer > 1)
            {
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
                                             pSharpenIntensity[0]);
            }
#ifdef  BUILD_OPENCV
            MInt32 i = 0;
            sprintf(filename, "m_SrcPyrImage870.jpg", i);
            mat_write255(m_SrcPyrImage0.lHeight, m_SrcPyrImage0.lStride, CV_8UC1, m_SrcPyrImage0.pData, filename, 1.0);
            sprintf(filename, "m_DstPyrImage870.jpg", i);
            mat_write255(m_DstPyrImage0.lHeight, m_DstPyrImage0.lStride, CV_8UC1, m_DstPyrImage0.pData, filename, 1.0);
#endif
        }

        if (isNewDst)
        {
            MMemFree(m_hMemMgr, pSrc);
            pSrc = pDst;
        }

        return lret;
    }


    MInt32 Arcsoft_Anisotropic_Pyramid_Down2Up::run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst,
                                                    MFloat* pEps, MInt32* pSharpenIntensity, LPASVLOFFSCREEN pShade,
                                                    MInt32 *absDifScale, MInt32 *weiEachRange)
    {
        MInt32 lret = 0;

        lret = run(pSrc->ppu8Plane[0], pDst->ppu8Plane[0], pShade,
                   pSrc->i32Width, pSrc->i32Height, pSrc->pi32Pitch[0],
                   pEps, pSharpenIntensity, absDifScale, weiEachRange);

        return lret;

    }


NS_SINFLE_IMAGE_ENHANCEMENT_END
