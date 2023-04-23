#include "Arcsoft_ReduceColorNoise.h"
#include "DefineForDebug.h"
//#include "convYUVToBGR.h"
#include <thread>
#include <math.h>
#include <merror.h>
#include <mobilecv.h>
#include "Arcsoft_NV21_DownScale4_To_I444_RGB.h"
#include "SetLPASVLOFFSCREEN.h"
#include "up_down_scale.h"

//#define TABLE_LOOKUP //加速算法，采用查表
#define FIXED_POINT //对运算进行定点化

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    MUInt8 pGammaTable[256] = { 0 };

    MUInt8 pAntiGammaTable[256] = { 0 };

    const MFloat radial[5] = { 1.0, 1.0, 0.8, 0.7, 0.7 }; //解决暗角问题，距离越远权重越低

    const MUInt8 pCubeTable[256] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,4,4,4,4,4,5,5,5,5,6,6,6,6,6,7,7,7,8,8,8,8,9,9,9,10,10,10,11,11,12,12,12,13,13,14,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,22,22,23,23,24,25,25,26,27,27,28,29,29,30,31,32,32,33,34,35,35,36,37,38,39,40,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,60,61,62,63,64,65,67,68,69,70,72,73,74,76,77,78,80,81,82,84,85,87,88,90,91,93,94,96,97,99,101,102,104,105,107,109,111,112,114,116,118,119,121,123,125,127,129,131,132,134,136,138,140,142,144,147,149,151,153,155,157,159,162,164,166,168,171,173,175,178,180,182,185,187,190,192,195,197,200,202,205,207,210,213,215,218,221,223,226,229,232,235,237,240,243,246,249,252,255};

    static MVoid MakeGammaTable(MUInt8* pTable, MInt32 lSize, MFloat fFactor)
    {
        for (MInt32 i = 0; i < lSize; i++) // i表示权重
        {
            pTable[i] = pow((i / 255.0), fFactor) * 255.0 + 0.5;
        }
    }


    static MVoid gammaMap(MUInt8* pSrc, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcPitch, MUInt8* pMap, MUInt8* pDst, MInt32 lDstPitch)
    {
        for (MInt32 y = 0; y < lHeight; y++)
        {
            auto* pTempSrc = pSrc + y * lSrcPitch;
            auto* pTempDst = pDst + y * lDstPitch;

            for (MInt32 x = 0; x < lWidth; x++)
            {
                pTempDst[x] = pMap[pTempSrc[x]];
            }
        }
    }

    Arcsoft_ReduceColorNoise::Arcsoft_ReduceColorNoise(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt16 nThreadCount)
    {
        m_hMemMgr = hMemMgr;
        m_mcvParallelMonitor = mcvParallelMonitor;
        m_nThreadCount = nThreadCount;
        m_pUV = MNull;

#ifdef GAMMA_CONVERT
        MakeGammaTable(pGammaTable, 256, 1.0 / 1.2);
        MakeGammaTable(pAntiGammaTable, 256, 1.2);
#endif
    }

    Arcsoft_ReduceColorNoise::~Arcsoft_ReduceColorNoise()
    {

    }

    MInt32 Arcsoft_ReduceColorNoise::init(MInt32 lWidth, MInt32 lHeight)
    {
        START_TIME;
        MInt32 lRet;

        lRet = AllocLPASVLOFFSCREEN(m_hMemMgr, &m_I444, lWidth / 4, lHeight / 4, lWidth / 4, ASVL_PAF_I444);
        m_pUV = SAFE_MALLOC(m_hMemMgr, MUInt8, m_I444.i32Height * m_I444.pi32Pitch[1] * 2);
        CHECK_MEMORY(m_pUV);

        END_TIME;
        return lRet;
    }

    MVoid Arcsoft_ReduceColorNoise::release()
    {
        FreeLPASVLOFFSCREEN(m_hMemMgr, &m_I444);
        SAFE_FREE_ARRAY(m_hMemMgr, m_pUV);
    }

    MInt32 Arcsoft_ReduceColorNoise::runNV21(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, MInt32 lUVIntensity)
    {
        DECOLORNOISE_PARAM decolornoiseParam;
        {
            decolornoiseParam.chroma = lUVIntensity;
            decolornoiseParam.smooth = lUVIntensity;
            decolornoiseParam.detail = 25 - decolornoiseParam.smooth / 4;
            MInt32 lStep = (lUVIntensity * 2 / 25) + 1;
            CLAMP(lStep, 1, 4);
            decolornoiseParam.step = lStep;
        }

        return runNV21(pSrcImage, pDstImage, decolornoiseParam);
    }

    MInt32 Arcsoft_ReduceColorNoise::runNV21(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, DECOLORNOISE_PARAM param)
    {
        START_TIME;
        MInt32 lRet;

        lRet = Arcsoft_NV21_DownScale4_To_I444(m_hMemMgr, m_mcvParallelMonitor, pSrcImage, m_I444);
        CHECK_ERROR(lRet);

        lRet = runYUV444(&m_I444, &m_I444, param);
        CHECK_ERROR(lRet);

        Merge_UV_Data(m_I444.ppu8Plane[1], m_I444.pi32Pitch[1], m_I444.ppu8Plane[2], m_I444.pi32Pitch[0], 
                      m_I444.i32Width*2, m_I444.i32Height, m_pUV, m_I444.pi32Pitch[1] * 2);

        lRet = Fast_Bilinear_Upscale2_8UC2(m_mcvParallelMonitor, m_pUV, m_I444.i32Width, m_I444.i32Height, m_I444.pi32Pitch[1] * 2,
            pDstImage->ppu8Plane[1], pDstImage->i32Width/2, pDstImage->i32Height / 2, pDstImage->pi32Pitch[1]);


        END_TIME;
        return lRet;
    }

    MInt32 Arcsoft_ReduceColorNoise::runYUV444(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage,
                                               DECOLORNOISE_PARAM param)
    {
        START_TIME;
        MInt32 lRet = 0;
        if (pSrcImage == MNull || pDstImage == MNull)
        {
            LOGE("pSrc == MNull || pDst == MNull");
            return -1;
        }

        // 转换外部参数到内部
        CNR_Chroma_Coef_t coef;
        GetCNRChromaCoef(param, coef);

        // 提前计算需要查表的值
        m_pSmoothVal = SAFE_MALLOC(m_hMemMgr, MFloat, coef.smooth_radius + 1);
        for (int i = 0; i < coef.smooth_radius + 1; i++)
        {
            m_pSmoothVal[i] = (coef.detail_value + 1.0 * i * i * coef.smooth_factor * (1.0 - coef.detail_value)) * 0.2;
        }
#ifdef TABLE_LOOKUP
        m_pDiffWeight[0] = SAFE_MALLOC(m_hMemMgr, MFloat, 512);
        m_pDiffWeight[1] = SAFE_MALLOC(m_hMemMgr, MFloat, 512);
        m_pDiffWeight[2] = SAFE_MALLOC(m_hMemMgr, MFloat, 512);
        if (m_pSmoothVal == MNull || m_pDiffWeight[0] == MNull || m_pDiffWeight[1] == MNull || m_pDiffWeight[2] == MNull)
        {
            lRet = MERR_NO_MEMORY;
            goto exit;
        }

        for (int i = 0; i < 512; i++)
        {
            m_pDiffWeight[0][i] = 1.5210e-05 * i * i * coef.diff_y_weight;
            m_pDiffWeight[1][i] = 1.5210e-05 * i * i * coef.diff_u_weight;
            m_pDiffWeight[2][i] = 1.5210e-05 * i * i * coef.diff_v_weight;
        }
#endif
#ifdef GAMMA_CONVERT
        // gamma变换
        gammaMap(pSrcImage->ppu8Plane[1], pSrcImage->i32Width, pSrcImage->i32Height, pSrcImage->pi32Pitch[1],
                 pGammaTable, pSrcImage->ppu8Plane[1], pSrcImage->pi32Pitch[1]);

        gammaMap(pSrcImage->ppu8Plane[2], pSrcImage->i32Width, pSrcImage->i32Height, pSrcImage->pi32Pitch[2],
                 pGammaTable, pSrcImage->ppu8Plane[2], pSrcImage->pi32Pitch[2]);
#endif

        // 降噪过程
        {
            m_TempImage.ppu8Plane[0] = pSrcImage->ppu8Plane[0];
            m_TempImage.ppu8Plane[1] = m_pUV;
            m_TempImage.ppu8Plane[2] = m_TempImage.ppu8Plane[1] + pDstImage->i32Width * pDstImage->i32Height;
            if (m_TempImage.ppu8Plane[1] == MNull || m_TempImage.ppu8Plane[2] == MNull)
            {
                lRet = MERR_NO_MEMORY;
                goto exit;
            }
            m_TempImage.i32Width = pSrcImage->i32Width;
            m_TempImage.i32Height = pSrcImage->i32Height;
            m_TempImage.pi32Pitch[0] = pSrcImage->pi32Pitch[0];
            m_TempImage.pi32Pitch[1] = pSrcImage->i32Width;
            m_TempImage.pi32Pitch[2] = pSrcImage->i32Width;
        }
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pSrcImage->ppu8Plane[0], "colorNoise_src_y.png");
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pSrcImage->ppu8Plane[1], "colorNoise_src_u.png");
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pSrcImage->ppu8Plane[2], "colorNoise_src_v.png");

        lRet = ReduceChromaNoise_Hor(pSrcImage, &m_TempImage, coef);
        if (lRet != 0)
        {
            return lRet;
        }
        mat_write(m_TempImage.i32Height, m_TempImage.i32Width, CV_8UC1, m_TempImage.ppu8Plane[1], "colorNoise_temp_u.png");
        mat_write(m_TempImage.i32Height, m_TempImage.i32Width, CV_8UC1, m_TempImage.ppu8Plane[2], "colorNoise_temp_v.png");

        lRet = ReduceChromaNoise_Ver(&m_TempImage, pDstImage, coef);
        if (lRet != 0)
        {
            return lRet;
        }
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pDstImage->ppu8Plane[0], "colorNoise_dst_y.png");
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pDstImage->ppu8Plane[1], "colorNoise_dst_u.png");
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pDstImage->ppu8Plane[2], "colorNoise_dst_v.png");


