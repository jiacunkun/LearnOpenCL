#include "anisGuide_8bit.h"
#include "DefineForDebug.h"
#include <mobilecv.h>
#include "median_cross.h"

//#define NEW_CALCU_VAR

const MInt32 g_lDirection = 4;
const MInt32 g_lRadius = 4;

typedef struct _tag_Ani_Guided_Filter_ST
{
    MInt32 task_ID;
    MHandle hMemMgr;
    MInt32 lRet;

    MInt32 lPitchSrc;
    MInt32 lPitchGuided;
    MInt32 lMeanPitch;
    MInt32 lWidth;
    MInt32 lHeight;
    MFloat Feps;

    MInt32 startRow;
    MInt32 endRow;

    MUInt8* pSrc;
    MUInt8* pGuide;
    MUInt8** Coff_A;
    MUInt8** Coff_B;
    MUInt8* pMeanA;
    MUInt8* pMeanB;

} Ani_Guided_Filter_ST, * LAni_Guided_Filter_ST;

MVoid Compute_AB_Offset_Direction(MInt32 Shift[][4], MInt32 lPitch)
{
    Shift[0][0] = 1; // hori
    Shift[0][1] = 2;
    Shift[0][2] = 3;
    Shift[0][3] = 4;

    Shift[1][0] = 1 + lPitch; // diag
    Shift[1][1] = 2 + lPitch * 2;
    Shift[1][2] = 3 + lPitch * 3;
    Shift[1][3] = 4 + lPitch * 4;

    Shift[2][0] = lPitch; // vect
    Shift[2][1] = lPitch * 2;
    Shift[2][2] = lPitch * 3;
    Shift[2][3] = lPitch * 4;

    Shift[3][0] = -1 + lPitch; // anti
    Shift[3][1] = -2 + lPitch * 2;
    Shift[3][2] = -3 + lPitch * 3;
    Shift[3][3] = -4 + lPitch * 4;
}

MVoid GetWeiOffset_Direction(MInt32* pOffset, MInt32 pitch, MInt32 nRadius)
{
    pOffset[0] = -nRadius; //水平
    pOffset[1] = nRadius;

    pOffset[2] = -nRadius - pitch * nRadius; // 斜对角
    pOffset[3] = nRadius + pitch * nRadius;

    pOffset[4] = -pitch * nRadius; // 竖着
    pOffset[5] = pitch * nRadius;

    pOffset[6] = nRadius - pitch * nRadius; // 反斜对角
    pOffset[7] = -nRadius + pitch * nRadius;
}

#if 1
static MUInt16 pWeights[5] = {6, 4, 2, 1, 9}; //原图高斯
static const MUInt16 SUM_WEIGHT = 35;
#else
static MUInt16 pWeights[5] = { 1, 1, 1, 1, 1 }; //原图均值
static const MUInt16 SUM_WEIGHT = 9;
#endif
static const MFloat INV_SUM_WEIGHT = 1.0f / SUM_WEIGHT;

MInt32 GetAB(MUInt8* pSrc,
    MInt32 lPitchSrc,
    MUInt8* pGuide,
    MInt32 lGuidePitch,
    MUInt8** Coff_A,
    MUInt8** Coff_B,
    MInt32 lWidth,
    MInt32 lHeight,
    MFloat Feps,
    MInt32 startRow,
    MInt32 endRow)
{
    MInt32 lKnlSize = g_lRadius * 2 + 1;
    const MFloat FTempEps = Feps * lKnlSize * lKnlSize;

    MInt32 AB_Offset_Src[4][4] = { MNull };

    Compute_AB_Offset_Direction(AB_Offset_Src, lPitchSrc);

    MInt32 startRow2 = MAX(g_lRadius, startRow);
    MInt32 endRow2 = MIN(lHeight - g_lRadius, endRow);

    MFloat fDivFactor = 1.0 / lKnlSize;

#ifdef USE_NEON
    float32x4_t fRoundValue = vdupq_n_f32(0.49999997f);
    float32x4_t fRoundValue9 = vdupq_n_f32(INV_SUM_WEIGHT);//vdupq_n_f32(fDivFactor);
    float32x4_t fRoundValue128 = vdupq_n_f32(128.0f);
    float32x4_t FTempEps_32x4 = vdupq_n_f32(FTempEps);

    uint8x8_t lCur_8x8;

    uint8x8_t lS0_8x8;
    uint8x8_t lS1_8x8;
#endif

    for (MInt32 y = startRow2; y < endRow2; y++)
    {
        MInt32 lShif_Num0 = y * lWidth;
        MUInt8* pDataSrc = pSrc + y * lPitchSrc;
        MUInt8* pDataGuide = pGuide + y * lGuidePitch;

        MInt32 x = g_lRadius;

#ifdef USE_NEON
        //////////////////////////////////// 求图像均值 ///////////////////////////////////////////////////
        for (; x < lWidth - g_lRadius - 8; x += 8)
        {
            // lode data
            lCur_8x8 = vld1_u8((MUInt8*)pDataSrc + x);

            for (MInt32 i = 0; i < g_lDirection; i++)
            {
                uint16x8_t sum_src_16x8 = vmovl_u8(lCur_8x8);
                uint16x8_t sum_src_weight_16x8 = vdupq_n_u16(pWeights[4]);
                sum_src_16x8 = vmulq_u16(sum_src_16x8, sum_src_weight_16x8);

                for (MInt32 j = 0; j < g_lRadius; j++)
                {
                    MInt32 s_0 = AB_Offset_Src[i][j];

                    lS0_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_0);
                    lS1_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_0);

                    sum_src_16x8 = vmlaq_n_u16(sum_src_16x8, vmovl_u8(lS0_8x8), pWeights[j]);
                    sum_src_16x8 = vmlaq_n_u16(sum_src_16x8, vmovl_u8(lS1_8x8), pWeights[j]);
                }
                // store
                // B
                float32x4_t fMean_high_32x4 = vcvtq_f32_u32(vmovl_u16(vget_high_u16(sum_src_16x8)));
                fMean_high_32x4 = vmulq_f32(fMean_high_32x4, fRoundValue9);
                uint16x4_t mean_src_16x4_high = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fMean_high_32x4, fRoundValue)));


                float32x4_t fMean_low_32x4 = vcvtq_f32_u32(vmovl_u16(vget_low_u16(sum_src_16x8)));
                fMean_low_32x4 = vmulq_f32(fMean_low_32x4, fRoundValue9);
                uint16x4_t mean_src_16x4_low = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fMean_low_32x4, fRoundValue)));

                vst1_u8((Coff_B[i] + lShif_Num0 + x), vmovn_u16(vcombine_u16(mean_src_16x4_low, mean_src_16x4_high)));
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 计算引导图的方差
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (x = g_lRadius; x < lWidth - g_lRadius - 8; x += 8)
        {

            // lode data
            lCur_8x8 = vld1_u8((MUInt8*)pDataGuide + x);

            for (MInt32 i = 0; i < g_lDirection; i++)
            {
                uint16x8_t sum_src_16x8 = vmovl_u8(lCur_8x8);
                uint16x8_t src_sqrt0_16x8 = vmull_u8(lCur_8x8, lCur_8x8);;
                uint32x4_t sum_src_sqrt_high_32x4;
                uint32x4_t sum_src_sqrt_low_32x4;
                uint32x4_t src_sqrt_sum_high_32x4 = vmovl_u16(vget_high_u16(src_sqrt0_16x8));
                uint32x4_t src_sqrt_sum_low_32x4 = vmovl_u16(vget_low_u16(src_sqrt0_16x8));
#ifdef NEW_CALCU_VAR
                uint16x8_t diff_16x8 = vdupq_n_u16(0);
#endif
                for (MInt32 j = 0; j < g_lRadius; j++)
                {
                    MInt32 s_0 = AB_Offset_Src[i][j];

                    lS0_8x8 = vld1_u8((MUInt8*)pDataGuide + x + s_0);
                    lS1_8x8 = vld1_u8((MUInt8*)pDataGuide + x - s_0);

#ifdef NEW_CALCU_VAR                  
                    diff_16x8 = vabal_u8(diff_16x8, lS0_8x8, lCur_8x8);
                    diff_16x8 = vabal_u8(diff_16x8, lS1_8x8, lCur_8x8);
#else
                    // 和的平方
                    sum_src_16x8 = vaddw_u8(sum_src_16x8, lS0_8x8);
                    sum_src_16x8 = vaddw_u8(sum_src_16x8, lS1_8x8);

                    // 平方的和
                    src_sqrt0_16x8 = vmull_u8(lS0_8x8, lS0_8x8);
                    src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt0_16x8));
                    src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt0_16x8));

                    src_sqrt0_16x8 = vmull_u8(lS1_8x8, lS1_8x8);
                    src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt0_16x8));
                    src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt0_16x8));
#endif
                }
#ifdef NEW_CALCU_VAR
                diff_16x8 = vmulq_u16(diff_16x8, diff_16x8);
                src_sqrt_sum_high_32x4 = vmovl_u16(vget_high_u16(diff_16x8));
                src_sqrt_sum_low_32x4 = vmovl_u16(vget_low_u16(diff_16x8));
