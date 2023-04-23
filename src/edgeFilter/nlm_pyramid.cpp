#include "nlm_pyramid.h"
#include "single_image_enhancement_define.h"
#include "DefineForDebug.h"
#include "SetLPASVLOFFSCREEN.h"
#include "build_pyramid.h"
#include "anisGuide_8bit.h"
#include "up_down_scale_gaussian5x5.h"
#include "up_down_scale.h"
#include "NLMeans2.h"
#include <mobilecv.h>

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

nlm_pyramid::nlm_pyramid()
{
    m_hMemMgr = MNull;
    m_mcvParallelMonitor = MNull;
    m_lPryLayer = 0;
    m_lWidth = 0;
    m_lHeight = 0;
    for (int i = 0; i < MAX_PYRAMID_LAYER; i++)
    {
        MMemSet(&m_pSrcPyrGau[i], 0, sizeof(ASVLOFFSCREEN));
        MMemSet(&m_pDstPyrGau[i], 0, sizeof(ASVLOFFSCREEN));
        MMemSet(&m_pTmpPyrGau[i], 0, sizeof(ASVLOFFSCREEN));
    }
}

nlm_pyramid::~nlm_pyramid()
{

}


MInt32 nlm_pyramid::init(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPryLayer)
{
    START_TIME;
    MInt32 lRet = 0;
    m_hMemMgr = hMemMgr;
    m_mcvParallelMonitor = mcvParallelMonitor;
    m_lPryLayer = lPryLayer;
    m_lExpandSize = 8;
    m_lWidth = lWidth;
    m_lHeight = lHeight;

    for (int i = 1; i < m_lPryLayer; i++)
    {
        int newWidth = (lWidth >> i) + m_lExpandSize * 2;
        int newHeight = (lHeight >> i) + m_lExpandSize * 2;
        int newStride = newWidth + 3 >> 2 << 2;

        lRet = AllocLPASVLOFFSCREEN(hMemMgr, &m_pSrcPyrGau[i], newWidth, newHeight, newStride, ASVL_PAF_GRAY);
        CHECK_ERROR(lRet);
        lRet = AllocLPASVLOFFSCREEN(hMemMgr, &m_pDstPyrGau[i], newWidth, newHeight, newStride, ASVL_PAF_GRAY);
        CHECK_ERROR(lRet);
        lRet = AllocLPASVLOFFSCREEN(hMemMgr, &m_pTmpPyrGau[i], newWidth, newHeight, newStride, ASVL_PAF_GRAY);
        CHECK_ERROR(lRet);
        lRet = AllocLPASVLOFFSCREEN(hMemMgr, &m_pMaskGau[i], lWidth >> i + 2, lHeight >> i + 2, lWidth >> i + 2, ASVL_PAF_GRAY);
        CHECK_ERROR(lRet);

        Rect Roi = { m_lExpandSize, m_lExpandSize, lWidth >> i, lHeight >> i };
        RectLPASVLOFFSCREEN(&m_pSrcPyrGau[i], &m_pSrcPyrGauInner[i], &Roi);
        RectLPASVLOFFSCREEN(&m_pDstPyrGau[i], &m_pDstPyrGauInner[i], &Roi);
        RectLPASVLOFFSCREEN(&m_pTmpPyrGau[i], &m_pTmpPyrGauInner[i], &Roi);
    }
    {
        int i = 0;
        int newWidth = (lWidth >> i) + m_lExpandSize * 2;
        int newHeight = (lHeight >> i) + m_lExpandSize * 2;
        int newStride = newWidth + 3 >> 2 << 2;

        lRet = AllocLPASVLOFFSCREEN(hMemMgr, &m_pTmpPyrGau[i], newWidth, newHeight, newStride, ASVL_PAF_GRAY);
        CHECK_ERROR(lRet);

        Rect Roi = { m_lExpandSize, m_lExpandSize, lWidth >> i, lHeight >> i };
        RectLPASVLOFFSCREEN(&m_pTmpPyrGau[i], &m_pTmpPyrGauInner[i], &Roi);
    }

    END_TIME;
    return lRet;
}

