#include <build_pyramid.h>
#include <up_down_scale.h>
#include "Sharpen_Pyramid_U8.h"
#include "anisGuide_8bit.h"
#include "SetLPASVLOFFSCREEN.h"
#include "GaussMinus5x5.h"
#include <thread>
#include "DefineForDebug.h"
#include "NLMeans2.h"
#include <mobilecv.h>
#include "gaussian_filter.h"
#include "SobelFilter.h"
#include <levels_adjust.h>

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

Sharpen_Pyramid_U8::Sharpen_Pyramid_U8()
{

}

Sharpen_Pyramid_U8::~Sharpen_Pyramid_U8()
{

}

MInt32 Sharpen_Pyramid_U8::init(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lPryLayer)
{
    START_TIME;
    MInt32 lRet = 0;
    m_hMemMgr = hMemMgr;
    m_mcvParallelMonitor = mcvParallelMonitor;
    m_lPryLayer = lPryLayer;
    m_lExpandSize = 8;
    m_lWidth = lWidth;
    m_lHeight = lHeight;

    for (int i = 0; i < m_lPryLayer; i++)
    {
        int newWidth = (lWidth >> i) + m_lExpandSize * 2;
        int newHeight = (lHeight >> i) + m_lExpandSize * 2;
        int newStride = newWidth + 3 >> 2 << 2;

        if (i != 0)
        {
            lRet = AllocLPASVLOFFSCREEN(hMemMgr, &m_pSrcPyrGau[i], newWidth, newHeight, newWidth, ASVL_PAF_GRAY);
            CHECK_ERROR(lRet);
        }
        lRet = AllocLPASVLOFFSCREEN(hMemMgr, &m_pDetailPyrGau[i], newWidth, newHeight, newWidth, ASVL_PAF_GRAY);
        CHECK_ERROR(lRet);

        lRet = AllocLPASVLOFFSCREEN(hMemMgr, &m_pMeanA[i], newWidth, newHeight, newWidth, ASVL_PAF_GRAY);
        CHECK_ERROR(lRet);
        lRet = AllocLPASVLOFFSCREEN(hMemMgr, &m_pMeanB[i], newWidth, newHeight, newWidth, ASVL_PAF_GRAY);
        CHECK_ERROR(lRet);


        Rect Roi = { m_lExpandSize, m_lExpandSize, lWidth >> i, lHeight >> i };
        RectLPASVLOFFSCREEN(&m_pSrcPyrGau[i], &m_pSrcPyrGauInner[i], &Roi);
        RectLPASVLOFFSCREEN(&m_pDetailPyrGau[i], &m_pDetailPyrGauInner[i], &Roi);
        RectLPASVLOFFSCREEN(&m_pMeanA[i], &m_pMeanAInner[i], &Roi);
        RectLPASVLOFFSCREEN(&m_pMeanB[i], &m_pMeanBInner[i], &Roi);
    }

    END_TIME;
    return lRet;
}

MVoid Sharpen_Pyramid_U8::release()
{
    START_TIME;
    for (int i = 0; i < m_lPryLayer; i++)
    {
        if (i != 0)
        {
            FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pSrcPyrGau[i]);
        }
        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pDetailPyrGau[i]);
        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pMeanA[i]);
        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_pMeanB[i]);
    }
    END_TIME;
}

MInt32 Sharpen_Pyramid_U8::run(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers)
{
    START_TIME;
    MInt32 lRet = 0;

    m_pSrcPyrGauInner[0] = *pSrc;
    lRet = BuildGauPyrs_u8(m_hMemMgr, m_mcvParallelMonitor, pSrc, m_pSrcPyrGauInner, lLayers, MNull);
    CHECK_ERROR(lRet);
    for (MInt32 i = lLayers - 1; i >= 0; i--)
    {
        lRet = GetDetail(m_hMemMgr, m_mcvParallelMonitor, &m_pSrcPyrGauInner[i], &m_pDetailPyrGauInner[i]);
        CHECK_ERROR(lRet);
        if (lFilterVal > 0)
        {
            lRet = FilterDetail(lFilterVal, i);
            CHECK_ERROR(lRet);
        }
    }
    for (MInt32 i = 0; i < lLayers; i++)
    {
        if (i > 0)
        {
            lRet = Img_Guass3x3_Up2_u8(m_hMemMgr, m_mcvParallelMonitor, &m_pDetailPyrGauInner[i], &m_pDetailPyrGauInner[i - 1]);
            CHECK_ERROR(lRet);
        }
        AddDetail_U8(m_hMemMgr, m_mcvParallelMonitor,
            pSrc, &m_pDetailPyrGauInner[0], pDst,
            pIntensity[i], pRange[i]);
        CHECK_ERROR(lRet);
#ifdef  BUILD_OPENCV
        char filename[255];
        sprintf(filename, "addDetail_detail[%d].png", i);
        mat_write255(m_pDetailPyrGauInner[0].i32Height, m_pDetailPyrGauInner[0].pi32Pitch[0], CV_8UC1, m_pDetailPyrGauInner[0].ppu8Plane[0], filename, 1.0);
        sprintf(filename, "addDetail_dst[%d].png", i);
        mat_write255(pSrc[0].i32Height, pSrc[0].pi32Pitch[0], CV_8UC1, pSrc[0].ppu8Plane[0], filename, 1.0);
#endif
    }


    END_TIME;
    return lRet;
}

