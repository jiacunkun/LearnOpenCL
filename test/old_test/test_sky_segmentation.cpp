#include <guided_filter_imgproc.h>
#include <imageproc.h>
#include <imgFilter/gaussian_filter.h>
#include "asvloffscreen.h"
#include "test_sky_segmentation.h"
#include "single_image_enhancement_define.h"
#include "ammem.h"
#include "merror.h"
#include "mobilecv.h"
#include "img_rotation.h"
#include "arcsoft_ai_depth_for_seg.h"
#include "NEON_2_SSE.h"


#define __ARM_NEON__


#define yuv_shift			14
#define	yuv_fix(x)			(MInt32)((x) * (1 << (yuv_shift)) + 0.5f)
#define yuv_descale(x)		(((x) + (1 << ((yuv_shift)-1))) >> (yuv_shift))
#define yuv_prescale(x)		((x) << yuv_shift)

#define	yuvYr	yuv_fix(0.299f)
#define	yuvYg	yuv_fix(0.587f)
#define	yuvYb	yuv_fix(0.114f)
#define	yuvCr	yuv_fix(0.713f)
#define	yuvCb	yuv_fix(0.564f)

#define	yuvRCr	yuv_fix(1.403f)
#define	yuvGCr	(-yuv_fix(0.714f))
#define	yuvGCb	(-yuv_fix(0.344f))
#define	yuvBCb	yuv_fix(1.773f)

#define ET_CAST_8U(t)       (MByte)( (t) < 0 ? 0  : ( (t) > 255 ? 255 : (t) ) )

#define ET_YUV_TO_R(y,v)	ET_CAST_8U(yuv_descale((y) + (v)))
#define ET_YUV_TO_G(y,u,v)	ET_CAST_8U(yuv_descale((y) + (v) + (u)))
#define ET_YUV_TO_B(y,u)	ET_CAST_8U(yuv_descale((y) + (u)))

#define ET_YUV_TO_R_2(y,v)	    (MByte)(ET_CAST_8U(yuv_descale((y) + yuvRCr * (v))))
#define ET_YUV_TO_G_2(y,u,v)	(MByte)(ET_CAST_8U(yuv_descale((y) + yuvGCr * (v) + yuvGCb * (u))))
#define ET_YUV_TO_B_2(y,u)	    (MByte)(ET_CAST_8U(yuv_descale((y) + yuvBCb * (u))))

#define ET_RGB_TO_Y(r,g,b)	(MInt32)(yuv_descale((b) * yuvYb + (g) * yuvYg + (r) * yuvYr))
#define ET_RGB_TO_U(y,b,offset)	(MInt32)(yuv_descale(((b) - (y)) * yuvCb) + (offset))
#define ET_RGB_TO_V(y,r,offset)	(MInt32)(yuv_descale(((r) - (y)) * yuvCr) + (offset))

USING_NS_SINFLE_IMAGE_ENHANCEMENT