MVoid nlm_pyramid::release()
{
    START_TIME;
    for (int i = 1; i < m_lPryLayer; i++)
    {
        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pSrcPyrGau[i]);
        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pDstPyrGau[i]);
        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pTmpPyrGau[i]);
        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pMaskGau[i]);
    }
    FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pTmpPyrGau[0]);
    END_TIME;
}

MInt32 nlm_pyramid::run(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MFloat* pValue, MInt32 lRadius)
{
    START_TIME;
    MInt32 lRet = 0;

    m_pDstPyrGauInner[0] = *pDstImg;
    m_pSrcPyrGauInner[0] = *pSrcImg;
    lRet = BuildGauPyrs_u8(m_hMemMgr, m_mcvParallelMonitor, pSrcImg, m_pSrcPyrGauInner, m_lPryLayer, MNull);

    {
        int i = m_lPryLayer - 1;
        MMemCpy(m_pDstPyrGau[i].ppu8Plane[0], m_pSrcPyrGau[i].ppu8Plane[0], m_pSrcPyrGau[i].i32Height * m_pSrcPyrGau[i].pi32Pitch[0]);
    }

    // init nlm
    NLMeans2 obj(m_hMemMgr, m_mcvParallelMonitor);

    for (MInt32 i = m_lPryLayer - 1; i >= 1; i--)
    {
        if (pValue[i] > 0.0)
        {
            FillExpandPixels(&m_pDstPyrGau[i], m_lExpandSize);
            obj.run_new(&m_pDstPyrGauInner[i], &m_pTmpPyrGauInner[i], MNull, pValue[i], lRadius);
        }
        else
        {
            CopyY(&m_pTmpPyrGauInner[i], &m_pSrcPyrGauInner[i]);
        }

        ImgSubImg_add128_u8(&m_pTmpPyrGauInner[i], &m_pSrcPyrGauInner[i], &m_pDstPyrGauInner[i]);
        //Img_Guass5x5_Up2_u8(m_hMemMgr, &m_pDstPyrGauInner[i], &m_pDstPyrGauInner[i - 1]);
        Img_Guass3x3_Up2_u8(m_hMemMgr, m_mcvParallelMonitor, &m_pDstPyrGauInner[i], &m_pDstPyrGauInner[i - 1]);
        if (pValue[0] > 0.0 && i == 1)
        {
            ImgAddDiff_sub128_u8(&m_pSrcPyrGauInner[i - 1], &m_pDstPyrGauInner[i - 1], &m_pTmpPyrGauInner[i - 1]);
        }
        else
        {
            ImgAddDiff_sub128_u8(&m_pSrcPyrGauInner[i - 1], &m_pDstPyrGauInner[i - 1], &m_pDstPyrGauInner[i - 1]);
        }
        

        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pSrcPyrGau[i]);
        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pDstPyrGau[i]);
    }


    //0层
    if (pValue[0] > 0.0)
    {
        MInt32 i = 0;
        if (m_lPryLayer == 1)
        {
            obj.run_new(&m_pSrcPyrGauInner[i], &m_pDstPyrGauInner[i], MNull, pValue[i], lRadius);
        }
        else
        {
            FillExpandPixels(&m_pTmpPyrGau[i], m_lExpandSize);
            obj.run_new(&m_pTmpPyrGauInner[i], &m_pDstPyrGauInner[i], MNull, pValue[i], lRadius);
        }
    }
    

#if defined(DEBUG_OUTPUT)
    mat_write255(pDstImg->i32Height, pDstImg->pi32Pitch[0], CV_8UC1, pDstImg->ppu8Plane[0], "newGuide.png", 1.0);
#endif

    END_TIME;
    return lRet;
}