MInt32 Sharpen_Pyramid_U8::run_nlm(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers)
{
    START_TIME;
    MInt32 lRet = 0;

    m_pSrcPyrGauInner[0] = *pSrc;
    lRet = BuildGauPyrs_u8(m_hMemMgr, m_mcvParallelMonitor, pSrc, m_pSrcPyrGauInner, lLayers, MNull);
    CHECK_ERROR(lRet);

    // init nlm
    NLMeans2 obj(m_hMemMgr, m_mcvParallelMonitor);

    for (MInt32 i = lLayers - 1; i >= 0; i--)
    {
        lRet = GetDetail(m_hMemMgr, m_mcvParallelMonitor, &m_pSrcPyrGauInner[i], &m_pMeanAInner[i]);
        CHECK_ERROR(lRet);
        if (lFilterVal > 0)
        {
            lFilterVal = lFilterVal >> (i);
            FillExpandPixels(&m_pMeanA[i], m_lExpandSize);
            lRet = obj.run_new(&m_pMeanAInner[i], &m_pDetailPyrGauInner[i], MNull, lFilterVal, 2);
            CHECK_ERROR(lRet);
        }
        else
        {
            CopyY(&m_pDetailPyrGauInner[i], &m_pMeanAInner[i]);
        }
    }
    for (MInt32 i = 0; i < lLayers; i++)
    {
        if (i > 0)
        {
            lRet = Img_Guass3x3_Up2_u8(m_hMemMgr, m_mcvParallelMonitor, &m_pDetailPyrGauInner[i], &m_pDetailPyrGauInner[i - 1]);
            CHECK_ERROR(lRet);
        }
        AddDetail_U8(m_hMemMgr, m_mcvParallelMonitor,
            pSrc, &m_pDetailPyrGauInner[0], pDst,
            pIntensity[i], pRange[i]);
        CHECK_ERROR(lRet);
#ifdef  BUILD_OPENCV
        char filename[255];
        sprintf(filename, "addDetail_detail_before[%d].png", i);
        mat_write255(m_pMeanAInner[0].i32Height, m_pMeanAInner[0].pi32Pitch[0], CV_8UC1, m_pMeanAInner[0].ppu8Plane[0], filename, 1.0);
        sprintf(filename, "addDetail_detail[%d].png", i);
        mat_write255(m_pDetailPyrGauInner[0].i32Height, m_pDetailPyrGauInner[0].pi32Pitch[0], CV_8UC1, m_pDetailPyrGauInner[0].ppu8Plane[0], filename, 1.0);
        sprintf(filename, "addDetail_dst[%d].png", i);
        mat_write255(pSrc[0].i32Height, pSrc[0].pi32Pitch[0], CV_8UC1, pSrc[0].ppu8Plane[0], filename, 1.0);
#endif
    }


    END_TIME;
    return lRet;
}