MVoid localConvYUV444ToBGR(MUInt8* pY, MInt32 lStepY, MUInt8* pU, MInt32 lStepU, MUInt8* pV, MInt32 lStepV,
                           MUInt8* pBGR, MInt32 lStepBGR, MInt32 lWidth, MInt32 lStart, MInt32 lEnd)
{

    MUInt8* pYRow = MNull, * pURow = MNull, * pVRow = MNull;
    MUInt8* pBGRRow = MNull;
    MInt32 i = 0, j = 0;
    MInt32 cr = 0, cb = 0;
    MInt32 y00 = 0, y01 = 0;
    MInt32 y10 = 0, y11 = 0;
    MInt32 offset = 0;

#ifdef __ARM_NEON__
    uint8x8_t ydata_8x8;
		uint8x8x2_t uvdata_8x8x2;
		uint8x8x3_t resdata;
		int16x8_t tmpdata, udata, vdata, combdata;
		int32x4_t ylowdata_32x4, yhighdata_32x4, lowsumdata, highsumdata;
		int16x4_t crdata_16x4, cbdata_16x4, lowdata, highdata;
		int16x4x2_t zip_cr, zip_cb;
		int16x8_t combine_cr, combine_cb;
		int32x4_t Gcrlowdata, Gcrhighdata, Rcrlowdata, Rcrhighdata;
		int32x4_t Bcblowdata, Bcbhighdata, Gcblowdata, Gcbhighdata;
#endif

    for (i = lStart; i < lEnd; i += 1)
    {
        pYRow = pY + i * lStepY;
        pURow = pU + i * lStepU;
        pVRow = pV + i * lStepV;

        pBGRRow = pBGR + i * lStepBGR;

        j = 0;
#ifdef __ARM_NEON__
        for (; j < lWidth - 8; j += 8)
			{
				offset = j * 3;

				ydata_8x8 = vld1_u8(pYRow + j);
				tmpdata = vreinterpretq_s16_u16(vmovl_u8(ydata_8x8));
				ylowdata_32x4 = vshll_n_s16(vget_low_s16(tmpdata), yuv_shift);
				yhighdata_32x4 = vshll_n_s16(vget_high_s16(tmpdata), yuv_shift);

				udata = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(pURow + j)));
				vdata = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(pVRow + j)));
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
				resdata.val[0] = vqmovun_s16(combdata);

				lowsumdata = vaddq_s32(ylowdata_32x4, Gcblowdata);
				lowsumdata = vaddq_s32(lowsumdata, Gcrlowdata);
				lowdata = vrshrn_n_s32(lowsumdata, yuv_shift);
				highsumdata = vaddq_s32(yhighdata_32x4, Gcbhighdata);
				highsumdata = vaddq_s32(highsumdata, Gcrhighdata);
				highdata = vrshrn_n_s32(highsumdata, yuv_shift);
				combdata = vcombine_s16(lowdata, highdata);
				resdata.val[1] = vqmovun_s16(combdata);

				lowsumdata = vaddq_s32(ylowdata_32x4, Rcrlowdata);
				lowdata = vrshrn_n_s32(lowsumdata, yuv_shift);
				highsumdata = vaddq_s32(yhighdata_32x4, Rcrhighdata);
				highdata = vrshrn_n_s32(highsumdata, yuv_shift);
				combdata = vcombine_s16(lowdata, highdata);
				resdata.val[2] = vqmovun_s16(combdata);
				vst3_u8(pBGRRow + offset, resdata);
			}
#endif
        for (; j < lWidth; j += 1)
        {
            offset = j * 3;

            y00 = pYRow[j];
            cr = (MInt32)pURow[j] - 128;
            cb = (MInt32)pVRow[j] - 128;

            y00 = yuv_prescale(y00);

            pBGRRow[offset + 0] = ET_YUV_TO_B_2(y00, cb);
            pBGRRow[offset + 1] = ET_YUV_TO_G_2(y00, cb, cr);
            pBGRRow[offset + 2] = ET_YUV_TO_R_2(y00, cr);
        }
    }
}

typedef struct __tag_localConvYUV444ToBGR_para {
    MUInt8* pY;
    MInt32 lStepY;
    MUInt8* pU;
    MInt32 lStepU;
    MUInt8* pV;
    MInt32 lStepV;
    MUInt8* pBGR;
    MInt32 lStepBGR;
    MInt32 lWidth;
    MInt32 lHeight;
    MInt32 lStart;
    MInt32 lEnd;
}ConvYUV444ToBGRPara;

MVoid thread_localConvYUV444ToBGR(MVoid* para)
{
    ConvYUV444ToBGRPara* pPara = (ConvYUV444ToBGRPara*)para;
    localConvYUV444ToBGR(pPara->pY, pPara->lStepY, pPara->pU, pPara->lStepU,
                         pPara->pV, pPara->lStepV, pPara->pBGR,
                         pPara->lStepBGR, pPara->lWidth, pPara->lStart, pPara->lEnd);
}