MInt32 nlm_pyramid::run_mask(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pMask, MFloat* pValue, MInt32 lRadius)
{
    START_TIME;
    MInt32 lRet = 0;

    m_pDstPyrGauInner[0] = *pDstImg;
    m_pSrcPyrGauInner[0] = *pSrcImg;
    m_pMaskGau[0] = *pMask;
    lRet = BuildGauPyrs_u8(m_hMemMgr, m_mcvParallelMonitor, pSrcImg, m_pSrcPyrGauInner, m_lPryLayer, MNull);
    lRet = BuildGauPyrs_u8(m_hMemMgr, m_mcvParallelMonitor, pMask, m_pMaskGau, m_lPryLayer, MNull);

    {
        int i = m_lPryLayer - 1;
        MMemCpy(m_pDstPyrGau[i].ppu8Plane[0], m_pSrcPyrGau[i].ppu8Plane[0], m_pSrcPyrGau[i].i32Height * m_pSrcPyrGau[i].pi32Pitch[0]);
    }

    // init nlm
    NLMeans2 obj(m_hMemMgr, m_mcvParallelMonitor);

    for (MInt32 i = m_lPryLayer - 1; i >= 1; i--)
    {
        if (pValue[i] > 0.0)
        {
            FillExpandPixels(&m_pDstPyrGau[i], m_lExpandSize);
            //CopyY(&m_pTmpPyrGauInner[i], &m_pSrcPyrGauInner[i]);
            obj.run_new(&m_pDstPyrGauInner[i], &m_pTmpPyrGauInner[i], &m_pMaskGau[i], pValue[i], lRadius);
        }
        else
        {
            CopyY(&m_pTmpPyrGauInner[i], &m_pSrcPyrGauInner[i]);
        }

        ImgSubImg_add128_u8(&m_pTmpPyrGauInner[i], &m_pSrcPyrGauInner[i], &m_pDstPyrGauInner[i]);
        //Img_Guass5x5_Up2_u8(m_hMemMgr, &m_pDstPyrGauInner[i], &m_pDstPyrGauInner[i - 1]);
        Img_Guass3x3_Up2_u8(m_hMemMgr, m_mcvParallelMonitor, &m_pDstPyrGauInner[i], &m_pDstPyrGauInner[i - 1]);
        if (pValue[0] > 0.0 && i == 1)
        {
            ImgAddDiff_sub128_u8(&m_pSrcPyrGauInner[i - 1], &m_pDstPyrGauInner[i - 1], &m_pTmpPyrGauInner[i - 1]);
        }
        else
        {
            ImgAddDiff_sub128_u8(&m_pSrcPyrGauInner[i - 1], &m_pDstPyrGauInner[i - 1], &m_pDstPyrGauInner[i - 1]);
        }


        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pSrcPyrGau[i]);
        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pDstPyrGau[i]);
    }


    //0层
    if (pValue[0] > 0.0)
    {
        MInt32 i = 0;
        if (m_lPryLayer == 1)
        {
            //CopyY(&m_pDstPyrGauInner[i], &m_pSrcPyrGauInner[i]);
            obj.run_new(&m_pSrcPyrGauInner[i], &m_pDstPyrGauInner[i], pMask, pValue[i], lRadius);
        }
        else
        {
            FillExpandPixels(&m_pTmpPyrGau[i], m_lExpandSize);
            //CopyY(&m_pDstPyrGauInner[i], &m_pTmpPyrGauInner[i]);
            obj.run_new(&m_pTmpPyrGauInner[i], &m_pDstPyrGauInner[i], pMask, pValue[i], lRadius);
        }
    }


#if defined(DEBUG_OUTPUT)
    mat_write255(pDstImg->i32Height, pDstImg->pi32Pitch[0], CV_8UC1, pDstImg->ppu8Plane[0], "newGuide.png", 1.0);
#endif

    END_TIME;
    return lRet;
}

template<typename T1>
MVoid fillExpandPixels(T1* pSrc,
    MInt32 lWidth,
    MInt32 lHeight,
    MInt32 lStride,
    MInt32 lExpandSize)
{

    for (MInt32 y = lExpandSize; y < lHeight - lExpandSize; y++)
    {
        T1* pData = pSrc + y * lStride;
        for (MInt32 k = 0; k < lExpandSize; k++)
        {
            pData[k] = pData[lExpandSize];
            pData[lWidth - lExpandSize + k] = pData[lWidth - lExpandSize - 1];
        }
    }

    for (MInt32 y = 0; y < lExpandSize; y++)
    {
        MMemCpy(pSrc + y * lStride,
            pSrc + lExpandSize * lStride, lStride * sizeof(T1));
    }

    for (MInt32 y = lHeight - lExpandSize; y < lHeight; y++)
    {
        MMemCpy(pSrc + y * lStride,
            pSrc + (lHeight - lExpandSize - 1) * lStride, lStride * sizeof(T1));
    }
}