#ifdef GAMMA_CONVERT
        // 反gamma变换
        gammaMap(pDstImage->ppu8Plane[1], pDstImage->i32Width, pDstImage->i32Height, pDstImage->pi32Pitch[1],
                 pAntiGammaTable, pDstImage->ppu8Plane[1], pDstImage->pi32Pitch[1]);

        gammaMap(pDstImage->ppu8Plane[2], pDstImage->i32Width, pDstImage->i32Height, pDstImage->pi32Pitch[2],
                 pAntiGammaTable, pDstImage->ppu8Plane[2], pDstImage->pi32Pitch[2]);
#endif
        exit:
        SAFE_FREE_ARRAY(m_hMemMgr, m_pSmoothVal);
#ifdef TABLE_LOOKUP
        SAFE_FREE_ARRAY(m_hMemMgr, m_pDiffWeight[0]);
        SAFE_FREE_ARRAY(m_hMemMgr, m_pDiffWeight[1]);
        SAFE_FREE_ARRAY(m_hMemMgr, m_pDiffWeight[2]);
#endif

        END_TIME;

        return lRet;
    }

    MInt32 Arcsoft_ReduceColorNoise::runYUV444(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage,
                                               DECOLORNOISE_PARAM param, MUInt8* pBuffer)
    {
        START_TIME;
        MInt32 lRet = 0;
        if (pSrcImage == MNull || pDstImage == MNull)
        {
            LOGE("pSrc == MNull || pDst == MNull");
            return -1;
        }

        // 转换外部参数到内部
        CNR_Chroma_Coef_t coef;
        GetCNRChromaCoef(param, coef);

        // 提前计算需要查表的值
        m_pSmoothVal = SAFE_MALLOC(m_hMemMgr, MFloat, coef.smooth_radius + 1);
        for (int i = 0; i < coef.smooth_radius + 1; i++)
        {
            m_pSmoothVal[i] = (coef.detail_value + 1.0 * i * i * coef.smooth_factor * (1.0 - coef.detail_value)) * 0.2;
        }
#ifdef TABLE_LOOKUP
        m_pDiffWeight[0] = SAFE_MALLOC(m_hMemMgr, MFloat, 512);
        m_pDiffWeight[1] = SAFE_MALLOC(m_hMemMgr, MFloat, 512);
        m_pDiffWeight[2] = SAFE_MALLOC(m_hMemMgr, MFloat, 512);
        if (m_pSmoothVal == MNull || m_pDiffWeight[0] == MNull || m_pDiffWeight[1] == MNull || m_pDiffWeight[2] == MNull)
        {
            lRet = MERR_NO_MEMORY;
            goto exit;
        }

        for (int i = 0; i < 512; i++)
        {
            m_pDiffWeight[0][i] = 1.5210e-05 * i * i * coef.diff_y_weight;
            m_pDiffWeight[1][i] = 1.5210e-05 * i * i * coef.diff_u_weight;
            m_pDiffWeight[2][i] = 1.5210e-05 * i * i * coef.diff_v_weight;
        }
#endif
#ifdef GAMMA_CONVERT
        // gamma变换
        gammaMap(pSrcImage->ppu8Plane[1], pSrcImage->i32Width, pSrcImage->i32Height, pSrcImage->pi32Pitch[1],
                 pGammaTable, pSrcImage->ppu8Plane[1], pSrcImage->pi32Pitch[1]);

        gammaMap(pSrcImage->ppu8Plane[2], pSrcImage->i32Width, pSrcImage->i32Height, pSrcImage->pi32Pitch[2],
                 pGammaTable, pSrcImage->ppu8Plane[2], pSrcImage->pi32Pitch[2]);
#endif

        // 降噪过程
        {
            m_TempImage.ppu8Plane[0] = pSrcImage->ppu8Plane[0];
            m_TempImage.ppu8Plane[1] = pBuffer;
            m_TempImage.ppu8Plane[2] = pBuffer + pDstImage->i32Width * pDstImage->i32Height;

            m_TempImage.i32Width = pSrcImage->i32Width;
            m_TempImage.i32Height = pSrcImage->i32Height;
            m_TempImage.pi32Pitch[0] = pSrcImage->pi32Pitch[0];
            m_TempImage.pi32Pitch[1] = pSrcImage->i32Width;
            m_TempImage.pi32Pitch[2] = pSrcImage->i32Width;
        }
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pSrcImage->ppu8Plane[0], "colorNoise_src_y.png");
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pSrcImage->ppu8Plane[1], "colorNoise_src_u.png");
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pSrcImage->ppu8Plane[2], "colorNoise_src_v.png");

        lRet = ReduceChromaNoise_Hor(pSrcImage, &m_TempImage, coef);
        if (lRet != 0)
        {
            return lRet;
        }
        mat_write(m_TempImage.i32Height, m_TempImage.i32Width, CV_8UC1, m_TempImage.ppu8Plane[1], "colorNoise_temp_u.png");
        mat_write(m_TempImage.i32Height, m_TempImage.i32Width, CV_8UC1, m_TempImage.ppu8Plane[2], "colorNoise_temp_v.png");

        lRet = ReduceChromaNoise_Ver(&m_TempImage, pDstImage, coef);
        if (lRet != 0)
        {
            return lRet;
        }
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pDstImage->ppu8Plane[0], "colorNoise_dst_y.png");
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pDstImage->ppu8Plane[1], "colorNoise_dst_u.png");
        mat_write(pSrcImage->i32Height, pSrcImage->i32Width, CV_8UC1, pDstImage->ppu8Plane[2], "colorNoise_dst_v.png");


