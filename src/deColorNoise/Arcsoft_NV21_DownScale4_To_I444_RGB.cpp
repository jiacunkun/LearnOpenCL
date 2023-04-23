#include "mobilecv.h"
#include "DefineForDebug.h"
#include "Arcsoft_NV21_DownScale4_To_I444_RGB.h"


#if defined(USE_NEON) || defined(__ARM_NEON__)
#if defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif

// =============== Color format transfer =============== //
#define yuv_shift			14
#define yuv_descale(x)		(((x) + (1 << ((yuv_shift)-1))) >> (yuv_shift))
#define yuv_prescale(x)		((x) << yuv_shift)

#define yuvRCr	22987		//yuv_fix(1.403f)
#define yuvGCr	-11698		//(-yuv_fix(0.714f))
#define yuvGCb	-5636		//(-yuv_fix(0.344f))
#define yuvBCb	29049		//yuv_fix(1.773f)

#define ET_CAST_8U(t)       (MByte)( (t) < 0 ? 0  : ( (t) > 255 ? 255 : (t) ) )

#define ET_YUV_TO_R(y,v)	ET_CAST_8U(yuv_descale((y) + (v)))
#define ET_YUV_TO_G(y,u,v)	ET_CAST_8U(yuv_descale((y) + (v) + (u)))
#define ET_YUV_TO_B(y,u)	ET_CAST_8U(yuv_descale((y) + (u)))

#define ET_YUV_TO_R_2(y,v)	    (MByte)(ET_CAST_8U(yuv_descale((y) + yuvRCr * (v))))
#define ET_YUV_TO_G_2(y,u,v)	(MByte)(ET_CAST_8U(yuv_descale((y) + yuvGCr * (v) + yuvGCb * (u))))
#define ET_YUV_TO_B_2(y,u)	    (MByte)(ET_CAST_8U(yuv_descale((y) + yuvBCb * (u))))

MInt32 GaussPyrDown4(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte *dstdata, MInt32 srcW, MInt32 srcH,
                     MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch);
MInt32 GaussPyrDown2_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte *dstdata, MInt32 srcW, MInt32 srcH,
                        MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch);
MRESULT convYUVToBGR(MHandle mcvParallelMonitor, MByte *pYdata, MByte *pUVdata, MInt32 lYPitch, MInt32 lUVPitch, LPASVLOFFSCREEN pDstBGR);

MInt32 GaussPyrDown4And2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte* srcUVdata, MByte *dstdata, MByte *rgbdata, MInt32 srcW, MInt32 srcH,
                         MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch, MInt32 rgbPitch);

MInt32 GaussPyrDown4And2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte* srcUVdata, MByte *dstdata, MInt32 srcW, MInt32 srcH,
                         MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch);
/**
 * @brief
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param pSrcImg
 * @param srcU
 * @param srcV
 * @param guideRGB
 * @return
 */
MInt32 Arcsoft_NV21_DownScale4_To_I444_RGB(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg,
                                           ASVLOFFSCREEN& YUV444, ASVLOFFSCREEN& guideRGB)
{
    START_TIME;
    MInt32 lret = 0;

    MInt32 lHeight, lWidth, lYPitch, lUVPitch;
    MInt32 smH, smW, smYPitch, smUVPitch;
    MByte *smoothY = MNull, *smoothUV = MNull, *resUV = MNull;
    lHeight = pSrcImg->i32Height;
    lWidth = pSrcImg->i32Width;
    lYPitch = pSrcImg->pi32Pitch[0];
    lUVPitch = pSrcImg->pi32Pitch[1];
    smH = YUV444.i32Height;
    smW = YUV444.pi32Pitch[0];
    smYPitch = YUV444.pi32Pitch[0];
    smUVPitch = YUV444.pi32Pitch[1];


    lret = GaussPyrDown4And2( hMemMgr, mcvParallelMonitor, pSrcImg->ppu8Plane[0], pSrcImg->ppu8Plane[1],
                              YUV444.ppu8Plane[0], guideRGB.ppu8Plane[0],  lWidth, lHeight,
                              lYPitch, smW, smH,  smYPitch, guideRGB.pi32Pitch[0]);

    mat_write255(smH, smW, CV_8UC1, YUV444.ppu8Plane[0], "Y.jpg", 1.0);
    mat_write255(smH, smW, CV_8UC1, YUV444.ppu8Plane[1], "U.jpg", 1.0);
    mat_write255(smH, smW, CV_8UC1, YUV444.ppu8Plane[2], "V.jpg", 1.0);
    mat_write255(smH, smW, CV_8UC3, guideRGB.ppu8Plane[0], "rgb.jpg", 1.0);

    END_TIME;
    return lret;
}