#else
                uint16x4_t sum_src_high_16x4 = vget_high_u16(sum_src_16x8);
                uint16x4_t sum_src_low_16x4 = vget_low_u16(sum_src_16x8);
                sum_src_sqrt_high_32x4 = vmull_u16(sum_src_high_16x4, sum_src_high_16x4);
                sum_src_sqrt_low_32x4 = vmull_u16(sum_src_low_16x4, sum_src_low_16x4);
                src_sqrt_sum_high_32x4 = vmulq_n_u32(src_sqrt_sum_high_32x4, lKnlSize);
                src_sqrt_sum_low_32x4 = vmulq_n_u32(src_sqrt_sum_low_32x4, lKnlSize);

                // 计算方差
                src_sqrt_sum_high_32x4 = vsubq_u32(src_sqrt_sum_high_32x4, sum_src_sqrt_high_32x4);
                src_sqrt_sum_low_32x4 = vsubq_u32(src_sqrt_sum_low_32x4, sum_src_sqrt_low_32x4);
#endif 
                float32x4_t fVar_high_32x4 = vcvtq_f32_u32(src_sqrt_sum_high_32x4);
                float32x4_t fVar_low_32x4 = vcvtq_f32_u32(src_sqrt_sum_low_32x4);

                // 计算AB
                //float32x4_t fVarEps_high_32x4 = vaddq_f32(fVar_high_32x4, fEps_high_32x4);
                float32x4_t fVarEps_high_32x4 = vaddq_f32(fVar_high_32x4, FTempEps_32x4);
                float32x4_t fInvVar_high_32x4 = vrecpeq_f32(fVarEps_high_32x4); // 倒数
                fInvVar_high_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_high_32x4, fInvVar_high_32x4), fInvVar_high_32x4);
                //fInvVar_high_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_high_32x4, fInvVar_high_32x4),
                //    fInvVar_high_32x4); // 精确化
                float32x4_t fCoefA_high_32x4 = vmulq_f32(fInvVar_high_32x4,
                    vmulq_f32(fVar_high_32x4, fRoundValue128));
                uint16x4_t lCoefA_high_16x4 = vmovn_u32(
                    vcvtq_u32_f32(vaddq_f32(fCoefA_high_32x4, fRoundValue)));


                //float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, fEps_low_32x4);
                float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, FTempEps_32x4);
                float32x4_t fInvVar_low_32x4 = vrecpeq_f32(fVarEps_low_32x4);
                fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4), fInvVar_low_32x4);
                //fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4),
                //    fInvVar_low_32x4); // 精确化
                float32x4_t fCoefA_low_32x4 = vmulq_f32(fInvVar_low_32x4, vmulq_f32(fVar_low_32x4, fRoundValue128));
                uint16x4_t lCoefA_low_16x4 = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fCoefA_low_32x4, fRoundValue)));

                // store
                // A
                vst1_u8((Coff_A[i] + lShif_Num0 + x), vmovn_u16(vcombine_u16(lCoefA_low_16x4, lCoefA_high_16x4)));
            }

        }
#endif

        for (; x < lWidth - g_lRadius; x++)
        {
            MUInt8 lCur = pDataSrc[x];
            MUInt8 lCur_ = pDataGuide[x];

            MInt32 src_sum = 0;
            MInt32 guide_sum = 0;
            MInt32 guide_sum_sqr = 0;
            MInt32 s[4] = { 0 };
            MUInt8 a0[4] = { 0 };
            MUInt8 a1[4] = { 0 };
            MUInt8 b0[4] = { 0 };
            MUInt8 b1[4] = { 0 };

            for (MInt32 i = 0; i < g_lDirection; i++)
            {
                src_sum = lCur * pWeights[4];
                guide_sum = lCur_;
                guide_sum_sqr = lCur_ * lCur_;
                MInt32 lDiff = 0;
                for (MInt32 j = 0; j < g_lRadius; j++)
                {
                    s[j] = AB_Offset_Src[i][j];

                    a0[j] = pDataSrc[x + s[j]];
                    a1[j] = pDataSrc[x - s[j]];

                    b0[j] = pDataGuide[x + s[j]];
                    b1[j] = pDataGuide[x - s[j]];

                    src_sum += ((a0[j] + a1[j]) * pWeights[j]);
#ifdef NEW_CALCU_VAR
                    lDiff += (ABS(b0[j] - lCur_) + ABS(b1[j] - lCur_));
#else
                    guide_sum += (b0[j] + b1[j]);
                    guide_sum_sqr += (b0[j] * b0[j] + b1[j] * b1[j]);
#endif
                }


                src_sum = (src_sum + SUM_WEIGHT / 2) / SUM_WEIGHT;
#ifdef NEW_CALCU_VAR
                lDiff = lDiff * lDiff;
                MInt32 lGVar = lDiff;// guide_sum_sqr* lKnlSize - guide_sum * guide_sum;
#else
                MInt32 lGVar = guide_sum_sqr * lKnlSize - guide_sum * guide_sum;
#endif
                MInt32 fCof_A = 128.0 * lGVar / (lGVar + FTempEps + 0.001) + 0.5;
                MInt32 fCof_B = src_sum > 255 ? 255 : src_sum; // * ( 1 - fCof_A ); // todo:将1-a放到后面去做


                Coff_A[i][lShif_Num0 + x] = fCof_A; // 0~128
                Coff_B[i][lShif_Num0 + x] = fCof_B; // 0~(7+sizeof(T))^2
            }
        }
    }

    return MOK;
}

MInt32 GetA(
    MUInt8* pGuide,
    MInt32 lGuidePitch,
    MUInt8** Coff_A,
    MInt32 lWidth,
    MInt32 lHeight,
    MFloat Feps,
    MInt32 startRow,
    MInt32 endRow)
{
    MInt32 lRadius = 3;
    MInt32 lKnlSize = lRadius * 2 + 1;
    const MFloat FTempEps = Feps * lKnlSize * lKnlSize;

    MInt32 AB_Offset_Src[4][4] = { MNull };

    Compute_AB_Offset_Direction(AB_Offset_Src, lGuidePitch);

    MInt32 startRow2 = MAX(lRadius, startRow);
    MInt32 endRow2 = MIN(lHeight - lRadius, endRow);

    MFloat fDivFactor = 1.0 / lKnlSize;

#ifdef USE_NEON
    float32x4_t fRoundValue = vdupq_n_f32(0.49999997f);
    float32x4_t fRoundValue9 = vdupq_n_f32(fDivFactor);
    float32x4_t fRoundValue128 = vdupq_n_f32(128.0f);
    float32x4_t FTempEps_32x4 = vdupq_n_f32(FTempEps);

    uint8x8_t lCur_8x8;

    uint8x8_t lS0_8x8;
    uint8x8_t lS1_8x8;
    uint8x8_t lS2_8x8;
    uint8x8_t lS3_8x8;
    uint8x8_t lS4_8x8;
    uint8x8_t lS5_8x8;
    uint8x8_t lS6_8x8;
    uint8x8_t lS7_8x8;

    uint16x8_t src_sqrt0_16x8;
    uint16x8_t src_sqrt1_16x8;
    uint16x8_t src_sqrt2_16x8;
    uint16x8_t src_sqrt3_16x8;
    uint16x8_t src_sqrt4_16x8;
    uint16x8_t src_sqrt5_16x8;
    uint16x8_t src_sqrt6_16x8;
    uint16x8_t src_sqrt7_16x8;
#endif

    for (MInt32 y = startRow2; y < endRow2; y++)
    {
        MInt32 lShif_Num0 = y * lWidth;
        MUInt8* pDataGuide = pGuide + y * lGuidePitch;

        MInt32 x = lRadius;

#ifdef USE_NEON
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 计算引导图的方差
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (x = lRadius; x < lWidth - lRadius - 8; x += 8)
        {

            // lode data
            lCur_8x8 = vld1_u8((MUInt8*)pDataGuide + x);

            for (MInt32 i = 0; i < g_lDirection; i++)
            {
                uint16x8_t sum_src_16x8 = vmovl_u8(lCur_8x8);
                uint16x8_t src_sqrt0_16x8 = vmull_u8(lCur_8x8, lCur_8x8);;
                uint32x4_t sum_src_sqrt_high_32x4;
                uint32x4_t sum_src_sqrt_low_32x4;
                uint32x4_t src_sqrt_sum_high_32x4 = vmovl_u16(vget_high_u16(src_sqrt0_16x8));
                uint32x4_t src_sqrt_sum_low_32x4 = vmovl_u16(vget_low_u16(src_sqrt0_16x8));
#ifdef NEW_CALCU_VAR
                uint16x8_t diff_16x8 = vdupq_n_u16(0);
#endif
                for (MInt32 j = 0; j < lRadius; j++)
                {
                    MInt32 s_0 = AB_Offset_Src[i][j];

                    lS0_8x8 = vld1_u8((MUInt8*)pDataGuide + x + s_0);
                    lS1_8x8 = vld1_u8((MUInt8*)pDataGuide + x - s_0);

#ifdef NEW_CALCU_VAR                  
                    diff_16x8 = vabal_u8(diff_16x8, lS0_8x8, lCur_8x8);
                    diff_16x8 = vabal_u8(diff_16x8, lS1_8x8, lCur_8x8);
#else
                    // 和的平方
                    sum_src_16x8 = vaddw_u8(sum_src_16x8, lS0_8x8);
                    sum_src_16x8 = vaddw_u8(sum_src_16x8, lS1_8x8);

                    // 平方的和
                    src_sqrt0_16x8 = vmull_u8(lS0_8x8, lS0_8x8);
                    src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt0_16x8));
                    src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt0_16x8));

                    src_sqrt0_16x8 = vmull_u8(lS1_8x8, lS1_8x8);
                    src_sqrt_sum_high_32x4 = vaddw_u16(src_sqrt_sum_high_32x4, vget_high_u16(src_sqrt0_16x8));
                    src_sqrt_sum_low_32x4 = vaddw_u16(src_sqrt_sum_low_32x4, vget_low_u16(src_sqrt0_16x8));
#endif
                }
