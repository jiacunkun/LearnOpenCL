/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary 
and confidential information. 

The information and code contained in this file is only for authorized ArcSoft 
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER 
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy, 
distribute, modify, or take any action in reliance on it. 

If you have received this file in error, please immediately notify ArcSoft and 
permanently delete the original and any copy of any file and any printout 
thereof.
*******************************************************************************/
#ifndef _IMAGE_BASE_H_
#define _IMAGE_BASE_H_

#include "amcomdef.h"
#include "asvloffscreen.h"
#include "defcompilesetting.h"

#define PX(FUNC)		acs##FUNC
#ifndef	ABS
#define ABS(x)		((x) > 0 ? (x) : -(x))
#endif

#define QUAD_MULTI_THREAD

//#define  DUMP_IMAGE
#define  FIXED_POINT_VERSION
//#define  FACE_DETECTION
#define  MAX_FACE_NUMBERS		30

//#define _OUTPUT_LOG_
//#define OUTPUT_PATH		"F:/MFNS_Debug/"

#ifdef _DEBUG
//#define _TIME_LOG_
//#define _OUTPUT_LOG_
//#define OUTPUT_PATH		"F:/MFNS_Debug/"
#endif

//#define _VENDOR_DEBUG_
#ifdef _VENDOR_DEBUG_
#ifdef PLATFORM_LINUX
#define VENDOR_DUMP_PATH		"/sdcard/test_sr/"
#else
#define VENDOR_DUMP_PATH		"F:/MFNS_Debug/"
#endif // PLATFORM_LINUX

#endif

#define WHITE_EDGE_BLEND
#define INPUT_10BIT

// =============== Color format transfer =============== //
#define yuv_shift			14
#define yuv_fix(x)			(MInt32)((x) * (1 << (yuv_shift)) + 0.5f)
#define yuv_descale(x)		(((x) + (1 << ((yuv_shift)-1))) >> (yuv_shift))
#define yuv_prescale(x)		((x) << yuv_shift)

#define yuvRCr	yuv_fix(1.403f)
#define yuvGCr	(-yuv_fix(0.714f))
#define yuvGCb	(-yuv_fix(0.344f))
#define yuvBCb	yuv_fix(1.773f)

#define	yuvYr	yuv_fix(0.299f)
#define	yuvYg	yuv_fix(0.587f)
#define	yuvYb	yuv_fix(0.114f)
#define	yuvCr	yuv_fix(0.713f)
#define	yuvCb	yuv_fix(0.564f)

#define ET_CAST_8U(t)		(MByte)((t)&(~255) ? ((-(t))>>31) : (t))

#define ET_YUV_TO_R(y,v)	ET_CAST_8U(yuv_descale((y) + (v)))
#define ET_YUV_TO_G(y,u,v)	ET_CAST_8U(yuv_descale((y) + (v) + (u)))
#define ET_YUV_TO_B(y,u)	ET_CAST_8U(yuv_descale((y) + (u)))

#define ET_YUV_TO_R_2(y,v)	    (MByte)(ET_CAST_8U(yuv_descale((y) + yuvRCr * (v))))
#define ET_YUV_TO_G_2(y,u,v)	(MByte)(ET_CAST_8U(yuv_descale((y) + yuvGCr * (v) + yuvGCb * (u))))
#define ET_YUV_TO_B_2(y,u)	    (MByte)(ET_CAST_8U(yuv_descale((y) + yuvBCb * (u))))

// image resize
#define FLT_TO_FIX(x,n)		(MInt32)((x)*(1<<(n))+0.5f)
#define DESCALE(x,n)		(((x) + (1 << ((n)-1))) >> (n))
#define WARP_SHIFT			7
#define WARP_MUL_ONE_8U(x)  ((x) << WARP_SHIFT)
#define WARP_DESCALE_8U(x)  DESCALE((x), (WARP_SHIFT<<1))
#define WARP_DESCALE2_8U(x)	DESCALE((x), WARP_SHIFT)


#ifndef MAX
#define MAX(a,b)	((a)>(b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b)	((a)<(b)?(a):(b))
#endif