MInt32 Arcsoft_NV21_DownScale4_To_I444(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg,
                                           ASVLOFFSCREEN& YUV444)
{
    START_TIME;
    MInt32 lret = 0;

    MInt32 lHeight, lWidth, lYPitch, lUVPitch;
    MInt32 smH, smW, smYPitch, smUVPitch;
    MByte *smoothY = MNull, *smoothUV = MNull, *resUV = MNull;
    lHeight = pSrcImg->i32Height;
    lWidth = pSrcImg->i32Width;
    lYPitch = pSrcImg->pi32Pitch[0];
    lUVPitch = pSrcImg->pi32Pitch[1];
    smH = YUV444.i32Height;
    smW = YUV444.pi32Pitch[0];
    smYPitch = YUV444.pi32Pitch[0];
    smUVPitch = YUV444.pi32Pitch[1];


    lret = GaussPyrDown4And2( hMemMgr, mcvParallelMonitor, pSrcImg->ppu8Plane[0], pSrcImg->ppu8Plane[1],
                              YUV444.ppu8Plane[0], lWidth, lHeight,
                              lYPitch, smW, smH, smYPitch);

    mat_write255(smH, smW, CV_8UC1, YUV444.ppu8Plane[0], "Y.jpg", 1.0);
    mat_write255(smH, smW, CV_8UC1, YUV444.ppu8Plane[1], "U.jpg", 1.0);
    mat_write255(smH, smW, CV_8UC1, YUV444.ppu8Plane[2], "V.jpg", 1.0);
    //mat_write255(smH, smW, CV_8UC3, guideRGB.ppu8Plane[0], "rgb.jpg", 1.0);

    END_TIME;
    return lret;
}
/*******************************************合并四倍Y和两倍UV下采样********************************************/
typedef struct _tag_GaussPyrDown4And2
{
    MInt32 taskID;
    MInt32 start;
    MInt32 end;
    MByte *srcdata;
    MByte* srcUVdata;
    MByte *dstdata;
    MByte *rgbdata;
    MWord *rowBuf;
    MWord *rowUVBuf;
    MInt32 srcW;
    MInt32 srcH;
    MInt32 srcPitch;
    MInt32 dstW;
    MInt32 dstH;
    MInt32 dstPitch;
    MInt32 rgbPitch;
}GaussPyrDownData4And2;