#ifdef NEW_CALCU_VAR
                diff_16x8 = vmulq_u16(diff_16x8, diff_16x8);
                src_sqrt_sum_high_32x4 = vmovl_u16(vget_high_u16(diff_16x8));
                src_sqrt_sum_low_32x4 = vmovl_u16(vget_low_u16(diff_16x8));
#else
                uint16x4_t sum_src_high_16x4 = vget_high_u16(sum_src_16x8);
                uint16x4_t sum_src_low_16x4 = vget_low_u16(sum_src_16x8);
                sum_src_sqrt_high_32x4 = vmull_u16(sum_src_high_16x4, sum_src_high_16x4);
                sum_src_sqrt_low_32x4 = vmull_u16(sum_src_low_16x4, sum_src_low_16x4);
                src_sqrt_sum_high_32x4 = vmulq_n_u32(src_sqrt_sum_high_32x4, lKnlSize);
                src_sqrt_sum_low_32x4 = vmulq_n_u32(src_sqrt_sum_low_32x4, lKnlSize);

                // 计算方差
                src_sqrt_sum_high_32x4 = vsubq_u32(src_sqrt_sum_high_32x4, sum_src_sqrt_high_32x4);
                src_sqrt_sum_low_32x4 = vsubq_u32(src_sqrt_sum_low_32x4, sum_src_sqrt_low_32x4);
#endif 
                float32x4_t fVar_high_32x4 = vcvtq_f32_u32(src_sqrt_sum_high_32x4);
                float32x4_t fVar_low_32x4 = vcvtq_f32_u32(src_sqrt_sum_low_32x4);

                // 计算AB
                //float32x4_t fVarEps_high_32x4 = vaddq_f32(fVar_high_32x4, fEps_high_32x4);
                float32x4_t fVarEps_high_32x4 = vaddq_f32(fVar_high_32x4, FTempEps_32x4);
                float32x4_t fInvVar_high_32x4 = vrecpeq_f32(fVarEps_high_32x4); // 倒数
                fInvVar_high_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_high_32x4, fInvVar_high_32x4), fInvVar_high_32x4);
                //fInvVar_high_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_high_32x4, fInvVar_high_32x4),
                //    fInvVar_high_32x4); // 精确化
                float32x4_t fCoefA_high_32x4 = vmulq_f32(fInvVar_high_32x4,
                    vmulq_f32(fVar_high_32x4, fRoundValue128));
                uint16x4_t lCoefA_high_16x4 = vmovn_u32(
                    vcvtq_u32_f32(vaddq_f32(fCoefA_high_32x4, fRoundValue)));


                //float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, fEps_low_32x4);
                float32x4_t fVarEps_low_32x4 = vaddq_f32(fVar_low_32x4, FTempEps_32x4);
                float32x4_t fInvVar_low_32x4 = vrecpeq_f32(fVarEps_low_32x4);
                fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4), fInvVar_low_32x4);
                //fInvVar_low_32x4 = vmulq_f32(vrecpsq_f32(fVarEps_low_32x4, fInvVar_low_32x4),
                //    fInvVar_low_32x4); // 精确化
                float32x4_t fCoefA_low_32x4 = vmulq_f32(fInvVar_low_32x4, vmulq_f32(fVar_low_32x4, fRoundValue128));
                uint16x4_t lCoefA_low_16x4 = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fCoefA_low_32x4, fRoundValue)));

                // store
                // A
                vst1_u8((Coff_A[i] + lShif_Num0 + x), vmovn_u16(vcombine_u16(lCoefA_low_16x4, lCoefA_high_16x4)));
                }

            }
#endif

        for (; x < lWidth - lRadius; x++)
        {
            MUInt8 lCur_ = pDataGuide[x];

            MInt32 guide_sum = 0;
            MInt32 guide_sum_sqr = 0;
            MInt32 s[4] = { 0 };
            MUInt8 a0[4] = { 0 };
            MUInt8 a1[4] = { 0 };
            MUInt8 b0[4] = { 0 };
            MUInt8 b1[4] = { 0 };

            for (MInt32 i = 0; i < g_lDirection; i++)
            {
                guide_sum = lCur_;
                guide_sum_sqr = lCur_ * lCur_;
                MInt32 lDiff = 0;
                for (MInt32 j = 0; j < lRadius; j++)
                {
                    s[j] = AB_Offset_Src[i][j];

                    b0[j] = pDataGuide[x + s[j]];
                    b1[j] = pDataGuide[x - s[j]];

#ifdef NEW_CALCU_VAR
                    lDiff += (ABS(b0[j] - lCur_) + ABS(b1[j] - lCur_));
#else
                    guide_sum += (b0[j] + b1[j]);
                    guide_sum_sqr += (b0[j] * b0[j] + b1[j] * b1[j]);
#endif
                }

#ifdef NEW_CALCU_VAR
                lDiff = lDiff * lDiff;
                MInt32 lGVar = lDiff;// guide_sum_sqr* lKnlSize - guide_sum * guide_sum;
#else
                MInt32 lGVar = guide_sum_sqr * lKnlSize - guide_sum * guide_sum;
#endif
                MInt32 fCof_A = 128.0 * lGVar / (lGVar + FTempEps + 0.001) + 0.5;

                Coff_A[i][lShif_Num0 + x] = fCof_A; // 0~128
            }
        }
    }

    return MOK;
}

MInt32 GetB(MUInt8* pSrc,
    MInt32 lPitchSrc,
    MUInt8** Coff_A,
    MUInt8** Coff_B,
    MUInt8* pMeanB,
    MInt32 lWidth,
    MInt32 lHeight,
    MFloat Feps,
    MInt32 startRow,
    MInt32 endRow)
{
    MInt32 lRadius = 3;
    MInt32 AB_Offset_Src[4][4] = { MNull };

    Compute_AB_Offset_Direction(AB_Offset_Src, lPitchSrc);

    MInt32 startRow2 = MAX(lRadius, startRow);
    MInt32 endRow2 = MIN(lHeight - lRadius, endRow);

#ifdef USE_NEON00
    float32x4_t fRoundValue = vdupq_n_f32(0.49999997f);
    float32x4_t fRoundValue128 = vdupq_n_f32(128.0f);

    uint8x8_t lCur_8x8;

    uint8x8_t lS0_8x8;
    uint8x8_t lS1_8x8;
    uint8x8_t lS2_8x8;
    uint8x8_t lS3_8x8;
    uint8x8_t lS4_8x8;
    uint8x8_t lS5_8x8;
    uint8x8_t lS6_8x8;
    uint8x8_t lS7_8x8;
#endif

    for (MInt32 y = startRow2; y < endRow2; y++)
    {
        MInt32 lShif_Num0 = y * lWidth;
        MUInt8* pDataSrc = pSrc + y * lPitchSrc;
        MUInt8* pCurFusB = pMeanB + lShif_Num0;

        MInt32 x = lRadius;
        MFloat fValue = 0.0000001*2;

#ifdef USE_NEON00
        //////////////////////////////////// 求图像均值 ///////////////////////////////////////////////////
        for (; x < lWidth - lRadius - 8; x += 8)
        {
            // lode data
            lCur_8x8 = vld1_u8((MUInt8*)pDataSrc + x);

            for (MInt32 i = 0; i < lDirection; i++)
            {
                MInt32 s_0 = AB_Offset_Src[i][0];
                MInt32 s_1 = AB_Offset_Src[i][1];
                MInt32 s_2 = AB_Offset_Src[i][2];
                MInt32 s_3 = AB_Offset_Src[i][3];

                lS0_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_0);
                lS1_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_0);
                lS2_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_1);
                lS3_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_1);
                lS4_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_2);
                lS5_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_2);
                lS6_8x8 = vld1_u8((MUInt8*)pDataSrc + x + s_3);
                lS7_8x8 = vld1_u8((MUInt8*)pDataSrc + x - s_3);


                // 和的平方
                uint16x8_t sum_src_16x8 = vaddl_u8(lS0_8x8, lS1_8x8);

                sum_src_16x8 = vaddw_u8(sum_src_16x8, lS2_8x8);
                sum_src_16x8 = vaddw_u8(sum_src_16x8, lS3_8x8);

                sum_src_16x8 = vaddw_u8(sum_src_16x8, lS4_8x8);
                sum_src_16x8 = vaddw_u8(sum_src_16x8, lS5_8x8);

                sum_src_16x8 = vaddw_u8(sum_src_16x8, lS6_8x8);
                sum_src_16x8 = vaddw_u8(sum_src_16x8, lS7_8x8);

                sum_src_16x8 = vaddw_u8(sum_src_16x8, lCur_8x8);

                // store
                // B
                float32x4_t fMean_high_32x4 = vcvtq_f32_u32(vmovl_u16(vget_high_u16(sum_src_16x8)));
                fMean_high_32x4 = vmulq_f32(fMean_high_32x4, fRoundValue9);
                uint16x4_t mean_src_16x4_high = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fMean_high_32x4, fRoundValue)));


                float32x4_t fMean_low_32x4 = vcvtq_f32_u32(vmovl_u16(vget_low_u16(sum_src_16x8)));
                fMean_low_32x4 = vmulq_f32(fMean_low_32x4, fRoundValue9);
                uint16x4_t mean_src_16x4_low = vmovn_u32(vcvtq_u32_f32(vaddq_f32(fMean_low_32x4, fRoundValue)));

                vst1_u8((Coff_B[i] + lShif_Num0 + x), vmovn_u16(vcombine_u16(mean_src_16x4_low, mean_src_16x4_high)));
            }
        }