#ifdef GAMMA_CONVERT
        // 反gamma变换
        gammaMap(pDstImage->ppu8Plane[1], pDstImage->i32Width, pDstImage->i32Height, pDstImage->pi32Pitch[1],
                 pAntiGammaTable, pDstImage->ppu8Plane[1], pDstImage->pi32Pitch[1]);

        gammaMap(pDstImage->ppu8Plane[2], pDstImage->i32Width, pDstImage->i32Height, pDstImage->pi32Pitch[2],
                 pAntiGammaTable, pDstImage->ppu8Plane[2], pDstImage->pi32Pitch[2]);
#endif
        //SAFE_FREE_ARRAY(m_hMemMgr, m_TempImage.ppu8Plane[1]);
        //SAFE_FREE_ARRAY(m_hMemMgr, m_TempImage.ppu8Plane[2]);
        SAFE_FREE_ARRAY(m_hMemMgr, m_pSmoothVal);
#ifdef TABLE_LOOKUP
        SAFE_FREE_ARRAY(m_hMemMgr, m_pDiffWeight[0]);
        SAFE_FREE_ARRAY(m_hMemMgr, m_pDiffWeight[1]);
        SAFE_FREE_ARRAY(m_hMemMgr, m_pDiffWeight[2]);
#endif

        END_TIME;

        return lRet;
    }
    struct IMG_SG_COLORNOISE
    {
        MVoid *obj;
        LPASVLOFFSCREEN pSrcImage;
        LPASVLOFFSCREEN pDstImage;
        CNR_Chroma_Coef_t coef;
        MInt32 lTopLine;
        MInt32 lBotLine;
    };

    MInt32 Arcsoft_ReduceColorNoise::ReduceChromaNoise_Hor(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, CNR_Chroma_Coef_t coef)
#ifdef USE_STD_THREAD
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lRet = 0;

        MInt32 nHeight = pDstImage->i32Height;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : (nHeight >= 1024 ? 16 : 8);
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

            ReduceChromaNoise_Hor(pSrcImage, pDstImage, coef, startHeight, endHeight);
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
        lRet = ReduceChromaNoise_Hor(pSrcImage, pDstImage, coef, 0, pDstImage->i32Height);
#endif


#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return lRet;
    }
#else
    {
        START_TIME;

        MInt32 lRet = 0;
        MInt32 lHeight = pSrcImage->i32Height;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        {
            MInt32 lTaskNum = m_nThreadCount > 0 ? m_nThreadCount : lHeight > 1024 ? 16 : 8;
            /// 设置回调函数
            auto func_lamda = [](MVoid* HParam) -> MVoid
            {
                auto SG_NLM_sturct = (IMG_SG_COLORNOISE*)HParam;
                Arcsoft_ReduceColorNoise* obj = (Arcsoft_ReduceColorNoise*)SG_NLM_sturct->obj;
                
                obj->ReduceChromaNoise_Hor(SG_NLM_sturct->pSrcImage,
                    SG_NLM_sturct->pDstImage, 
                    SG_NLM_sturct->coef, 
                    SG_NLM_sturct->lTopLine,
                    SG_NLM_sturct->lBotLine);

            };
            MVoid(*func)(MVoid*) = func_lamda;



            /// 设置参数
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = (lTaskHeight >> 2) << 2;

            IMG_SG_COLORNOISE pParam[16] = { MNull };
            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[lnum].lTopLine = lTaskHeight * lnum;
                pParam[lnum].lBotLine = lTaskHeight * (lnum + 1);
            }
            pParam[0].lTopLine = 0;
            pParam[lTaskNum - 1].lBotLine = lHeight;


            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[lnum].pSrcImage = pSrcImage;
                pParam[lnum].pDstImage = pDstImage;
                pParam[lnum].coef = coef;
                pParam[lnum].obj = this;
            }

            /// 创建线程     
            MInt32 lTaskID[16] = { MNull };
            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                lTaskID[lnum] = mcvAddTask(m_mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
                if (lTaskID[lnum] < 0)
                {
                    lRet = MERR_BAD_STATE;
                }
            }

            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                mcvWaitTask(m_mcvParallelMonitor, lTaskID[lnum]);
            }
        }

#else
        lRet = ReduceChromaNoise_Hor(pSrcImage, pDstImage, coef, 0, pDstImage->i32Height);
#endif
        END_TIME;
        return lRet;
    }