/******************************************合并四倍Y和两倍UV下采样*****************************************************/
static MVoid LocalPyrDown4And2(MByte* srcPtr, MByte *srcUVPtr, MByte *dstPtr, MByte *rgbPtr, MWord *rowBuf, MWord *rowUVBuf, MInt32 sw, MInt32 sh, MInt32 src_step,
                           MInt32 dw, MInt32 dh, MInt32 dst_step, MInt32 rgb_step, MInt32 start, MInt32 end)
{
    MWord* buf = rowBuf + 2;
    MInt32 k, x, y;
    MInt32 sy0 = -(5 >> 1);
    MInt32 sy = sy0;
    MByte *src00, *src01, *src02, *src03, *src04;

    MWord* uvbuf = rowUVBuf + 2;
    MInt32 suvy;
    MByte *uvsrc00, *uvsrc01, *uvsrc02;

    MInt32 offset = 0;
    MUInt8 *pBGRRow = MNull, *pBGRNextRow = MNull;
    MInt32 cr = 0, cb = 0;
    MInt32 y00 = 0, y01 = 0;
    MInt32 y10 = 0, y11 = 0;
#ifdef __ARM_NEON__
    uint8x8_t vConst6_8x8;
	uint16x8_t vConst4, vConst6_16x8;
	vConst4 = vdupq_n_u16(4);
	vConst6_8x8 = vdup_n_u8(6);
	vConst6_16x8 = vdupq_n_u16(6);

	//uv
	uint8x8_t vConst2_8x8;
	uint16x8_t vConst2_16x8;
	uint8x8x2_t resdata;
	uint16x8x4_t src00_16x8x4;
	uint16x8x4_t src01_16x8x4;
	uint16x8_t uvsum00_16x8;
	vConst2_8x8 = vdup_n_u8(2);
	vConst2_16x8 = vdupq_n_u16(2);

	//rgb
    uint8x8_t ydata_8x8;
    uint8x8x2_t uvdata_8x8x2;
    uint8x8x3_t rgbresdata;
    int16x8_t tmpdata, udata, vdata, combdata;
    int32x4_t ylowdata_32x4, yhighdata_32x4, lowsumdata, highsumdata;
    int16x4_t crdata_16x4, cbdata_16x4, lowdata, highdata;
    int16x4x2_t zip_cr, zip_cb;
    int16x8_t combine_cr, combine_cb;
    int32x4_t Gcrlowdata, Gcrhighdata, Rcrlowdata, Rcrhighdata;
    int32x4_t Bcblowdata, Bcbhighdata, Gcblowdata, Gcbhighdata;
#endif
    MByte *uvsrcPtr = srcUVPtr;
    //下采样
    for (y = start; y < end; y++)
    {
        MByte *tmpdst = (MByte *) (dstPtr + dst_step * y);
        MByte *tmpuvdst = (MByte *) (dstPtr + dh*dw + dst_step * y); // uv基于Y的偏移
        pBGRRow = rgbPtr + y * rgb_step;

        sy = 4 * y;
        if (0 == y)
        {
            src00 = srcPtr;
            src01 = srcPtr;
        }
        else
        {
            //如下写法在某几张测试图上访问第0行数据会crash，具体原因不详
            src00 = ((sy - 2) >= 0) ? (srcPtr + src_step * (sy - 2)) : (srcPtr + src_step * (2 - sy));
            src01 = ((sy - 1) >= 0) ? (srcPtr + src_step * (sy - 1)) : (srcPtr + src_step * (1 - sy));
        }
        src02 = srcPtr + src_step * sy;
        src03 = (sy + 1) <= (sh - 1) ? (srcPtr + src_step * (sy + 1)) : (srcPtr + src_step * (2 * sh - 2 - sy - 1));
        src04 = (sy + 2) <= (sh - 1) ? (srcPtr + src_step * (sy + 2)) : (srcPtr + src_step * (2 * sh - 2 - sy - 2));

        sy = y * 2;
        uvsrc00 = (sy - 1 >= 0) ? uvsrcPtr + src_step * (sy - 1) : uvsrcPtr;
        uvsrc01 = uvsrcPtr + src_step * sy;
        uvsrc02 = (sy + 1 <= sh/2 - 1) ? uvsrcPtr + src_step * (sy + 1) : uvsrcPtr + src_step * (sh/2 - 2);


        x = 0;
#ifdef __ARM_NEON__
        for (; x < sw - 15; x += 16)
        {
            uint8x16_t s00_8x16, s01_8x16, s02_8x16, s03_8x16, s04_8x16;
            uint16x8_t sum00_16x8, sum01_16x8;
            uint16x8_t tmpsum00, tmpsum01;
            s00_8x16 = vld1q_u8(src00 + x);
            s01_8x16 = vld1q_u8(src01 + x);
            s02_8x16 = vld1q_u8(src02 + x);
            s03_8x16 = vld1q_u8(src03 + x);
            s04_8x16 = vld1q_u8(src04 + x);
            tmpsum00 = vaddl_u8(vget_low_u8(s01_8x16), vget_low_u8(s03_8x16));
            tmpsum01 = vaddl_u8(vget_high_u8(s01_8x16), vget_high_u8(s03_8x16));
            sum00_16x8 = vmull_u8(vget_low_u8(s02_8x16), vConst6_8x8);
            sum01_16x8 = vmull_u8(vget_high_u8(s02_8x16), vConst6_8x8);
            sum00_16x8 = vmlaq_u16(sum00_16x8, tmpsum00, vConst4);
            sum01_16x8 = vmlaq_u16(sum01_16x8, tmpsum01, vConst4);
            tmpsum00 = vaddl_u8(vget_low_u8(s00_8x16), vget_low_u8(s04_8x16));
            tmpsum01 = vaddl_u8(vget_high_u8(s00_8x16), vget_high_u8(s04_8x16));
            sum00_16x8 = vaddq_u16(sum00_16x8, tmpsum00);
            sum01_16x8 = vaddq_u16(sum01_16x8, tmpsum01);
            vst1q_u16(buf + x, sum00_16x8);
            vst1q_u16(buf + x + 8, sum01_16x8);

            // uv
            uint8x16_t vSrc00_8x16, vSrc01_8x16, vSrc02_8x16;
            uint16x8_t vSum00, vSum01;

            vSrc00_8x16 = vld1q_u8(uvsrc00 + x);
            vSrc02_8x16 = vld1q_u8(uvsrc02 + x);
            vSrc01_8x16 = vld1q_u8(uvsrc01 + x);

            vSum00 = vaddl_u8(vget_low_u8(vSrc00_8x16), vget_low_u8(vSrc02_8x16));
            vSum01 = vaddl_u8(vget_high_u8(vSrc00_8x16), vget_high_u8(vSrc02_8x16));
            vSum00 = vmlal_u8(vSum00, vget_low_u8(vSrc01_8x16), vConst2_8x8);
            vSum01 = vmlal_u8(vSum01, vget_high_u8(vSrc01_8x16), vConst2_8x8);
            vst1q_u16(uvbuf + x, vSum00);
            vst1q_u16(uvbuf + x + 8, vSum01);
        }
        for (; x < sw - 7; x += 8)
        {
            uint8x8_t s00_8x8, s01_8x8, s02_8x8, s03_8x8, s04_8x8;
            uint16x8_t sum00_16x8;
            uint16x8_t tmpsum00, tmpsum01;
            s00_8x8 = vld1_u8(src00 + x);
            s01_8x8 = vld1_u8(src01 + x);
            s02_8x8 = vld1_u8(src02 + x);
            s03_8x8 = vld1_u8(src03 + x);
            s04_8x8 = vld1_u8(src04 + x);
            tmpsum00 = vaddl_u8(s01_8x8, s03_8x8);
            sum00_16x8 = vmull_u8(s02_8x8, vConst6_8x8);
            tmpsum01 = vaddl_u8(s00_8x8, s04_8x8);
            sum00_16x8 = vmlaq_u16(sum00_16x8, tmpsum00, vConst4);
            sum00_16x8 = vaddq_u16(sum00_16x8, tmpsum01);
            vst1q_u16(buf + x, sum00_16x8);

            //uv
            uint8x8_t vSrc00_8x8, vSrc01_8x8, vSrc02_8x8;
            uint16x8_t vSum00;

            vSrc00_8x8 = vld1_u8(uvsrc00 + x);
            vSrc02_8x8 = vld1_u8(uvsrc02 + x);
            vSrc01_8x8 = vld1_u8(uvsrc01 + x);

            vSum00 = vaddl_u8(vSrc00_8x8, vSrc02_8x8);
            vSum00 = vmlal_u8(vSum00, vSrc01_8x8, vConst2_8x8);
            vst1q_u16(uvbuf + x, vSum00);

        }
#endif
        for (; x < sw; x++)
        {
            buf[x] = src02[x] * 6 + (src01[x] + src03[x]) * 4 + src00[x] + src04[x];
            uvbuf[x] = uvsrc01[x] * 2 + (uvsrc00[x] + uvsrc02[x]);
        }

        // fill the ring buffer (horizontal convolution and decimation)
        buf[-1] = buf[0];
        buf[-2] = buf[0];
        buf[sw] = buf[sw - 2];
        buf[sw + 1] = buf[sw - 3];

        uvbuf[-1] = uvbuf[1];
        uvbuf[-2] = uvbuf[0];
        uvbuf[sw] = uvbuf[sw - 2];
        uvbuf[sw + 1] = uvbuf[sw - 1];

        x = 0;
        k = 0;

#ifdef __ARM_NEON__
        for (; x < dw - 7; x += 8, k += 32)
        {
            uint16x8x4_t bufsrc00_16x8x4 = vld4q_u16(buf + k - 2);
            uint16x8x4_t bufsrc01_16x8x4 = vld4q_u16(buf + k + 2);
            uint16x8_t tmp_sum16x8 = vaddq_u16(bufsrc00_16x8x4.val[0], bufsrc01_16x8x4.val[0]);
            uint16x8_t sum00_16x8 = vaddq_u16(bufsrc00_16x8x4.val[1], bufsrc00_16x8x4.val[3]);
            tmp_sum16x8 = vmlaq_u16(tmp_sum16x8, bufsrc00_16x8x4.val[2], vConst6_16x8);
            sum00_16x8 = vmlaq_u16(tmp_sum16x8, sum00_16x8, vConst4);
            rgbresdata.val[0] = vrshrn_n_u16(sum00_16x8, 8);
            vst1_u8(tmpdst + x, rgbresdata.val[0]);

            //uv
            src00_16x8x4 = vld4q_u16(uvbuf + k - 2);
            src01_16x8x4 = vld4q_u16(uvbuf + k + 2);
            uvsum00_16x8 = vaddq_u16(src00_16x8x4.val[0], src01_16x8x4.val[0]);
            uvsum00_16x8 = vmlaq_u16(uvsum00_16x8, src00_16x8x4.val[2], vConst2_16x8);
            resdata.val[0] = vrshrn_n_u16(uvsum00_16x8, 4);
            vst1_u8(tmpuvdst + x, resdata.val[0]);

            uvsum00_16x8 = vaddq_u16(src00_16x8x4.val[1], src01_16x8x4.val[1]);
            uvsum00_16x8 = vmlaq_u16(uvsum00_16x8, src00_16x8x4.val[3], vConst2_16x8);
            resdata.val[1] = vrshrn_n_u16(uvsum00_16x8, 4);
            vst1_u8(tmpuvdst + x + dh*dw, resdata.val[1]);

            //yuv to rgb
            {
                offset = x * 3;

//                ydata_8x8 = vld1_u8(pYRow + j);
                tmpdata = vreinterpretq_s16_u16(vmovl_u8(rgbresdata.val[0]));
                ylowdata_32x4 = vshll_n_s16(vget_low_s16(tmpdata), yuv_shift);
                yhighdata_32x4 = vshll_n_s16(vget_high_s16(tmpdata), yuv_shift);

//                uvdata_8x8x2 = vld2_u8(pUVRow + 2*j);
                udata = vreinterpretq_s16_u16(vmovl_u8(resdata.val[0]));
                vdata = vreinterpretq_s16_u16(vmovl_u8(resdata.val[1]));
                combine_cr = vsubq_s16(udata, vdupq_n_s16(128));
                combine_cb = vsubq_s16(vdata, vdupq_n_s16(128));

                Bcblowdata = vmull_s16(vget_low_s16(combine_cb), vdup_n_s16(yuvBCb));
                Bcbhighdata = vmull_s16(vget_high_s16(combine_cb), vdup_n_s16(yuvBCb));
                Gcblowdata = vmull_s16(vget_low_s16(combine_cb), vdup_n_s16(yuvGCb));
                Gcbhighdata = vmull_s16(vget_high_s16(combine_cb), vdup_n_s16(yuvGCb));
                Gcrlowdata = vmull_s16(vget_low_s16(combine_cr), vdup_n_s16(yuvGCr));
                Gcrhighdata = vmull_s16(vget_high_s16(combine_cr), vdup_n_s16(yuvGCr));
                Rcrlowdata = vmull_s16(vget_low_s16(combine_cr), vdup_n_s16(yuvRCr));
                Rcrhighdata = vmull_s16(vget_high_s16(combine_cr), vdup_n_s16(yuvRCr));

                lowsumdata = vaddq_s32(ylowdata_32x4, Bcblowdata);
                lowdata = vrshrn_n_s32(lowsumdata, yuv_shift);
                highsumdata = vaddq_s32(yhighdata_32x4, Bcbhighdata);
                highdata = vrshrn_n_s32(highsumdata, yuv_shift);
                combdata = vcombine_s16(lowdata, highdata);
                rgbresdata.val[0] = vqmovun_s16(combdata);

                lowsumdata = vaddq_s32(ylowdata_32x4, Gcblowdata);
                lowsumdata = vaddq_s32(lowsumdata, Gcrlowdata);
                lowdata = vrshrn_n_s32(lowsumdata, yuv_shift);
                highsumdata = vaddq_s32(yhighdata_32x4, Gcbhighdata);
                highsumdata = vaddq_s32(highsumdata, Gcrhighdata);
                highdata = vrshrn_n_s32(highsumdata, yuv_shift);
                combdata = vcombine_s16(lowdata, highdata);
                rgbresdata.val[1] = vqmovun_s16(combdata);

                lowsumdata = vaddq_s32(ylowdata_32x4, Rcrlowdata);
                lowdata = vrshrn_n_s32(lowsumdata, yuv_shift);
                highsumdata = vaddq_s32(yhighdata_32x4, Rcrhighdata);
                highdata = vrshrn_n_s32(highsumdata, yuv_shift);
                combdata = vcombine_s16(lowdata, highdata);
                rgbresdata.val[2] = vqmovun_s16(combdata);
                vst3_u8(pBGRRow + offset, rgbresdata);
            }
        }
#endif
        for (; x < dw; x++, k += 4)//x = 0, k = 0
        {
            tmpdst[x] = (buf[k] * 6 + (buf[k - 1] + buf[k + 1]) * 4 + buf[k - 2] + buf[k + 2] + 128) >> 8;

            tmpuvdst[x] = (uvbuf[k] * 2 + (uvbuf[k - 2] + uvbuf[k + 2]) + 8) >> 4;
            tmpuvdst[x + dh*dw] = (uvbuf[k + 1] * 2 + (uvbuf[k - 1] + uvbuf[k + 3]) + 8) >> 4;

            // yuv转rgb
            offset = x * 3;

            y00 = tmpdst[x];
            cr = (MInt32)tmpuvdst[x] - 128;
            cb = (MInt32)tmpuvdst[x + dh*dw] - 128;

            y00 = yuv_prescale(y00);

            pBGRRow[offset + 0] = ET_YUV_TO_B_2(y00, cb);
            pBGRRow[offset + 1] = ET_YUV_TO_G_2(y00, cb, cr);
            pBGRRow[offset + 2] = ET_YUV_TO_R_2(y00, cr);
        }
    }
}