#endif

        for (; x < lWidth - lRadius; x++)
        {
            MUInt8 lCur = pDataSrc[x];

            MFloat src_sum = 0;
            MFloat src_wei_sum = 0;
            MFloat wei = 0;
            MFloat detailVal = 0;
            MInt32 s[4] = { 0 };
            MUInt8 a0[4] = { 0 };
            MUInt8 a1[4] = { 0 };

            // 加权平均
            MInt32 lShif_Num = lShif_Num0 + x;
            MInt32 lSumA = 0;
            MInt32 lSumB = 0;
            MInt32 lSumWei = 0;
            MInt32 lCof_A;
            MInt32 lCof_B;
            MInt32 lWei;

            for (MInt32 i = 0; i < g_lDirection; i++)
            {
                src_sum = lCur;
                src_wei_sum = 1.0;
                MInt32 lDiff = 0;

                // 
                lCof_A = Coff_A[i][lShif_Num];
                lWei = 128 - lCof_A;
                lWei = (lWei + lWei * lWei + 8) >> 4;
                lWei++;
                detailVal = fValue*64*128;// *lCof_A* lCof_A;

                for (MInt32 j = 0; j < lRadius; j++)
                {
                    s[j] = AB_Offset_Src[i][j];

                    a0[j] = pDataSrc[x + s[j]];
                    a1[j] = pDataSrc[x - s[j]];

                    lDiff = a0[j] - lCur;
                    wei = 1.0 - lDiff * lDiff * detailVal;
                    wei = wei * wei * wei;
                    CLAMP(wei, 0.0, 1.0);
                    src_sum += a0[j] * wei;
                    src_wei_sum += wei;

                    lDiff = a1[j] - lCur;
                    wei = 1.0 - lDiff * lDiff * detailVal;
                    wei = wei * wei * wei;
                    CLAMP(wei, 0.0, 1.0);
                    src_sum += a1[j] * wei;
                    src_wei_sum += wei;
                }
               
                MInt32 fCof_B = src_sum / src_wei_sum + 0.5;
                Coff_B[i][lShif_Num0 + x] = fCof_B > 255 ? 255 : fCof_B;
                          
                // 加权平均
                lSumWei += lWei;
                lSumB += fCof_B * lWei;
            }

            MInt32 fMean_b = (lSumB + (lSumWei >> 1)) / (lSumWei); //lSumB * 1.0 / lSumWei + 0.5;//
            pCurFusB[x] = fMean_b;
        }
    }

    return MOK;
}

MInt32 GetAThreads(MHandle mcvParallelMonitor, 
    MUInt8* pGuide, MInt32 lGuidePitch,
    MUInt8** Coff_A, 
    MInt32 lWidth, MInt32 lHeight, MFloat Feps)
{
    START_TIME;
    MInt32 lRet = MOK;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    {
        /// 设置回调函数
        auto func_lamda = [](MVoid* HParam) -> MVoid
        {
            LAni_Guided_Filter_ST filter_sturct = (LAni_Guided_Filter_ST)HParam;

            GetA(
                filter_sturct->pGuide,
                filter_sturct->lPitchGuided,
                filter_sturct->Coff_A,
                filter_sturct->lWidth,
                filter_sturct->lHeight,
                filter_sturct->Feps,
                filter_sturct->startRow,
                filter_sturct->endRow);
        };
        MVoid(*func)(MVoid*) = func_lamda;


        /// 设置参数
        MInt32 lTaskNum = lHeight > 1024 ? 16 : 8;
        MInt32 lTaskHeight = lHeight / lTaskNum;
        lTaskHeight = (lTaskHeight >> 2) << 2;

        Ani_Guided_Filter_ST pParam[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].startRow = lTaskHeight * lnum;
            pParam[lnum].endRow = lTaskHeight * (lnum + 1);
        }
        pParam[lTaskNum - 1].endRow = lHeight;


        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].task_ID = lnum;

            pParam[lnum].pGuide = pGuide;
            pParam[lnum].lPitchGuided = lGuidePitch;
            pParam[lnum].Coff_A = Coff_A;
            pParam[lnum].lWidth = lWidth;
            pParam[lnum].lHeight = lHeight;
            pParam[lnum].Feps = Feps;
        }


        /// 创建线程
        MInt32 lTaskID[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
            if (lTaskID[lnum] < 0)
            {
                lRet = MERR_BAD_STATE;
                goto exit;
            }
        }

        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
        }
    }
#else
    {
        lRet = GetA(
            pGuide,
            lGuidePitch,
            Coff_A,
            lWidth,
            lHeight,
            Feps, 0, lHeight);
    }
#endif

exit:

    END_TIME;
    return lRet;
}

MInt32 GetBThreads(MHandle mcvParallelMonitor, MUInt8* pSrc, MInt32 lPitchSrc,
    MUInt8** Coff_A, MUInt8** Coff_B, MUInt8* pMeanB,
    MInt32 lWidth, MInt32 lHeight, MFloat Feps)
{
    START_TIME;
    MInt32 lRet = MOK;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    {
        /// 设置回调函数
        auto func_lamda = [](MVoid* HParam) -> MVoid
        {
            LAni_Guided_Filter_ST filter_sturct = (LAni_Guided_Filter_ST)HParam;

            GetB(filter_sturct->pSrc,
                filter_sturct->lPitchSrc,
                filter_sturct->Coff_A,
                filter_sturct->Coff_B,
                filter_sturct->pMeanB,
                filter_sturct->lWidth,
                filter_sturct->lHeight,
                filter_sturct->Feps,
                filter_sturct->startRow,
                filter_sturct->endRow);
        };
        MVoid(*func)(MVoid*) = func_lamda;



        /// 设置参数
        MInt32 lTaskNum = lHeight > 1024 ? 16 : 8;
        MInt32 lTaskHeight = lHeight / lTaskNum;
        lTaskHeight = (lTaskHeight >> 2) << 2;

        Ani_Guided_Filter_ST pParam[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].startRow = lTaskHeight * lnum;
            pParam[lnum].endRow = lTaskHeight * (lnum + 1);
        }
        pParam[lTaskNum - 1].endRow = lHeight;


        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].task_ID = lnum;

            pParam[lnum].pSrc = pSrc;
            pParam[lnum].lPitchSrc = lPitchSrc;
            pParam[lnum].Coff_B = Coff_B;
            pParam[lnum].Coff_A = Coff_A;
            pParam[lnum].pMeanB = pMeanB;
            pParam[lnum].lWidth = lWidth;
            pParam[lnum].lHeight = lHeight;
            pParam[lnum].Feps = Feps;
        }


        /// 创建线程
        MInt32 lTaskID[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
            if (lTaskID[lnum] < 0)
            {
                lRet = MERR_BAD_STATE;
                goto exit;
            }
        }

        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
        }
    }
#else
    {
        lRet = GetB(pSrc,
            lPitchSrc,
            Coff_A,
            Coff_B,
            pMeanB,
            lWidth,
            lHeight,
            Feps, 0, lHeight);
    }
#endif

exit:

    END_TIME;
    return lRet;
}

MInt32 GetABThreads(MHandle mcvParallelMonitor, MUInt8* pSrc, MInt32 lPitchSrc,
    MUInt8* pGuide, MInt32 lGuidePitch,
    MUInt8** Coff_A, MUInt8** Coff_B,
    MInt32 lWidth, MInt32 lHeight, MFloat Feps)
{
    START_TIME;
    MInt32 lRet = MOK;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    {
        /// 设置回调函数
        auto func_lamda = [](MVoid* HParam) -> MVoid
        {
            LAni_Guided_Filter_ST filter_sturct = (LAni_Guided_Filter_ST)HParam;

            GetAB(filter_sturct->pSrc,
                filter_sturct->lPitchSrc,
                filter_sturct->pGuide,
                filter_sturct->lPitchGuided,
                filter_sturct->Coff_A,
                filter_sturct->Coff_B,
                filter_sturct->lWidth,
                filter_sturct->lHeight,
                filter_sturct->Feps,
                filter_sturct->startRow, 
                filter_sturct->endRow);
        };
        MVoid(*func)(MVoid*) = func_lamda;



        /// 设置参数
        MInt32 lTaskNum = lHeight > 1024 ? 16 : 8;
        MInt32 lTaskHeight = lHeight / lTaskNum;
        lTaskHeight = (lTaskHeight >> 2) << 2;

        Ani_Guided_Filter_ST pParam[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].startRow = lTaskHeight * lnum;
            pParam[lnum].endRow = lTaskHeight * (lnum + 1);
        }
        pParam[lTaskNum - 1].endRow = lHeight;


        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].task_ID = lnum;

            pParam[lnum].pSrc = pSrc;
            pParam[lnum].lPitchSrc = lPitchSrc;
            pParam[lnum].pGuide = pGuide;
            pParam[lnum].lPitchGuided = lGuidePitch;
            pParam[lnum].Coff_A = Coff_A;
            pParam[lnum].Coff_B = Coff_B;
            pParam[lnum].lWidth = lWidth;
            pParam[lnum].lHeight = lHeight;
            pParam[lnum].Feps = Feps;
        }


        /// 创建线程
        MInt32 lTaskID[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
            if (lTaskID[lnum] < 0)
            {
                lRet = MERR_BAD_STATE;
                goto exit;
            }
        }

        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
        }
    }
