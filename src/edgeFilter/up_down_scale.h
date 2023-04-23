#ifndef _ARCSOFT_DOWNSCALE_H_
#define _ARCSOFT_DOWNSCALE_H_

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

#define NH_OMP_THREAD_NUM (4)

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

#ifndef MAX
#define MAX(a,b)	((a) < (b) ? (b) : (a))
#endif

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

#ifndef	ABS
#define ABS(a)		(((a) < 0) ? -(a) : (a))
#endif

#ifndef TRIMBYTE
#define TRIMBYTE(x)	(MByte)( (x) < 0 ? 0  : ( (x) > 255 ? 255 : (x) ) )
#endif

#ifndef TRIM_10B
#define TRIM_10B(x)	(MUInt16)((x)<0?0:((x)>1023?1023:(x)))
#endif

#ifndef TRIM_16B
#define TRIM_16B(x)	(MUInt16)((x)<0?0:((x)>65535?65535:(x)))
#endif

// 3x3
MInt32 Img_Guass3x3_Down2_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MInt32 cn);

MInt32 Img_Guass3x3_Down2(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch, MInt32 cn);

MInt32 Img_Guass3x3_Up2_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);

// 5x5
MInt32 Img_Guass5x5_Down4(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* pSrcImg, MInt32 lSrcWidth, MInt32 lSrcHeight, MInt32 lSrcPitch,
	MByte* pDstImg, MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstPitch);

MInt32 Img_Guass5x5_Down2_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MUInt16** tmpBuf);

MRESULT Img_Guass5x5_Up2_u8(MHandle hMemMgr, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);

MRESULT Img_Guass5x5_Up2AndSub_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSmallSrcImg, LPASVLOFFSCREEN pLargeRefImg, LPASVLOFFSCREEN pLargeDstImg);

//
MInt32 Fast_Bilinear_Upscale2_8UC2(MHandle mcvParallelMonitor, MByte* srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch, MByte* dstBuf, MInt32 dstW,
	MInt32 dstH, MInt32 dstPitch);

MVoid Img_BilinearUp2_Scale_F32(MFloat* pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
	MFloat* pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst, MFloat fScale);

MVoid Img_MeanDown2_Scale_F32(MFloat* pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
	MFloat* pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst, MFloat fScale);

MVoid Img_BilinearUp2_U16_C1(MUInt16* pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
	MUInt16* pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst);

MVoid Img_BilinearUp4_U16_C1(MUInt16* pSrcImg, MInt32 lWidthSrc, MInt32 lHeigthSrc, MInt32 lPitchSrc,
	MUInt16* pDstImg, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 lPitchDst);

MInt32 Image_Mean_Down2_C1_C2(MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lPitchSrc, MInt32 lWidthSrc, MInt32 lHeightSrc,
	MByte* pDst, MInt32 lPitchDst, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 cn);

MInt32 Image_Mean_Down4_C1_C2(MHandle mcvParallelMonitor, MByte* pSrc, MInt32 lPitchSrc, MInt32 lWidthSrc, MInt32 lHeightSrc,
	MByte* pDst, MInt32 lPitchDst, MInt32 lWidthDst, MInt32 lHeightDst, MInt32 cn);

MInt32 Img_Guass3x3_s16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);

MRESULT Img_Guass5x5_Up2_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);

MRESULT Img_Guass5x5_Down2_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);

MRESULT Img_Guass5x5_Up2_sub_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pRefImg, LPASVLOFFSCREEN pDstImg);

MRESULT Img_Guass5x5_Up2_add_u16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pRefImg, LPASVLOFFSCREEN pDstImg);


// bilinear resize function
typedef struct __tag_BilinearResize_Param {
	MVoid* pBuf;		// store corresponding left-top axis(x, y) && bilinear weight
	MInt32 channelsNum;
}BilinearResize_Param;

MRESULT ImgBilinearResizeAllocMem_F32C1(MHandle hMemMgr, MInt32 srcWidth, MInt32 srcHeight,
	MInt32 dstWidth, MInt32 dstHeight, MInt32 taskNum, BilinearResize_Param* pParam);

MRESULT ImgBilinearResize_F32C1(MHandle parEngine, MInt32 taskNum,
	const MFloat* pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
	MFloat* pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch,
	const BilinearResize_Param* pBLParam);

MRESULT ImgBilinearResize_F32C1_Scale(MHandle parEngine, MInt32 taskNum,
                                      const MFloat* pSrc, MInt32 srcWidth, MInt32 srcHeight, MInt32 srcPitch,
                                      MFloat* pDst, MInt32 dstWidth, MInt32 dstHeight, MInt32 dstPitch,
                                      const BilinearResize_Param* pBLParam, MFloat fScale);

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif // _ARCSOFT_IMAGEPROC_H_