static MVoid LocalPyrDown4And2(MByte* srcPtr, MByte *srcUVPtr, MByte *dstPtr, MWord *rowBuf, MWord *rowUVBuf, MInt32 sw, MInt32 sh, MInt32 src_step,
                               MInt32 dw, MInt32 dh, MInt32 dst_step, MInt32 start, MInt32 end)
{
    START_TIME;
    MWord* buf = rowBuf + 2;
    MInt32 k, x, y;
    MInt32 sy0 = -(5 >> 1);
    MInt32 sy = sy0;
    MByte *src00, *src01, *src02, *src03, *src04;

    MWord* uvbuf = rowUVBuf + 2;
    MInt32 suvy;
    MByte *uvsrc00, *uvsrc01, *uvsrc02;

    MInt32 offset = 0;
    MUInt8 *pBGRRow = MNull, *pBGRNextRow = MNull;
    MInt32 cr = 0, cb = 0;
    MInt32 y00 = 0, y01 = 0;
    MInt32 y10 = 0, y11 = 0;
#ifdef USE_NEON
    uint8x8_t vConst6_8x8;
    uint16x8_t vConst4, vConst6_16x8;
    vConst4 = vdupq_n_u16(4);
    vConst6_8x8 = vdup_n_u8(6);
    vConst6_16x8 = vdupq_n_u16(6);

    //uv
    uint8x8_t vConst2_8x8;
    uint16x8_t vConst2_16x8;
    uint8x8x2_t resdata;
    uint16x8x4_t src00_16x8x4;
    uint16x8x4_t src01_16x8x4;
    uint16x8_t uvsum00_16x8;
    vConst2_8x8 = vdup_n_u8(2);
    vConst2_16x8 = vdupq_n_u16(2);

#endif
    MByte *uvsrcPtr = srcUVPtr;
    //下采样
    for (y = start; y < end; y++)
    {
        MByte *tmpdst = (MByte *) (dstPtr + dst_step * y);
        MByte *tmpuvdst = (MByte *) (dstPtr + dh*dw + dst_step * y); // uv基于Y的偏移

        sy = 4 * y;
        if (0 == y)
        {
            src00 = srcPtr;
            src01 = srcPtr;
        }
        else
        {
            //如下写法在某几张测试图上访问第0行数据会crash，具体原因不详
            src00 = ((sy - 2) >= 0) ? (srcPtr + src_step * (sy - 2)) : (srcPtr + src_step * (2 - sy));
            src01 = ((sy - 1) >= 0) ? (srcPtr + src_step * (sy - 1)) : (srcPtr + src_step * (1 - sy));
        }
        src02 = srcPtr + src_step * sy;
        src03 = (sy + 1) <= (sh - 1) ? (srcPtr + src_step * (sy + 1)) : (srcPtr + src_step * (2 * sh - 2 - sy - 1));
        src04 = (sy + 2) <= (sh - 1) ? (srcPtr + src_step * (sy + 2)) : (srcPtr + src_step * (2 * sh - 2 - sy - 2));

        sy = y * 2;
        uvsrc00 = (sy - 1 >= 0) ? uvsrcPtr + src_step * (sy - 1) : uvsrcPtr;
        uvsrc01 = uvsrcPtr + src_step * sy;
        uvsrc02 = (sy + 1 <= sh/2 - 1) ? uvsrcPtr + src_step * (sy + 1) : uvsrcPtr + src_step * (sh/2 - 2);


        x = 0;
#ifdef USE_NEON
        for (; x < sw - 15; x += 16)
        {
            uint8x16_t s00_8x16, s01_8x16, s02_8x16, s03_8x16, s04_8x16;
            uint16x8_t sum00_16x8, sum01_16x8;
            uint16x8_t tmpsum00, tmpsum01;
            s00_8x16 = vld1q_u8(src00 + x);
            s01_8x16 = vld1q_u8(src01 + x);
            s02_8x16 = vld1q_u8(src02 + x);
            s03_8x16 = vld1q_u8(src03 + x);
            s04_8x16 = vld1q_u8(src04 + x);
            tmpsum00 = vaddl_u8(vget_low_u8(s01_8x16), vget_low_u8(s03_8x16));
            tmpsum01 = vaddl_u8(vget_high_u8(s01_8x16), vget_high_u8(s03_8x16));
            sum00_16x8 = vmull_u8(vget_low_u8(s02_8x16), vConst6_8x8);
            sum01_16x8 = vmull_u8(vget_high_u8(s02_8x16), vConst6_8x8);
            sum00_16x8 = vmlaq_u16(sum00_16x8, tmpsum00, vConst4);
            sum01_16x8 = vmlaq_u16(sum01_16x8, tmpsum01, vConst4);
            tmpsum00 = vaddl_u8(vget_low_u8(s00_8x16), vget_low_u8(s04_8x16));
            tmpsum01 = vaddl_u8(vget_high_u8(s00_8x16), vget_high_u8(s04_8x16));
            sum00_16x8 = vaddq_u16(sum00_16x8, tmpsum00);
            sum01_16x8 = vaddq_u16(sum01_16x8, tmpsum01);
            vst1q_u16(buf + x, sum00_16x8);
            vst1q_u16(buf + x + 8, sum01_16x8);

            // uv
            uint8x16_t vSrc00_8x16, vSrc01_8x16, vSrc02_8x16;
            uint16x8_t vSum00, vSum01;

            vSrc00_8x16 = vld1q_u8(uvsrc00 + x);
            vSrc02_8x16 = vld1q_u8(uvsrc02 + x);
            vSrc01_8x16 = vld1q_u8(uvsrc01 + x);

            vSum00 = vaddl_u8(vget_low_u8(vSrc00_8x16), vget_low_u8(vSrc02_8x16));
            vSum01 = vaddl_u8(vget_high_u8(vSrc00_8x16), vget_high_u8(vSrc02_8x16));
            vSum00 = vmlal_u8(vSum00, vget_low_u8(vSrc01_8x16), vConst2_8x8);
            vSum01 = vmlal_u8(vSum01, vget_high_u8(vSrc01_8x16), vConst2_8x8);
            vst1q_u16(uvbuf + x, vSum00);
            vst1q_u16(uvbuf + x + 8, vSum01);
        }
        for (; x < sw - 7; x += 8)
        {
            uint8x8_t s00_8x8, s01_8x8, s02_8x8, s03_8x8, s04_8x8;
            uint16x8_t sum00_16x8;
            uint16x8_t tmpsum00, tmpsum01;
            s00_8x8 = vld1_u8(src00 + x);
            s01_8x8 = vld1_u8(src01 + x);
            s02_8x8 = vld1_u8(src02 + x);
            s03_8x8 = vld1_u8(src03 + x);
            s04_8x8 = vld1_u8(src04 + x);
            tmpsum00 = vaddl_u8(s01_8x8, s03_8x8);
            sum00_16x8 = vmull_u8(s02_8x8, vConst6_8x8);
            tmpsum01 = vaddl_u8(s00_8x8, s04_8x8);
            sum00_16x8 = vmlaq_u16(sum00_16x8, tmpsum00, vConst4);
            sum00_16x8 = vaddq_u16(sum00_16x8, tmpsum01);
            vst1q_u16(buf + x, sum00_16x8);

            //uv
            uint8x8_t vSrc00_8x8, vSrc01_8x8, vSrc02_8x8;
            uint16x8_t vSum00;

            vSrc00_8x8 = vld1_u8(uvsrc00 + x);
            vSrc02_8x8 = vld1_u8(uvsrc02 + x);
            vSrc01_8x8 = vld1_u8(uvsrc01 + x);

            vSum00 = vaddl_u8(vSrc00_8x8, vSrc02_8x8);
            vSum00 = vmlal_u8(vSum00, vSrc01_8x8, vConst2_8x8);
            vst1q_u16(uvbuf + x, vSum00);

        }
#endif
        for (; x < sw; x++)
        {
            buf[x] = src02[x] * 6 + (src01[x] + src03[x]) * 4 + src00[x] + src04[x];
            uvbuf[x] = uvsrc01[x] * 2 + (uvsrc00[x] + uvsrc02[x]);
        }

        // fill the ring buffer (horizontal convolution and decimation)
        buf[-1] = buf[0];
        buf[-2] = buf[0];
        buf[sw] = buf[sw - 2];
        buf[sw + 1] = buf[sw - 3];

        uvbuf[-1] = uvbuf[1];
        uvbuf[-2] = uvbuf[0];
        uvbuf[sw] = uvbuf[sw - 2];
        uvbuf[sw + 1] = uvbuf[sw - 1];

        x = 0;
        k = 0;

#ifdef USE_NEON
        for (; x < dw - 7; x += 8, k += 32)
        {
            //y
            uint16x8x4_t bufsrc00_16x8x4 = vld4q_u16(buf + k - 2);
            uint16x8x4_t bufsrc01_16x8x4 = vld4q_u16(buf + k + 2);
            uint16x8_t tmp_sum16x8 = vaddq_u16(bufsrc00_16x8x4.val[0], bufsrc01_16x8x4.val[0]);
            uint16x8_t sum00_16x8 = vaddq_u16(bufsrc00_16x8x4.val[1], bufsrc00_16x8x4.val[3]);
            tmp_sum16x8 = vmlaq_u16(tmp_sum16x8, bufsrc00_16x8x4.val[2], vConst6_16x8);
            sum00_16x8 = vmlaq_u16(tmp_sum16x8, sum00_16x8, vConst4);
            vst1_u8(tmpdst + x, vrshrn_n_u16(sum00_16x8, 8));

            //uv
            src00_16x8x4 = vld4q_u16(uvbuf + k - 2);
            src01_16x8x4 = vld4q_u16(uvbuf + k + 2);
            uvsum00_16x8 = vaddq_u16(src00_16x8x4.val[0], src01_16x8x4.val[0]);
            uvsum00_16x8 = vmlaq_u16(uvsum00_16x8, src00_16x8x4.val[2], vConst2_16x8);
            resdata.val[0] = vrshrn_n_u16(uvsum00_16x8, 4);
            vst1_u8(tmpuvdst + x, resdata.val[0]);

            uvsum00_16x8 = vaddq_u16(src00_16x8x4.val[1], src01_16x8x4.val[1]);
            uvsum00_16x8 = vmlaq_u16(uvsum00_16x8, src00_16x8x4.val[3], vConst2_16x8);
            resdata.val[1] = vrshrn_n_u16(uvsum00_16x8, 4);
            vst1_u8(tmpuvdst + x + dh*dw, resdata.val[1]);
        }
#endif
        for (; x < dw; x++, k += 4)//x = 0, k = 0
        {
            tmpdst[x] = (buf[k] * 6 + (buf[k - 1] + buf[k + 1]) * 4 + buf[k - 2] + buf[k + 2] + 128) >> 8;

            tmpuvdst[x] = (uvbuf[k] * 2 + (uvbuf[k - 2] + uvbuf[k + 2]) + 8) >> 4;
            tmpuvdst[x + dh*dw] = (uvbuf[k + 1] * 2 + (uvbuf[k - 1] + uvbuf[k + 3]) + 8) >> 4;
 
        }
    }

    END_TIME;
}