#else
    {
        lRet = GetAB(pSrc,
                     lPitchSrc,
                     pGuide,
                     lGuidePitch,
                     Coff_A,
                     Coff_B,
                     lWidth,
                     lHeight,
                     Feps, 0, lHeight);
    }
#endif

exit:

    END_TIME;
    return lRet;
}

#define USE_EDGE_POINT 0

MInt32 MeanAB(MUInt8** pA,
              MUInt8** pB,
              MUInt8* pMeanA,
              MUInt8* pMeanB,
              MInt32 lWidth,
              MInt32 lHeight,
              MInt32 startRow,
              MInt32 endRow)
{
    MInt32 lOffset = 255;

#if USE_EDGE_POINT
    MInt32 pWeiOffset[8] = { 0 };
    GetWeiOffset_Direction(pWeiOffset, lWidth, g_lRadius);
#else
    MInt32 AB_Offset_Src[4][4] = { MNull };
    Compute_AB_Offset_Direction(AB_Offset_Src, lWidth);
#endif

    MInt32 startRow2 = MAX(g_lRadius, startRow);
    MInt32 endRow2 = MIN(lHeight - g_lRadius, endRow);

    const int DETAIL_THRESHOLD = 64; //动态方向阈值

#ifdef USE_NEON
    uint16x4_t const1_16x4 = vdup_n_u16(1);
    uint32x4_t const128_32x4 = vdupq_n_u32(128);
    uint32x4_t const0_32x4 = vdupq_n_u32(0);
    uint32x4_t const1_32x4 = vdupq_n_u32(1);
    uint32x4_t const32_32x4 = vdupq_n_u32(32);
    uint16x4_t const32_16x4 = vdup_n_u16(32);
    float32x4_t fRoundValue = vdupq_n_f32(0.5);
#endif

    for (MInt32 y = startRow2; y < endRow2; y++)
    {
        MInt32 lShif_Num0 = y * lWidth;
        auto* pCurFusA = pMeanA + lShif_Num0;
        auto* pCurFusB = pMeanB + lShif_Num0;

        MInt32 x = g_lRadius;
#ifdef USE_NEON

        for (; x < lWidth - g_lRadius - 3; x += 4)
        {
            MInt32 lShif_Num = lShif_Num0 + x;

            uint32x4_t lSumA_32x4 = const0_32x4;
            uint32x4_t lSumB_32x4 = const0_32x4;
            uint32x4_t lSumWei_32x4 = const0_32x4;
            uint8x8_t lCof_A_8x8;
            uint8x8_t lCof_B_8x8;
            uint16x4_t lCof_A_16x4;
            uint16x4_t lCof_B_16x4;
            uint32x4_t lWei_32x4;

            for (MInt32 k = 0; k < g_lDirection; k++)
            {
#if USE_EDGE_POINT
                MInt32 weiOffset0 = pWeiOffset[(k << 1)];
                MInt32 weiOffset1 = pWeiOffset[(k << 1) + 1];

                // 中心点
                //lCof_A_8x8 = vld1_u8(pA[k] + lShif_Num);
                //lCof_B_8x8 = vld1_u8(pB[k] + lShif_Num);
                //lCof_A_16x4 = vget_low_u16(vmovl_u8(lCof_A_8x8));
                //lCof_B_16x4 = vget_low_u16(vmovl_u8(lCof_B_8x8));
//
                //lWei_32x4 = vsubw_u16(const128_32x4, lCof_A_16x4);
                //lWei_32x4 = vrshrq_n_u32(vmlaq_u32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                //lWei_32x4 = vaddq_u32(lWei_32x4, const1_32x4);
//
                //lSumWei_32x4 = vaddq_u32(lSumWei_32x4, lWei_32x4);
                //lSumA_32x4 = vmlaq_u32(lSumA_32x4, vmovl_u16(lCof_A_16x4), lWei_32x4);
                //lSumB_32x4 = vmlaq_u32(lSumB_32x4, vmovl_u16(lCof_B_16x4), lWei_32x4);

                // 右边的点
                lCof_A_8x8 = vld1_u8(pA[k] + lShif_Num + weiOffset0);
                lCof_B_8x8 = vld1_u8(pB[k] + lShif_Num + weiOffset0);
                lCof_A_16x4 = vget_low_u16(vmovl_u8(lCof_A_8x8));
                lCof_B_16x4 = vget_low_u16(vmovl_u8(lCof_B_8x8));

                lWei_32x4 = vsubw_u16(const128_32x4, lCof_A_16x4);
                lWei_32x4 = vrshrq_n_u32(vmlaq_u32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                lWei_32x4 = vaddq_u32(lWei_32x4, const1_32x4);

                lSumWei_32x4 = vaddq_u32(lSumWei_32x4, lWei_32x4);
                lSumA_32x4 = vmlaq_u32(lSumA_32x4, vmovl_u16(lCof_A_16x4), lWei_32x4);
                lSumB_32x4 = vmlaq_u32(lSumB_32x4, vmovl_u16(lCof_B_16x4), lWei_32x4);

                // 左边的点
                lCof_A_8x8 = vld1_u8(pA[k] + lShif_Num + weiOffset1);
                lCof_B_8x8 = vld1_u8(pB[k] + lShif_Num + weiOffset1);
                lCof_A_16x4 = vget_low_u16(vmovl_u8(lCof_A_8x8));
                lCof_B_16x4 = vget_low_u16(vmovl_u8(lCof_B_8x8));

                lWei_32x4 = vsubw_u16(const128_32x4, lCof_A_16x4);
                lWei_32x4 = vrshrq_n_u32(vmlaq_u32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                lWei_32x4 = vaddq_u32(lWei_32x4, const1_32x4);

                lSumWei_32x4 = vaddq_u32(lSumWei_32x4, lWei_32x4);
                lSumA_32x4 = vmlaq_u32(lSumA_32x4, vmovl_u16(lCof_A_16x4), lWei_32x4);
                lSumB_32x4 = vmlaq_u32(lSumB_32x4, vmovl_u16(lCof_B_16x4), lWei_32x4);
#else
                for (MInt32 j = 0; j < g_lRadius; j++)
                {
                    MInt32 weiOffset0 = AB_Offset_Src[k][j];
                    MInt32 weiOffset1 = -weiOffset0;

                    // 中心点
                    lCof_A_8x8 = vld1_u8(pA[k] + lShif_Num);
                    lCof_B_8x8 = vld1_u8(pB[k] + lShif_Num);
                    lCof_A_16x4 = vget_low_u16(vmovl_u8(lCof_A_8x8));
                    lCof_B_16x4 = vget_low_u16(vmovl_u8(lCof_B_8x8));
    
                    lWei_32x4 = vsubw_u16(const128_32x4, lCof_A_16x4);
                    lWei_32x4 = vrshrq_n_u32(vmlaq_u32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                    lWei_32x4 = vaddq_u32(lWei_32x4, const1_32x4);
    
                    lSumWei_32x4 = vaddq_u32(lSumWei_32x4, lWei_32x4);
                    lSumA_32x4 = vmlaq_u32(lSumA_32x4, vmovl_u16(lCof_A_16x4), lWei_32x4);
                    lSumB_32x4 = vmlaq_u32(lSumB_32x4, vmovl_u16(lCof_B_16x4), lWei_32x4);

                    // 右边的点
                    lCof_A_8x8 = vld1_u8(pA[k] + lShif_Num + weiOffset0);
                    lCof_B_8x8 = vld1_u8(pB[k] + lShif_Num + weiOffset0);
                    lCof_A_16x4 = vget_low_u16(vmovl_u8(lCof_A_8x8));
                    lCof_B_16x4 = vget_low_u16(vmovl_u8(lCof_B_8x8));

                    lWei_32x4 = vsubw_u16(const128_32x4, lCof_A_16x4);
                    lWei_32x4 = vrshrq_n_u32(vmlaq_u32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                    lWei_32x4 = vaddq_u32(lWei_32x4, const1_32x4);

                    lSumWei_32x4 = vaddq_u32(lSumWei_32x4, lWei_32x4);
                    lSumA_32x4 = vmlaq_u32(lSumA_32x4, vmovl_u16(lCof_A_16x4), lWei_32x4);
                    lSumB_32x4 = vmlaq_u32(lSumB_32x4, vmovl_u16(lCof_B_16x4), lWei_32x4);

                    // 左边的点
                    lCof_A_8x8 = vld1_u8(pA[k] + lShif_Num + weiOffset1);
                    lCof_B_8x8 = vld1_u8(pB[k] + lShif_Num + weiOffset1);
                    lCof_A_16x4 = vget_low_u16(vmovl_u8(lCof_A_8x8));
                    lCof_B_16x4 = vget_low_u16(vmovl_u8(lCof_B_8x8));

                    lWei_32x4 = vsubw_u16(const128_32x4, lCof_A_16x4);
                    lWei_32x4 = vrshrq_n_u32(vmlaq_u32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                    lWei_32x4 = vaddq_u32(lWei_32x4, const1_32x4);

                    lSumWei_32x4 = vaddq_u32(lSumWei_32x4, lWei_32x4);
                    lSumA_32x4 = vmlaq_u32(lSumA_32x4, vmovl_u16(lCof_A_16x4), lWei_32x4);
                    lSumB_32x4 = vmlaq_u32(lSumB_32x4, vmovl_u16(lCof_B_16x4), lWei_32x4);
                }
#endif
            }

            // 取权重的倒数
            float32x4_t fSumWei_32x4 = vcvtq_f32_u32(lSumWei_32x4);
            float32x4_t fInvSumWei_32x4 = vrecpeq_f32(fSumWei_32x4); // 倒数
            fInvSumWei_32x4 = vmulq_f32(vrecpsq_f32(fSumWei_32x4, fInvSumWei_32x4), fInvSumWei_32x4);
            //fInvSumWei_32x4 = vmulq_f32(vrecpsq_f32(fSumWei_32x4, fInvSumWei_32x4), fInvSumWei_32x4); // 精确化

            // 计算A
            uint32x4_t lMean_a_32x4 = vcvtq_u32_f32(vmlaq_f32(fRoundValue, vcvtq_f32_s32(lSumA_32x4), fInvSumWei_32x4));

            // 计算B
            uint32x4_t lMean_b_32x4 = vcvtq_u32_f32(vmlaq_f32(fRoundValue, vcvtq_f32_s32(lSumB_32x4), fInvSumWei_32x4));
            lCof_A_8x8 = vmovn_u16(vcombine_u16(vmovn_u32(lMean_a_32x4), vmovn_u32(lMean_a_32x4)));
            lCof_B_8x8 = vmovn_u16(vcombine_u16(vmovn_u32(lMean_b_32x4), vmovn_u32(lMean_b_32x4)));
            // 取值范围[0, 127]
            vst1_u8(pCurFusA + x, lCof_A_8x8);
            vst1_u8(pCurFusB + x, lCof_B_8x8);
        }

#endif // neon end
        for (; x < lWidth - g_lRadius; x++)
        {
            MInt32 lShif_Num = lShif_Num0 + x;
            MInt32 lSumA = 0;
            MInt32 lSumB = 0;
            MInt32 lSumWei = 0;
            MInt32 lCof_A;
            MInt32 lCof_B;
            MInt32 lWei;

            for (MInt32 k = 0; k < g_lDirection; k++)
            {
#if USE_EDGE_POINT
                MInt32 weiOffset0 = pWeiOffset[k * 2];
                MInt32 weiOffset1 = pWeiOffset[k * 2 + 1];

                //lCof_A = pA[k][lShif_Num]; // 加上中心点
                //lCof_B = pB[k][lShif_Num];
                //lWei = 128 - lCof_A;
                //lWei = (lWei + lWei * lWei + 8) >> 4;
                //lWei++;
                //lSumWei += lWei;
                //lSumA += lCof_A * lWei;
                //lSumB += lCof_B * lWei;

                lCof_A = pA[k][lShif_Num + weiOffset0];
                lCof_B = pB[k][lShif_Num + weiOffset0];
                lWei = 128 - lCof_A;
                lWei = (lWei + lWei * lWei + 8) >> 4;
                lWei++;
                lSumWei += lWei;
                lSumA += lCof_A * lWei;
                lSumB += lCof_B * lWei;

                lCof_A = pA[k][lShif_Num + weiOffset1];
                lCof_B = pB[k][lShif_Num + weiOffset1];
                lWei = 128 - lCof_A;
                lWei = (lWei + lWei * lWei + 8) >> 4;
                lWei++;
                lSumWei += lWei;
                lSumA += lCof_A * lWei;
                lSumB += lCof_B * lWei;

#else
                for (MInt32 j = 0; j < g_lRadius; j++)
                {
                    MInt32 weiOffset0 = AB_Offset_Src[k][j];
                    MInt32 weiOffset1 = -weiOffset0;

                    lCof_A = pA[k][lShif_Num]; // 加上中心点
                    lCof_B = pB[k][lShif_Num];
                    lWei = 128 - lCof_A;
                    lWei = (lWei + lWei * lWei + 8) >> 4;
                    lWei++;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;

                    lCof_A = pA[k][lShif_Num + weiOffset0];
                    lCof_B = pB[k][lShif_Num + weiOffset0];
                    lWei = 128 - lCof_A;
                    lWei = (lWei + lWei * lWei + 8) >> 4;
                    lWei++;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;

                    lCof_A = pA[k][lShif_Num + weiOffset1];
                    lCof_B = pB[k][lShif_Num + weiOffset1];
                    lWei = 128 - lCof_A;
                    lWei = (lWei + lWei * lWei + 8) >> 4;
                    lWei++;
                    lSumWei += lWei;
                    lSumA += lCof_A * lWei;
                    lSumB += lCof_B * lWei;
                }
#endif
            }

            // 取值范围[0, 127]
            // fMean_a 越大 原来像素的值占比越大
            MInt32 fMean_a = 128;
            MInt32 fMean_b = lOffset;

            fMean_a = (lSumA + (lSumWei >> 1)) / (lSumWei); //lSumA * 1.0 / lSumWei + 0.5;//
            fMean_b = (lSumB + (lSumWei >> 1)) / (lSumWei); //lSumB * 1.0 / lSumWei + 0.5;//

            pCurFusA[x] = fMean_a;
            pCurFusB[x] = fMean_b > 255 ? 255 : fMean_b;
        }
    }

    return MOK;
}

MInt32 MeanABThreads(MHandle mcvParallelMonitor, 
    MUInt8** pA, MUInt8** pB, 
    MUInt8* pMeanA, MUInt8* pMeanB, 
    MInt32 lWidth, MInt32 lHeight)
{
    START_TIME;
    MInt32 res = MOK;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    {
        /// 设置回调函数
        auto func_lamda = [](MVoid* HParam) -> MVoid
        {
            LAni_Guided_Filter_ST filter_sturct = (LAni_Guided_Filter_ST)HParam;

            MeanAB(filter_sturct->Coff_A, filter_sturct->Coff_B, 
                filter_sturct->pMeanA, filter_sturct->pMeanB, 
                filter_sturct->lWidth, filter_sturct->lHeight,
                filter_sturct->startRow, filter_sturct->endRow);
        };
        MVoid(*func)(MVoid*) = func_lamda;

        /// 设置参数
        MInt32 lTaskNum = lHeight > 1024 ? 16 : 8;
        lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;
        MInt32 lTaskHeight = lHeight / lTaskNum;
        lTaskHeight = (lTaskHeight >> 2) << 2;

        Ani_Guided_Filter_ST pParam[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].startRow = lTaskHeight * lnum;
            pParam[lnum].endRow = lTaskHeight * (lnum + 1);
        }
        pParam[lTaskNum - 1].endRow = lHeight;


        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].task_ID = lnum;

            pParam[lnum].Coff_A = pA;
            pParam[lnum].Coff_B = pB;
            pParam[lnum].pMeanA = pMeanA;
            pParam[lnum].pMeanB = pMeanB;
            pParam[lnum].lWidth = lWidth;
            pParam[lnum].lHeight = lHeight;
        }


        /// 创建线程
        MInt32 lTaskID[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
            if (lTaskID[lnum] < 0)
            {
                res = MERR_BAD_STATE;
                goto exit;
            }
        }

        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
        }
    }
#else
    {
        res = MeanAB(pA, pB, pMeanA, pMeanB, lWidth, lHeight, 0, lHeight);
    }
#endif

exit:
    END_TIME;
    return res;
}

MInt32 MeanA(MUInt8** pA,
    MUInt8* pMeanA,
    MInt32 lWidth,
    MInt32 lHeight,
    MInt32 startRow,
    MInt32 endRow)
{
    MInt32 lOffset = 255;

    MInt32 lDirection = 4;
    MInt32 nRadius = 4; // 半径
    MInt32 pWeiOffset[8] = { 0 };

    GetWeiOffset_Direction(pWeiOffset, lWidth, nRadius);

    MInt32 startRow2 = MAX(nRadius, startRow);
    MInt32 endRow2 = MIN(lHeight - nRadius, endRow);

    const int DETAIL_THRESHOLD = 64; //动态方向阈值

#ifdef USE_NEON
    uint16x4_t const1_16x4 = vdup_n_u16(1);
    uint32x4_t const128_32x4 = vdupq_n_u32(128);
    uint32x4_t const0_32x4 = vdupq_n_u32(0);
    uint32x4_t const1_32x4 = vdupq_n_u32(1);
    uint32x4_t const32_32x4 = vdupq_n_u32(32);
    uint16x4_t const32_16x4 = vdup_n_u16(32);
    float32x4_t fRoundValue = vdupq_n_f32(0.5);
#endif

    for (MInt32 y = startRow2; y < endRow2; y++)
    {
        MInt32 lShif_Num0 = y * lWidth;
        auto* pCurFusA = pMeanA + lShif_Num0;

        MInt32 x = nRadius;
#ifdef USE_NEON
        // 输入是MInt16
        for (; x < lWidth - nRadius - 3; x += 4)
        {
            MInt32 lShif_Num = lShif_Num0 + x;

            uint32x4_t lSumA_32x4 = const0_32x4;
            uint32x4_t lSumWei_32x4 = const0_32x4;
            uint8x8_t lCof_A_8x8;
            uint16x4_t lCof_A_16x4;
            uint32x4_t lWei_32x4;

            for (MInt32 k = 0; k < lDirection; k++)
            {
                MInt32 weiOffset0 = pWeiOffset[(k << 1)];
                MInt32 weiOffset1 = pWeiOffset[(k << 1) + 1];

                // 中心点
                lCof_A_8x8 = vld1_u8(pA[k] + lShif_Num);
                lCof_A_16x4 = vget_low_u16(vmovl_u8(lCof_A_8x8));

                lWei_32x4 = vsubw_u16(const128_32x4, lCof_A_16x4);
                lWei_32x4 = vrshrq_n_u32(vmlaq_u32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                lWei_32x4 = vaddq_u32(lWei_32x4, const1_32x4);

                lSumWei_32x4 = vaddq_u32(lSumWei_32x4, lWei_32x4);
                lSumA_32x4 = vmlaq_u32(lSumA_32x4, vmovl_u16(lCof_A_16x4), lWei_32x4);

                // 右边的点
                lCof_A_8x8 = vld1_u8(pA[k] + lShif_Num + weiOffset0);
                lCof_A_16x4 = vget_low_u16(vmovl_u8(lCof_A_8x8));

                lWei_32x4 = vsubw_u16(const128_32x4, lCof_A_16x4);
                lWei_32x4 = vrshrq_n_u32(vmlaq_u32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                lWei_32x4 = vaddq_u32(lWei_32x4, const1_32x4);

                lSumWei_32x4 = vaddq_u32(lSumWei_32x4, lWei_32x4);
                lSumA_32x4 = vmlaq_u32(lSumA_32x4, vmovl_u16(lCof_A_16x4), lWei_32x4);

                // 左边的点
                lCof_A_8x8 = vld1_u8(pA[k] + lShif_Num + weiOffset1);
                lCof_A_16x4 = vget_low_u16(vmovl_u8(lCof_A_8x8));

                lWei_32x4 = vsubw_u16(const128_32x4, lCof_A_16x4);
                lWei_32x4 = vrshrq_n_u32(vmlaq_u32(lWei_32x4, lWei_32x4, lWei_32x4), 4);
                lWei_32x4 = vaddq_u32(lWei_32x4, const1_32x4);

                lSumWei_32x4 = vaddq_u32(lSumWei_32x4, lWei_32x4);
                lSumA_32x4 = vmlaq_u32(lSumA_32x4, vmovl_u16(lCof_A_16x4), lWei_32x4);
            }

            // 取权重的倒数
            float32x4_t fSumWei_32x4 = vcvtq_f32_u32(lSumWei_32x4);
            float32x4_t fInvSumWei_32x4 = vrecpeq_f32(fSumWei_32x4); // 倒数
            fInvSumWei_32x4 = vmulq_f32(vrecpsq_f32(fSumWei_32x4, fInvSumWei_32x4), fInvSumWei_32x4);
            //fInvSumWei_32x4 = vmulq_f32(vrecpsq_f32(fSumWei_32x4, fInvSumWei_32x4), fInvSumWei_32x4); // 精确化

            // 计算A
            uint32x4_t lMean_a_32x4 = vcvtq_u32_f32(vmlaq_f32(fRoundValue, vcvtq_f32_s32(lSumA_32x4), fInvSumWei_32x4));

            // 取值范围[0, 127]
            vst1_u8(pCurFusA + x, lCof_A_8x8);
        }

#endif // neon end
        for (; x < lWidth - nRadius; x++)
        {
            MInt32 lShif_Num = lShif_Num0 + x;
            MInt32 lSumA = 0;
            MInt32 lSumWei = 0;

            for (MInt32 k = 0; k < lDirection; k++)
            {
                MInt32 weiOffset0 = pWeiOffset[k * 2];
                MInt32 weiOffset1 = pWeiOffset[k * 2 + 1];

                auto lCof_A = pA[k][lShif_Num + weiOffset0];
                MInt32 lWei = 128 - lCof_A;
                lWei = (lWei + lWei * lWei + 8) >> 4;

                lWei++;
                lSumWei += lWei;
                lSumA += lCof_A * lWei;

                lCof_A = pA[k][lShif_Num + weiOffset1];
                lWei = 128 - lCof_A;
                lWei = (lWei + lWei * lWei + 8) >> 4;

                lWei++;
                lSumWei += lWei;
                lSumA += lCof_A * lWei;

                lCof_A = pA[k][lShif_Num]; // 加上中心点
                lWei = 128 - lCof_A;
                lWei = (lWei + lWei * lWei + 8) >> 4;

                lWei++;
                lSumWei += lWei;
                lSumA += lCof_A * lWei;
            }

            // 取值范围[0, 127]
            // fMean_a 越大 原来像素的值占比越大
            MInt32 fMean_a = 128;

            fMean_a = (lSumA + (lSumWei >> 1)) / (lSumWei); //lSumA * 1.0 / lSumWei + 0.5;//

            pCurFusA[x] = fMean_a;
        }
    }

    return MOK;
}

MInt32 MeanAThreads(MHandle mcvParallelMonitor,
    MUInt8** pA, 
    MUInt8* pMeanA,
    MInt32 lWidth, MInt32 lHeight)
{
    START_TIME;
    MInt32 res = MOK;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    {
        /// 设置回调函数
        auto func_lamda = [](MVoid* HParam) -> MVoid
        {
            LAni_Guided_Filter_ST filter_sturct = (LAni_Guided_Filter_ST)HParam;

            MeanA(filter_sturct->Coff_A, 
                filter_sturct->pMeanA, 
                filter_sturct->lWidth, filter_sturct->lHeight,
                filter_sturct->startRow, filter_sturct->endRow);
        };
        MVoid(*func)(MVoid*) = func_lamda;

        /// 设置参数
        MInt32 lTaskNum = lHeight > 1024 ? 16 : 8;
        lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;
        MInt32 lTaskHeight = lHeight / lTaskNum;
        lTaskHeight = (lTaskHeight >> 2) << 2;

        Ani_Guided_Filter_ST pParam[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].startRow = lTaskHeight * lnum;
            pParam[lnum].endRow = lTaskHeight * (lnum + 1);
        }
        pParam[lTaskNum - 1].endRow = lHeight;


        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].task_ID = lnum;

            pParam[lnum].Coff_A = pA;
            pParam[lnum].pMeanA = pMeanA;
            pParam[lnum].lWidth = lWidth;
            pParam[lnum].lHeight = lHeight;
        }


        /// 创建线程
        MInt32 lTaskID[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
            if (lTaskID[lnum] < 0)
            {
                res = MERR_BAD_STATE;
                goto exit;
            }
        }

        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
        }
    }