MRESULT convYUV444ToBGR(MHandle mcvParallelMonitor, MByte* pYdata, MInt32 lYPitch, MByte* pUdata, MInt32 lUPitch,
                        MByte* pVdata, MInt32 lVPitch, MByte *pDstBGR, MInt32 lBGRPitch, MInt32 lWidth, MInt32 lHeight)
{
    MRESULT lret = MOK;
    MInt32 lBlockH = 0;
    const MInt32 taskNum = 4;
    ConvYUV444ToBGRPara para[taskNum] = { MNull };
    MInt32 taskId[taskNum] = { MNull };
    MInt32 k = 0;

    if (!pDstBGR || !pYdata || !pUdata || !pVdata)
        return MERR_INVALID_PARAM;


    lBlockH = (lHeight / taskNum) >> 1 << 1;

    for (k = 0; k < taskNum; ++k)
    {
        para[k].pY = pYdata;
        para[k].lStepY = lYPitch;
        para[k].pU = pUdata;
        para[k].lStepU = lUPitch;
        para[k].pV = pVdata;
        para[k].lStepV = lVPitch;
        para[k].pBGR = pDstBGR;
        para[k].lStepBGR = lBGRPitch;
        para[k].lWidth = lWidth;
        para[k].lHeight = lHeight;
        para[k].lStart = k * lBlockH;
        para[k].lEnd = (k + 1) * lBlockH;
    }
    para[taskNum - 1].lEnd = lHeight;

    for (k = 0; k < taskNum; ++k)
    {
        taskId[k] = mcvAddTask(mcvParallelMonitor, thread_localConvYUV444ToBGR, (MVoid*)&para[k]);
    }

    for (k = 0; k < taskNum; ++k)
    {
        lret = mcvWaitTask(mcvParallelMonitor, taskId[k]);
        if (MOK != lret)
        {
            lret = MERR_BAD_STATE;
            return lret;
        }
    }

    return lret;
}

static MVoid Split_UV_Data(MByte* pSrcUV, MInt32 lPitchUV, MInt32 lWidth, MInt32 lHeight, MByte* pU, MInt32 lPitchU, MByte* pV, MInt32 lPitchV)
{
    MInt32 x, y;

    for (y = 0; y < lHeight; y++)
    {
        MByte* pCurUV = pSrcUV + y * lPitchUV;
        MByte* pCurU = pU + y * lPitchU;
        MByte* pCurV = pV + y * lPitchV;
        x = 0;
#ifdef __ARM_NEON__
        for (; x < lWidth - 15; x+=16)
			{
				uint8x16x2_t vdata_uv = vld2q_u8(pCurUV + x * 2);
				vst1q_u8(pCurU + x, vdata_uv.val[0]);
				vst1q_u8(pCurV + x, vdata_uv.val[1]);

			}
#endif
        for (; x < lWidth; x++)
        {
            pCurU[x] = pCurUV[x * 2];
            pCurV[x] = pCurUV[x * 2 + 1];
        }
    }
}

MInt32 Image_Resize(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcData, MInt32 pSrcWidth, MInt32 pSrcHeight, MInt32 lSrcPitch,
                    MByte* pDstData, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch)
{
    MInt32 lret = MOK;
// 		BLParam YParam = { MNull };
//
// 		lret = ImageResize_AllocMem_BL(hMemMgr, pSrcWidth, pSrcHeight, 1, lDstWidth, lDstHeight, 2, &YParam);
// 		if (lret != MOK)
// 			goto exit;
//
// 		ImageResize_BL_CS(pSrcData, pSrcWidth, pSrcHeight, lSrcPitch, 1, pDstData,
// 			lDstWidth, lDstHeight, 0, lDstHeight, lDstPitch, 0, &YParam);

    MInt32 taskNum = 1;
    if (lDstHeight > 1000)
    {
        taskNum = 8;
    }
    else if (lDstHeight > 500)
    {
        taskNum = 4;
    }
    else if (lDstHeight > 200)
    {
        taskNum = 2;
    }
    BilinearResize_Param blParam = { 0 };
    blParam.pBuf = MNull;
    lret = ImgBilinearResizeAllocMem_U8C1(hMemMgr, pSrcWidth, pSrcHeight, lDstWidth, lDstHeight, taskNum, &blParam);

    lret = ImgBilinearResize_U8C1(mcvParallelMonitor, taskNum, pSrcData, pSrcWidth, pSrcHeight, lSrcPitch,
                                  pDstData, lDstWidth, lDstHeight, lDstPitch, &blParam);

    exit:
// 		if (YParam.pTempBuf)
// 			MMemFree(hMemMgr, YParam.pTempBuf);

    if (blParam.pBuf)
    {
        MMemFree(hMemMgr, blParam.pBuf);
        blParam.pBuf = MNull;
    }

    return lret;
}

