#include <ammem.h>
#include <ArcsoftSharpen.h>
#include <up16_fix.h>
#include "Arcsoft_Up_Down_Scale_Handle.h"
#include "ArcsoftLog.h"
#include "DefineForDebug.h"
#include "anis_filtering_process8.h"
#include "anis_filtering_process16.h"


NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template<typename T>
    Arcsoft_Anisotropic_Pyramid_Up2Down<T>::Arcsoft_Anisotropic_Pyramid_Up2Down(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MInt32 layer, MInt32 nThreadCount)
    {
        LOGD("Arcsoft_Anisotropic_Pyramid_Up2Down++");
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_nThreadCount = nThreadCount;
        m_lDirection = 8;
        m_lLayer = layer;
        m_lWidth = lWidth;
        m_lHeight = lHeight;
        m_lPitch = lPitch;

        m_SrcPyrImage[0].pData = MNull; // 0层为传入图像
        m_SrcPyrImage[0].lWidth = lWidth;
        m_SrcPyrImage[0].lHeight = lHeight;
        m_SrcPyrImage[0].lStride = lPitch;

        m_DstPyrImage[0].pData = MNull; // 0层为传入图像
        m_DstPyrImage[0].lWidth = lWidth;
        m_DstPyrImage[0].lHeight = lHeight;
        m_DstPyrImage[0].lStride = lPitch;


        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            m_SrcPyrImage[i].lWidth  = lWidth >>  (i);
            m_SrcPyrImage[i].lHeight = lHeight >> (i);
            m_SrcPyrImage[i].lStride = lWidth >>  (i);
            m_SrcPyrImage[i].pData = (MUInt8*)MMemAlloc(hMemMgr, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride * sizeof(MUInt8));

            m_DstPyrImage[i].lWidth  = lWidth >>  (i);
            m_DstPyrImage[i].lHeight = lHeight >> (i);
            m_DstPyrImage[i].lStride = lWidth >>  (i);
            m_DstPyrImage[i].pData = (MUInt8*)MMemAlloc(hMemMgr, m_DstPyrImage[i].lHeight * m_DstPyrImage[i].lStride * sizeof(MUInt8));


            m_TempBuffer[i].lWidth  = lWidth >> (i);
            m_TempBuffer[i].lHeight = lHeight >> (i);
            m_TempBuffer[i].lStride = lWidth >> (i);
            m_TempBuffer[i].pData = (MUInt8*)MMemAlloc(hMemMgr, m_TempBuffer[i].lHeight * m_TempBuffer[i].lStride * sizeof(MUInt8));
        }


    }

    template<typename T>
    Arcsoft_Anisotropic_Pyramid_Up2Down<T>::~Arcsoft_Anisotropic_Pyramid_Up2Down()
    {

        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            SAFE_FREE_ARRAY(m_hMemMgr, m_SrcPyrImage[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_DstPyrImage[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_TempBuffer[i].pData);
        }

    }

    template<typename T>
    MInt32 Arcsoft_Anisotropic_Pyramid_Up2Down<T>::run(MUInt8 *pSrc, MUInt8 *pDst, LPASVLOFFSCREEN pShade,
                                                      MInt32 lWidth, MInt32 lHeight, MInt32 lPitch,
                                                      MFloat* pEps, MInt32* pSharpenIntensity,
                                                       MInt32 *absDifScale, MInt32 *weiEachRange)

    {
        LOGD("Arcsoft_Anisotropic_Pyramid_Up2Down++");
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
            LOGD("pDst == pSrc");
            isNewDst = true;
            pSrc = (T*)MMemAlloc(m_hMemMgr, lHeight*lPitch*sizeof(T));
            MMemCpy(pSrc, pDst, lHeight*lPitch*sizeof(T));
        }

        m_SrcPyrImage[0].pData = pSrc;
        m_SrcPyrImage[0].lStride = lPitch;
        m_DstPyrImage[0].pData = pDst;
        m_DstPyrImage[0].lStride = lPitch;

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
        // 获取图像金字塔
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor);

        for (MInt32 i = 1; i < m_lLayer; i++)
        {

            up_down_scale.downScale2(m_SrcPyrImage[i - 1], m_SrcPyrImage[i], gaussian3x3);

            MMemCpy(m_TempBuffer[i].pData, m_SrcPyrImage[i].pData,
                    m_SrcPyrImage[i].lStride*m_SrcPyrImage[i].lHeight*sizeof(T));
        }



        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 每层金字塔进行操作
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (MInt32 i = m_lLayer - 1; i > 0; i--)
        {
            // 单层降噪
            if (pEps[i] > 0)
            {
                anis_filtering_process8(m_mcvParallelMonitor, m_SrcPyrImage[i].pData, m_DstPyrImage[i].pData,
                                        m_DstPyrImage[i].lWidth, m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, 1,
                                        pEps[i], absDifScale[i],
                                        weiEachRange + 3 * (i),
                                        pTempShadeImg, lTempShadePitch);

            }
            else
            {

                MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride);

            }