MInt32 Sharpen_Pyramid_U8::run_nlm2(LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDst, MInt32* pIntensity, MInt32* pRange, MInt32 lFilterVal, MInt32 lLayers)
{
    START_TIME;
    MInt32 lRet = 0;

    m_pSrcPyrGauInner[0] = *pSrc;
    lRet = BuildGauPyrs_u8(m_hMemMgr, m_mcvParallelMonitor, pSrc, m_pSrcPyrGauInner, lLayers, MNull);
    CHECK_ERROR(lRet);

    // init nlm
    NLMeans2 obj(m_hMemMgr, m_mcvParallelMonitor);

    for (MInt32 i = lLayers - 1; i >= 0; i--)
    {
        if (lFilterVal > 0)
        {
            lFilterVal = lFilterVal >> (i + 1);
            lRet = obj.Run(&m_pSrcPyrGauInner[i], &m_pMeanAInner[i], MNull, lFilterVal, 2);
            CHECK_ERROR(lRet);
        }
        else
        {
            MMemCpy(&m_pMeanAInner[i], &m_pSrcPyrGauInner[i], m_pMeanAInner[0].i32Height * m_pMeanAInner[0].pi32Pitch[0]);
        }
        lRet = GetDetail(m_hMemMgr, m_mcvParallelMonitor, &m_pMeanAInner[i], &m_pDetailPyrGauInner[i]);
        CHECK_ERROR(lRet);     
    }
    for (MInt32 i = 0; i < 2; i++)
    {
        if (i > 0)
        {
            lRet = Img_Guass3x3_Up2_u8(m_hMemMgr, m_mcvParallelMonitor, &m_pDetailPyrGauInner[i], &m_pDetailPyrGauInner[i - 1]);
            CHECK_ERROR(lRet);
        }
        AddDetail_U8(m_hMemMgr, m_mcvParallelMonitor,
            pSrc, &m_pDetailPyrGauInner[0], pDst,
            pIntensity[i], pRange[i]);
        CHECK_ERROR(lRet);
#ifdef  BUILD_OPENCV
        char filename[255];
        sprintf(filename, "addDetail_detail_before[%d].png", i);
        mat_write255(m_pMeanAInner[0].i32Height, m_pMeanAInner[0].pi32Pitch[0], CV_8UC1, m_pMeanAInner[0].ppu8Plane[0], filename, 1.0);
        sprintf(filename, "addDetail_detail[%d].png", i);
        mat_write255(m_pDetailPyrGauInner[0].i32Height, m_pDetailPyrGauInner[0].pi32Pitch[0], CV_8UC1, m_pDetailPyrGauInner[0].ppu8Plane[0], filename, 1.0);
        sprintf(filename, "addDetail_dst[%d].png", i);
        mat_write255(pSrc[0].i32Height, pSrc[0].pi32Pitch[0], CV_8UC1, pSrc[0].ppu8Plane[0], filename, 1.0);
#endif
    }


    END_TIME;
    return lRet;
}


MInt32 Sharpen_Pyramid_U8::GetDetail(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDetail)
{
    START_TIME;
    MInt32 lRet = 0;

    lRet = GaussMinus5x5_U8(hMemMgr, mcvParallelMonitor, pSrc->ppu8Plane[0], pSrc->i32Width, pSrc->i32Height, pSrc->pi32Pitch[0],
        pDetail->ppu8Plane[0], pDetail->pi32Pitch[0]);
#ifdef  BUILD_OPENCV
    char filename[255];
    MInt32 i = 0;
    sprintf(filename, "new_detailImage[%d].png", i);
    mat_write255(pDetail[i].i32Height, pDetail[i].pi32Pitch[0], CV_8UC1, pDetail[i].ppu8Plane[0], filename, 1.0);
    sprintf(filename, "new_srcImage[%d].png", 0);
    mat_write255(pSrc[i].i32Height, pSrc[i].pi32Pitch[0], CV_8UC1, pSrc[i].ppu8Plane[0], filename, 1.0);
#endif
    END_TIME;
    return lRet;
}

MInt32 Sharpen_Pyramid_U8::FilterDetail(MInt32 lFilterVal, MInt32 index)
{
    START_TIME;
    MInt32 lRet = 0;
    MFloat fEps = lFilterVal * 0.2;

    if (index == 0 || (m_lPryLayer == 3 && index == 1))
    {
        //Img_Guass5x5_Up2_u8(m_hMemMgr, &m_pMeanAInner[index + 1], &m_pMeanAInner[index]);
        //Img_Guass5x5_Up2_u8(m_hMemMgr, &m_pMeanBInner[index + 1], &m_pMeanBInner[index]);
        Img_Guass3x3_Up2_u8(m_hMemMgr, m_mcvParallelMonitor, &m_pMeanAInner[index + 1], &m_pMeanAInner[index]);
        Img_Guass3x3_Up2_u8(m_hMemMgr, m_mcvParallelMonitor, &m_pMeanBInner[index + 1], &m_pMeanBInner[index]);

        Mul_A_Plus_B(
            m_mcvParallelMonitor,
            &m_pDetailPyrGauInner[index],
            &m_pDetailPyrGauInner[index],
            &m_pMeanAInner[index],
            &m_pMeanBInner[index]);
    }
    else
    {
        FillExpandPixels(&m_pDetailPyrGau[index], m_lExpandSize);
        FillExpandPixels(&m_pSrcPyrGau[index], m_lExpandSize);

        GetMeanAB(m_hMemMgr,
            m_mcvParallelMonitor,
            &m_pDetailPyrGau[index],
            &m_pSrcPyrGau[index],
            &m_pMeanA[index],
            &m_pMeanB[index],
            fEps);

        if (index == 1)
        {
            Mul_A_Plus_B(
                m_mcvParallelMonitor,
                &m_pDetailPyrGauInner[index],
                &m_pDetailPyrGauInner[index],
                &m_pMeanAInner[index],
                &m_pMeanBInner[index]);
        }
    }

#ifdef  BUILD_OPENCV
    char filename[255];
    MInt32 i = index;
    sprintf(filename, "filtered_detailImage[%d].png", i);
    mat_write255(m_pDetailPyrGau[i].i32Height, m_pDetailPyrGau[i].pi32Pitch[0], CV_8UC1, m_pDetailPyrGau[i].ppu8Plane[0], filename, 1.0);
#endif

    END_TIME;
    return lRet;
}