#ifdef MCV_MULTI_THREAD
typedef struct _tag_UPSCALEFORE {
    MByte* pSrcData;
    MByte* pDstData;
    MInt32 srcH;
    MInt32 srcW;
    MInt32 srcPitch;
    MInt32 dstH;
    MInt32 dstW;
    MInt32 dstPitch;
    MFloat invscaleX;
    MFloat invscaleY;
    MInt32 startRow;
    MInt32 endRow;
}UpscaleFore;
#endif

static MVoid localUpscaleForeMask(MByte* pSrcData, MByte* pDstData, MInt32 srcH, MInt32 srcW, MInt32 srcPitch,
                                  MInt32 dstH, MInt32 dstW, MInt32 dstPitch, MFloat invscaleY, MFloat invscaleX, MInt32 startRow, MInt32 endRow)
{
    MInt32 i, j, x, y;
    MInt32 ypre = -1;

    MFloat fy, fx;
    fy = startRow * invscaleY + 0.5;

    for (i = startRow; i < endRow; i++, fy += invscaleY)
    {
        y = MInt32(fy);
        y = (y >= srcH) ? srcH - 1 : y;

        if (y == ypre)
        {
            MByte* pDstRow0 = pDstData + (i - 1) * dstPitch;
            MByte* pDstRow1 = pDstData + i * dstPitch;

            MMemCpy(pDstRow1, pDstRow0, dstW);
        }
        else
        {
            MByte* pSrcRow = pSrcData + y * srcPitch;
            MByte* pDstRow = pDstData + i * dstPitch;
            for (j = 0, fx = 0.5f; j < dstW; j++, fx += invscaleX)
            {
                x = MInt32(fx);
                x = (x >= srcW) ? srcW - 1 : x;
                pDstRow[j] = pSrcRow[x];
            }
        }

        ypre = y;
    }
}

static MVoid thread_localUpscaleForeMask(MVoid* pParam)
{
    UpscaleFore* param = (UpscaleFore*)pParam;

    localUpscaleForeMask(param->pSrcData, param->pDstData, param->srcH, param->srcW, param->srcPitch,
                         param->dstH, param->dstW, param->dstPitch, param->invscaleY, param->invscaleX, param->startRow, param->endRow);
}