#ifdef  BUILD_OPENCV
            sprintf(filename, "m_SrcPyrImage86[%d].jpg", i);
            mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
            sprintf(filename, "m_DstPyrImage86[%d].jpg", i);
            mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
#endif


            // 得到拉普拉斯结果图
            ImageSubImage(&m_DstPyrImage[i], &m_TempBuffer[i], 0, m_SrcPyrImage[i].lHeight);

            // 拉普拉斯上采样后，加到原图像中

            up_down_scale.upScale2(m_DstPyrImage[i], m_DstPyrImage[i-1], gaussian3x3);

            ImageAddImage(&m_SrcPyrImage[i-1], &m_DstPyrImage[i-1], 0, m_SrcPyrImage[i-1].lHeight);

        }

        {
            // 对0层进行降噪
            MInt32 i = 0;

            // 单层降噪
            if (pEps[i] > 0)
            {

                anis_filtering_process8(m_mcvParallelMonitor, m_SrcPyrImage[i].pData, m_DstPyrImage[i].pData,
                                        m_DstPyrImage[i].lWidth, m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, 1,
                                        pEps[i], absDifScale[i],
                                        weiEachRange + 3 * (i),
                                        pTempShadeImg, lTempShadePitch);

            }
            else
            {

                MMemCpy(m_DstPyrImage[i].pData, m_SrcPyrImage[i].pData, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lStride);

            }
#ifdef  BUILD_OPENCV
            sprintf(filename, "m_SrcPyrImage86[%d].jpg", i);
            mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
            sprintf(filename, "m_DstPyrImage86[%d].jpg", i);
            mat_write255(m_DstPyrImage[i].lHeight, m_DstPyrImage[i].lStride, CV_8UC1, m_DstPyrImage[i].pData, filename, 1.0);
#endif

        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 释放内存
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (isNewDst)
        {
            MMemFree(m_hMemMgr, pSrc);
            pSrc = pDst;
        }


        LOGD("Arcsoft_Anisotropic_Pyramid_Up2Down--");
        return lret;
    }

    template<typename T>
    MInt32 Arcsoft_Anisotropic_Pyramid_Up2Down<T>::run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MFloat* pEps, MInt32* pSharpenIntensity, LPASVLOFFSCREEN pShade,MInt32 *absDifScale, MInt32 *weiEachRange)
    {
        MInt32 lret = 0;

        lret = run(pSrc->ppu8Plane[0], pDst->ppu8Plane[0], pShade,
                   pSrc->i32Width, pSrc->i32Height, pSrc->pi32Pitch[0],
                   pEps, pSharpenIntensity);

        return lret;

    }

    template<class T>
    MVoid Arcsoft_Anisotropic_Pyramid_Up2Down<T>::ImageSubImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pSubImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt32 lOffset = sizeof(T) > 1 ? 512 : 128;

        T *pSrcDstData = m_pImage->pData;
        T *pSubData = pSubImg->pData;
        MInt32 lWidth = m_pImage->lWidth;
        MInt32 lHeight = m_pImage->lHeight;
        MInt32 lPitch = m_pImage->lStride;
        MInt32 lSubPitch = pSubImg->lStride;
        MInt32 x, y;

#ifdef USE_NEON_PYRAMID00
        uint8x16_t srcdata, subdata;
        uint16x8_t tmpdata;
        uint8x8_t resdata;
#endif
        for(y = lTopLine; y < lBotLine; y++)
        {
            T *tmpSrcDst = pSrcDstData + y * lPitch;
            T *tmpSub = pSubData + y * lSubPitch;

            x = 0;
#ifdef USE_NEON_PYRAMID00
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

#else
            for(; x < lWidth; x++)
            {
                MInt32 lVal = (MInt16)tmpSrcDst[ x ] - (MInt16)tmpSub[ x ] + lOffset;
                CLAMP(lVal, 0, lOffset*2-1);
                tmpSrcDst[ x ] = lVal;
            }
#endif
        }
        return;
    }

    template<class T>
    MVoid Arcsoft_Anisotropic_Pyramid_Up2Down<T>::ImageAddImage(ImageInfo<T>* m_pImage, ImageInfo<T>* pAddImg, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt32 lOffset = sizeof(T) > 1 ? 512 : 128;

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
            for(x = 0; x < lWidth; x++)
            {
                MInt32 lVal = tmpSrcDst[ x ] + tmpAdd[ x ] - lOffset;
                CLAMP(lVal, 0, lOffset*2-1);
                tmpSrcDst[ x ] = lVal;
            }
        }
        return;
    }


NS_SINFLE_IMAGE_ENHANCEMENT_END