#ifdef __cplusplus
extern "C" {
#endif

#if defined(_OUTPUT_LOG_) || defined(_OUTPUT_LOG_OCL_FUS_)
long SaveToBMP(char* szFile, unsigned char *pData, long nWidth, long nHeight, 
			   long nLineBytes, long nBitCount);

long SaveToBMP_Bin(char* szFile, unsigned char *pTmpData, long nWidth, long nHeight,
	long nLineBytes, long nBitCount);

long SaveToBMP_10BIT(char* szFile, unsigned char *pTmpData, long nWidth, long nHeight,
	long nLineBytes, long nBitCount);

long SaveToBMP_ShortImg(char* szFile, unsigned char *pTmpData, long nWidth, long nHeight,
	long nLineBytes, long nBitCount, MInt32 lShiftScale, MBool bSingle);

MInt32 SaveFloatToBMP(char* szFile, float* fSrcData, MInt32 nWidth, MInt32 nHeight, 
	MInt32 lf_Pitch, MFloat fsclae, MFloat fshift);
#endif

MVoid* MMemAlloc_Opt(MHandle hMemMgr01,MHandle hMemMgr02, MInt32 lsize);
MVoid  MMemFree_Opt(MHandle hMemMgr01, MHandle hMemMgr02, MVoid* pMem);

MRESULT MedianBlur_8u3x3_C1(MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
	MInt32 width, MInt32 height, MInt32 lCPUNum);

MVoid ContrastEnhancement_C1(MByte *pSrcBuf,MInt32 lWidth,MInt32 lHeight,MInt32 lSrcLineBytes,
							 MByte *pDstBuf,MInt32 lDstLineBytes,MInt32 lContrast);


MInt32 SharpenUSM_Box_GuideFilter_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcData, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcPitch,
	MByte* pDstData, MInt32 lDstPitch, MInt32 lAmount, MInt32 lRadius, MByte lThresh, MInt32 lMaxDif, MInt32 lCPUNum);

MInt32 Img_Sharpen_Method_Med_USM(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcDstImg, LPASVLOFFSCREEN pDetaiSmoothImg,
	LPASVLOFFSCREEN pTextuSmoothImg01, LPASVLOFFSCREEN pTextuSmoothImg02,
	MInt32 lSharpenIntensity, MFloat DRCGainVal, LPASVLOFFSCREEN weightMap, MFloat fScale);

MInt32 SharpenUSM_BilateFilter_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSmoothImg,
	MInt32 lSharpenIntensity, MInt32 lVar, MInt32 lMaxDif, MInt32 lCPUNum, MByte lMeanGray, MFloat fScale, MBool lPostFlag, MBool lRefFlag);


MInt32 Image_USM_Process(MHandle hMemMgr, MByte*  pSrcData, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcLineBytes, MByte* pSmoothData, MInt32 lSmoothPitch,
	MByte* pDstData, MInt32 lDstLineBytes, MInt32* pWeiAmount, MInt32 *pTextureWei, MInt32 lCPUNum, MBool lPostFlag);

MVoid compute_wei_amount(MInt32* pWeiAmount, MInt32 lVar, MInt32 lAmount, MInt32 lMaxDif = 15);

MVoid compute_texture_wei_amount(MInt32* pWeiAmount, MInt32 lVar, MInt32 lAmount, MInt32 lMaxDif, MInt32 lThresh);

MInt32 set_multi_img_mem(LPASVLOFFSCREEN* pSmoothImg, MInt32 lWidth, MInt32 lHeight,
	MInt32 lImgNum, MInt32 lPAF);

MRESULT CopyFixImageData(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg);

MRESULT CopyImageData(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg);

MRESULT CopyImageData_U8ToU16(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg);

MRESULT CopyImageData_UV(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg);

MRESULT CopyImageData_Y(LPASVLOFFSCREEN pDstImg, LPASVLOFFSCREEN pSrcImg);

MVoid Set_OffscreenMemory_Init(LPASVLOFFSCREEN pSrcImg);