#else
    {
        res = MeanA(pA, pMeanA, lWidth, lHeight, 0, lHeight);
    }
#endif

exit:
    END_TIME;
    return res;
}


MInt32 GetMeanAB(MHandle hMemMgr,
    MHandle mcvParallelMonitor,
    MUInt8* pSrc,
    MInt32 lPitch,
    MUInt8* pGuide,
    MInt32 lGuidePitch,
    MInt32 lWidth,
    MInt32 lHeight,
    MUInt8* pMeanA,
    MUInt8* pMeanB,
    MFloat fEps)
{
    START_TIME;
    MInt32 lRet = MOK;

    // 申请存放 ab 值的内存
    MUInt8* pA[4] = { MNull };
    MUInt8* pB[4] = { MNull };
    MInt32 lSumCount = lWidth * lHeight;
    auto* pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lSumCount * 8);
    if (pMemory == MNull)
    {
        lRet = MERR_NO_MEMORY;
        goto exit;
    }
    MMemSet(pMemory, 0, lSumCount * 8);
    for (MInt32 k = 0; k < g_lDirection; k++)
    {
        pA[k] = pMemory + lSumCount * (2 * k);//SAFE_MALLOC(hMemMgr, MUInt8, lWidth * lHeight);
        pB[k] = pMemory + lSumCount * (2 * k + 1);//SAFE_MALLOC(hMemMgr, MUInt8, lWidth * lHeight);
    }

    // 获取4个方向的 ab 值
    lRet = GetABThreads(mcvParallelMonitor, pSrc, lPitch, pGuide, lGuidePitch, pA, pB, lWidth, lHeight, fEps);

    // 取ab均值
    lRet = MeanABThreads(mcvParallelMonitor, pA, pB, pMeanA, pMeanB, lWidth, lHeight);