#ifdef  MCV_MULTI_THREAD
static MVoid threadLocalPyrDown4And2(MVoid* pParam)
{
    GaussPyrDownData4And2 *pyrdown4_D = (GaussPyrDownData4And2*)pParam;
    LocalPyrDown4And2(pyrdown4_D->srcdata, pyrdown4_D->srcUVdata, pyrdown4_D->dstdata, pyrdown4_D->rgbdata, pyrdown4_D->rowBuf, pyrdown4_D->rowUVBuf, pyrdown4_D->srcW, pyrdown4_D->srcH, pyrdown4_D->srcPitch,
                  pyrdown4_D->dstW, pyrdown4_D->dstH, pyrdown4_D->dstPitch, pyrdown4_D->rgbPitch, pyrdown4_D->start, pyrdown4_D->end);
}

static MVoid threadLocalPyrDown4And2_yuv(MVoid* pParam)
{
    GaussPyrDownData4And2 *pyrdown4_D = (GaussPyrDownData4And2*)pParam;
    LocalPyrDown4And2(pyrdown4_D->srcdata, pyrdown4_D->srcUVdata, pyrdown4_D->dstdata, pyrdown4_D->rowBuf, pyrdown4_D->rowUVBuf, pyrdown4_D->srcW, pyrdown4_D->srcH, pyrdown4_D->srcPitch,
                      pyrdown4_D->dstW, pyrdown4_D->dstH, pyrdown4_D->dstPitch, pyrdown4_D->start, pyrdown4_D->end);
}
#endif