MRESULT SetOffscreenMemory(MByte* bBufData, MInt32 lWidth, MInt32 lHeight, MInt32 lPAF, LPASVLOFFSCREEN pImgOut, LPASVLOFFSCREEN pRefImg);

MInt32 AllocFixOffscreenMemory(MHandle hMemMgr, MInt32 lWidth, MInt32 lHeight, MInt32 lPAF, LPASVLOFFSCREEN pImgOut, MInt32 lYPitch = 0, MInt32 lUVPitch = 0);

MRESULT AllocOffscreenMemory(MHandle hMemMgr, MInt32 lWidth, MInt32 lHeight, MInt32 lPAF, LPASVLOFFSCREEN pImgOut, MInt32 lYPitch = 0, MInt32 lUVPitch = 0);

MVoid FreeOffscreenMemory(MHandle hMemMgr, LPASVLOFFSCREEN pImgIn);

MInt32 Image_Adjust(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 lPadLeft, MInt32 lPadRight, MInt32 lPadTop, MInt32 lPadBot);

MVoid CopyImageBorder(MByte *pImgBuf, MInt32 lImgHeight, MInt32 lImgWidth, MInt32 lImgPitch, MInt32 topPadding,
	MInt32 bottomPadding, MInt32 leftPadding, MInt32 rightPadding);

// box blur & fast gaussian blur
typedef MBool (*PBoxBlurSumLine)(MByte *pSrc, MInt32 nWidth, MDWord *pSum, MBool bAdd);

typedef MBool (*PBoxBlurProcessRow)(MDWord *pSumBuf, MByte *pDst, MInt32 nWidth, MInt32 nLeft, MInt32 nRight, MDWord *pDivBuff);


MRESULT Box_Filter_C1_10bit(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
	MInt32 lSrcLineBytes, MByte *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius);
// box blur
MRESULT ImageBoxBlur(MHandle hMemMgr, MByte *pSrcImage, MInt32 nWidth, MInt32 nHeight, MInt32 nSrcPitch, 
					 MInt32 lPAF, MByte *pDstImage, MInt32 nDstPitch, MInt32 nLeft, MInt32 nTop, MInt32 nRight, MInt32 nBottom);

// gaussian blur with 3 times box blur to improve performance
MRESULT ImageGaussBlur_3BB(MHandle hMemMgr, MByte *pSrcImage, MInt32 nWidth, MInt32 nHeight, MInt32 nSrcPitch, 
						   MInt32 lPAF, MByte *pDstImage, MInt32 nDstPitch, MInt32 nBlurSize);

MInt32 multi_img_smooth_Y(MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImgs, LPASVLOFFSCREEN* pDstImgs,
	MInt32* pBuf, MInt32 lImgNum, MInt32 lDifType, MBool bShortImg);


MInt32 multi_img_smooth_downsample(MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImgs, LPASVLOFFSCREEN* pDstImgs,
	MInt32* pBuf, MInt32 lImgNum, MInt32 lScale, MInt32 lDifType, MBool bShortImg);

MInt32 ImageData_Gauss3x3_C2_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcDataC2, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstC1_01, MByte* pDstC1_02, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch, MInt16* pBuf);

MVoid ImageData_Gauss3x3_Down2_C2_C1(MHandle mcvParallelMonitor, MByte* pSrcDataC2, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstC1_01, MByte* pDstC1_02, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch, MInt16* pBuf);



MVoid ImageData_Box3x3_C2_C1(MHandle mcvParallelMonitor, MByte* pSrcDataC2, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstC1_01, MByte* pDstC1_02, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch, MInt16* pBuf);

MVoid ImageData_Box3x3_Down2_C2_C1(MHandle mcvParallelMonitor, MByte* pSrcDataC2, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstC1_01, MByte* pDstC1_02, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch, MInt16* pBuf);


MInt32 ImgOffscreen_Box3x3_DownSample4(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32* pBuf);
MInt32 ImgOffscreen_Gauss3x3_DownSample4(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32* pBuf);

MInt32 ImgOffscreen_Box3x3_DownSample4_U16ToU8(MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32* pBuf);
MInt32 ImgOffscreen_Gauss3x3_DownSample4_U16ToU8(MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32* pBuf);