#endif
    
    MInt32 Arcsoft_ReduceColorNoise::ReduceChromaNoise_Hor(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, CNR_Chroma_Coef_t coef, MInt32 lTopLine, MInt32 lBotLine)
    {
        START_TIME;
        MInt32 lRet = 0;

        MInt32 lWidth = pDstImage->i32Width;
        MInt32 lHeight = pDstImage->i32Height;
        MInt32 lSrcPitchY = pSrcImage->pi32Pitch[0];
        MInt32 lSrcPitchUV = pSrcImage->pi32Pitch[1];
        MInt32 lDstPitchUV = pDstImage->pi32Pitch[1];

        MInt32 lHalfWidth = lWidth >> 1;
        MInt32 lHalfHeight = lHeight >> 1;
        MInt32 center = lHalfWidth + lHalfHeight; //中间点
#ifdef USE_NEON

        float32x4_t diff_y_weight32x4 = vdupq_n_f32(coef.diff_y_weight);
        float32x4_t diff_u_weight32x4 = vdupq_n_f32(coef.diff_u_weight);
        float32x4_t diff_v_weight32x4 = vdupq_n_f32(coef.diff_v_weight);
        float32x4_t detail32x4 = vdupq_n_f32(coef.detail_value);
        float32x4_t val255 = vdupq_n_f32(255.0);
        float32x4_t val10 = vdupq_n_f32(1.0);
        float32x4_t val02 = vdupq_n_f32(0.2);
        float32x4_t val00 = vdupq_n_f32(0.0);
        float32x4_t val1_255 = vdupq_n_f32(0.0039215686);
        float32x4_t val2_255 = vdupq_n_f32(0.0078431372);
        float32x4_t val0_5 = vdupq_n_f32(0.5);

#endif
        for (MInt32 y = lTopLine; y < lBotLine; y++)
        {
            auto* pTempY = pSrcImage->ppu8Plane[0] + y * lSrcPitchY;
            auto* pTempSrcU = pSrcImage->ppu8Plane[1] + y * lSrcPitchUV;
            auto* pTempSrcV = pSrcImage->ppu8Plane[2] + y * lSrcPitchUV;
            auto* pTempDstU = pDstImage->ppu8Plane[1] + y * lDstPitchUV; 
            auto* pTempDstV = pDstImage->ppu8Plane[2] + y * lDstPitchUV;
            MInt32 x = 0;
#ifdef USE_NEON
            for (; x < lWidth - 32; x += 8)
            {
                uint8x8_t tmpY8x8 = vld1_u8((MUInt8*)pTempY + x);
                uint8x8_t tmpU8x8 = vld1_u8((MUInt8*)pTempSrcU + x);
                uint8x8_t tmpV8x8 = vld1_u8((MUInt8*)pTempSrcV + x);
                float32x4_t fSumULow = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(tmpU8x8))));
                float32x4_t fSumVLow = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(tmpV8x8))));
                float32x4_t fSumUHigh = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(tmpU8x8))));
                float32x4_t fSumVHigh = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(tmpV8x8))));
                float32x4_t fSumWeightLow = val10;
                float32x4_t fSumWeightHign = val10;

                //对边缘处理，超过边缘的点直接舍弃，不拷贝扩充
                MInt16 nLeftRadius = -coef.smooth_radius + MAX(0, coef.smooth_radius - x);
                MInt16 nRightRadius = coef.smooth_radius - MAX(0, coef.smooth_radius - (lWidth - 1 - x));
                for (MInt16 i = nLeftRadius; i <= nRightRadius; i += coef.smooth_step)
                {
                    if (i == 0)
                    {
                        continue;
                    }
                    MInt16 nIndex = x + i;

                    uint8x8_t tmpY_offset8x8 = vld1_u8((MUInt8*)pTempY + nIndex);
                    uint8x8_t tmpU_offset8x8 = vld1_u8((MUInt8*)pTempSrcU + nIndex);
                    uint8x8_t tmpV_offset8x8 = vld1_u8((MUInt8*)pTempSrcV + nIndex);

                    uint8x8_t diffY8x8 = vabd_u8(tmpY8x8, tmpY_offset8x8);
                    uint8x8_t diffU8x8 = vabd_u8(tmpU8x8, tmpU_offset8x8);
                    uint8x8_t diffV8x8 = vabd_u8(tmpV8x8, tmpV_offset8x8);

                    float32x4_t fSmoothVal32x4 = vdupq_n_f32(m_pSmoothVal[ABS(i)]);

                    //低4位
                    float32x4_t diffY = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(diffY8x8))));
                    float32x4_t diffU = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(diffU8x8))));
                    float32x4_t diffV = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(diffV8x8))));
                    diffY = vmulq_f32(diffY, val1_255);
                    diffU = vmulq_f32(diffU, val2_255);
                    diffV = vmulq_f32(diffV, val2_255);

                    diffY = vmulq_f32(vmulq_f32(diffY, diffY), diff_y_weight32x4);
                    diffU = vmulq_f32(vmulq_f32(diffU, diffU), diff_u_weight32x4);
                    diffV = vmulq_f32(vmulq_f32(diffV, diffV), diff_v_weight32x4);
                    float32x4_t fWeight = vmulq_f32(vaddq_f32(diffY, vaddq_f32(diffU, diffV)), fSmoothVal32x4);//vaddq_f32(vmulq_f32(vsubq_f32(val10, fSmoothVal32x4), detail32x4), fSmoothVal32x4));
                    fWeight = vaddq_f32(fWeight, val10);//vaddq_f32(vmulq_f32(fWeight, val02), val10);
                    fWeight = vmulq_f32(vmulq_f32(fWeight, fWeight), fWeight);
                    fWeight = vminq_f32(fWeight, val10);
                    fWeight = vmaxq_f32(fWeight, val00);

                    fSumWeightLow = vaddq_f32(fSumWeightLow, fWeight);
                    fSumULow = vaddq_f32(fSumULow, vmulq_f32(fWeight, vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(tmpU_offset8x8))))));
                    fSumVLow = vaddq_f32(fSumVLow, vmulq_f32(fWeight, vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(tmpV_offset8x8))))));

                    // 高4位
                    diffY = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(diffY8x8))));
                    diffU = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(diffU8x8))));
                    diffV = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(diffV8x8))));
                    diffY = vmulq_f32(diffY, val1_255);
                    diffU = vmulq_f32(diffU, val2_255);
                    diffV = vmulq_f32(diffV, val2_255);

                    diffY = vmulq_f32(vmulq_f32(diffY, diffY), diff_y_weight32x4);
                    diffU = vmulq_f32(vmulq_f32(diffU, diffU), diff_u_weight32x4);
                    diffV = vmulq_f32(vmulq_f32(diffV, diffV), diff_v_weight32x4);
                    fWeight = vmulq_f32(vaddq_f32(diffY, vaddq_f32(diffU, diffV)), fSmoothVal32x4);//vaddq_f32(vmulq_f32(vsubq_f32(val10, fSmoothVal32x4), detail32x4), fSmoothVal32x4));
                    fWeight = vaddq_f32(fWeight, val10);//vaddq_f32(vmulq_f32(fWeight, val02), val10);
                    fWeight = vmulq_f32(vmulq_f32(fWeight, fWeight), fWeight);
                    fWeight = vminq_f32(fWeight, val10);
                    fWeight = vmaxq_f32(fWeight, val00);

                    fSumWeightHign = vaddq_f32(fSumWeightHign, fWeight);
                    fSumUHigh = vaddq_f32(fSumUHigh, vmulq_f32(fWeight, vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(tmpU_offset8x8))))));
                    fSumVHigh = vaddq_f32(fSumVHigh, vmulq_f32(fWeight, vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(tmpV_offset8x8))))));
                }
                float32x4_t fInvVar_low_32x4 = vrecpeq_f32(fSumWeightLow);
                fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fSumWeightLow, fInvVar_low_32x4), fInvVar_low_32x4);
                fSumULow = vmulq_f32(fSumULow, fInvVar_low_32x4);
                fSumVLow = vmulq_f32(fSumVLow, fInvVar_low_32x4);
                fSumULow = vaddq_f32(fSumULow, val0_5);
                fSumVLow = vaddq_f32(fSumVLow, val0_5);
                fSumULow = vminq_f32(fSumULow, val255);
                fSumVLow = vminq_f32(fSumVLow, val255);

                fInvVar_low_32x4 = vrecpeq_f32(fSumWeightHign);
                fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fSumWeightHign, fInvVar_low_32x4), fInvVar_low_32x4);
                fSumUHigh = vmulq_f32(fSumUHigh, fInvVar_low_32x4);
                fSumVHigh = vmulq_f32(fSumVHigh, fInvVar_low_32x4);
                fSumUHigh = vaddq_f32(fSumUHigh, val0_5);
                fSumVHigh = vaddq_f32(fSumVHigh, val0_5);
                fSumUHigh = vminq_f32(fSumUHigh, val255);
                fSumVHigh = vminq_f32(fSumVHigh, val255);

                uint8x8_t resU8x8 = vmovn_u16(vcombine_u16(vmovn_u32(vcvtq_u32_f32(fSumULow)), vmovn_u32(vcvtq_u32_f32(fSumUHigh))));
                uint8x8_t resV8x8 = vmovn_u16(vcombine_u16(vmovn_u32(vcvtq_u32_f32(fSumVLow)), vmovn_u32(vcvtq_u32_f32(fSumVHigh))));

                vst1_u8(pTempDstU + x, resU8x8);
                vst1_u8(pTempDstV + x, resV8x8);
            }
