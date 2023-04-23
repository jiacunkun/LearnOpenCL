
#ifndef ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_REDUCECOLORNOISE_H
#define ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_REDUCECOLORNOISE_H

#include "single_image_enhancement_define.h"
#include "asvloffscreen.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

    typedef struct _tag_DECOLORNOISE_PARAM
    {
        MInt32 chroma;
        MInt32 detail;
        MInt32 smooth;
        MInt32 step;
    } DECOLORNOISE_PARAM;

    typedef struct CNR_Chroma_Coef
    {
        MInt16 smooth_step;
        MInt16 smooth_radius;
        MFloat detail_value;
        MFloat smooth_factor;
        MFloat diff_y_weight;
        MFloat diff_u_weight;
        MFloat diff_v_weight;
        MFloat gain_radial_lut[8];
    } CNR_Chroma_Coef_t;


    class Arcsoft_ReduceColorNoise
    {
    public:
        Arcsoft_ReduceColorNoise(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt16 nThreadCount = 16);
        ~Arcsoft_ReduceColorNoise();
        MInt32 init(MInt32 lWidth, MInt32 lHeight);
        MVoid release();
        MInt32 runNV21(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, MInt32 lVal);
        MInt32 runYUV444(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, DECOLORNOISE_PARAM param);
        MInt32 runYUV444(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, DECOLORNOISE_PARAM param, MUInt8* pBuffer);

    private:
        MInt32 runNV21(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, DECOLORNOISE_PARAM param);

    private:
        MInt32 ReduceChromaNoise_Hor(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, CNR_Chroma_Coef_t coef);
        MInt32 ReduceChromaNoise_Ver(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, CNR_Chroma_Coef_t coef);
        MInt32 ReduceChromaNoise_Hor(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, CNR_Chroma_Coef_t coef, MInt32 lTopLine, MInt32 lBotLine);
        MInt32 ReduceChromaNoise_Ver(LPASVLOFFSCREEN pSrcImage, LPASVLOFFSCREEN pDstImage, CNR_Chroma_Coef_t coef, MInt32 lTopLine, MInt32 lBotLine);
        MVoid GetCNRChromaCoef(DECOLORNOISE_PARAM param, CNR_Chroma_Coef_t &coef);
        MInt32 runBGR2YUV(LPASVLOFFSCREEN pBGRSrcImage, LPASVLOFFSCREEN pDstImage, DECOLORNOISE_PARAM param);
        MVoid Split_UV_Data(MByte* pSrcUV, MInt32 lPitchUV, MInt32 lDstWidth, MInt32 lDstHeight, MByte* pU, MInt32 lPitchU, MByte* pV, MInt32 lPitchV);
        MVoid Merge_UV_Data(MByte* pU, MInt32 lPitchU, MByte* pV, MInt32 lPitchV, MInt32 lDstWidth, MInt32 lDstHeight, MByte* pSrcUV, MInt32 lPitchUV);


    private:
        MHandle m_hMemMgr = MNull;
        MHandle m_mcvParallelMonitor = MNull;
        MInt16 m_nThreadCount = 0;
        ASVLOFFSCREEN m_TempImage = { MNull };
        ASVLOFFSCREEN m_I444 = { MNull };
        MUInt8* m_pUV = MNull;
        MFloat* m_pSmoothVal = MNull;
        MFloat* m_pDiffWeight[3] = { MNull };
    };

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif //ARCSOFT_SINGLE_IMAGE_ENHANCEMENT_ARCSOFT_REDUCECOLORNOISE_H