MInt32 ImgOffscreen_Box3x3_DownSample2(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32* pBuf);
MInt32 ImgOffscreen_Gauss3x3_DownSample2(MHandle hMemMgr, MHandle mcvParallelMonitor, 
	LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32* pBuf, MBool bUVDownScale);

MInt32 ImageData_Gauss3x3_Down2_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcData, MInt32  lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstData, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch, MInt16* pInBuf);



MInt32 ImageData_Gauss5x5_Down4_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcData, MInt32  lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstData, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch, MInt16* pInBuf);

MInt32 ImageData_Gauss5x5_Down2_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcData, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstData, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch, MInt16* pInBuf);

MInt32 ImageData_Gauss5x5_Down2_C2_C1(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcDataC2, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstC1_01, MByte* pDstC1_02, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch, MInt16* pInBuf);

MInt32 ImgOffscreen_Box3x3_DownSample2_U16ToU8(MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32* pBuf);
MInt32 ImgOffscreen_Gauss3x3_DownSample2_U16ToU8(MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32* pBuf);

MRESULT GaussianBlur5x5_10bit(MHandle hMemMgr, MHandle mcvParallelMonitor,
	MUInt8 *pSrc, MInt32 width, MInt32 height, MInt32 srcPitch,
	MUInt8 *pDst, MInt32 dstPitch);


MRESULT GaussianBlur3x3_10bit(MHandle hMemMgr, MHandle mcvParallelMonitor, MUInt8 *src, MInt32 width, MInt32 height, MInt32 srcPitch,
	MUInt8 *dst, MInt32 dstPitch);
MInt32 compute_multiImage_dif(MHandle mcvParallelMonitor, LPASVLOFFSCREEN* pSrcImg, LPASVLOFFSCREEN* pSumDifImg, LPASVLOFFSCREEN pBufImg,
							  MInt32* pBufData, MInt32 lref_num, MInt32 image_num, MInt32 lScale);

MVoid ImageUSMSharpen_Box_Stripe(MByte* pSrcImg, MInt32 lImgWidth, MInt32 lImgHeight, MInt32 lSrcPitch,
	MByte* pDstImg, MInt32 lDstPitch, MInt32 lStartRow, MInt32 lEndRow,
	LPASVLOFFSCREEN ghostMap, MInt32 lRadius, MInt32* pWeiAmount, MInt32 *pBoxSumBuf, MBool bMReduce);


MInt32 Cross_Median_Filter_S32_3x3_ST(MInt32 *srcImg, MInt32 lHeight, MInt32 lWidth, MInt32 lPitch, MInt32 *dstImg);

MInt32 Cross_Median_Filter_3x3(MHandle hHandle, MHandle mcvParallelMonitor, MByte *srcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MByte *dstImg, MInt32 lDstPitch);
MInt32 Cross_Median_Filter_5x5(MHandle hHandle, MHandle mcvParallelMonitor, MByte *srcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, MByte *dstImg, MInt32 lDstPitch);
MInt32 Box_Median_Filter_3x3(MHandle hHandle, MHandle mcvParallelMonitor, MByte *srcImg, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcPitch, MByte *dstImg, MInt32 lDstPitch);

MInt32 BoxMedianFilter_3x3_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcDstImg, MRECT *pRoi);

MInt32 Compute_Image_Local_Mean_And_Var(MHandle hMemMgr, MHandle  mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, 
	LPASVLOFFSCREEN pMeanImg, LPASVLOFFSCREEN pVarImg, MInt32 lRadius, MBool bSqrt);

MInt32 Img_Local_Var_C1(MByte *pSrc, MByte *VarImg, MInt32 lImgWidth, MInt32 lImgHeight, MInt32 lSrcPitch, MInt32 lDstPitch, MInt32* pTmpBuf, MInt32 lRadius, MBool bSqrt);

MInt32 Remove_Singular_Point_By_CMed5x5(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcDstImg, LPASVLOFFSCREEN pMean_Img, LPASVLOFFSCREEN pVar_Img);

MInt32 Img_Bad_Point_Remove_3x3(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrc_Img, LPASVLOFFSCREEN pDst_Img);