#endif
            for (; x < lWidth; x++)
            {
                auto nCurY = pTempY[x];
                auto nCurU = pTempSrcU[x];
                auto nCurV = pTempSrcV[x];

                MFloat fSumU = nCurU;
                MFloat fSumV = nCurV;
                MFloat fSumWeight = 1.0;

                MInt32 lSumWeight = 255;
                MInt32 lSumU = nCurU * lSumWeight;
                MInt32 lSumV = nCurV * lSumWeight;

                //解决暗角脏的问题
                MInt32 distance = ABS(x - lHalfWidth) + ABS(y - lHalfHeight) << 2;
                MInt16 radial_index = distance / center;
                MFloat radial_factor = radial[radial_index];
                MFloat fDiff_y_weight = coef.diff_y_weight;
                MFloat fDiff_u_weight = coef.diff_u_weight;
                MFloat fDiff_v_weight = coef.diff_v_weight;

                //对边缘处理，超过边缘的点直接舍弃，不拷贝扩充
                MInt16 nLeftRadius = -coef.smooth_radius + MAX(0, coef.smooth_radius - x);
                MInt16 nRightRadius = coef.smooth_radius - MAX(0, coef.smooth_radius - (lWidth - 1 - x));
                for (MInt16 i = nLeftRadius; i <= nRightRadius; i += coef.smooth_step)
                {
                    if (i == 0)
                    {
                        continue;
                    }

                    MInt16 nIndex = x + i;

#ifdef TABLE_LOOKUP //table look-up 

                    MFloat diffY_weight = m_pDiffWeight[0][ABS(nCurY - pTempY[nIndex])];
                    MFloat diffU_weight = m_pDiffWeight[1][ABS((nCurU - pTempSrcU[nIndex]) << 1)];
                    MFloat diffV_weight = m_pDiffWeight[2][ABS((nCurV - pTempSrcV[nIndex]) << 1)];

                    MFloat fSmoothVal = m_pSmoothVal[ABS(i)];

                    MFloat fWeight = (diffY_weight + diffU_weight + diffV_weight) * radial_factor
                                     * fSmoothVal + 1.0;
#else
                    MFloat diffY = (nCurY - pTempY[nIndex]) / 255.0;
                    MFloat diffU = 2 * (nCurU - pTempSrcU[nIndex]) / 255.0;
                    MFloat diffV = 2 * (nCurV - pTempSrcV[nIndex]) / 255.0;

                    MFloat fSmoothVal = m_pSmoothVal[ABS(i)];//1.0 * i * i * coef.smooth_factor;

                    MFloat fWeight = (diffY * diffY * fDiff_y_weight + diffU * diffU * fDiff_u_weight + diffV * diffV * fDiff_v_weight) * radial_factor
                        * fSmoothVal + 1.0;
#endif

#ifdef FIXED_POINT
                    MInt32 lWeight = fWeight*255.0 + 0.5;
                    CLAMP(lWeight, 0, 255);
                    lWeight = pCubeTable[lWeight];

                    lSumWeight += lWeight;
                    lSumU += pTempSrcU[nIndex] * lWeight;
                    lSumV += pTempSrcV[nIndex] * lWeight;

#else
                    fWeight = fWeight * fWeight * fWeight;
                    CLAMP(fWeight, 0.0, 1.0);

                    fSumWeight += fWeight;
                    fSumU += 1.0 * pTempSrcU[nIndex] * fWeight;
                    fSumV += 1.0 * pTempSrcV[nIndex] * fWeight;
#endif
                }
#ifdef FIXED_POINT
                pTempDstU[x] = MIN(255, (lSumU + (lSumWeight >> 1)) / lSumWeight);
                pTempDstV[x] = MIN(255, (lSumV + (lSumWeight >> 1)) / lSumWeight);
#else
                pTempDstU[x] = MIN(255.0, fSumU / fSumWeight + 0.5);
                pTempDstV[x] = MIN(255.0, fSumV / fSumWeight + 0.5);
#endif
            }
        }

        END_TIME;
        return lRet;
    }

    MInt32 Arcsoft_ReduceColorNoise::ReduceChromaNoise_Ver(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, CNR_Chroma_Coef_t coef)
#ifdef USE_STD_THREAD
    {
#if CALCULATE_TIME
        BasicTimer time;
#endif
        MInt32 lRet = 0;

        MInt32 nHeight = pDstImage->i32Height;
#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        int threadCount = m_nThreadCount > 0 ? m_nThreadCount : (nHeight >= 1024 ? 16 : 8);
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

            ReduceChromaNoise_Ver(pSrcImage, pDstImage, coef, startHeight, endHeight);
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
            lRet = ReduceChromaNoise_Ver(pSrcImage, pDstImage, coef, 0, pDstImage->i32Height);
#endif


#if CALCULATE_TIME
        LOGD("%s[%d]: is finished timer count = %fms!\n", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());
#endif
        return lRet;
    }
#else
    {
        START_TIME;

        MInt32 lRet = 0;
        MInt32 lHeight = pSrcImage->i32Height;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
        {
            MInt32 lTaskNum = m_nThreadCount > 0 ? m_nThreadCount : lHeight > 1024 ? 16 : 8;
            /// 设置回调函数
            auto func_lamda = [](MVoid* HParam) -> MVoid
            {
                auto SG_NLM_sturct = (IMG_SG_COLORNOISE*)HParam;
                Arcsoft_ReduceColorNoise* obj = (Arcsoft_ReduceColorNoise*)SG_NLM_sturct->obj;

                obj->ReduceChromaNoise_Ver(SG_NLM_sturct->pSrcImage,
                    SG_NLM_sturct->pDstImage,
                    SG_NLM_sturct->coef,
                    SG_NLM_sturct->lTopLine,
                    SG_NLM_sturct->lBotLine);

            };
            MVoid(*func)(MVoid*) = func_lamda;



            /// 设置参数
            MInt32 lTaskHeight = lHeight / lTaskNum;
            lTaskHeight = (lTaskHeight >> 2) << 2;

            IMG_SG_COLORNOISE pParam[16] = { MNull };
            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[lnum].lTopLine = lTaskHeight * lnum;
                pParam[lnum].lBotLine = lTaskHeight * (lnum + 1);
            }
            pParam[0].lTopLine = 0;
            pParam[lTaskNum - 1].lBotLine = lHeight;


            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                pParam[lnum].pSrcImage = pSrcImage;
                pParam[lnum].pDstImage = pDstImage;
                pParam[lnum].coef = coef;
                pParam[lnum].obj = this;
            }

            /// 创建线程     
            MInt32 lTaskID[16] = { MNull };
            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                lTaskID[lnum] = mcvAddTask(m_mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
                if (lTaskID[lnum] < 0)
                {
                    lRet = MERR_BAD_STATE;
                }
            }

            for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
            {
                mcvWaitTask(m_mcvParallelMonitor, lTaskID[lnum]);
            }
        }

#else
        lRet = ReduceChromaNoise_Ver(pSrcImage, pDstImage, coef, 0, pDstImage->i32Height);
#endif
        END_TIME
        return lRet;
    }