MInt32 UpscaleForeMask(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pForeMask, LPASVLOFFSCREEN pBigMaskImg)
{
    MInt32 lret = MOK, i, j, x, y;

    MFloat invscaleY = (MFloat)pForeMask->i32Height / pBigMaskImg->i32Height;
    MFloat invscaleX = (MFloat)pForeMask->i32Width / pBigMaskImg->i32Width;


    if (mcvParallelMonitor)
    {
        const MInt32 lTaskNum = 8;
#ifdef MCV_MULTI_THREAD
        MInt32 lSize;
        MInt32 taskID[lTaskNum] = { 0 };
        UpscaleFore pParams[lTaskNum] = { 0 };

        lSize = pBigMaskImg->i32Height / lTaskNum;
        lSize = lSize >> 1 << 1;

        pParams[0].startRow = 0;
        pParams[0].endRow = lSize;
        for (i = 1; i < lTaskNum; i++)
        {
            pParams[i].startRow = i * lSize;
            pParams[i].endRow = (i + 1) * lSize;
        }
        pParams[i - 1].endRow = pBigMaskImg->i32Height;

        for (i = 0; i < lTaskNum; i++)
        {
            pParams[i].pSrcData = pForeMask->ppu8Plane[0];
            pParams[i].pDstData = pBigMaskImg->ppu8Plane[0];
            pParams[i].srcH = pForeMask->i32Height;
            pParams[i].srcW = pForeMask->i32Width;
            pParams[i].srcPitch = pForeMask->pi32Pitch[0];
            pParams[i].dstH = pBigMaskImg->i32Height;
            pParams[i].dstW = pBigMaskImg->i32Width;
            pParams[i].dstPitch = pBigMaskImg->pi32Pitch[0];
            pParams[i].invscaleY = invscaleY;
            pParams[i].invscaleX = invscaleX;

        }

        for (i = 0; i < lTaskNum; i++)
        {
            taskID[i] = mcvAddTask(mcvParallelMonitor, thread_localUpscaleForeMask, (MVoid*)&pParams[i]);
        }
        for (i = 0; i < lTaskNum; i++)
        {
            mcvWaitTask(mcvParallelMonitor, taskID[i]);
        }
#endif
    }
    else
    {
        localUpscaleForeMask(pForeMask->ppu8Plane[0], pBigMaskImg->ppu8Plane[0], pForeMask->i32Height, pForeMask->i32Width, pForeMask->pi32Pitch[0],
                             pBigMaskImg->i32Height, pBigMaskImg->i32Width, pBigMaskImg->pi32Pitch[0], invscaleY, invscaleX, 0, pBigMaskImg->i32Height);
    }
    exit:
    return lret;
}

static MInt32 GenYUVDown8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, ASVLOFFSCREEN& imgYDown8, ASVLOFFSCREEN& imgUDown4, ASVLOFFSCREEN& imgVDown4)
{
    MInt32 lret = MOK;
    MInt32 lWidth = pSrcImg->i32Width, lHeight = pSrcImg->i32Height;

    ASVLOFFSCREEN ImgYDown4 = { MNull };
    ASVLOFFSCREEN imgUFull = { MNull }, imgVFull = { MNull };
    lret = AllocOffscreenMemory(hMemMgr, lWidth >> 2, lHeight >> 2, ASVL_PAF_GRAY, &ImgYDown4);
    if (MOK != lret)
    {
        goto exit;
    }
    lret = AllocOffscreenMemory(hMemMgr, lWidth >> 1, lHeight >> 1, ASVL_PAF_GRAY, &imgUFull);
    if (MOK != lret)
    {
        goto exit;
    }

    lret = AllocOffscreenMemory(hMemMgr, lWidth >> 1, lHeight >> 1, ASVL_PAF_GRAY, &imgVFull);
    if (MOK != lret)
    {
        goto exit;
    }

    // Y Down8
    {

        GaussPyrDown4(hMemMgr, mcvParallelMonitor, pSrcImg->ppu8Plane[0], ImgYDown4.ppu8Plane[0], pSrcImg->i32Width, pSrcImg->i32Height,
                      pSrcImg->pi32Pitch[0], ImgYDown4.i32Width, ImgYDown4.i32Height, ImgYDown4.pi32Pitch[0]);


        GaussPyrDown2(hMemMgr, mcvParallelMonitor, ImgYDown4.ppu8Plane[0], imgYDown8.ppu8Plane[0], ImgYDown4.i32Width, ImgYDown4.i32Height,
                      ImgYDown4.pi32Pitch[0], imgYDown8.i32Width, imgYDown8.i32Height, imgYDown8.pi32Pitch[0]);


    }

    // UV Down4
    {


        // uv
        Split_UV_Data(pSrcImg->ppu8Plane[1], pSrcImg->pi32Pitch[1], lWidth >> 1, lHeight >> 1,
                      imgUFull.ppu8Plane[0], imgUFull.pi32Pitch[0], imgVFull.ppu8Plane[0], imgVFull.pi32Pitch[0]);

        GaussPyrDown4(hMemMgr, mcvParallelMonitor, imgUFull.ppu8Plane[0], imgUDown4.ppu8Plane[0], imgUFull.i32Width, imgUFull.i32Height,
                      imgUFull.pi32Pitch[0], imgUDown4.i32Width, imgUDown4.i32Height, imgUDown4.pi32Pitch[0]);

        GaussPyrDown4(hMemMgr, mcvParallelMonitor, imgVFull.ppu8Plane[0], imgVDown4.ppu8Plane[0], imgVFull.i32Width, imgVFull.i32Height,
                      imgVFull.pi32Pitch[0], imgVDown4.i32Width, imgVDown4.i32Height, imgVDown4.pi32Pitch[0]);

        // 		SaveToBMP("Q:/u_down4.bmp", imgUDown4.ppu8Plane[0], imgUDown4.i32Width, imgUDown4.i32Height, imgUDown4.pi32Pitch[0], 8);
        // 		SaveToBMP("Q:/v_down4.bmp", imgVDown4.ppu8Plane[0], imgVDown4.i32Width, imgVDown4.i32Height, imgVDown4.pi32Pitch[0], 8);


    }

    exit:

    FreeOffscreenMemory(hMemMgr, &ImgYDown4);
    FreeOffscreenMemory(hMemMgr, &imgUFull);
    FreeOffscreenMemory(hMemMgr, &imgVFull);

    return lret;
}