MVoid  nlm_pyramid::FillExpandPixels(LPASVLOFFSCREEN pSrcDst, MInt32 lExpandSize)
{
    START_TIME;
    fillExpandPixels<MUInt8>(
        pSrcDst->ppu8Plane[0],
        pSrcDst->i32Width,
        pSrcDst->i32Height,
        pSrcDst->pi32Pitch[0],
        lExpandSize);
    END_TIME;
}

static MVoid ImgAddDiff_sub128_u8_(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDiffImg, LPASVLOFFSCREEN pDstImg, MInt32 lTopLine, MInt32 lBotLine)
{
    MInt32 lWidth = pSrcImg->i32Width;
    MInt32 lHeight = pSrcImg->i32Height;

#ifdef USE_NEON
    uint16x8_t tmp128 = vdupq_n_u16(128);
#endif
#ifdef NH_ENABLE_OPENMP0
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
    for (MInt32 y = lTopLine; y < lBotLine; y++)
    {
        MByte* pDataSrc1 = pSrcImg->ppu8Plane[0] + y * pSrcImg->pi32Pitch[0];
        MByte* pDataSrc2 = pDiffImg->ppu8Plane[0] + y * pDiffImg->pi32Pitch[0];
        MByte* pDataDst = pDstImg->ppu8Plane[0] + y * pDstImg->pi32Pitch[0];
        MInt32 x = 0;
#ifdef USE_NEON

        for (; x < lWidth - 7; x += 8)
        {
            uint8x8_t srcdata = vld1_u8(pDataSrc1 + x);
            uint8x8_t subdata = vld1_u8(pDataSrc2 + x);
            uint16x8_t tmpdata = vqsubq_u16(vaddl_u8(srcdata, subdata), tmp128);
            uint8x8_t resdata = vqmovn_u16(tmpdata);
            vst1_u8((MUInt8*)pDataDst + x, resdata);
        }
#endif
        for (; x < lWidth; x++)
        {
            pDataDst[x] = MIN(255, ABS(pDataSrc1[x] + pDataSrc2[x] - 128));
        }
    }
}

struct IMG_SG_UPSCALE
{
    LPASVLOFFSCREEN pSrcImg;
    LPASVLOFFSCREEN pDiffImg;
    LPASVLOFFSCREEN pDstImg;
    MInt32 lTopLine;
    MInt32 lBotLine;
};

MVoid nlm_pyramid::ImgAddDiff_sub128_u8(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDiffImg, LPASVLOFFSCREEN pDstImg)
{
    START_TIME;
    MInt32 lHeight = pSrcImg->i32Height;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    {
        MInt32 lTaskNum = lHeight > 1024 ? 16 : 8;
        /// 设置回调函数
        auto func_lamda = [](MVoid* HParam) -> MVoid
        {
            auto SG_NLM_sturct = (IMG_SG_UPSCALE*)HParam;

            ImgAddDiff_sub128_u8_(SG_NLM_sturct->pSrcImg,
                SG_NLM_sturct->pDiffImg,
                SG_NLM_sturct->pDstImg,
                SG_NLM_sturct->lTopLine,
                SG_NLM_sturct->lBotLine);

    };
        MVoid(*func)(MVoid*) = func_lamda;



        /// 设置参数
        MInt32 lTaskHeight = (lHeight + lTaskNum / 2) / lTaskNum;;
        lTaskHeight = (lTaskHeight >> 2) << 2;

        IMG_SG_UPSCALE pParam[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].lTopLine = lTaskHeight * lnum;
            pParam[lnum].lBotLine = lTaskHeight * (lnum + 1);
        }
        pParam[0].lTopLine = 0;
        pParam[lTaskNum - 1].lBotLine = lHeight;


        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].pSrcImg = pSrcImg;
            pParam[lnum].pDiffImg = pDiffImg;
            pParam[lnum].pDstImg = pDstImg;
        }

        /// 创建线程     
        MInt32 lTaskID[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[lnum] = mcvAddTask(m_mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);  
        }

        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            mcvWaitTask(m_mcvParallelMonitor, lTaskID[lnum]);
        }
}