#endif

    MInt32 Arcsoft_ReduceColorNoise::ReduceChromaNoise_Ver(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, CNR_Chroma_Coef_t coef, MInt32 lTopLine, MInt32 lBotLine)
    {
        MInt32 lRet = 0;

        MInt32 lWidth = pDstImage->i32Width;
        MInt32 lHeight = pDstImage->i32Height;
        MInt32 lSrcPitchY = pSrcImage->pi32Pitch[0];
        MInt32 lSrcPitchUV = pSrcImage->pi32Pitch[1];
        MInt32 lDstPitchUV = pDstImage->pi32Pitch[1];

        MInt32 lHalfWidth = lWidth >> 1;
        MInt32 lHalfHeight = lHeight >> 1;
        MInt32 center = lHalfWidth + lHalfHeight; //中间点
#ifdef USE_NEON

        float32x4_t diff_y_weight32x4 = vdupq_n_f32(coef.diff_y_weight);
        float32x4_t diff_u_weight32x4 = vdupq_n_f32(coef.diff_u_weight);
        float32x4_t diff_v_weight32x4 = vdupq_n_f32(coef.diff_v_weight);
        float32x4_t detail32x4 = vdupq_n_f32(coef.detail_value);
        float32x4_t val255 = vdupq_n_f32(255.0);
        float32x4_t val10 = vdupq_n_f32(1.0);
        float32x4_t val02 = vdupq_n_f32(0.2);
        float32x4_t val00 = vdupq_n_f32(0.0);
        float32x4_t val1_255 = vdupq_n_f32(0.0039215686);
        float32x4_t val2_255 = vdupq_n_f32(0.0078431372);
        float32x4_t val0_5 = vdupq_n_f32(0.5);

#endif

        for (MInt32 y = lTopLine; y < lBotLine; y++)
        {
            auto* pTempY = pSrcImage->ppu8Plane[0] + y * lSrcPitchY;
            auto* pTempSrcU = pSrcImage->ppu8Plane[1] + y * lSrcPitchUV;
            auto* pTempSrcV = pSrcImage->ppu8Plane[2] + y * lSrcPitchUV;
            auto* pTempDstU = pDstImage->ppu8Plane[1] + y * lDstPitchUV;
            auto* pTempDstV = pDstImage->ppu8Plane[2] + y * lDstPitchUV;

            //对边缘处理，超过边缘的点直接舍弃，不拷贝扩充
            MInt16 nLeftRadius = -coef.smooth_radius + MAX(0, coef.smooth_radius - y);
            MInt16 nRightRadius = coef.smooth_radius - MAX(0, coef.smooth_radius - (lHeight - 1 - y));

            MInt32 x = 0;
#ifdef USE_NEON
            for (; x < lWidth - 7; x += 8)
            {
                uint8x8_t tmpY8x8 = vld1_u8((MUInt8*)pTempY + x);
                uint8x8_t tmpU8x8 = vld1_u8((MUInt8*)pTempSrcU + x);
                uint8x8_t tmpV8x8 = vld1_u8((MUInt8*)pTempSrcV + x);
                float32x4_t fSumULow = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(tmpU8x8))));
                float32x4_t fSumVLow = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(tmpV8x8))));
                float32x4_t fSumUHigh = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(tmpU8x8))));
                float32x4_t fSumVHigh = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(tmpV8x8))));
                float32x4_t fSumWeightLow = val10;
                float32x4_t fSumWeightHign = val10;

                for (MInt16 i = nLeftRadius; i <= nRightRadius; i += coef.smooth_step)
                {
                    if (i == 0)
                    {
                        continue;
                    }
                    MInt32 nIndexY = x + i * lSrcPitchY;
                    MInt32 nIndexUV = x + i * lSrcPitchUV;

                    uint8x8_t tmpY_offset8x8 = vld1_u8((MUInt8*)pTempY + nIndexY);
                    uint8x8_t tmpU_offset8x8 = vld1_u8((MUInt8*)pTempSrcU + nIndexUV);
                    uint8x8_t tmpV_offset8x8 = vld1_u8((MUInt8*)pTempSrcV + nIndexUV);

                    uint8x8_t diffY8x8 = vabd_u8(tmpY8x8, tmpY_offset8x8);
                    uint8x8_t diffU8x8 = vabd_u8(tmpU8x8, tmpU_offset8x8);
                    uint8x8_t diffV8x8 = vabd_u8(tmpV8x8, tmpV_offset8x8);

                    float32x4_t fSmoothVal32x4 = vdupq_n_f32(m_pSmoothVal[ABS(i)]);

                    //低4位
                    float32x4_t diffY = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(diffY8x8))));
                    float32x4_t diffU = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(diffU8x8))));
                    float32x4_t diffV = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(diffV8x8))));
                    diffY = vmulq_f32(diffY, val1_255);
                    diffU = vmulq_f32(diffU, val2_255);
                    diffV = vmulq_f32(diffV, val2_255);

                    diffY = vmulq_f32(vmulq_f32(diffY, diffY), diff_y_weight32x4);
                    diffU = vmulq_f32(vmulq_f32(diffU, diffU), diff_u_weight32x4);
                    diffV = vmulq_f32(vmulq_f32(diffV, diffV), diff_v_weight32x4);
                    float32x4_t fWeight = vmulq_f32(vaddq_f32(diffY, vaddq_f32(diffU, diffV)), fSmoothVal32x4);//vaddq_f32(vmulq_f32(vsubq_f32(val10, fSmoothVal32x4), detail32x4), fSmoothVal32x4));
                    fWeight = vaddq_f32(fWeight, val10);//vaddq_f32(vmulq_f32(fWeight, val02), val10);
                    fWeight = vmulq_f32(vmulq_f32(fWeight, fWeight), fWeight);
                    fWeight = vminq_f32(fWeight, val10);
                    fWeight = vmaxq_f32(fWeight, val00);

                    fSumWeightLow = vaddq_f32(fSumWeightLow, fWeight);
                    fSumULow = vaddq_f32(fSumULow, vmulq_f32(fWeight, vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(tmpU_offset8x8))))));
                    fSumVLow = vaddq_f32(fSumVLow, vmulq_f32(fWeight, vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(tmpV_offset8x8))))));

                    // 高4位
                    diffY = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(diffY8x8))));
                    diffU = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(diffU8x8))));
                    diffV = vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(diffV8x8))));
                    diffY = vmulq_f32(diffY, val1_255);
                    diffU = vmulq_f32(diffU, val2_255);
                    diffV = vmulq_f32(diffV, val2_255);

                    diffY = vmulq_f32(vmulq_f32(diffY, diffY), diff_y_weight32x4);
                    diffU = vmulq_f32(vmulq_f32(diffU, diffU), diff_u_weight32x4);
                    diffV = vmulq_f32(vmulq_f32(diffV, diffV), diff_v_weight32x4);
                    fWeight = vmulq_f32(vaddq_f32(diffY, vaddq_f32(diffU, diffV)), fSmoothVal32x4);//vaddq_f32(vmulq_f32(vsubq_f32(val10, fSmoothVal32x4), detail32x4), fSmoothVal32x4));
                    fWeight = vaddq_f32(fWeight, val10);//vaddq_f32(vmulq_f32(fWeight, val02), val10);
                    fWeight = vmulq_f32(vmulq_f32(fWeight, fWeight), fWeight);
                    fWeight = vminq_f32(fWeight, val10);
                    fWeight = vmaxq_f32(fWeight, val00);

                    fSumWeightHign = vaddq_f32(fSumWeightHign, fWeight);
                    fSumUHigh = vaddq_f32(fSumUHigh, vmulq_f32(fWeight, vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(tmpU_offset8x8))))));
                    fSumVHigh = vaddq_f32(fSumVHigh, vmulq_f32(fWeight, vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(tmpV_offset8x8))))));
                }
                float32x4_t fInvVar_low_32x4 = vrecpeq_f32(fSumWeightLow);
                fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fSumWeightLow, fInvVar_low_32x4), fInvVar_low_32x4);
                fSumULow = vmulq_f32(fSumULow, fInvVar_low_32x4);
                fSumVLow = vmulq_f32(fSumVLow, fInvVar_low_32x4);
                fSumULow = vaddq_f32(fSumULow, val0_5);
                fSumVLow = vaddq_f32(fSumVLow, val0_5);
                fSumULow = vminq_f32(fSumULow, val255);
                fSumVLow = vminq_f32(fSumVLow, val255);

                fInvVar_low_32x4 = vrecpeq_f32(fSumWeightHign);
                fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fSumWeightHign, fInvVar_low_32x4), fInvVar_low_32x4);
                fSumUHigh = vmulq_f32(fSumUHigh, fInvVar_low_32x4);
                fSumVHigh = vmulq_f32(fSumVHigh, fInvVar_low_32x4);
                fSumUHigh = vaddq_f32(fSumUHigh, val0_5);
                fSumVHigh = vaddq_f32(fSumVHigh, val0_5);
                fSumUHigh = vminq_f32(fSumUHigh, val255);
                fSumVHigh = vminq_f32(fSumVHigh, val255);

                uint8x8_t resU8x8 = vmovn_u16(vcombine_u16(vmovn_u32(vcvtq_u32_f32(fSumULow)), vmovn_u32(vcvtq_u32_f32(fSumUHigh))));
                uint8x8_t resV8x8 = vmovn_u16(vcombine_u16(vmovn_u32(vcvtq_u32_f32(fSumVLow)), vmovn_u32(vcvtq_u32_f32(fSumVHigh))));

                vst1_u8(pTempDstU + x, resU8x8);
                vst1_u8(pTempDstV + x, resV8x8);
            }