static MInt32 GenSegInputBGR(MHandle hMemMgr, MHandle mcvParallelMonitor, ASVLOFFSCREEN& imgYDown8,ASVLOFFSCREEN& imgUDown4, ASVLOFFSCREEN& imgVDown4, ASVLOFFSCREEN& pSrcSegBGR, MInt32 degree1)
{
    MInt32 lret = MOK;
    MInt32 inWidth = pSrcSegBGR.i32Width, inHeight = pSrcSegBGR.i32Height;
    MUInt8* m_pSegImgY = MNull, * m_pSegImgU = MNull, * m_pSegImgV = MNull, * m_pSegImgTmp = MNull;

    m_pSegImgY = (MByte*)MMemAlloc(hMemMgr, inWidth * inHeight * sizeof(MByte));
    if (MNull == m_pSegImgY)
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    m_pSegImgU = (MByte*)MMemAlloc(hMemMgr, inWidth * inHeight * sizeof(MByte));
    if (MNull == m_pSegImgU)
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    m_pSegImgV = (MByte*)MMemAlloc(hMemMgr, inWidth * inHeight * sizeof(MByte));
    if (MNull == m_pSegImgV)
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    m_pSegImgTmp = (MByte*)MMemAlloc(hMemMgr, inWidth * inHeight * sizeof(MByte));
    if (MNull == m_pSegImgTmp)
    {
        lret = MERR_NO_MEMORY;
        goto exit;
    }

    // SmImg02 -> m_pMask
    Image_Resize(hMemMgr, mcvParallelMonitor, imgYDown8.ppu8Plane[0], imgYDown8.i32Width, imgYDown8.i32Height, imgYDown8.pi32Pitch[0],
                 m_pSegImgTmp, inWidth, inHeight, inWidth);
    ImgRotateRestrictAngle_C1(mcvParallelMonitor, 2, degree1,
                              m_pSegImgTmp, inWidth, inWidth, inWidth,
                              m_pSegImgY, inWidth, inWidth, inWidth);

    Image_Resize(hMemMgr, mcvParallelMonitor, imgUDown4.ppu8Plane[0], imgUDown4.i32Width, imgUDown4.i32Height, imgUDown4.pi32Pitch[0],
                 m_pSegImgTmp, inWidth, inHeight, inWidth);
    ImgRotateRestrictAngle_C1(mcvParallelMonitor, 2, degree1,
                              m_pSegImgTmp, inWidth, inWidth, inWidth,
                              m_pSegImgU, inWidth, inWidth, inWidth);

    Image_Resize(hMemMgr, mcvParallelMonitor, imgVDown4.ppu8Plane[0], imgVDown4.i32Width, imgVDown4.i32Height, imgVDown4.pi32Pitch[0],
                 m_pSegImgTmp, inWidth, inHeight, inWidth);
    ImgRotateRestrictAngle_C1(mcvParallelMonitor, 2, degree1,
                              m_pSegImgTmp, inWidth, inWidth, inWidth,
                              m_pSegImgV, inWidth, inWidth, inWidth);


    convYUV444ToBGR(mcvParallelMonitor, m_pSegImgY, inWidth, m_pSegImgU, inWidth,
                    m_pSegImgV, inWidth, pSrcSegBGR.ppu8Plane[0], pSrcSegBGR.pi32Pitch[0], inWidth, inHeight);

    exit:
    if (m_pSegImgY)
    {
        MMemFree(hMemMgr, m_pSegImgY);
    }
    if (m_pSegImgU)
    {
        MMemFree(hMemMgr, m_pSegImgU);
    }
    if (m_pSegImgV)
    {
        MMemFree(hMemMgr, m_pSegImgV);
    }
    if (m_pSegImgTmp)
    {
        MMemFree(hMemMgr, m_pSegImgTmp);
    }
    return lret;
}