MInt32 GaussPyrDown4And2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte* srcUVdata, MByte *dstdata, MByte *rgbdata, MInt32 srcW, MInt32 srcH,
                     MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch, MInt32 rgbPitch)
{
    START_TIME;
    MInt32 i, lret = MOK;
    const MInt32 lTask_Num = 8;
    MWord *rowBuf[lTask_Num] = { MNull };
    MWord *tmpBuf = (MWord*)MMemAlloc(hMemMgr, (srcW + 5)*lTask_Num*sizeof(MWord));

    MWord *rowUVBuf[lTask_Num] = { MNull };
    MWord *tmpUVBuf = (MWord*)MMemAlloc(hMemMgr, (srcW + 6)*lTask_Num*sizeof(MWord));
    if (tmpBuf == MNull)
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    for (i = 0; i < lTask_Num; i++)
    {
        rowBuf[i] = tmpBuf + i*(srcW + 5);
        rowUVBuf[i] = tmpUVBuf + i*(srcW + 6);
    }

#ifdef MCV_MULTI_THREAD
    {
        MFloat  scale = 1.0f / lTask_Num;
        MInt32 taskID[lTask_Num] = { 0 };
        GaussPyrDownData4And2 pParams[lTask_Num] = { 0 };

        for (i = 0; i < lTask_Num; ++i)
        {
            pParams[i].taskID = i;
            pParams[i].srcdata = srcdata;
            pParams[i].srcUVdata = srcUVdata;
            pParams[i].dstdata = dstdata;
            pParams[i].rgbdata = rgbdata;
            pParams[i].rowBuf = rowBuf[i];
            pParams[i].rowUVBuf = rowUVBuf[i];
            pParams[i].srcW = srcW;
            pParams[i].srcH = srcH;
            pParams[i].srcPitch = srcPitch;
            pParams[i].dstW = dstW;
            pParams[i].dstH = dstH;
            pParams[i].dstPitch = dstPitch;
            pParams[i].rgbPitch = rgbPitch;
            pParams[i].start = (MInt32)(dstH*scale*i);
            pParams[i].end = (MInt32)(dstH*scale*(i + 1));
        }
        pParams[lTask_Num - 1].end = dstH;

        for (i = 0; i < lTask_Num; i++)
        {
            taskID[i] = mcvAddTask(mcvParallelMonitor, threadLocalPyrDown4And2, (MVoid*)&pParams[i]);
        }
        for (i = 0; i < lTask_Num; i++)
        {
            mcvWaitTask(mcvParallelMonitor, taskID[i]);
        }
    }
#endif

    exit:
    if (tmpBuf)
    {
        MMemFree(hMemMgr, tmpBuf);
        tmpBuf = MNull;
    }
    if (tmpUVBuf)
    {
        MMemFree(hMemMgr, tmpUVBuf);
        tmpUVBuf = MNull;
    }
    END_TIME;
    return lret;
}