#endif
            for (; x < lWidth; x++)
            {
                auto nCurY = pTempY[x];
                auto nCurU = pTempSrcU[x];
                auto nCurV = pTempSrcV[x];

                MFloat fSumU = nCurU;
                MFloat fSumV = nCurV;
                MFloat fSumWeight = 1.0;

                MInt32 lSumWeight = 255;
                MInt32 lSumU = nCurU * lSumWeight;
                MInt32 lSumV = nCurV * lSumWeight;

                //解决暗角脏的问题
                MInt32 distance = ABS(x - lHalfWidth) + ABS(y - lHalfHeight) << 2;
                MInt16 radial_index = distance / center;
                MFloat radial_factor = radial[radial_index];
                MFloat fDiff_y_weight = coef.diff_y_weight;
                MFloat fDiff_u_weight = coef.diff_u_weight;
                MFloat fDiff_v_weight = coef.diff_v_weight;

                for (MInt16 i = nLeftRadius; i <= nRightRadius; i += coef.smooth_step)
                {
                    if (i == 0)
                    {
                        continue;
                    }

                    MInt32 nIndexY = x + i * lSrcPitchY;
                    MInt32 nIndexUV = x + i * lSrcPitchUV;

#ifdef TABLE_LOOKUP //table look-up 
                    MFloat diffY_weight = m_pDiffWeight[0][ABS(nCurY - pTempY[nIndexY])];
                    MFloat diffU_weight = m_pDiffWeight[1][ABS((nCurU - pTempSrcU[nIndexUV]) << 1)];
                    MFloat diffV_weight = m_pDiffWeight[2][ABS((nCurV - pTempSrcV[nIndexUV]) << 1)];

                    MFloat fSmoothVal = m_pSmoothVal[ABS(i)];

                    MFloat fWeight = (diffY_weight + diffU_weight + diffV_weight) * radial_factor
                                     * fSmoothVal + 1.0;
#else
                    MFloat diffY = (nCurY - pTempY[nIndexY]) / 255.0;
                    MFloat diffU = 2 * (nCurU - pTempSrcU[nIndexUV]) / 255.0;
                    MFloat diffV = 2 * (nCurV - pTempSrcV[nIndexUV]) / 255.0;

                    MFloat fSmoothVal = m_pSmoothVal[ABS(i)];//1.0 * i * i * coef.smooth_factor;

                    MFloat fWeight = (diffY * diffY * fDiff_y_weight + diffU * diffU * fDiff_u_weight + diffV * diffV * fDiff_v_weight) * radial_factor
                        * fSmoothVal + 1.0;
#endif


#ifdef FIXED_POINT
                    MInt32 lWeight = fWeight*255.0 + 0.5;
                    CLAMP(lWeight, 0, 255);
                    lWeight = pCubeTable[lWeight];

                    lSumWeight += lWeight;
                    lSumU += pTempSrcU[nIndexUV] * lWeight;
                    lSumV += pTempSrcV[nIndexUV] * lWeight;

#else
                    fWeight = fWeight * fWeight * fWeight;
                    CLAMP(fWeight, 0.0, 1.0);

                    fSumWeight += fWeight;
                    fSumU += 1.0 * pTempSrcU[nIndexUV] * fWeight;
                    fSumV += 1.0 * pTempSrcV[nIndexUV] * fWeight;
#endif
                }
#ifdef FIXED_POINT
                pTempDstU[x] = MIN(255, (lSumU + (lSumWeight >> 1)) / lSumWeight);
                pTempDstV[x] = MIN(255, (lSumV + (lSumWeight >> 1)) / lSumWeight);
#else
                pTempDstU[x] = MIN(255.0, fSumU / fSumWeight + 0.5);
                pTempDstV[x] = MIN(255.0, fSumV / fSumWeight + 0.5);
#endif
            }
        }

        return lRet;
    }

    MVoid Arcsoft_ReduceColorNoise::GetCNRChromaCoef(DECOLORNOISE_PARAM param, CNR_Chroma_Coef_t& coef)
    {
        MFloat smooth_value = 0;
        // 计算颜色平滑参数
        if (param.smooth > 50)
        {
            smooth_value = 0.24 * (param.smooth - 50) + 12.0;
        }
        else
        {
            smooth_value = 0.12 * param.smooth + 6.0;
        }
        coef.smooth_step = param.step;
        //smooth_value *= coef.smooth_step;
        coef.smooth_factor = 1.0f / (smooth_value * smooth_value);
        // 基于颜色平滑参数得到的平滑半径
        coef.smooth_radius = (smooth_value / coef.smooth_step) * coef.smooth_step;

        //颜色参数计算
        MFloat chroma_val = MAX(0.0, MIN(1.0, param.chroma * 0.01));
        chroma_val = chroma_val * 0.027;// *1.2;

        coef.diff_y_weight = -1.0 / (chroma_val * chroma_val);
        coef.diff_u_weight = -1.0 / (chroma_val * chroma_val);
        coef.diff_v_weight = -1.0 / (chroma_val * chroma_val);

        //颜色细节参数计算
        MFloat detail_vale = MAX(0.0, MIN(1.0, param.detail * 0.01));
        coef.detail_value = detail_vale * detail_vale;
    }