// pDispImgErode: 比实际前景略小一点，用于对齐。防止对齐找到的点都在前景外面。
MInt32 Process_SkySegment(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg,
                          LPASVLOFFSCREEN pDispImg,  MInt32 lImageOrient)
{
    MInt32 lret = MOK;
    // 		FILE *fp;
    MInt32 lWidth = pSrcImg->i32Width;
    MInt32 lHeight = pSrcImg->i32Height;

    ASVLOFFSCREEN imgYDown8 = { MNull };
    ASVLOFFSCREEN imgUDown4 = { MNull }, imgVDown4{ MNull };

    MInt32 rotDegree1[] = { 0, 0, 90, 270, 180 };
    MInt32 rotDegree2[] = { 0, 0, 270, 90, 180 };

    MHandle hModelData = MNull, hEngine = MNull;
    MInt32 inWidth = 0, inHeight = 0;
    MInt32 outWidth = 0, outHeight = 0;

    AIDSEG_ModelInfo modelInfo{ 0 };
    MBool bReset = MTrue;

    ASVLOFFSCREEN pMask = { 0 }, pMask2 = { 0 }, pSrcSegBGR = { 0 };
    AIDSEG_Rst rst{ 0 };

    if (lImageOrient <= 0 || lImageOrient >= 5)
    {
        return MERR_UNSUPPORTED;
    }


    lret = AllocOffscreenMemory(hMemMgr, lWidth >> 3, lHeight >> 3, ASVL_PAF_GRAY, &imgYDown8);
    if (MOK != lret)
    {
        goto exit;
    }

    lret = AllocOffscreenMemory(hMemMgr, lWidth >> 3, lHeight >> 3, ASVL_PAF_GRAY, &imgUDown4);
    if (MOK != lret)
    {
        goto exit;
    }

    lret = AllocOffscreenMemory(hMemMgr, lWidth >> 3, lHeight >> 3, ASVL_PAF_GRAY, &imgVDown4);
    if (MOK != lret)
    {
        goto exit;
    }

    lret = lret = GenYUVDown8(hMemMgr, mcvParallelMonitor, pSrcImg, imgYDown8, imgUDown4, imgVDown4);
    if (MOK != lret)
    {
        goto exit;
    }

    {
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
        MInt32 lTime;
			START_PROFILE();
#endif

        const char* pSpecialParam = "This SDK can be used only in Zang Jiong's team internally, do not distribute!";
        //INIT
        lret = AIDSEG_GetModelData_201012_CL_QC(hMemMgr, &hModelData);
        if (MOK != lret)
            goto exit;

        lret = AIDSEG_Init(hMemMgr, hModelData, (MVoid *)pSpecialParam, &hEngine);
        if (MOK != lret)
            goto exit;

        lret = AIDSEG_GetModelInfo(hEngine, &modelInfo);
        if (MOK != lret)
            goto exit;

        inWidth = modelInfo.widthIn;
        inHeight = modelInfo.heightIn;
        outWidth = modelInfo.widthOut;
        outHeight = modelInfo.heightOut;

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
        END_PROFILE(lTime);
			PrintfB(6, "LowLight", "ASEG_Init Total time: %ldms\r\n", lTime);
#endif
    }


    lret = AllocOffscreenMemory(hMemMgr, inWidth, inHeight, ASVL_PAF_RGB24_B8G8R8, &pSrcSegBGR);
    if (lret != MOK)
    {
        goto exit;
    }
    lret = AllocOffscreenMemory(hMemMgr, outWidth, outHeight, ASVL_PAF_GRAY, &pMask);
    if (lret != MOK)
    {
        goto exit;
    }
    lret = AllocOffscreenMemory(hMemMgr, outWidth, outHeight, ASVL_PAF_GRAY, &pMask2);

    if (lret != MOK)
    {
        goto exit;
    }
    rst.pMask = &pMask;

    lret = GenSegInputBGR(hMemMgr, mcvParallelMonitor, imgYDown8, imgUDown4, imgVDown4, pSrcSegBGR, rotDegree1[lImageOrient]);
    if (lret != MOK)
    {
        goto exit;
    }
    //SaveToBMP("Q:/bgr_rot.bmp", pSrcSegBGR.ppu8Plane[0], pSrcSegBGR.i32Width, pSrcSegBGR.i32Height, pSrcSegBGR.pi32Pitch[0], 24);


    {
#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
        MInt32 lTime;
			START_PROFILE();
#endif

        lret = AIDSEG_AddSrc(hEngine, &pSrcSegBGR);
        if (MOK != lret)
            goto exit;

        lret = AIDSEG_Process(hEngine, &rst);
        PrintfB(6, "LowLight", "ASEG_DoSegment return  %d\n", lret);
        if (MOK != lret)
            goto exit;

#if defined(PLATFORM_LINUX) && defined(_ARM_TIME_)
        END_PROFILE(lTime);
			PrintfB(6, "LowLight", "ASEG_DoSegment Total time: %ldms\r\n", lTime);
#endif
    }

    //Save_ASVL("pMask", pMask);

    ImgRotateRestrictAngle_C1(mcvParallelMonitor, 2, rotDegree2[lImageOrient],
                              pMask.ppu8Plane[0], pMask.pi32Pitch[0], pMask.i32Width, pMask.i32Height,
                              pMask2.ppu8Plane[0], pMask2.pi32Pitch[0], pMask2.i32Width, pMask2.i32Height);

    //Save_ASVL("pMaskRot", pMask2);


    for (MInt32 y = 0; y < outHeight; y++)
    {
        MByte* pData = pMask2.ppu8Plane[0] + y * pMask2.pi32Pitch[0];
        for (MInt32 x = 0; x < inWidth * inHeight; x++)
        {
            pData[x] = pData[x] > 64 ? 255 : 0;
        }
    }

    UpscaleForeMask(hMemMgr, mcvParallelMonitor, &pMask2, pDispImg);



    exit:
    FreeOffscreenMemory(hMemMgr, &pSrcSegBGR);
    FreeOffscreenMemory(hMemMgr, &pMask);
    FreeOffscreenMemory(hMemMgr, &pMask2);

    FreeOffscreenMemory(hMemMgr, &imgUDown4);
    FreeOffscreenMemory(hMemMgr, &imgVDown4);
    FreeOffscreenMemory(hMemMgr, &imgYDown8);

    if (hEngine)
        AIDSEG_Uninit(&hEngine);
    if (hModelData)
        AIDSEG_ReleaseModelData(&hModelData);


    return lret;
}