#else
    ImgAddDiff_sub128_u8_(pSrcImg, pDiffImg, pDstImg, 0, lHeight);
#endif
    END_TIME;
}

static MVoid ImgSubImg_add128_u8_(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pSubImg, LPASVLOFFSCREEN pDstImg, MInt32 lTopLine, MInt32 lBotLine)
{
    MInt32 lWidth = pSrcImg->i32Width;
    MInt32 lHeight = pSrcImg->i32Height;

#ifdef USE_NEON
    uint16x8_t tmp128 = vdupq_n_u16(128);
#endif

#ifdef NH_ENABLE_OPENMP0
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
    for (MInt32 y = lTopLine; y < lBotLine; y++)
    {
        MByte* pDataSrc1 = pSrcImg->ppu8Plane[0] + y * pSrcImg->pi32Pitch[0];
        MByte* pDataSrc2 = pSubImg->ppu8Plane[0] + y * pSubImg->pi32Pitch[0];
        MByte* pDataDst = pDstImg->ppu8Plane[0] + y * pDstImg->pi32Pitch[0];
        MInt32 x = 0;
#ifdef USE_NEON

        for (; x < lWidth - 7; x += 8)
        {
            uint8x8_t srcdata = vld1_u8(pDataSrc1 + x);
            uint8x8_t subdata = vld1_u8(pDataSrc2 + x);
            uint16x8_t tmpdata = vsubw_u8(vaddw_u8(tmp128, srcdata), subdata);
            uint8x8_t resdata = vqmovn_u16(tmpdata);
            vst1_u8((MUInt8*)pDataDst + x, resdata);
        }
#endif
        for (; x < lWidth; x++)
        {
            pDataDst[x] = MIN(255, ABS(128 + pDataSrc1[x] - pDataSrc2[x]));
        }
    }
}

MVoid nlm_pyramid::ImgSubImg_add128_u8(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pSubImg, LPASVLOFFSCREEN pDstImg)
{
    START_TIME;
    MInt32 lHeight = pSrcImg->i32Height;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    {
        MInt32 lTaskNum = lHeight > 1024 ? 16 : 8;
        /// 设置回调函数
        auto func_lamda = [](MVoid* HParam) -> MVoid
        {
            auto SG_NLM_sturct = (IMG_SG_UPSCALE*)HParam;

            ImgSubImg_add128_u8_(SG_NLM_sturct->pSrcImg,
                SG_NLM_sturct->pDiffImg,
                SG_NLM_sturct->pDstImg,
                SG_NLM_sturct->lTopLine,
                SG_NLM_sturct->lBotLine);

        };
        MVoid(*func)(MVoid*) = func_lamda;



        /// 设置参数
        MInt32 lTaskHeight = (lHeight + lTaskNum / 2) / lTaskNum;;
        lTaskHeight = (lTaskHeight >> 2) << 2;

        IMG_SG_UPSCALE pParam[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].lTopLine = lTaskHeight * lnum;
            pParam[lnum].lBotLine = lTaskHeight * (lnum + 1);
        }
        pParam[0].lTopLine = 0;
        pParam[lTaskNum - 1].lBotLine = lHeight;


        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].pSrcImg = pSrcImg;
            pParam[lnum].pDiffImg = pSubImg;
            pParam[lnum].pDstImg = pDstImg;
        }

        /// 创建线程     
        MInt32 lTaskID[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[lnum] = mcvAddTask(m_mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
        }

        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            mcvWaitTask(m_mcvParallelMonitor, lTaskID[lnum]);
        }
    }

#else
    ImgSubImg_add128_u8_(pSrcImg, pSubImg, pDstImg, 0, lHeight);
#endif
    END_TIME;
}


NS_SINFLE_IMAGE_ENHANCEMENT_END