MRESULT Box_Filter_C1(MHandle hMemMgr, MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
	MInt32 lSrcLineBytes, MByte *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius);

MRESULT Box_Filter_C1_ST(MHandle hMemMgr, MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
	MInt32 lSrcLineBytes, MByte *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius);

MRESULT Box_Filter_C2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte *pSrcBuf, MInt32 lWidth, MInt32 lHeight,
	MInt32 lSrcLineBytes, MByte *pDstBuf, MInt32 lDstLineBytes, MInt32 lRadius);

MRESULT MedianBlur_8u(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
	MInt32 width, MInt32 height, MInt32 size, MInt32 cn, MInt32 lCPUNum);

MRESULT MedianBlur_16u(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
	MInt32 width, MInt32 height, MInt32 size, MInt32 cn, MInt32 lCPUNum);

MInt32 MedianBlur_8u_C1_5x5_Sim(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
	MInt32 width, MInt32 height);

MVoid ImageFormat8Bit_ShortBit(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 lShiftVal);
MVoid ImageFormatShortBit_8Bit(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 lShiftVal);


MVoid ImageFormat8Bit_10Bit(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MBool bCopyUV = MTrue);

MVoid ImageFormat10Bit_8Bit(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MBool bCopyUV = MTrue);

MVoid ImageFormat10Bit_14Bit(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);

MVoid ImageFormat10Bit_8Bit_2(MByte *pSrcData, MInt32 lWidth, MInt32 lHeight, MInt32 lPitch, LPASVLOFFSCREEN pDstImg);

MVoid DataFormat10Bit_8Bit(MUInt16 *pSrcData, MInt32 lWidth, MInt32 lHeight, MInt32 lSrcPitch,
	MByte* pDstData, MInt32 lDstPitch);

MInt32 image_gauss_downscale2_5x5_8Bit(MByte *pLarge, MInt32 lLWidth, MInt32 lLHeight, MInt32 lLPitch,
	MByte *pSmall, MInt32 lSmWidth, MInt32 lSmHeight, MInt32 lSmPitch, MInt32 cn, MInt32 *convbuf);

MInt32 dif_gauss_expand_add_8Bit(MByte *pLarge, MInt32 lLWidth, MInt32 lLHeight, MInt32 lLPitch,
	MByte *pRef, MInt32 lRefWidth, MInt32 lRefHeight, MInt32 lRefpitch, MByte *pDe, MInt32 lDePitch, MInt32* expand_size, MInt32 cn, MInt32 *convbuf);

MInt32 image_gauss_downscale2_5x5_10Bit(MUInt16 *pLarge, MInt32 lLWidth, MInt32 lLHeight, MInt32 lLPitch,
	MUInt16 *pSmall, MInt32 lSmWidth, MInt32 lSmHeight, MInt32 lSmPitch, MInt32 cn, MInt32 *convbuf);

MInt32 dif_gauss_expand_add_10Bit(MUInt16 *pLarge, MInt32 lLWidth, MInt32 lLHeight, MInt32 lLPitch,
	MUInt16 *pRef, MInt32 lRefWidth, MInt32 lRefHeight, MInt32 lRefpitch, MUInt16 *pDe, MInt32 lDePitch, MInt32* expand_size, MInt32 cn, MInt32 *convbuf);

MInt32 img_guass5x5_down_sample_10Bit(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 *convbuf, MInt32 convbufSize);

MVoid Conve_FloatData_To_Img_Normal(MFloat *fSrcImgData, LPASVLOFFSCREEN pDstImg, MInt32 lfPitch, MBool bShortImg);

MVoid Conve_Img_To_FloatData_Normal(LPASVLOFFSCREEN pSrcImg, MFloat *fDstImgData, MInt32 lfPitch, MBool bShortImg);

MVoid Conve_YUVImg_To_FloatData_Normal(LPASVLOFFSCREEN pSrcImg, MFloat *fDstImgData, MInt32 lfPitch, MBool bShortImg);

#ifdef __cplusplus
}
#endif

#endif // _IMAGE_BASE_H_