MInt32 Sharpen_Pyramid_U8::GetMask(MHandle hMemMgr, MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDetailMask, MInt32 lMaskRange)
{
    START_TIME;
    MInt32 lRet = 0;

    MInt32 width = pSrc->i32Width;
    MInt32 height = pSrc->i32Height;
    MInt32 pitch = pSrc->pi32Pitch[0];
    MInt32 maskPitch = pDetailMask->pi32Pitch[0];

    auto* pHor = SAFE_MALLOC(hMemMgr, MInt16, width * height);
    auto* pVer = SAFE_MALLOC(hMemMgr, MInt16, width * height);

    auto* pSharpenMask = pDetailMask->ppu8Plane[0];
    auto* pImage = pSrc->ppu8Plane[0];
    GaussianBlur3x3(hMemMgr, mcvParallelMonitor, (MUInt8*)pImage, width, height, pitch, pSharpenMask, maskPitch, 1);
    SobelFilter_Hor((MUInt8*)pSharpenMask, maskPitch, pHor, width, width, height);
    SobelFilter_Ver((MUInt8*)pSharpenMask, maskPitch, pVer, width, width, height);
    CalcSobelIntensity(pHor, pVer, width, pSharpenMask, maskPitch, width, height);
    //CalcSobelDirection(pHor, pVer, width, pSharpenMask, width, width, height);
    GaussianBlur3x3(hMemMgr, mcvParallelMonitor, pSharpenMask, width, height, maskPitch, (MUInt8*)pHor, width, 1);
    levels_adjust((MUInt8*)pHor, width, pSharpenMask, maskPitch, width, height, lMaskRange / 8, lMaskRange);

    SAFE_FREE_ARRAY(hMemMgr, pHor);
    SAFE_FREE_ARRAY(hMemMgr, pVer);

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

MVoid  Sharpen_Pyramid_U8::FillExpandPixels(LPASVLOFFSCREEN pSrcDst, MInt32 lExpandSize)
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

static void AddDetail_U8_(
    LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDetailImg, LPASVLOFFSCREEN pDst,
    MInt32 intensity, MInt32 lRange,
    MInt16 starLine, MInt16 endLine)
{
    MInt32 lWidth = pDst->i32Width;
    MInt32 lDetailMax = lRange << 4;
    MInt32 lDetailMin = -lDetailMax;

#ifdef NH_ENABLE_OPENMP0
#pragma omp parallel for num_threads(8) schedule(dynamic)
#endif
    for (MInt32 i = starLine; i < endLine; i++)
    {
        auto* pIn = pSrc->ppu8Plane[0] + i * pSrc->pi32Pitch[0];
        auto* pOut = pDst->ppu8Plane[0] + i * pDst->pi32Pitch[0];
        auto* pDetail = pDetailImg->ppu8Plane[0] + i * pDetailImg->pi32Pitch[0];

        MInt32 j = 0;

#ifdef USE_NEON

        int16x8_t range_down_16x8 = vdupq_n_s16(lDetailMin);
        int16x8_t range_up_16x8 = vdupq_n_s16(lDetailMax);


        //int16x8_t const0_16x8 = vdupq_n_s16(0);
        //int16x8_t const255_16x8 = vdupq_n_s16(256);
        int16x8_t const128_16x8 = vdupq_n_s16(128);

        uint8x8_t srcdata_8x8;
        int16x8_t srcdata_16x8;
        int16x8_t detail_16x8;

        for (; j < lWidth - 7; j += 8)
        {
            srcdata_8x8 = vld1_u8((MUInt8*)pIn + j);
            detail_16x8 = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(vld1_u8(pDetail + j))), const128_16x8);

            detail_16x8 = vmulq_n_s16(detail_16x8, intensity);
            detail_16x8 = vmaxq_s16(detail_16x8, range_down_16x8);
            detail_16x8 = vminq_s16(detail_16x8, range_up_16x8);

            srcdata_16x8 = vreinterpretq_s16_u16(vshlq_n_u16(vmovl_u8(srcdata_8x8), 4));
            srcdata_16x8 = vaddq_s16(srcdata_16x8, detail_16x8);
            srcdata_16x8 = vrshrq_n_s16(srcdata_16x8, 4);
            //srcdata_16x8 = vmaxq_s16(srcdata_16x8, const0_16x8);
            //srcdata_16x8 = vminq_s16(srcdata_16x8, const255_16x8);

            srcdata_8x8 = vqmovun_s16(srcdata_16x8);

            vst1_u8((MUInt8*)pOut + j, srcdata_8x8);
        }

#endif

        for (; j < lWidth; j++)
        {
            MInt16 detail = (pDetail[j] - 128) * intensity;
            CLAMP(detail, lDetailMin, lDetailMax);

            MInt16 out = detail + (pIn[j] << 4);
            out = (out + 8) >> 4;
            CLAMP(out, 0, 255);
            pOut[j] = out;
        }
    }
}