#ifdef BUILD_OPENCV
    mat_write255(lHeight, lPitch, CV_8UC1, pSrc, "pSrc.png", 1.0);
    mat_write255(lHeight, lPitch, CV_8UC1, pGuide, "pGuide.png", 1.0);
    mat_write255(lHeight, lWidth, CV_8UC1, pMeanA, "MeanA.png", 1.0);
    mat_write255(lHeight, lWidth, CV_8UC1, pMeanB, "MeanB.png", 1.0);

    //Cross_Median_Filter_3x3(hMemMgr, mcvParallelMonitor, pMeanA, lWidth,
    //    pA[0], lWidth, lWidth, lHeight, 1);
    //memcpy(pMeanA, pA[0], lWidth*lHeight);
    for (int i = 0; i < g_lDirection; i++)
    {
        char dstFilename[100] = "";
        sprintf(dstFilename, "pA4[%d].png", i);
        mat_write255(lHeight, lWidth, CV_8UC1, pA[i], dstFilename, 1.0);
        sprintf(dstFilename, "pB4[%d].png", i);
        mat_write255(lHeight, lWidth, CV_8UC1, pB[i], dstFilename, 1.0);
    }
#endif

exit:
    SAFE_FREE_ARRAY(hMemMgr, pMemory);

    return lRet;
}