MInt32 GaussPyrDown4And2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *srcdata, MByte* srcUVdata, MByte *dstdata, MInt32 srcW, MInt32 srcH,
                         MInt32 srcPitch, MInt32 dstW, MInt32 dstH, MInt32 dstPitch)
{
    START_TIME;
    MInt32 i, lret = MOK;
    const MInt32 lTask_Num = 16;
    MWord *rowBuf[lTask_Num] = { MNull };
    MWord *tmpBuf = (MWord*)MMemAlloc(hMemMgr, (srcW + 5)*lTask_Num*sizeof(MWord));

    MWord *rowUVBuf[lTask_Num] = { MNull };
    MWord *tmpUVBuf = (MWord*)MMemAlloc(hMemMgr, (srcW + 6)*lTask_Num*sizeof(MWord));
    if (tmpBuf == MNull)
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    for (i = 0; i < lTask_Num; i++)
    {
        rowBuf[i] = tmpBuf + i*(srcW + 5);
        rowUVBuf[i] = tmpUVBuf + i*(srcW + 6);
    }

#ifdef MCV_MULTI_THREAD
    {
        MFloat  scale = 1.0f / lTask_Num;
        MInt32 taskID[lTask_Num] = { 0 };
        GaussPyrDownData4And2 pParams[lTask_Num] = { 0 };

        for (i = 0; i < lTask_Num; ++i)
        {
            pParams[i].taskID = i;
            pParams[i].srcdata = srcdata;
            pParams[i].srcUVdata = srcUVdata;
            pParams[i].dstdata = dstdata;
            pParams[i].rgbdata = MNull;
            pParams[i].rowBuf = rowBuf[i];
            pParams[i].rowUVBuf = rowUVBuf[i];
            pParams[i].srcW = srcW;
            pParams[i].srcH = srcH;
            pParams[i].srcPitch = srcPitch;
            pParams[i].dstW = dstW;
            pParams[i].dstH = dstH;
            pParams[i].dstPitch = dstPitch;
            pParams[i].rgbPitch = 0;
            pParams[i].start = (MInt32)(dstH*scale*i);
            pParams[i].end = (MInt32)(dstH*scale*(i + 1));
        }
        pParams[lTask_Num - 1].end = dstH;

        for (i = 0; i < lTask_Num; i++)
        {
            taskID[i] = mcvAddTask(mcvParallelMonitor, threadLocalPyrDown4And2_yuv, (MVoid*)&pParams[i]);
        }
        for (i = 0; i < lTask_Num; i++)
        {
            mcvWaitTask(mcvParallelMonitor, taskID[i]);
        }
    }
#endif

    exit:
    if (tmpBuf)
    {
        MMemFree(hMemMgr, tmpBuf);
        tmpBuf = MNull;
    }
    if (tmpUVBuf)
    {
        MMemFree(hMemMgr, tmpUVBuf);
        tmpUVBuf = MNull;
    }
    END_TIME;
    return lret;
}