#if 1
struct IMG_SG_UPSCALE
{
    LPASVLOFFSCREEN pSrcImg;
    LPASVLOFFSCREEN pDiffImg;
    LPASVLOFFSCREEN pDstImg;
    MInt32 intensity;
    MInt32 lRange;
    MInt32 lTopLine;
    MInt32 lBotLine;
};

MVoid Sharpen_Pyramid_U8::AddDetail_U8(MHandle hMemMgr, MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDetailImg, LPASVLOFFSCREEN pDst,
    MInt32 intensity, MInt32 lRange)

{
    START_TIME;
    MInt32 lHeight = pSrc->i32Height;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    {
        MInt32 lTaskNum = lHeight > 1024 ? 16 : 8;
        /// 设置回调函数
        auto func_lamda = [](MVoid* HParam) -> MVoid
        {
            auto SG_NLM_sturct = (IMG_SG_UPSCALE*)HParam;

            AddDetail_U8_(SG_NLM_sturct->pSrcImg,
                SG_NLM_sturct->pDiffImg,
                SG_NLM_sturct->pDstImg,
                SG_NLM_sturct->intensity,
                SG_NLM_sturct->lRange,
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
            pParam[lnum].pSrcImg = pSrc;
            pParam[lnum].pDiffImg = pDetailImg;
            pParam[lnum].pDstImg = pDst;
            pParam[lnum].intensity = intensity;
            pParam[lnum].lRange = lRange;
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
    AddDetail_U8_(pSrc, pDetailImg, pDst, intensity, lRange, 0, nHeight);
#endif
    END_TIME;
}

#else
MInt32 Sharpen_Pyramid_U8::AddDetail_U8(MHandle hMemMgr, MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrc, LPASVLOFFSCREEN pDetailImg, LPASVLOFFSCREEN pDst,
    MInt32 intensity, MInt32 lRange)
{
    START_TIME;

    MInt32 lret = 0;

    LOGD("intensity = %d", intensity);
    LOGD("lRange = %d", lRange);
    MInt32 nHeight = pDst->i32Height;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    int threadCount = nHeight >= 1024 ? 16 : 8;
    auto expand_functor = [&](int currentThreadId)
    {
        // 线程分割
        int startHeight = 0;
        int endHeight = nHeight;
        if (threadCount > 1)
        {
            int countStride = nHeight / threadCount;
            countStride = (countStride >> 2) << 2; // 必须是4的倍数

            startHeight = currentThreadId * countStride;
            if (currentThreadId != threadCount - 1)
            {
                endHeight = startHeight + countStride;
            }
        }

        AddDetail_U8(hMemMgr,
            pSrc, pDetailImg, pDst,
            intensity, lRange, startHeight, endHeight);
    };

    std::thread* expand_thread = new std::thread[threadCount - 1];
    for (int i = 0; i < threadCount - 1; ++i)
    {
        expand_thread[i] = std::thread(expand_functor, i);
    }
    expand_functor(threadCount - 1);
    for (int i = 0; i < threadCount - 1; ++i)
    {
        expand_thread[i].join();
    }
    if (expand_thread)
    {
        delete[] expand_thread;
        expand_thread = MNull;
    }

#else
    lret = AddDetail_U8(hMemMgr,
        pSrc, pDetailImg, pDst,
        intensity, lRange, 0, nHeight);
#endif

    END_TIME;
    return lret;
}
#endif
NS_SINFLE_IMAGE_ENHANCEMENT_END