MInt32 Mul_A_Plus_B(MUInt8* pSrc,
    MInt32 lSrcPitch,
    MUInt8* pDst,
    MInt32 lDstPitch,
    MInt32 lWidth,
    MInt32 lHeight,
    MUInt8* pMeanA,
    MUInt8* pMeanB,
    MInt32 lMeanPitch,
    MInt32 startRow,
    MInt32 endRow)
{
#ifdef USE_NEON
    uint8x8_t const128_8x8 = vdup_n_u8(128);
#endif

    for (MInt32 y = startRow; y < endRow; y++)
    {
        auto* pCurSrc = pSrc + y * lSrcPitch;
        auto* pCurDst = pDst + y * lDstPitch;
        auto* pA = pMeanA + y * lMeanPitch;
        auto* pB = pMeanB + y * lMeanPitch;

        MInt32 x = 0;
#ifdef USE_NEON
        for (; x < lWidth - 7; x += 8)
        {
            uint8x8_t  val_8x8 = vld1_u8((MUInt8*)pCurSrc + x);
            uint8x8_t  A_8x8 = vld1_u8((MUInt8*)pA + x);
            uint8x8_t  B_8x8 = vld1_u8((MUInt8*)pB + x);

            A_8x8 = vmin_u8(A_8x8, const128_8x8);
            val_8x8 = vqrshrn_n_u16(vaddq_s16(vmull_u8(val_8x8, A_8x8), vmull_u8(B_8x8, vsub_u8(const128_8x8, A_8x8))), 7);

            vst1_u8((MUInt8*)pCurDst + x, val_8x8);
        }
#endif
        for (; x < lWidth; x++)
        {
            MInt32 val = pCurSrc[x];
            MInt32 tempA = MIN(128, pA[x]);
            val = (tempA * val + (128 - tempA) * pB[x] + 64) >> 7;
            val = val > 255 ? 255 : val;
            pCurDst[x] = val;
        }
    }

    return MOK;
}

MInt32 Mul_A_Plus_B_Threads(MHandle mcvParallelMonitor, 
    MUInt8* pSrc,
    MInt32 lSrcPitch,
    MUInt8* pDst,
    MInt32 lDstPitch,
    MInt32 lWidth,
    MInt32 lHeight,
    MUInt8* pMeanA,
    MUInt8* pMeanB,
    MInt32 lMeanPitch)
{
    MInt32 res = MOK;

#if defined(QUAD_MULTI_THREAD) || defined(MULTI_THREAD)
    {
        /// 设置回调函数
        auto func_lamda = [](MVoid* HParam) -> MVoid
        {
            LAni_Guided_Filter_ST filter_sturct = (LAni_Guided_Filter_ST)HParam;

            Mul_A_Plus_B(filter_sturct->pSrc,
                filter_sturct->lPitchSrc,
                filter_sturct->pGuide,
                filter_sturct->lPitchGuided,
                filter_sturct->lWidth,
                filter_sturct->lHeight,
                filter_sturct->pMeanA,
                filter_sturct->pMeanB,
                filter_sturct->lMeanPitch,
                filter_sturct->startRow, 
                filter_sturct->endRow);
        };
        MVoid(*func)(MVoid*) = func_lamda;



        /// 设置参数
        MInt32 lTaskNum = lHeight > 1024 ? 16 : 8;
        lTaskNum = lTaskNum > 16 ? 16 : lTaskNum;
        MInt32 lTaskHeight = lHeight / lTaskNum;
        lTaskHeight = (lTaskHeight >> 2) << 2;

        Ani_Guided_Filter_ST pParam[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].startRow = lTaskHeight * lnum;
            pParam[lnum].endRow = lTaskHeight * (lnum + 1);
        }
        pParam[lTaskNum - 1].endRow = lHeight;


        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            pParam[lnum].task_ID = lnum;

            pParam[lnum].pSrc = pSrc;
            pParam[lnum].pGuide = pDst;
            pParam[lnum].pMeanA = pMeanA;
            pParam[lnum].pMeanB = pMeanB;
            pParam[lnum].lWidth = lWidth;
            pParam[lnum].lHeight = lHeight;

            pParam[lnum].lPitchSrc = lSrcPitch;
            pParam[lnum].lPitchGuided = lDstPitch;
            pParam[lnum].lMeanPitch = lMeanPitch;
        }


        /// 创建线程
        MInt32 lTaskID[16] = { MNull };
        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            lTaskID[lnum] = mcvAddTask(mcvParallelMonitor, func, (MVoid*)&pParam[lnum]);
            if (lTaskID[lnum] < 0)
            {
                res = MERR_BAD_STATE;
                goto exit;
            }
        }

        for (MInt32 lnum = 0; lnum < lTaskNum; lnum++)
        {
            mcvWaitTask(mcvParallelMonitor, lTaskID[lnum]);
        }
    }
#else
    {
        res = Mul_A_Plus_B(pSrc,
            lSrcPitch,
            pDst,
            lDstPitch,
            pMeanA,
            pMeanB,
            lWidth,
            lHeight,
            0, lHeight);
    }
#endif

exit:

    return res;
}

MInt32 GetMeanB(MHandle hMemMgr,
    MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrcImgPad,
    LPASVLOFFSCREEN pGuideImgPad,
    LPASVLOFFSCREEN pMeanBImg,
    MFloat fEps)
{
    START_TIME;
    MInt32 lRet = MOK;

    MInt32 lWidth = pSrcImgPad->i32Width;
    MInt32 lHeight = pSrcImgPad->i32Height;

    // 申请存放 ab 值的内存
    MUInt8* pA[4] = { MNull };
    MUInt8* pB[4] = { MNull };
    MInt32 lSumCount = lWidth * lHeight;
    auto* pMemory = SAFE_MALLOC(hMemMgr, MUInt8, lSumCount * 8);
    if (pMemory == MNull)
    {
        lRet = MERR_NO_MEMORY;
        goto exit;
    }
    for (MInt32 k = 0; k < g_lDirection; k++)
    {
        pA[k] = pMemory + lSumCount * (2 * k);//SAFE_MALLOC(hMemMgr, MUInt8, lWidth * lHeight);
        pB[k] = pMemory + lSumCount * (2 * k + 1);//SAFE_MALLOC(hMemMgr, MUInt8, lWidth * lHeight);
    }

    // 获取4个方向的 ab 值
    lRet = GetAThreads(mcvParallelMonitor, pGuideImgPad->ppu8Plane[0], pGuideImgPad->pi32Pitch[0], pA, lWidth, lHeight, fEps);
    lRet = GetBThreads(mcvParallelMonitor, pSrcImgPad->ppu8Plane[0], pSrcImgPad->pi32Pitch[0], pA, pB, pMeanBImg->ppu8Plane[0], lWidth, lHeight, fEps);


#ifdef BUILD_OPENCV
    mat_write255(lHeight, pSrcImgPad->pi32Pitch[0], CV_8UC1, pSrcImgPad->ppu8Plane[0], "pSrc.png", 1.0);
    mat_write255(lHeight, lWidth, CV_8UC1, pMeanBImg->ppu8Plane[0], "MeanB.png", 1.0);

    for (int i = 0; i < g_lDirection; i++)
    {
        char dstFilename[100] = "";
        sprintf(dstFilename, "pA4[%d].png", i);
        mat_write255(lHeight, lWidth, CV_8UC1, pA[i], dstFilename, 1.0);
        //Cross_Median_Filter_3x3(hMemMgr, mcvParallelMonitor, pA[i], lWidth,
        //    pB[i], lWidth, lWidth, lHeight, 1);
        sprintf(dstFilename, "pB4[%d].png", i);
        mat_write255(lHeight, lWidth, CV_8UC1, pB[i], dstFilename, 1.0);

    }
#endif

exit:
    SAFE_FREE_ARRAY(hMemMgr, pMemory);

    END_TIME;
    return lRet;
}

MInt32 GetMeanAB(MHandle hMemMgr,
    MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrcImgPad,
    LPASVLOFFSCREEN pGuideImgPad,
    LPASVLOFFSCREEN pMeanAImg,
    LPASVLOFFSCREEN pMeanBImg,
    MFloat fEps)
{
    START_TIME;
    MInt32 lRet = MOK;

    lRet = GetMeanAB(hMemMgr, 
        mcvParallelMonitor,
        pSrcImgPad->ppu8Plane[0],
        pSrcImgPad->pi32Pitch[0],
        pGuideImgPad->ppu8Plane[0],
        pGuideImgPad->pi32Pitch[0],
        pSrcImgPad->i32Width,
        pSrcImgPad->i32Height,
        pMeanAImg->ppu8Plane[0],
        pMeanBImg->ppu8Plane[0],
        fEps);

    END_TIME;
    return lRet;
}

MInt32 Mul_A_Plus_B(MHandle mcvParallelMonitor,
    LPASVLOFFSCREEN pSrcImg,
    LPASVLOFFSCREEN pDstImg,
    LPASVLOFFSCREEN pMeanAImg,
    LPASVLOFFSCREEN pMeanBImg)
{
    START_TIME;
    MInt32 lRet = MOK;

    lRet = Mul_A_Plus_B_Threads(mcvParallelMonitor,
        pSrcImg->ppu8Plane[0],
        pSrcImg->pi32Pitch[0],
        pDstImg->ppu8Plane[0],
        pDstImg->pi32Pitch[0],
        pDstImg->i32Width,
        pDstImg->i32Height,
        pMeanAImg->ppu8Plane[0],
        pMeanBImg->ppu8Plane[0], 
        pMeanBImg->pi32Pitch[0]);

    END_TIME;
    return lRet;
}