#if 0
    MInt32 Arcsoft_ReduceColorNoise::runBGR2YUV(LPASVLOFFSCREEN pBGRSrcImage, LPASVLOFFSCREEN pDstImage,
        DECOLORNOISE_PARAM param)
    {
        MInt32 lRet = 0;

        if (pBGRSrcImage == MNull || pDstImage == MNull)
        {
            LOGE("pSrc == MNull || pDst == MNull");
            return -1;
        }

        if (pBGRSrcImage->u32PixelArrayFormat != ASVL_PAF_RGB24_B8G8R8)
        {
            return -2;
        }

        ASVLOFFSCREEN TempYUVImage;
        {
            TempYUVImage.u32PixelArrayFormat = ASVL_PAF_YUV;
            TempYUVImage.ppu8Plane[0] = SAFE_MALLOC(m_hMemMgr, MUInt8, pDstImage->i32Width * pDstImage->i32Height);
            TempYUVImage.ppu8Plane[1] = SAFE_MALLOC(m_hMemMgr, MUInt8, pDstImage->i32Width * pDstImage->i32Height);
            TempYUVImage.ppu8Plane[2] = SAFE_MALLOC(m_hMemMgr, MUInt8, pDstImage->i32Width * pDstImage->i32Height);
            TempYUVImage.i32Width = pBGRSrcImage->i32Width;
            TempYUVImage.i32Height = pBGRSrcImage->i32Height;
            TempYUVImage.pi32Pitch[0] = pBGRSrcImage->i32Width;
            TempYUVImage.pi32Pitch[1] = pBGRSrcImage->i32Width;
            TempYUVImage.pi32Pitch[2] = pBGRSrcImage->i32Width;
        }

        mat_write(pBGRSrcImage->i32Height, pBGRSrcImage->i32Width, CV_8UC3, pBGRSrcImage->ppu8Plane[0], "srcBGR.jpg");

#ifdef GAMMA_CONVERT
        // gamma变换
        gammaMap(pBGRSrcImage->ppu8Plane[0], pBGRSrcImage->i32Width * 3, pBGRSrcImage->i32Height, pBGRSrcImage->pi32Pitch[0],
            pGammaTable, pBGRSrcImage->ppu8Plane[0], pBGRSrcImage->pi32Pitch[0]);
#endif // GAMMA_CONVERT

        //BGR转YUV
        BGRToYUV444Planar(pBGRSrcImage->ppu8Plane[0], pBGRSrcImage->i32Width, pBGRSrcImage->i32Height, pBGRSrcImage->pi32Pitch[0],
            TempYUVImage.ppu8Plane[0], TempYUVImage.pi32Pitch[0],
            TempYUVImage.ppu8Plane[1], TempYUVImage.pi32Pitch[1],
            TempYUVImage.ppu8Plane[2], TempYUVImage.pi32Pitch[2]);

        lRet = runYUV444(&TempYUVImage, &TempYUVImage, param);

        mat_write(TempYUVImage.i32Height, TempYUVImage.i32Width, CV_8UC1, TempYUVImage.ppu8Plane[0], "Noise_dst_y.jpg");
        mat_write(TempYUVImage.i32Height, TempYUVImage.i32Width, CV_8UC1, TempYUVImage.ppu8Plane[1], "Noise_dst_u.jpg");
        mat_write(TempYUVImage.i32Height, TempYUVImage.i32Width, CV_8UC1, TempYUVImage.ppu8Plane[2], "Noise_dst_v.jpg");

        //YUV转BGR
        YUV444ToBGRPlanar(TempYUVImage.ppu8Plane[0], TempYUVImage.pi32Pitch[0],
            TempYUVImage.ppu8Plane[1], TempYUVImage.pi32Pitch[1],
            TempYUVImage.ppu8Plane[2], TempYUVImage.pi32Pitch[2],
            pBGRSrcImage->ppu8Plane[0], pBGRSrcImage->i32Width, pBGRSrcImage->i32Height, pBGRSrcImage->pi32Pitch[0]
        );

#ifdef GAMMA_CONVERT
        // 反gamma变换
        gammaMap(pBGRSrcImage->ppu8Plane[0], pBGRSrcImage->i32Width * 3, pBGRSrcImage->i32Height, pBGRSrcImage->pi32Pitch[0],
            pAntiGammaTable, pBGRSrcImage->ppu8Plane[0], pBGRSrcImage->pi32Pitch[0]);
#endif // GAMMA_CONVERT

        mat_write(pBGRSrcImage->i32Height, pBGRSrcImage->i32Width, CV_8UC3, pBGRSrcImage->ppu8Plane[0], "dstBGR.jpg");

        //BGR转YUV
        BGRToYUV444Planar(pBGRSrcImage->ppu8Plane[0], pBGRSrcImage->i32Width, pBGRSrcImage->i32Height, pBGRSrcImage->pi32Pitch[0],
            pDstImage->ppu8Plane[0], pDstImage->pi32Pitch[0],
            pDstImage->ppu8Plane[1], pDstImage->pi32Pitch[1],
            pDstImage->ppu8Plane[2], pDstImage->pi32Pitch[2]);

        SAFE_FREE_ARRAY(m_hMemMgr, TempYUVImage.ppu8Plane[0]);
        SAFE_FREE_ARRAY(m_hMemMgr, TempYUVImage.ppu8Plane[1]);
        SAFE_FREE_ARRAY(m_hMemMgr, TempYUVImage.ppu8Plane[2]);

        return lRet;
    }
#endif

    MVoid Arcsoft_ReduceColorNoise::Split_UV_Data(MByte* pSrcUV, MInt32 lPitchUV, MInt32 lDstWidth, MInt32 lDstHeight, MByte* pU, MInt32 lPitchU, MByte* pV, MInt32 lPitchV)
    {
        START_TIME;

        MInt32 x, y;

        for (y = 0; y < lDstHeight; y++)
        {
            MByte* pCurUV = pSrcUV + y * lPitchUV;
            MByte* pCurU = pU + y * lPitchU;
            MByte* pCurV = pV + y * lPitchV;
            x = 0;
#ifdef USE_NEON
            for (; x < lDstWidth - 16; x += 16)
            {
                uint8x16x2_t vdata_uv = vld2q_u8(pCurUV + x * 2);
                vst1q_u8(pCurU + x, vdata_uv.val[0]);
                vst1q_u8(pCurV + x, vdata_uv.val[1]);

            }
#endif
            for (; x < lDstWidth; x++)
            {
                pCurU[x] = pCurUV[x * 2];
                pCurV[x] = pCurUV[x * 2 + 1];
            }
        }


#if defined(DEBUG_OUTPUT)
        cv::Mat tempU(lDstHeight, lPitchU, CV_8UC1, pU);
        cv::Mat tempV(lDstHeight, lPitchV, CV_8UC1, pV);
        cv::Mat tempUV(lDstHeight, lPitchUV, CV_8UC1, pSrcUV);
#endif

        END_TIME;
    }

    MVoid Arcsoft_ReduceColorNoise::Merge_UV_Data(MByte* pU, MInt32 lPitchU, MByte* pV, MInt32 lPitchV, MInt32 lDstWidth, MInt32 lDstHeight, MByte* pSrcUV, MInt32 lPitchUV)
    {
        START_TIME;

        MInt32 x, y;

        lDstWidth /= 2;
        for (y = 0; y < lDstHeight; y++)
        {
            MByte* pCurUV = pSrcUV + y * lPitchUV;
            MByte* pCurU = pU + y * lPitchU;
            MByte* pCurV = pV + y * lPitchV;
            x = 0;
#ifdef USE_NEON
            for (; x < lDstWidth - 16; x += 16)
            {
                uint8x16x2_t vdata_uv;
                vdata_uv.val[0] = vld1q_u8(pCurU + x);
                vdata_uv.val[1] = vld1q_u8(pCurV + x);
                vst2q_u8(pCurUV + x * 2, vdata_uv);

            }
#endif
            for (; x < lDstWidth; x++)
            {
                pCurUV[x * 2] = pCurU[x];
                pCurUV[x * 2 + 1] = pCurV[x];
            }
        }

#if defined(DEBUG_OUTPUT)
        cv::Mat tempU(lDstHeight, lPitchU, CV_8UC1, pU);
        cv::Mat tempV(lDstHeight, lPitchV, CV_8UC1, pV);
        cv::Mat tempUV(lDstHeight, lPitchUV, CV_8UC1, pSrcUV);
#endif

        END_TIME;
    }


NS_SINFLE_IMAGE_ENHANCEMENT_END
