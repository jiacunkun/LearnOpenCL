#include <thread>
#include "ammem.h"
#include "ArcsoftLog.h"
#include "BasicTimer.h"

#include "Arcsoft_Up_Down_Scale_Handle.h"
#include "Arcsoft_Copy_To_FilledImage.h"
#include "CopyImageToImage.h"
#include "DefineForDebug.h"
#include "NLMeans.h"
#include "NLMeans16.h"
#include "anis_filtering_process8.h"
#include "arcsoft_guided_filter.h"

#ifdef  BUILD_OPENCV
char filename[255];
#endif

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    template <typename T>
MInt32 Arcsoft_Pyramid<T>::SingleLayerDenoise(ImageInfo<T>* pSrc, ImageInfo<T>* pDst, ImageInfo<T>* pShade, ImageParam param)
{
    MInt32 lRet = 0;

    if (param.fIntensity < 1e-6)
    {
        pSrc->CopyTo(*pDst);
        return lRet;
    }

    //1、填充图像
    PaddingImage(pSrc, m_lExpandSize);
    if (pShade != MNull)
    {
        PaddingImage(pShade, m_lExpandSize/4);
    }

    //2、调用函数
    if (m_lMethod == 0)
    {
        lRet = (pShade == MNull) ? 0 : pSrc->CopyTo(*pDst);
        NLMeans nlmObj(m_hMemMgr, m_mcvParallelMonitor);
        lRet += nlmObj.Run(pSrc, pDst, pShade, param.fIntensity, m_lThreadCount);
    }
    else if (m_lMethod == 1)
    {
        MInt32 defaultWeiRange[] = { 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2 };

        MInt32 defaultDifScale[] = { 16384, 16384, 16384, 16384 };

        lRet += anis_filtering_process8(m_mcvParallelMonitor, pSrc->pData, pDst->pData,
                                        pDst->lWidth, pDst->lHeight, pDst->lStride, 1,
                                        param.fIntensity, defaultDifScale[0],
                                        defaultWeiRange,
                                        MNull, 0);
    }
    else if (m_lMethod == 2)
    {
        MInt32 defaultWeiRange[] = { 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2 };

        MInt32 defaultDifScale[] = { 16384, 16384, 16384, 16384 };

        lRet += anis_filtering_process8(m_mcvParallelMonitor, pSrc->pData, pDst->pData,
            pDst->lWidth, pDst->lHeight, pDst->lStride, 1,
            param.fIntensity, defaultDifScale[0],
            defaultWeiRange,
            MNull, 0);
    }
    else if (m_lMethod == 3) //引导滤波
    {
        ARCGF_PARAM pParam;
        ARC_GuidedFilter_GetDefaultParam(&pParam);
        pParam.gfRadius = 4;
        pParam.gfEpsilon = param.fIntensity; // * 0.0001f * 255 * 255/16;
        pParam.gfScale = 2;

        MHandle algorithmEngine = MNull;
        MInt32 gfMode = ARCGF_FAST_GUIDED_FILTER;

        lRet += ARC_GuidedFilter_Init(m_hMemMgr, &algorithmEngine, gfMode);
        CheckFuncStatus("ARC_GuidedFilter_Init", lRet);

        ASVLOFFSCREEN srcRGB;
        Arcsoft_Up_Down_Scale_Handle::ImageInfo2ASVLOFFSCREEN(*pSrc, srcRGB);

        lRet += ARC_GuidedFilter_Create(m_mcvParallelMonitor, algorithmEngine, &srcRGB, &pParam, 1);
        CheckFuncStatus("ARC_GuidedFilter_Create", lRet);

        ASVLOFFSCREEN dstRGB;
        Arcsoft_Up_Down_Scale_Handle::ImageInfo2ASVLOFFSCREEN(*pDst, dstRGB);

        lRet += ARC_GuidedFilter_Filter(m_mcvParallelMonitor, algorithmEngine, &srcRGB, &dstRGB);
        CheckFuncStatus("ARC_GuidedFilter_Filter", lRet);

        lRet += ARC_GuidedFilter_Uninit(m_hMemMgr, &algorithmEngine);
        CheckFuncStatus("ARC_GuidedFilter_Uninit", lRet);
    }
    else
    {
        lRet = (pShade == MNull) ? 0 : pSrc->CopyTo(*pDst);
    }


    ASVLOFFSCREEN src, dst, shade; // 用来显示图像，不涉及内存和算法处理，可忽略
    ImageInfo<>::ImageInfo2ASVLOFFSCREEN(pSrc, src);
    ImageInfo<>::ImageInfo2ASVLOFFSCREEN(pDst, dst);
    ImageInfo<>::ImageInfo2ASVLOFFSCREEN(pShade, shade);

    return lRet;
}

    template <typename T>
    Arcsoft_Pyramid<T>::Arcsoft_Pyramid(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lStride, MInt32 lLayer, MInt32 lThreadCount)
    {
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_lWidth = lWidth;
        m_lHeight = lHeight;
        m_lStride = lStride;
        m_lLayer = lLayer;
        m_lThreadCount = lThreadCount;
        m_lExpandSize = 8;

        for (MInt32 i = 0; i < m_lLayer; i++)
        {
            m_SrcPyrImage[i].pData = MNull;
            m_DstPyrImage[i].pData = MNull;
            m_TempBuffer[i].pData = MNull;
            m_ShadePyrImage[i].pData = MNull;
        }
    }

    template <typename T>
    Arcsoft_Pyramid<T>::~Arcsoft_Pyramid()
    {
        release();
    }

    template <typename T>
    MInt32 Arcsoft_Pyramid<T>::init()
    {
        MInt32 lRet = 0;
        for (MInt32 i = 0; i < m_lLayer; i++)
        {
            MInt32 newWidth = (m_lWidth >> i) + m_lExpandSize * 2;
            MInt32 newHeight = (m_lHeight >> i) + m_lExpandSize * 2;

            m_SrcPyrImage[i].lWidth = newWidth;
            m_SrcPyrImage[i].lHeight = newHeight;
            m_SrcPyrImage[i].lStride = newWidth;
            m_SrcPyrImage[i].pData = SAFE_MALLOC(m_hMemMgr, T, m_SrcPyrImage[i].lHeight * m_SrcPyrImage[i].lWidth);
            if (m_SrcPyrImage[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }

            m_DstPyrImage[i].lWidth = newWidth;
            m_DstPyrImage[i].lHeight = newHeight;
            m_DstPyrImage[i].lStride = newWidth;
            m_DstPyrImage[i].pData = SAFE_MALLOC(m_hMemMgr, T, m_DstPyrImage[i].lHeight * m_DstPyrImage[i].lWidth);
            if (m_SrcPyrImage[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }

            m_TempBuffer[i].lWidth = newWidth;
            m_TempBuffer[i].lHeight = newHeight;
            m_TempBuffer[i].lStride = newWidth;
            m_TempBuffer[i].pData = (i == 0) ? MNull : SAFE_MALLOC(m_hMemMgr, T, m_TempBuffer[i].lHeight * m_TempBuffer[i].lWidth);
            if (m_SrcPyrImage[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }

            m_ShadePyrImage[i].lWidth = newWidth;
            m_ShadePyrImage[i].lHeight = newHeight;
            m_ShadePyrImage[i].lStride = newWidth;
            m_ShadePyrImage[i].pData = SAFE_MALLOC(m_hMemMgr, T, m_ShadePyrImage[i].lHeight * m_ShadePyrImage[i].lWidth);
            if (m_ShadePyrImage[i].pData == MNull)
            {
                release();
                return MERR_NO_MEMORY;
            }
            MMemSet(m_ShadePyrImage[i].pData, 0, m_ShadePyrImage[i].lHeight * m_ShadePyrImage[i].lWidth * sizeof(T));
        }
        return lRet;
    }

    template <typename T>
    MVoid Arcsoft_Pyramid<T>::release()
    {
        for (MInt32 i = 0; i < m_lLayer; i++)
        {
            SAFE_FREE_ARRAY(m_hMemMgr, m_SrcPyrImage[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_DstPyrImage[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_TempBuffer[i].pData);
            SAFE_FREE_ARRAY(m_hMemMgr, m_ShadePyrImage[i].pData);
        }
    }

    template <typename T>
    MInt32 Arcsoft_Pyramid<T>::run(ImageInfo<T>* pSrc, ImageInfo<T>* pDst, ImageInfo<T>* pShade, ImageParam *pParamArray, MInt32 lMethod)
    {
        MInt32 lRet = 0;
        MInt32 lWidth = pSrc->lWidth;
        MInt32 lHeight = pSrc->lHeight;
        m_lMethod = lMethod;

        ////////////////////////////////////////////////////////////////////////////
        // 创建金字塔
        ////////////////////////////////////////////////////////////////////////////
        //src
        ImageInfo<T> SrcPyrImageInner[4];
        BuildPyramid(pSrc, m_SrcPyrImage, SrcPyrImageInner, m_lLayer, m_lExpandSize);
        for (MInt32 i = 1; i < m_lLayer; i++)
        {
            lRet += m_SrcPyrImage[i].CopyTo(m_TempBuffer[i]);
        }
        //shade
        pShade = (pShade->pData == MNull) ? MNull : pShade;
        ImageInfo<T> ShadePyrImageInner[4];
        BuildPyramid(pShade, m_ShadePyrImage, ShadePyrImageInner, m_lLayer, m_lExpandSize/4);

        
        //////////////////////////////////////////////////////////////////////////
        // 调用函数，最后拷出最大层结果
        /////////////////////////////////////////////////////////////////////////
        // 小层
        for (MInt32 i = m_lLayer - 1; i > 0; i--)
        {

            ImageInfo<T>* pTempShade = (pShade == MNull) ? MNull : &m_ShadePyrImage[i];
            lRet += SingleLayerDenoise(&m_SrcPyrImage[i], &m_DstPyrImage[i], pTempShade, pParamArray[i]);
            lRet += RestorePyramid(m_SrcPyrImage, m_DstPyrImage, m_TempBuffer, i, m_lExpandSize);
        }

        // 最大层
        {
            MInt32 i = 0;
            Rect roi = { m_lExpandSize, m_lExpandSize, lWidth, lHeight };
            ImageInfo<T> temp;

            if (pParamArray[i].fIntensity > 0.0)
            {
                ImageInfo<T>* pTempShade = (pShade == MNull) ? MNull : &m_ShadePyrImage[i];
                lRet += SingleLayerDenoise(&m_SrcPyrImage[i], &m_DstPyrImage[i], pTempShade, pParamArray[i]);
                temp = ImageInfo<T>(m_DstPyrImage[i], roi);
            }
            else
            {
                temp = ImageInfo<T>(m_SrcPyrImage[i], roi);
            }

            CopyImageToImage<T>(&temp, pDst);
        }


#ifdef  BUILD_OPENCV
        mat_write255(pSrc->lHeight, pSrc->lStride, CV_8UC1, pSrc->pData, "pSrc.png", 1.0);
        for (MInt32 i = 0; i < m_lLayer; i++)
        {
            sprintf(filename, "m_SrcPyrImage[%d].png", i);
            mat_write255(m_SrcPyrImage[i].lHeight, m_SrcPyrImage[i].lStride, CV_8UC1, m_SrcPyrImage[i].pData, filename, 1.0);
            ASVLOFFSCREEN temp;
            ImageInfo<>::ImageInfo2ASVLOFFSCREEN(&m_SrcPyrImage[i], temp);
        }
#endif

        return lRet;
    }

    template<class T>
    MInt32 Arcsoft_Pyramid<T>::BuildPyramid(ImageInfo<T>* pSrc, ImageInfo<T> *pPyramid, ImageInfo<T>* pPyramidInner, MInt32 lLayer, MInt32 lExpandSize)
    {
        LOGD("BuildPyramid++");
        MInt32 lRet = 0;

        if (pSrc == MNull)
        {
            return 0;
        }

        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor, m_lThreadCount);

        MInt32 lWidth = pSrc->lWidth;
        MInt32 lHeight = pSrc->lHeight;

        // 构造roi区域
        for (MInt32 i = 0; i < lLayer; i++)
        {
            MInt32 newWidth = (lWidth >> i);
            MInt32 newHeight = (lHeight >> i);
            Rect roi = {lExpandSize, lExpandSize, newWidth, newHeight};
            pPyramidInner[i] = ImageInfo<T>(pPyramid[i], roi);
            pPyramid[i].lWidth = newWidth + lExpandSize*2;
            pPyramid[i].lHeight = newHeight + lExpandSize*2;
        }

        // 拷贝并填充第0层
        CopyImageToImage<T>(pSrc, &pPyramidInner[0]);
        //Arcsoft_Copy_To_FilledImage<T>(pSrc, &pPyramid[0], lExpandSize);

        // 下采样，填充
        for (MInt32 i = 1; i < lLayer; i++)
        {
            lRet += up_down_scale.downScale2(pPyramidInner[i - 1], pPyramidInner[i], gaussian3x3);
            //PaddingImage(&pPyramid[i], lExpandSize);
        }

        LOGD("BuildPyramid--");
        return lRet;
    }

    template<class T>
    MInt32 Arcsoft_Pyramid<T>::RestorePyramid(ImageInfo<T>* pSrcPyramid, ImageInfo<T>* pDstPyramid, ImageInfo<T>* pTempPyramid, MInt32 iLayer, MInt32 lExpandSize)
    {
        MInt32 lRet = 0;

        Arcsoft_Up_Down_Scale_Handle up_down_scale(m_hMemMgr, m_mcvParallelMonitor, 1);

        MInt32 i = iLayer;
        ImageInfo<T> pDstPyramidInner[4];

        ImageInfo<T>::ImageSubImage(&pDstPyramid[i], &pTempPyramid[i]);
        
        Rect roi = {lExpandSize, lExpandSize, pDstPyramid[i].lWidth - lExpandSize*2, pDstPyramid[i].lHeight - lExpandSize*2};
        Rect roi1 = {lExpandSize, lExpandSize, pDstPyramid[i-1].lWidth - lExpandSize*2, pDstPyramid[i-1].lHeight - lExpandSize*2};
        pDstPyramidInner[i] = ImageInfo<T>(pDstPyramid[i], roi);
        pDstPyramidInner[i-1] = ImageInfo<T>(pDstPyramid[i-1], roi1);

        lRet += up_down_scale.upScale2(pDstPyramidInner[i], pDstPyramidInner[i - 1], gaussian3x3);
        

        ImageInfo<T>::ImageAddImage(&pSrcPyramid[i - 1], &pDstPyramid[i - 1]);

        return lRet;
    }

NS_SINFLE_IMAGE_ENHANCEMENT_END