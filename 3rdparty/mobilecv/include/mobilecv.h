/*----------------------------------------------------------------------------------------------
*
* This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary and 		
* confidential information. 
* 
* The information and code contained in this file is only for authorized ArcSoft employees 
* to design, create, modify, or review.
* 
* DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER AUTHORIZATION.
* 
* If you are not an intended recipient of this file, you must not copy, distribute, modify, 
* or take any action in reliance on it. 
* 
* If you have received this file in error, please immediately notify ArcSoft and 
* permanently delete the original and any copy of any file and any printout thereof.
*
*-------------------------------------------------------------------------------------------------*/

#ifndef _MOBILECV_H_
#define _MOBILECV_H_

#include "amcomdef.h"
#include "asvloffscreen.h"

#ifdef FORDLLEXPORT
#define MCV_API __declspec(dllexport)
#else
#define MCV_API
#endif


#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
/// @brief
///  Defines operational mode of interface to allow the end developer to dictate how the target optimized implementation should behave.
//------------------------------------------------------------------------------
typedef enum
{
   /// Target-optimized implementation uses all CPU highest performance implementation.
   MCV_OP_ALL_CPU_PERFORMANCE = 0,
   
   /// Target-optimized implementation uses Single CPU highest performance implementation.
   MCV_OP_SINGLE_CPU_PERFORMANCE = 1,
   
   /// Target-optimized implementation uses highest performance implementation.All cpu added on GPU and DSP.
   MCV_OP_PERFORMANCE     = 2,

   /// Target-optimized implementation offloads as much of the CPU as possible.Only loaded on GPU and DSP.
   MCV_OP_CPU_OFFLOAD     = 3,

   /// Target-optimized implementation uses lowest power consuming implementation.Loaded on DSP.
   MCV_OP_LOW_POWER       = 4,
   
   /// Values >= 0x80000000 are reserved
   MCV_OP_RESERVED        = 0x80000000
   
} mcvOperationMode;


typedef enum 
{
   /// Bilinear interpolation
   MCV_INTERPOLATION_MODE_YBILINEAR_UVNEAREST = 0,
   
   /// Bilinear interpolation
   MCV_INTERPOLATION_MODE_BILINEAR,

   /// Nearest neighbor interpolation
   MCV_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
   
   /// Interpolation by area
   MCV_INTERPOLATION_MODE_AREA
} mcvInterpolationMode;

typedef MVoid (*MPVoidProc)(MVoid* lpPara); 
#define MWAIT_INFINITE		(~0)

typedef struct 
{
	MInt32 alpha;
}mcvColorExtParam;
//---------------------------------------------------------------------------
/// @brief
///   Selects HW units for all routines at run-time.  Can be called anytime to
///   update choice.
///
/// @param mode
///   See enum for details. Default is 0(MCV_OP_ALL_CPU_PERFORMANCE).
///
/// @return
///   0 if successful.
///   999 if minmum HW requirement not met.
///   other #'s if unsuccessful.
///
/// @ingroup misc
//---------------------------------------------------------------------------
MInt32 mcvSetOperationMode(mcvOperationMode mode );

/*************************
 * separate yuyv to planner y and planner uv
 *************************/
MInt32 mcvYUYVToOrgData(MUInt8 *pYUYVData, MLong lImgWidth, MLong lImgHeight, MUInt8 *pGreyData, MUInt8 *pCbCrData);

				  
/*************************
 * sqrt(a) for 64bits size,fast version.Accuracy:1% more or less
 *************************/
MInt32 mcvFastSqrts64(MInt64 a);

/*************************
 * sqrt(x) for 32bits size.Accuracy:1% more or less
 *************************/
MInt32 mcvFastSqrts32(MInt32 x); 

/**************************************
Fast sqrt approx.
note:fX should > 0;
**************************************/
MFloat mcvSqrtf32(MFloat fX);

/**************************************
Fast vector sqrt approx.
* pSrc	: Input singel precision numbers.
* pDst	: Output singel precision numbers.
* lLength	: The length of the numbers.
* note:
*       every element of pSrc should be > 0;
**************************************/
MInt32 mcvSqrtVectorf32(MFloat* pSrc, MFloat* pDst, MInt32 lLength);


/**************************************
Fast reciprocal sqrt approx.  return 1/sqrt(fX)
note:fX should > 0;
**************************************/
MFloat mcvInvSqrtf32(MFloat fX);

/**************************************
Fast vector reciprocal sqrt approx. 
* pSrc	: Input singel precision numbers.
* pDst	: Output singel precision numbers.
* lLength	: The length of the numbers.
* note:
*       every element of pSrc should be > 0;
**************************************/
MInt32 mcvInvSqrtVectorf32(MFloat* pSrc, MFloat* pDst, MInt32 lLength);

/**************************************
Fast vector reciprocal sqrt approx. 
* fDividend	: the dividend.
* fDivisor	:  the divisor.
* note:
*       fDivisor should be > 0;
**************************************/
MFloat mcvDivf32(MFloat fDividend, MFloat fDivisor);

/**************************************
Fast vector reciprocal sqrt approx. 
* pDividend	: the dividend vector.
* pQuotient	:  the quotient vector.
* fDivisor	:  the divisor.
* lLength	:  the length.
* note:
*       fDivisor should be > 0;
**************************************/
MInt32 mcvVectorDivf32(MFloat* pDividend,  MFloat* pQuotient, MFloat fDivisor, MInt32 lLength);

/**************************************
Performs per-element bitwise-OR operation on two 8-bit single channel images.
* pSrc1  : First Input image.
* pSrc2  : Second Input image, must has the same size as pSrc1.
* pDst  : Output image, must has the same size as pSrc1.
* iStrideSrc1 : stride of pSrc1.
* iStrideSrc2 : stride of pSrc2.
* iStrideDst  : stride of pDst.
**************************************/
MInt32 mcvBitwiseOru8(MByte* pSrc1, MByte* pSrc2, MByte* pDst, 
            MInt32 iwidth,MInt32 iheight,
            MInt32 iStrideSrc1,MInt32 iStrideSrc2,MInt32 iStrideDst);

/*************************
 * Dot product of two 8-bit vectors.
 *************************/
MLong mcvDotProducts8(MUInt8* a,MUInt8*  b,MUInt32 absize );
 
 /*************************
 * Counts vectors "1" bits .
 *************************/
MLong mcvBitCountu8 (MUInt8*  src,MLong  srcLength);

/***********2-norm of two vectors************/
MDouble mcvVectorDiffNorm2u32(MUInt32 *vec1,MUInt32 *vec2,MInt32 len);

MDouble mcvVectorDiffNorm2s32(MInt32 *vec1,MInt32 *vec2,MInt32 len);

MDouble mcvVectorDiffNorm2f32(MFloat*vec1,MFloat *vec2,MInt32 len);

MDouble mcvVectorDiffNorm2Fasts16(MInt16 *vec1,MInt16 *vec2,MInt32 len);

MDouble mcvVectorDiffNorm2Fastu16(MUInt16 *vec1,MUInt16 *vec2,MInt32 len);

MDouble mcvVectorDiffNorm2Fasts8(MInt8 *vec1,MInt8 *vec2,MInt32 len);

MDouble mcvVectorDiffNorm2Fastu8(MUInt8 *vec1,MUInt8 *vec2,MInt32 len);
/***********End of 2-norm of two vectors************/


/***********calc of two matrixs************/
//calc M1 + M2
MInt32 mcvMatrixAddMatrix_f32(MFloat *matrixOut,MFloat *matrix1,MFloat *matrix2,MInt32 row,MInt32 column);

//calc M1 - M2
MInt32 mcvMatrixSubMatrix_f32(MFloat *matrixOut,MFloat *matrix1,MFloat *matrix2,MInt32 row,MInt32 column);

//calc  scaler x (M1)
MInt32 mcvMatrixMulScalar_f32(MFloat *matrixOut,MFloat *matrixIn,MFloat lamda,MInt32 row,MInt32 column);

//calc M1 * M2
MInt32 mcvMatrixMulMatrixRowMajor_f32(MFloat *matrixOut,MFloat *matrix1,MFloat *matrix2,MInt32 row1,MInt32 column1,MInt32 column2);
MInt32 mcvMatrixMulMatrixRowMajor_s32(MInt32 *matrixOut,MInt32 *matrix1,MInt32 *matrix2,MInt32 row1,MInt32 column1,MInt32 column2);
MInt32 mcvMatrixMulMatrixRowMajor_s64(MInt64 *matrixOut,MInt16 *matrix1,MInt32 *matrix2,MInt32 row1,MInt32 column1,MInt32 column2);
MInt32 mcvMatrixMulMatrixColMajor_f32(MFloat *matrixOut,MFloat *matrix1,MFloat *matrix2,MInt32 row1,MInt32 column1,MInt32 column2);
MInt32 mcvMatrixMulMatrixColMajor_s32(MInt32 *matrixOut,MInt32 *matrix1,MInt32 *matrix2,MInt32 row1,MInt32 column1,MInt32 column2);

//calc M1 * M2 + M3
MInt32 mcvMatrixMulAddRowMajor_f32(MFloat *matrixOut,MFloat *matrix1,MFloat *matrix2,MFloat *matrix3,MInt32 row1,MInt32 column1,MInt32 column2);

/***********End of calc of two matrixs************/

/*************************
* src  : Input grey image.
* dst  : Output grey image.
* srcWidth : src Image width.
          NOTE :  must be equal or greater than 4.
* srcHeight: src Image Height.
          NOTE :  must be equal or greater than 4.
 *************************/
MInt32 mcvScaleDownBy2u8(MUInt8*  src,MLong  srcWidth,MLong  srcHeight,MUInt8* dst );

/**************************************
*function:mcvResize
*		Resize the img  .
*input:
*   		plTmpBuf     		temp buffer. if buffer is NULL , a new temp buffer will be created internally, if handle is yes,  pls input the buffer memory.		
*   		buflength     		The length of plTmpBuf in Bytes.	must not be less than sizeof(MInt16)*((lDstWidth * 6)))
*   		pSrcImg 			src image, should not be NULL.
*   		mcvInterMode        mcvInterpolationMode, default be MCV_INTERPOLATION_MODE_YBILINEAR_UVNEAREST.
*           lStartRow           the start row of the dst image
*           lEndRow             the end row of the dst image
*output:
*   		pDstImg              	image to be resized.
*return:
*           0      ---  success                        (MCV_OK)
*           -1    ---  invalid input poniter       (MCV_NULL_POINTER)
*           -2    ---  invalid input param        (MCV_INVALID_PARAM)
*note:
*		Support Format 	ResizeNV21BilinearUVNearest
*						ResizeNV21Nearest
*						ResizeSingleComponentBilinear
*						ResizeSingleComponentNearest
*		buflength     		The length of plTmpBuf in Bytes.	must not be less than sizeof(MInt16)*((lDstWidth * 6)))
**************************************/
MInt32 mcvResize(MUInt16 *plTmpBuf,MInt32 buflength,LPASVLOFFSCREEN pSrcImg,LPASVLOFFSCREEN pDstImg, 
                 MUInt32 mcvInterMode, MInt32 lStartRow, MInt32 lEndRow);

/**************************************
  Function Description:mcvResizeYUYVToYUYVBilinear	
  	Resize one frame in YUYV format for both input and output.
  	Use bilinear interpolation for Y component and neighbor interpolation for CbCr. 
  In:
  	plTmpWidthBuf:     index value for H direction.
  			  should be (lDstWidth*3*sizeof(MInt32)+lDstWidth*2*sizeof(MUInt16)).
  	pYUYVData: YUYV input
  	lSrcLineStep: line stride of input buffer. normally lSrcWidth*2 for YUYV format.
  	lDstWidth
  	buflength: size of buffer plTmpWidthBuf, in byte unit.
  Out:
  	pDstYUYVData: output buffer.
  Note:
  	lDstWidth should be '2n',or else  we will lost last 1 column pixels.
**************************************/
MInt32 mcvResizeYUYVToYUYVBilinear(MInt32 *plTmpWidthBuf,MInt32 buflength,
						MUInt8 *pYUYVData, MInt32 lSrcWidth, MInt32 lSrcHeight, 
						MInt32 lSrcLineStep, MUInt8 *pDstYUYVData,
						MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstLineStep);

/**************************************
  Function Description:mcvResizeYUYVToLPI422HBilinear
	Resize one frame. Input frame is ASVL_PAF_YUYV format and output frame is ASVL_PAF_LPI422H format.
	Use bilinear interpolation for Y component and neighbor interpolation for CbCr.
  In:
  	plTmpBuf:     index value for H direction.
  			  should be (lDstWidth*5*sizeof(MUInt16)).
	buflength: size of buffer plTmpBuf, in byte unit.
  	srcImage: YUYV input
  	
  Out:
  	dstImage:  LPI422H output.
  Note:
  	width of dstImage should be '2n',or else  we will lost last 1 column pixels.
**************************************/
MInt32 mcvResizeYUYVToLPI422HBilinear(MUInt16 *plTmpBuf,MInt32 buflength,
                        LPASVLOFFSCREEN srcImage,LPASVLOFFSCREEN dstImage);


/**************************************
  *	Resize an image and split  to 3 plane. input :YUYV output: y... u...v...separated. 
  *  Y: bilinear interpolation. C: neighbor interpolation.
  *  plTmpBuf:at least  MInt16*(lDstWidth * 4 + lDstWidth/2) 
  buflength: size of buffer plTmpWidthBuf, in byte unit.
  **************************************/
MInt32 mcvResizeYUYVToI422HBilinearY(MUInt16 *plTmpBuf,MInt32 buflength,
						MUInt8 *pSrcYUYV, MInt32 lSrcWidth, MInt32 lSrcHeight, 
						MInt32 lSrcLineStep, MUInt8 *pDstY, MUInt8 *pDstCb, MUInt8 *pDstCr,
						MInt32 lDstWidth, MInt32 lDstHeight, 
						MInt32 lDstStrideY,MInt32 lDstStrideCb,MInt32 lDstStrideCr);
						
/**************************************
  *	Resize an image and split  to 3 plane. input :YUYV output: y... u...v...separated. 
  *  Y: bilinear interpolation. C: neighbor interpolation.
  *  plTmpBuf:at least  MInt16*(lDstWidth * 4 + lDstWidth/2) 
  buflength: size of buffer plTmpWidthBuf, in byte unit.
  **************************************/
MInt32 mcvResizeYUYVToI420BilinearY(MUInt16 *plTmpBuf,MInt32 buflength,
						MUInt8 *pSrcYUYV, MInt32 lSrcWidth, MInt32 lSrcHeight, 
						MInt32 lSrcLineStep, MUInt8 *pDstY, MUInt8 *pDstCb, MUInt8 *pDstCr,
						MInt32 lDstWidth, MInt32 lDstHeight, 
						MInt32 lDstStrideY,MInt32 lDstStrideCb,MInt32 lDstStrideCr);



/**************************************
  Function Description:
  	bilinear interpolation for one line in YUYV format for input and output with Y format.
**************************************/
MInt32 mcvResizeYUYVToYBilinear(MInt32 *plTmpWidthBuf,MInt32 buflength,
						MUInt8 *pYUYVData, MInt32 lSrcWidth, MInt32 lSrcHeight, 
						MInt32 lSrcLineStep, MUInt8 *pDstYData, 
						MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstLineStep);



/**************************************
  *	Resize an image . no matter it is y or u or v.
  *  nearest neighbor interpolation. C: neighbor interpolation.
  *  plTmpBuf:at least MInt16*(lDstWidth * 5) 
  *  buflength: 
  	size of buffer plTmpWidthBuf, in byte unit.
  	used to check if plTmpBuf is valid!
  **************************************/
MInt32 mcvResizeSingleComponentNearest(MUInt16 *plTmpBuf,MInt32 buflength,
                                        MUInt8 *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, 
                                        MInt32 lSrcStride, MUInt8 *pDst,MInt32 lDstWidth, 
                                        MInt32 lDstHeight, MInt32 lDstStride);

/**************************************
  *	Resize an image . no matter it is y or u or v.
  *  bilinear interpolation. C: neighbor interpolation.
  *  plTmpBuf:at least MInt16*(lDstWidth * 5) 
  *  buflength: 
  	size of buffer plTmpWidthBuf, in byte unit.
  	used to check if plTmpBuf is valid!
  **************************************/
MInt32 mcvResizeSingleComponentBilinear(MUInt16 *plTmpBuf,MInt32 buflength,
                        MUInt8 *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, 
						MInt32 lSrcStride, MUInt8 *pDst,MInt32 lDstWidth, 
						MInt32 lDstHeight, MInt32 lDstStride);

/**************************************
  Function Description:
  	Resize the an image of ASVL_PAF_NV21 format and convert to ASVL_PAF_LPI422H format.
  	bilinear for Y,nearest for UV
  In:
  	plTmpBuf  :     tmp buffer allocated outside.
  			  must not be less than sizeof(MInt16)*((lDstWidth << 2)+(lDstWidth>>1))
  	buflength  : The length of plTmpBuf in Bytes.
  	srcImage  :  input image descriptor
  Out:
  	dstImage: output image descriptor.
  Note:
	1. Width and Height of both input and output image should be even and greater than 2.
	1. plTmpBuf should not be null.

**************************************/
MInt32 mcvResizeNV21ToLPI422HBilinear(MUInt16 *plTmpBuf,MInt32 buflength,
						LPASVLOFFSCREEN srcImage,LPASVLOFFSCREEN dstImage);

/**************************************
  Function Description:
    Resize the an image of ASVL_PAF_NV21 format and convert to ASVL_PAF_LPI420 format.
    bilinear for Y,nearest for UV
  In:
    plTmpBuf  :     tmp buffer allocated outside.
          must not be less than sizeof(MInt16)*((lDstWidth << 2))
    buflength  : The length of plTmpBuf in Bytes.
    pSrcY  :  input src Y data.
    pSrcUV  :  input src UV data.
  Out:
    pDstY : Output dest Y data.
    pDstU: Output dest U data.
    pDstV: Output dest V data.
  Note:
  1. Width and Height of both input and output image should be even and greater than 2.
  2. plTmpBuf should not be null.

**************************************/
MUInt32 mcvResizeNV21ToI420Bilinear(MUInt16 *plTmpBuf, MUInt32 buflength,
     MUInt8 *pSrcY, MUInt32 lSrcStrideY,  MUInt8* pSrcUV, MUInt32 lSrcStrideUV, 
     MUInt32 lSrcWidth, MUInt32 lSrcHeight, 
     MUInt8 *pDstY, MUInt32 lDstStrideY, MUInt8 *pDstU, MUInt32 lDstStrideU, MUInt8 *pDstV, MUInt32 lDstStrideV,
     MUInt32 lDstWidth, MUInt32 lDstHeight);

/**************************************
  Function Description:
    Resize the an image of ASVL_PAF_LPI422H format and convert to ASVL_PAF_LPI420 format.
    bilinear for Y,nearest for UV
  In:
    plTmpBuf  :     tmp buffer allocated outside.
          must not be less than sizeof(MInt16)*((lDstWidth << 2))
    buflength  : The length of plTmpBuf in Bytes.
    pSrcY  :  input src Y data.
    pSrcUV  :  input src UV data.
  Out:
    pDstY : Output dest Y data.
    pDstU: Output dest U data.
    pDstV: Output dest V data.
  Note:
  1. Width and Height of both input and output image should be even and greater than 2.
  2. plTmpBuf should not be null.

**************************************/
MUInt32 mcvResizeLPI422HToI420Bilinear(MUInt16 *plTmpBuf, MUInt32 buflength,
    MUInt8 *pSrcY, MUInt32 lSrcStrideY, MUInt8* pSrcUV, MUInt32 lSrcStrideUV, 
    MUInt32 lSrcWidth, MUInt32 lSrcHeight, 
    MUInt8 *pDstY, MUInt32 lDstStrideY, MUInt8 *pDstU, MUInt32 lDstStrideU, MUInt8 *pDstV, MUInt32 lDstStrideV,
    MUInt32 lDstWidth, MUInt32 lDstHeight);

/**************************************
  Function Description:
  	bilinear interpolation for ASVL_PAF_NV21 format.
  	bilinear for Y,nearest for VU
  In:
  	plTmpBuf:     tmp buffer allocated outside.
  			  must not be less than sizeof(MInt16)*((lDstWidth * 6)))
  	buflength  : The length of plTmpBuf in Bytes.
  Out:
  	pDst: output buffer pointer.
  Note:
	1. lDstWidth and lDstHeight should be even and greater than 2.
	1. plTmpBuf,pSrc,pDst should not be null.

**************************************/
MInt32 mcvResizeNV21Bilinear(MUInt16 *plTmpBuf,MInt32 buflength,
						     LPASVLOFFSCREEN srcImage,LPASVLOFFSCREEN dstImage);

/**************************************
  Function Description:
  	bilinear interpolation for ASVL_PAF_NV21 format.
  	bilinear for Y,nearest for VU
  In:
  	plTmpBuf:     tmp buffer allocated outside.
  			  must not be less than sizeof(MInt16)*((lDstWidth << 2)+(lDstWidth>>1))
  	buflength  : The length of plTmpBuf in Bytes.
  Out:
  	pDst: output buffer pointer.
  Note:
	1. lDstWidth and lDstHeight should be even and greater than 2.
	1. plTmpBuf,pSrc,pDst should not be null.

**************************************/
MInt32 mcvResizeNV21Bilinear_v1(MUInt16 *plTmpBuf,MInt32 buflength,
						MUInt8 *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, 
						MInt32 lSrcStride, MUInt8 *pDst, 
						MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstStride);
MInt32 mcvResizeNV21Nearest(MUInt16 *plTmpBuf,MInt32 buflength,
						     LPASVLOFFSCREEN srcImage,LPASVLOFFSCREEN dstImage);

/**************************************
  Function Description:
  	bilinear interpolation for ASVL_PAF_NV12 format.
  	bilinear for Y,nearest for UV
  In:
  	plTmpBuf:    tmp buffer allocated outside.
  			  must not be less than sizeof(MInt16)*((lDstWidth << 2)+(lDstWidth>>1))
  	buflength  : The length of plTmpBuf in Bytes.
  Out:
  	pDst: output buffer pointer.
  Note:
	1. lDstWidth and lDstHeight should be even and greater than 2.
	1. plTmpBuf,pSrc,pDst should not be null.

**************************************/
MInt32 mcvResizeNV12Bilinear(MUInt16 *plTmpBuf,MInt32 buflength,
						MUInt8 *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, 
						MInt32 lSrcStride, MUInt8 *pDst, 
						MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstStride);

/**************************************
  Function Description:
  	bilinear interpolation for ASVL_PAF_I420 format.
  	bilinear for Y,nearest for U and V
  In:
  	plTmpBuf:    tmp buffer allocated outside.
  			  must not be less than sizeof(MInt16)*((lDstWidth << 2)+(lDstWidth>>1))
  	buflength  : The length of plTmpBuf in Bytes.
  Out:
  	pDst: output buffer pointer.
  Note:
	1. lDstWidth and lDstHeight should be even and greater than 2.
	1. plTmpBuf,pSrc,pDst should not be null.

**************************************/
MInt32 mcvResizeI420Bilinear(MUInt16 *plTmpBuf,MInt32 buflength,
                             LPASVLOFFSCREEN srcImage,LPASVLOFFSCREEN dstImage);

/**************************************
* Resize rectangle region of YUYV image to I422H Y U V channel down samply by 2.
* pSrc			 :  Source image with YUYV format.
* lSrcStep		 : 	Line step of source image.
* lSrcWidth		 : 	Width of source image.
* lSrcHeight  	 :	Height of source image.
* pDstY		 	 :	Output dest Y data.
* lDstYStep		 :	Line step of dest Y.
* pDstU		 	 :	Output dest U data.
* lDstUStep		 :	Line step of dest U.
	Note : lDstYStep, lDstUStep should be euqual.
* pDstV		  	 :	Output dest V data.
* lDstVStep		 :	Line step of dest V.
	Note : lDstVStep should be double size of lDstYStep and lDstUStep.
* roi			 :  The process rectangle region.
**************************************/
MInt32 mcvResizeYUYVtoI422HDownSampleby2WithRect(MByte* pSrc, MLong lSrcStep, MLong lSrcWidth, MLong lSrcHeight, 
	MByte* pDstY, MLong lDstYStep, MByte* pDstU, MLong lDstUStep, MByte* pDstV, MLong lDstVStep, MRECT *roi);


/**************************************
* Resize YUYV image to I422H Y U V channel down sample by 2.
* pSrc			 :  Source image with YUYV format.
* lSrcStep		 : 	Line step of source image.
* lSrcWidth		 : 	Width of source image.
* lSrcHeight  	 :	Height of source image.
* pDstY		 	 :	Output dest Y data.
* lDstYStep		 :	Line step of dest Y.
* pDstU		 	 :	Output dest U data.
* lDstUStep		 :	Line step of dest U.
	Note : lDstYStep, lDstUStep should be euqual.
* pDstV		  	 :	Output dest V data.
* lDstVStep		 :	Line step of dest V.
	Note : lDstVStep should be double size of lDstYStep and lDstUStep.
**************************************/
MInt32 mcvResizeYUYVtoI422HDownSampleby2(MByte* pSrc, MLong lSrcStep, MLong lSrcWidth, MLong lSrcHeight, 
	MByte* pDstY, MLong lDstYStep, MByte* pDstU, MLong lDstUStep, MByte* pDstV, MLong lDstVStep);


/**************************************
  Function Description:
  	bilinear interpolation and color transformation for ASVL_PAF_NV21 to YUYV .
  	bilinear for Y,nearest for VU
  In:
  	pSrcNV21:
  				the source img
  	plTmpBuf:   
  				tmp buffer allocated outside.
  			  	must not be less than sizeof(MInt16)*((lDstWidth << 2)+(lDstWidth>>1))
  	buflength: 
  				The length of plTmpBuf in Bytes.
  Out:
  	pDstYUYV:
  				the dst img
  Note:
	1. pSrcNV21,plTmpBuf,pDstYUYV should not be null.

**************************************/
MInt32 mcvResizeNV21toYUYVBilinear(LPASVLOFFSCREEN pSrcNV21, 
								   LPASVLOFFSCREEN pDstYUYV, 
								   MUInt16 *plTmpBuf, MInt32 buflength);

/**************************************
  Function Description:
  	bilinear interpolation for ASVL_PAF_RGB24_R8G8B8 .
  In:
  	plTmpBuf:   
  				tmp buffer allocated outside.
  			  	must not be less than sizeof(MUInt16)*(lDstWidth << 3)
  	buflength:      The length of plTmpBuf in Bytes.
  	pSrc       :       the source RGB888 image
  	lSrcWidth       :       the source RGB888 image width in [pixel] unit.
  	lSrcHeight       :       the source RGB888 image height in [pixel] unit
  	lSrcStride       :       the source RGB888 image line stride in [byte] unit
  	lDstWidth       :       the dst RGB888 image width in [pixel] unit.
  	lDstHeight       :       the dst RGB888 image height in [pixel] unit
  	lDstStride       :       the dst RGB888 image line stride in [byte] unit
  Out:
  	pDstYUYV:
  				the dst img
  Note:
	1. pSrc,plTmpBuf,pDst should not be null.
	2. lSrcStride >= 3*lSrcWidth,lDstStride >= 3*lDstWidth
	3. the size of plTmpBuf should not less than (lDstWidth<<3)*sizeof(MUInt16)

**************************************/
MInt32 mcvResizeRGB888Bilinear(MUInt16 *plTmpBuf,MInt32 buflength,
                        MUInt8 *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, 
                        MInt32 lSrcStride, MUInt8 *pDst, 
                        MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstStride);
						
/**************************************
  Function Description:
  	bilinear interpolation for RGBA8888  .
  In:
  	plTmpBuf:   
  				tmp buffer allocated outside.
  			  	must not be less than sizeof(MUInt16)*(lDstWidth *10)
  	buflength:      The length of plTmpBuf in Bytes.
  	pSrc       :       the source RGBA8888 image
  	lSrcWidth       :       the source RGBA8888 image width in [pixel] unit.
  	lSrcHeight       :       the source RGBA8888 image height in [pixel] unit
  	lSrcStride       :       the source RGBA8888 image line stride in [byte] unit
  	lDstWidth       :       the dst RGBA8888 image width in [pixel] unit.
  	lDstHeight       :       the dst RGBA8888 image height in [pixel] unit
  	lDstStride       :       the dst RGBA8888 image line stride in [byte] unit
  Out:
  	pDst:
  				the dst img
  Note:
	1. pSrc,plTmpBuf,pDst should not be null.
	2. lSrcStride >= 4*lSrcWidth,lDstStride >= 4*lDstWidth
	3. the size of plTmpBuf should not less than (lDstWidth*10)*sizeof(MUInt16)

**************************************/
MInt32 mcvResizeRGBA8888Bilinear(MUInt16 *plTmpBuf,MInt32 buflength,
                        MUInt8 *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, 
                        MInt32 lSrcStride, MUInt8 *pDst, 
                        MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstStride);

/**************************************
  Function Description:
    bilinear interpolation for RGBA8888  .
  In:
    plTmpBuf:   
          tmp buffer allocated outside.
            must not be less than sizeof(MUInt16)*(lDstWidth *10)
    buflength:      The length of plTmpBuf in Bytes.
    pSrc       :       the source RGBA8888 image
    lSrcWidth       :       the source RGBA8888 image width in [pixel] unit.
    lSrcHeight       :       the source RGBA8888 image height in [pixel] unit
    lSrcStride       :       the source RGBA8888 image line stride in [byte] unit
    lDstWidth       :       the dst RGBA8888 image width in [pixel] unit.
    lDstHeight       :       the dst RGBA8888 image height in [pixel] unit
    lDstStride       :       the dst RGBA8888 image line stride in [byte] unit
    lRegionPositionX ：      the x position of region in src image to be resized
    lRegionPositionY ：      the y position of region in src image to be resized
    lRegionWidth     ：      the region width
    lRegionHeight    ：      the region height
  Out:
    pDst:
          the dst img
  Note:
  1. pSrc,plTmpBuf,pDst should not be null.
  2. lSrcStride >= 4*lSrcWidth,lDstStride >= 4*lDstWidth
  3. the size of plTmpBuf should not less than (lDstWidth*10)*sizeof(MUInt16)

**************************************/
MInt32 mcvResizeRGBA8888BilinearFromRegion(MUInt16 *plTmpBuf,MInt32 buflength,
                        MUInt8 *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, 
                        MInt32 lSrcStride, MUInt8 *pDst, 
                        MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstStride,
                        MInt32 lRegionPositionX,MInt32 lRegionPositionY,MInt32 lRegionWidth,MInt32 lRegionHeight);

/**************************************
  Function Description:
    Nearest interpolation for RGBA8888  .
  In:
    plTmpBuf:   
          tmp buffer allocated outside.
            must not be less than sizeof(MUInt16)*(lDstWidth)
    buflength:      The length of plTmpBuf in Bytes.
    pSrc       :       the source RGBA8888 image
    lSrcWidth       :       the source RGBA8888 image width in [pixel] unit.
    lSrcHeight       :       the source RGBA8888 image height in [pixel] unit
    lSrcStride       :       the source RGBA8888 image line stride in [byte] unit
    lDstWidth       :       the dst RGBA8888 image width in [pixel] unit.
    lDstHeight       :       the dst RGBA8888 image height in [pixel] unit
    lDstStride       :       the dst RGBA8888 image line stride in [byte] unit
    lRegionPositionX ：      the x position of region in src image to be resized
    lRegionPositionY ：      the y position of region in src image to be resized
    lRegionWidth     ：      the region width
    lRegionHeight    ：      the region height
  Out:
    pDst:
          the dst img
  Note:
  1. pSrc,plTmpBuf,pDst should not be null.
  2. lSrcStride >= 4*lSrcWidth,lDstStride >= 4*lDstWidth
  3. the size of plTmpBuf should not less than lDstWidth*sizeof(MUInt16)

**************************************/
MInt32 mcvResizeRGBA8888NearestFromRegion(MUInt16 *plTmpBuf,MInt32 buflength,
                        MUInt8 *pSrc, MInt32 lSrcWidth, MInt32 lSrcHeight, 
                        MInt32 lSrcStride, MUInt8 *pDst, 
                        MInt32 lDstWidth, MInt32 lDstHeight, MInt32 lDstStride,
                        MInt32 lRegionPositionX,MInt32 lRegionPositionY,MInt32 lRegionWidth,MInt32 lRegionHeight);

/**************************************
  Function Description:
    Bicubic interpolation for SingleComponent.
  In:
    plTmpBuf:   
          tmp buffer allocated outside.
            must not be less than lDstW*(6*sizeof(MInt32)).
    buflength:      The length of plTmpBuf in Bytes.
    pSrc       :       the source image
    lSrcW       :       the source image width in [pixel] unit.
    lSrcH       :       the source image height in [pixel] unit
    lSrcLB       :       the source image line stride in [byte] unit
    lDstW       :       the dst image width in [pixel] unit.
    lDstH       :       the dst image height in [pixel] unit
    lDstLB       :       the dst  image line stride in [byte] unit
  Out:
    pDst:
          the dst img
  Note:
  1. pSrc,plTmpBuf,pDst should not be null.
  2. lSrcW,lSrcH,lDstW,lDstW should  be greater than 4.
  3. the size of plTmpBuf should not be less than lDstW*(6*sizeof(MInt32))

**************************************/
MUInt32 mcvResizeSingleComponentBicubicu8(MUInt32 *plTmpBuf,MInt32 buflength, 
						MUInt8 *pSrc, MUInt32 lSrcW, MUInt32 lSrcH, MUInt32 lSrcLB, 
						MUInt8 *pDst, MUInt32 lDstW, MUInt32 lDstH, MUInt32 lDstLB);

/**************************************
  Function Description:
    Resize an NV21 image.
    Bicubic interpolation for Y.Nearest interpolation for UV.
  In:
    plTmpBuf:   
          tmp buffer allocated outside.
            must not be less than lDstW*(6*sizeof(MInt32)).
    buflength:      The length of plTmpBuf in Bytes.
    pSrcY       :       the src Y data.
    lSrcStrideY       :       the src Y line stride.
    pSrcUV       :       the src UV data.
    lSrcStrideUV       :       the src UV line stride.
    lSrcWidth       :       the src image width in [pixel] unit.
    lSrcHeight       :       the src image height in [pixel] unit.
    lDstStrideY       :       the dst Y line stride.
    lDstStrideUV       :       the dst UV line stride.
    lDstWidth       :       the dst  image line stride in [byte] unit
    lDstHeight       :       the dst  image line stride in [byte] unit
  Out:
    pDstY:            the dst Y data.  
    pDstUV:            the dst UV data.  
  Note:
  1. pSrcY,pSrcUV,plTmpBuf,pDstY,pDstUV should not be null.
  2. lSrcWidth,lSrcHeight,lDstWidth,lDstHeight should  be greater than 4.
  3. the size of plTmpBuf should not be less than lDstW*(6*sizeof(MInt32))

**************************************/
MUInt32 mcvResizeNV21Bicubicu8(MUInt32 *plTmpBuf,MInt32 buflength,
					MUInt8 *pSrcY, MUInt32 lSrcStrideY,  MUInt8* pSrcUV, MUInt32 lSrcStrideUV, MUInt32 lSrcWidth, MUInt32 lSrcHeight,
					MUInt8 *pDstY, MUInt32 lDstStrideY, MUInt8 *pDstUV, MUInt32 lDstStrideUV,  MUInt32 lDstWidth, MUInt32 lDstHeight);

/**************************************
  Function Description:
    Resize an I420 image.
    Bicubicu interpolation for Y.Nearest interpolation for UV.
  In:
    plTmpBuf:   
          tmp buffer allocated outside.
            must not be less than lDstW*(6*sizeof(MInt32)).
    buflength:      The length of plTmpBuf in Bytes.
    pSrcY       :       the src Y data.
    lSrcStrideY       :       the src Y line stride.
    pSrcU       :       the src U data.
    lSrcStrideU       :       the src U line stride.
    pSrcV       :       the src V data.
    lSrcStrideV       :       the src V line stride.
    lSrcWidth       :       the src image width in [pixel] unit.
    lSrcHeight       :       the src image height in [pixel] unit.
    lDstStrideY       :       the dst Y line stride.
    lDstStrideU       :       the dst U line stride.
    lDstStrideV       :       the dst V line stride.
    lDstWidth       :       the dst  image line stride in [byte] unit
    lDstHeight       :       the dst  image line stride in [byte] unit
  Out:
    pDstY:            the dst Y data.  
    pDstU:            the dst U data.
    pDstV:            the dst V data.    
  Note:
  1. pSrcY,pSrcU,pSrcV,plTmpBuf,pDstY,pDstU,pDstV should not be null.
  2. lSrcWidth,lSrcHeight,lDstWidth,lDstHeight should  be greater than 4.
  3. the size of plTmpBuf should not be less than lDstW*(6*sizeof(MInt32))

**************************************/
MUInt32 mcvResizeI420Bicubicu8(MUInt32 *plTmpBuf,MInt32 buflength,
	MUInt8 *pSrcY, MUInt32 lSrcStrideY,  MUInt8* pSrcU, MUInt32 lSrcStrideU,MUInt8* pSrcV, MUInt32 lSrcStrideV, MUInt32 lSrcWidth, MUInt32 lSrcHeight,
	MUInt8 *pDstY, MUInt32 lDstStrideY,	MUInt8 *pDstU, MUInt32 lDstStrideU,MUInt8 *pDstV, MUInt32 lDstStrideV,  MUInt32 lDstWidth, MUInt32 lDstHeight);

/**************************************
  Function Description:
    WarpAffine a SingleComponent Image.
  In:
    pSrc       :       the src image.
    lSrcWidth       :       the src image width in [pixel] unit.
    lSrcHeight       :      the src image height in [pixel] unit.
    lSrcLineStep       :    the src image line step.
    lDstWidth       :       the dst image width in [pixel] unit.
    lDstHeight       :      the dst image height in [pixel] unit.
    lDstLineStep       :    the dst image line step.
    rotMat       :       	the transformation matrix.
  Out:
    pDst:            the dst image.    
  Note:
  1. pSrc,pDst,rotMat should not be null.
  2. lSrcWidth,lSrcHeight,lDstWidth,lDstHeight should  be greater than 4.
  3. rotMat should be set as follows:
		rotMat_[0] = (MFloat)cos(angle*0.017453292519943295769236907684886l);
		rotMat_[1] = (MFloat)(-sin(angle*0.017453292519943295769236907684886l));
		rotMat_[2] = xoffset;
		rotMat_[3] = (MFloat)sin(angle*0.017453292519943295769236907684886l);
		rotMat_[4] = (MFloat)cos(angle*0.017453292519943295769236907684886l);
		rotMat_[5] = yoffset;
	angle: rotation angle(clockwise).
	xoffset,yoffset：translation offset. 
**************************************/
MUInt32 mcvWarpAffineSingleComponentu8(MUInt8 *pSrc,MUInt32 lSrcWidth,MUInt32 lSrcHeight,MUInt32 lSrcLineStep,
									   MUInt8 *pDst,MUInt32 lDstWidth,MUInt32 lDstHeight,MUInt32 lDstLineStep,MFloat *rotMat);

/**************************************
  Function Description:
    WarpAffine a RGBA8888 Image.
  In:
    pSrc       :       the src image.
    lSrcWidth       :       the src image width in [pixel] unit.
    lSrcHeight       :      the src image height in [pixel] unit.
    lSrcLineStep       :    the src image line step.
    lDstWidth       :       the dst image width in [pixel] unit.
    lDstHeight       :      the dst image height in [pixel] unit.
    lDstLineStep       :    the dst image line step.
    rotMat       :       	the transformation matrix.
  Out:
    pDst:            the dst image.    
  Note:
  1. pSrc,pDst,rotMat should not be null.
  2. lSrcWidth,lSrcHeight,lDstWidth,lDstHeight should  be greater than 4.
  3. rotMat should be set as follows:
		rotMat_[0] = (MFloat)cos(angle*0.017453292519943295769236907684886l);
		rotMat_[1] = (MFloat)(-sin(angle*0.017453292519943295769236907684886l));
		rotMat_[2] = xoffset;
		rotMat_[3] = (MFloat)sin(angle*0.017453292519943295769236907684886l);
		rotMat_[4] = (MFloat)cos(angle*0.017453292519943295769236907684886l);
		rotMat_[5] = yoffset;
	angle: rotation angle(clockwise).
	xoffset,yoffset：translation offset. 
**************************************/
MUInt32 mcvWarpAffineRGBA8888u8(MUInt8 *pSrc,MUInt32 lSrcWidth,MUInt32 lSrcHeight,MUInt32 lSrcLineStep,
									   MUInt8 *pDst,MUInt32 lDstWidth,MUInt32 lDstHeight,MUInt32 lDstLineStep,MFloat *rotMat);

/**************************************
  Function Description:
    WarpAffine a NV21 Image.
  In:
    pSrcY        :       the src Y data.
    lSrcStrideY  :       the src Y line stride.
    pSrcUV       :       the src UV data.
    lSrcStrideUV :       the src UV line stride.
    lSrcWidth    :       the src image width in [pixel] unit.
    lSrcHeight   :       the src image height in [pixel] unit.
    lDstStrideY  :       the dst Y line stride.
    lDstStrideUV :       the dst UV line stride.
    lDstWidth    :       the dst image width in [pixel] unit.
    lDstHeight   :       the dst image height in [pixel] unit.
    rotMat       :       the transformation matrix.
  Out:
    pDstY        :        the dst Y data.    
    pDstUV       :        the dst UV data.  
  Note:
  1. pSrcY,pSrcUV,pDstY,pDstUV,rotMat should not be null.
  2. lSrcWidth,lSrcHeight,lDstWidth,lDstHeight should  be greater than 4.
  3. rotMat should be set as follows:
		rotMat_[0] = (MFloat)cos(angle*0.017453292519943295769236907684886l);
		rotMat_[1] = (MFloat)(-sin(angle*0.017453292519943295769236907684886l));
		rotMat_[2] = xoffset;
		rotMat_[3] = (MFloat)sin(angle*0.017453292519943295769236907684886l);
		rotMat_[4] = (MFloat)cos(angle*0.017453292519943295769236907684886l);
		rotMat_[5] = xoffset;
	angle: rotation angle(clockwise).
	xoffset,yoffset：translation offset. 
**************************************/
MUInt32 mcvWarpAffineNV21u8(MUInt8 *pSrcY, MUInt32 lSrcStrideY,  MUInt8* pSrcUV, MUInt32 lSrcStrideUV, MUInt32 lSrcWidth, MUInt32 lSrcHeight,
						    MUInt8 *pDstY, MUInt32 lDstStrideY, MUInt8 *pDstUV, MUInt32 lDstStrideUV,  MUInt32 lDstWidth, MUInt32 lDstHeight,MFloat *rotMat);

/**************************************
  Function Description:
    WarpAffine an I420 Image.
  In:
    pSrcY        :       the src Y data.
    lSrcStrideY  :       the src Y line stride.
    pSrcU       :       the src U data.
    lSrcStrideU :       the src U line stride.
    pSrcV       :       the src V data.
    lSrcStrideV :       the src V line stride.
    lSrcWidth    :       the src image width in [pixel] unit.
    lSrcHeight   :       the src image height in [pixel] unit.
    lDstStrideY  :       the dst Y line stride.
    lDstStrideU :       the dst U line stride.
    lDstStrideV :       the dst V line stride.
    lDstWidth    :       the dst image width in [pixel] unit.
    lDstHeight   :       the dst image height in [pixel] unit.
    rotMat       :       the transformation matrix.
  Out:
    pDstY        :        the dst Y data.    
    pDstU        :        the dst U data. 
    pDstV        :        the dst V data.   
  Note:
  1. pSrcY,pSrcU,pSrcV,pDstY,pDstU,pSrcV,rotMat should not be null.
  2. lSrcWidth,lSrcHeight,lDstWidth,lDstHeight should  be greater than 4.
  3. rotMat should be set as follows:
		rotMat_[0] = (MFloat)cos(angle*0.017453292519943295769236907684886l);
		rotMat_[1] = (MFloat)(-sin(angle*0.017453292519943295769236907684886l));
		rotMat_[2] = xoffset;
		rotMat_[3] = (MFloat)sin(angle*0.017453292519943295769236907684886l);
		rotMat_[4] = (MFloat)cos(angle*0.017453292519943295769236907684886l);
		rotMat_[5] = xoffset;
	angle: rotation angle(clockwise).
	xoffset,yoffset：translation offset. 
**************************************/
MUInt32 mcvWarpAffineI420u8(MUInt8 *pSrcY, MUInt32 lSrcStrideY,  MUInt8* pSrcU, MUInt32 lSrcStrideU, MUInt8* pSrcV, MUInt32 lSrcStrideV,  MUInt32 lSrcWidth,MUInt32 lSrcHeight,
							MUInt8 *pDstY, MUInt32 lDstStrideY,  MUInt8 *pDstU, MUInt32 lDstStrideU, MUInt8 *pDstV, MUInt32 lDstStrideV,  MUInt32 lDstWidth, MUInt32 lDstHeight,
							MFloat *rotMat);


/**************************************
  *	Motion detect in three image：current frame，prev frame，prevprev frame.
  **************************************/
MInt32 mcvDetectMotion(ASVLOFFSCREEN* prevprev_colors[3], 
                      ASVLOFFSCREEN* prev_colors[3], ASVLOFFSCREEN* curr_colors[3], 
					  ASVLOFFSCREEN* motionimage);	

/**********************************************
Function:    mcvIntegral.
Description:
	Calc the Integral value of an image.
Input:
	pSrc
		src image,will not be changed inside this function.
	lSumPitch: in byte unit.
		next row will be :sum + lSumPitch.
Output:
	sum  
Note: 
	1.  lSrcWidth  < lSumPitch
	2.  lSrcWidth  <= lSrcPitch
	3. The 1'st row and the 1'st column of sum will always be 0
**********************************************/
MInt32 mcvIntegral(MUInt8* pSrc, MUInt32 lSrcWidth, MUInt32 lSrcHeight,
					MUInt32 lSrcPitch,MUInt32* sum,MUInt32 lSumPitch);

/**********************************************
Function:    mcvIntegral.
Description:
	Calc the Integral value of an image.
Input:
	pSrc
		src image,will not be changed inside this function.
	lSumPitch: in byte unit.
		next row will be :sum + lSumPitch.
	roi
		dst rect you want to calculate integral
Output:
	sum  
Note: 
	1.  lSrcWidth  < lSumPitch
	2.  lSrcWidth  <= lSrcPitch
	3. The 1'st row and the 1'st column of sum will always be 0
**********************************************/

MInt32 mcvIntegralWithRect(MByte* src, MLong srcstep, MInt32* sum, MLong sumstep, MLong width, MLong height, MRECT *roi);

 /*************************
 * Calculate the integral image.
 *************************/
MInt32 mcvImgIntegralu8(MUInt8* src,MLong  srcWidth,MLong  srcHeight,MLong *pIntegralImg, MLong  *pIntegralImg2);

MInt32 mcvImgIntegralu8v2(MUInt8*  src,MUInt32  srcWidth,MUInt32  srcHeight,MUInt32 srcPitch,
    MUInt32 *pIntegralImg,  MUInt32 sumPitch1, 
    MUInt32 *pIntegralImg2, MUInt32 sumPitch2);

MInt32 mcvImgIntegralu8v3(MUInt8*  src,MUInt32  srcWidth,MUInt32  srcHeight,MUInt32 srcPitch,
    MWord *pIntegralImg,  MUInt32 sumPitch1, 
    MUInt32 *pIntegralImg2, MUInt32 sumPitch2);


/*********************************************
* Calc the absolute difference of two image.
**********************************************/
MInt32 mcvGetMotionCue(MUInt8 *pCurGreyImage,MUInt8 *pPreGreyImage,
				   MInt32 lImgWidth, MInt32 lImgHeight, MInt32 lLineStep, 
				   MUInt8 *pFrameDiffImage);


/**************************************
  Function Description:
  	mcvICmCalc_Bx_By
	Original version: lack of description
  In:
	pSrcI: buffer size--->nHeight*nPitch*sizeof(MUInt8)
	pSrcJ: buffer size--->nHeight*nPitch*sizeof(MUInt8)
	nPitch
	nWidth
	nHeight
	pIxIy:buffer size--->nHeight*nIPitch*sizeof(MInt32)
	nIPitch
	nDx:range--->[0,256]
	nDy:range--->[0,256]
  Out:
	pBx: Pointer of output
	pBy: Pointer of output
  Note:
	1.pSrcI,pSrcJ,pBx,pBy should not be 0;
	2.nPitch >= nWidth+1;nPitch is in [BYTE] unit.
	3.nIPitch >= (nWidth+1)*2;nIPitch is in [MInt32] unit.
	4.nWidth >= 2;
	5.nHeight >=0;
**************************************/
MInt32 mcvICmCalc_Bx_By(MUInt8*pSrcI,MUInt8 *pSrcJ,MInt32 nPitch,
								MInt32 nWidth,MInt32 nHeight,MInt32 *pIxIy,
								MInt32 nIPitch,MInt32 nDx,MInt32 nDy,
								MInt32 *pBx,MInt32 *pBy);

/**************************************
  Function Description:
  	mcvIcmCalc_Bx_By_Gxx_Gxy_Gyy,just like mcvICmCalc_Bx_By
	Original version: lack of description
  In:
	pSrcI: buffer size--->nHeight*nPitch*sizeof(MUInt8)
	pSrcJ: buffer size--->nHeight*nPitch*sizeof(MUInt8)
	nPitch
	nWidth
	nHeight
	pIxIy:buffer size--->nHeight*nIPitch*sizeof(MInt32)
	nIPitch
	nDx:range--->[0,256]
	nDy:range--->[0,256]
  Out:
	pBx,pBy,pGxx,pGxy,pGyy: Pointer of output
  Note:
	1.pSrcI,pSrcJ,pBx,pBy should not be 0;
	2.nPitch >= nWidth+1;nPitch is in [BYTE] unit.
	3.nIPitch >= (nWidth+1)*2;nIPitch is in [MInt32] unit.
	4.nWidth >= 2;
	5.nHeight >=0;
**************************************/
MInt32 mcvIcmCalc_Bx_By_Gxx_Gxy_Gyy(MUInt8 *pSrcI,MUInt8 *pSrcJ,MInt32 nPitch,MInt32 nWidth,MInt32 nHeight,
		MInt32 *pIxIy,MInt32 nIPitch,MInt32 nDx,MInt32 nDy,MInt32 *pBx,MInt32 *pBy,MInt32 *pGxx,MInt32 *pGxy,MInt32 *pGyy);

/**************************************
* Extract Y component from YUYV fromat.
* pDst : width*height valid, buffer size is lDstStride*height
* width: should be 2n.
**************************************/
MInt32 mcvExtract_Y_From_YUYV(MUInt8 *pSrc,MUInt8 *pDst,MUInt32 width,MUInt32 height,MUInt32 lSrcStride,MUInt32 lDstStride);

/***************************************
*function:mcvColorRGB888toBGR565u8
* Convert RGB888 to BGR565  .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_RGB16_B5G6R5
****************************************/
MInt32 mcvColorRGB888toBGR565u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGR888toRGB565u8
* Convert BGR888 to RGB565  .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_RGB16_R5G6B5
****************************************/
MInt32 mcvColorBGR888toRGB565u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGR888toBGR565u8
* Convert BGR888 to BGR565  .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_RGB16_B5G6R5
****************************************/
MInt32 mcvColorBGR888toBGR565u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGR888toARGB8888u8
* Convert BGR888 to ARGB8888  .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_RGB32_A8R8G8B8
****************************************/
MInt32 mcvColorBGR888toARGB8888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toARGB8888u8
* Convert RGB888 to ARGB8888  .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_RGB32_A8R8G8B8
****************************************/
MInt32 mcvColorRGB888toARGB8888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toYUYVu8
* Convert RGB888 to YUYV .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_YUYV
****************************************/
MInt32 mcvColorRGB888toYUYVu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGBA8888toYUYVu8
* Convert RGBA8888 to YUYV .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB32_R8G8B8A8
*						ASVL_PAF_YUYV
****************************************/
MInt32 mcvColorRGBA8888toYUYVu8(LPASVLOFFSCREEN srcImg, LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGR888toYUYVu8
* Convert BGR888 to YUYV .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_YUYV
****************************************/
MInt32 mcvColorBGR888toYUYVu8(LPASVLOFFSCREEN srcImg, LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toNV21u8
* Convert RGB888 to NV21 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_NV21
****************************************/
MInt32 mcvColorRGB888toNV21u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGR888toNV21u8
* Convert BGR888 to NV21 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_NV21
****************************************/
MInt32 mcvColorBGR888toNV21u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);


/*********** RGB888 to other formats******************/
MInt32 mcvColorRGB888toBGR888u8(MByte* pSrc, MByte* pDst, MLong width,MLong height);

/***************************************
*function:mcvColorRGB888toBGR888u8_2
* Convert RGB888 to BGR888 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_RGB24_B8G8R8
****************************************/
MInt32 mcvColorRGB888toBGR888u8_2(LPASVLOFFSCREEN srcImg, LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toRGB565u8
* Convert RGB888 to RGB565 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_RGB16_R5G6B5
****************************************/
MInt32 mcvColorRGB888toRGB565u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toYVYUu8
* Convert RGB888 to YUYV .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_YUYV
****************************************/
MInt32 mcvColorRGB888toYVYUu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toUYVYu8
* Convert RGB888 to UYVY .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_UYVY
****************************************/
MInt32 mcvColorRGB888toUYVYu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toVYUYu8
* Convert RGB888 to VYUY .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_VYUY
****************************************/
MInt32 mcvColorRGB888toVYUYu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toYV24u8
* Convert RGB888 to YV24 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_YV24
****************************************/
MInt32 mcvColorRGB888toYV24u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toI422Hu8
* Convert RGB888 to I422H .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_I422H
****************************************/
MInt32 mcvColorRGB888toI422Hu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toNV12u8
* Convert RGB888 to NV12 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_NV12
****************************************/
MInt32 mcvColorRGB888toNV12u8(LPASVLOFFSCREEN srcImg, LPASVLOFFSCREEN dstImg);
/*********** End of RGB888 to other formats******************/

/***************************************
*function:mcvColorBGR888toYVYUu8
* Convert BGR888 to YUYV .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_YUYV
****************************************/
MInt32 mcvColorBGR888toYVYUu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGR888toUYVYu8
* Convert BGR888 to UYVY .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_UYVY
****************************************/
MInt32 mcvColorBGR888toUYVYu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGR888toVYUYu8
* Convert BGR888 to VYUY .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_VYUY
****************************************/
MInt32 mcvColorBGR888toVYUYu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGR888toI422Hu8
* Convert BGR888 to I422H .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_I422H
****************************************/
MInt32 mcvColorBGR888toI422Hu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGR888toNV12u8
* Convert BGR888 to NV12 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_NV12
****************************************/
MInt32 mcvColorBGR888toNV12u8(LPASVLOFFSCREEN srcImg, LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGBA8888toNV21u8
* Convert RGBA8888 to NV21 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB32_R8G8B8A8
*						ASVL_PAF_NV21
****************************************/
MInt32 mcvColorRGBA8888toNV21u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);


/***************************************
*function:mcvColorRGBA8888toBGR888u8
* Convert RGBA8888 to BGR888 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB32_R8G8B8A8
*						ASVL_PAF_RGB24_B8G8R8
****************************************/
MInt32 mcvColorRGBA8888toBGR888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGRA8888toNV21u8
* Convert BGRA8888 to NV21  .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB32_B8G8R8A8
*						ASVL_PAF_NV21
****************************************/
MInt32 mcvColorBGRA8888toNV21u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorNV21toBGRA8888u8
* Convert NV21 to BGRA8888  .
*input:
*   		srcImg     		src image, should not be NULL.
*           alpha           alpha value to set.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB32_B8G8R8A8
*						ASVL_PAF_NV21
****************************************/
MInt32 mcvColorNV21toBGRA8888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg, MUInt8 alpha);

/**************************************
* Convert RGBA8888 to NV12 .
* srcImg   : Input ASVL_PAF_RGB32_R8G8B8A8 format image.
* dstImg  : Output ASVL_PAF_NV12  format image.
* NOTE : 
*        Image width and height must be a multiple of 2.
**************************************/

MInt32 mcvColorRGBA8888toNV12u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGRA8888toNV12u8
* Convert BGRA8888 to NV12 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB32_B8G8R8A8
*						ASVL_PAF_NV12
****************************************/
MInt32 mcvColorBGRA8888toNV12u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/**************************************
* Convert RGBA8888 to NV21 .
* srcImg   : Input ASVL_PAF_RGB32_R8G8B8A8 format image.
* dstImg  : Output ASVL_PAF_I420  format image.
* NOTE : 
*        Image width and height must be a multiple of 2.
**************************************/
MInt32 mcvColorRGBA8888toI420u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGRA8888toI420u8
* Convert BGRA8888 to I420 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB32_B8G8R8A8
*						ASVL_PAF_I420
****************************************/
MInt32 mcvColorBGRA8888toI420u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorYUVtoNV21u8
* Convert YUV to NV21 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_YUV
*						ASVL_PAF_NV21
****************************************/
MInt32 mcvColorYUVtoNV21u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorNV21toYUVu8
* Convert NV21 to YUV .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_NV21
*						ASVL_PAF_YUV
****************************************/
MInt32 mcvColorNV21toYUVu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

MInt32 mcvColorNV41toNV21(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);
MInt32 mcvColorNV21toNV41(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);
MInt32 mcvColorNV41toNV21_2(LPASVLOFFSCREEN srcImg, LPASVLOFFSCREEN dstImg);
MInt32 mcvColorNV21toNV41_2(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/**************************************
* Convert NV21 to RGB888 .
* srcImg  : Input ASVL_PAF_NV21 format image.
* dstImg  : Output ASVL_PAF_RGB24_R8G8B8 format image.
* NOTE:
*   image width and height must be a multiple of 2 and >=2.
**************************************/
MInt32 mcvColorNV21toRGB888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);
MInt32 mcvColorNV12toBGR888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);
/**************************************
* Convert NV21 to BGR888 .
* srcImg  : Input ASVL_PAF_NV21 format image.
* dstImg  : Output ASVL_PAF_RGB24_B8G8R8 format image.
* NOTE:
*   image width and height must be a multiple of 2 and >=2.
**************************************/
MInt32 mcvColorNV21toBGR888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/**************************************
* Convert NV21 to RGBA8888 .
* srcImg   : Input ASVL_PAF_NV21 format image.
* dstImg  : Output ASVL_PAF_RGB32_R8G8B8A8 format image.
* alpha    : alpha value to set.
* NOTE : 
*        Image width and height must be a multiple of 2.
**************************************/
MInt32 mcvColorNV21toRGBA8888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg, MUInt8 alpha);


/**************************************
* Convert NV12 to RGBA8888 .
* srcImg   : Input ASVL_PAF_NV12 format image.
* dstImg  : Output ASVL_PAF_RGB32_R8G8B8A8 format image.
* alpha    : alpha value to set.
* NOTE : 
*        Image width and height must be a multiple of 2.
**************************************/
MInt32 mcvColorNV12toRGBA8888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg, MUInt8 alpha);

/***************************************
*function:mcvColorNV12toBGRA8888u8
* Convert NV12 to BGRA8888 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_NV12
*						ASVL_PAF_RGB32_B8G8R8A8
****************************************/
MInt32 mcvColorNV12toBGRA8888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg, MUInt8 alpha);

/**************************************
* Convert I420 to RGBA8888 .
* srcImg   : Input ASVL_PAF_I420 format image.
* dstImg  : Output ASVL_PAF_RGB32_R8G8B8A8 format image.
* alpha    : alpha value to set.
* NOTE : 
*        Image width and height must be a multiple of 2.
**************************************/
MInt32 mcvColorI420toRGBA8888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg, MUInt8 alpha);

/***************************************
*function:mcvColorI420toBGRA8888u8
* Convert I420 to BGRA8888 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I420
*						ASVL_PAF_RGB32_B8G8R8A8
****************************************/
MInt32 mcvColorI420toBGRA8888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg, MUInt8 alpha);


/**************************************
* Convert NV21 to YUV420 .
* pSrc  : Input NV21 format image.
* pDst  : Output I420(YUV420) format image.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvColorNV21toI420u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/**************************************
* Convert YUYV to I420 .
* srcImg  : Input YUYV format image.
* dstImg  : Output I420 format image.
* NOTE: 
    image width and height must be a multiple of 2 and >= 2;
**************************************/
MInt32 mcvColorYUYVtoI420u8(MByte *yuvSrc,MByte *yuvDst,MInt32 width,MInt32 height);

/***************************************
*function:mcvColorYUYVtoI420u8_2
* Convert YUYV to I420 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_YUYV
*						ASVL_PAF_I420
****************************************/
MInt32 mcvColorYUYVtoI420u8_2(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/**************************************
* Convert YUYV to NV21 .
* srcImg  : Input YUYV format image.
* dstImg  : Output NV21 format image.
* NOTE: 
image width and height must be a multiple of 2 and >= 2;
**************************************/
MInt32 mcvColorYUYVtoNV21u8_v2(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);


/**************************************
* Convert NV21 to YUYV .
* srcImg  : Input NV21 format image.
* dstImg  : Output YUYV format image.
* NOTE: 
image width and height must be a multiple of 2 and >= 2;
**************************************/
MInt32 mcvColorNV21toYUYVu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorNV12toYUYVu8
* Convert NV12 to YUYV .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_NV12
*						ASVL_PAF_YUYV
****************************************/
MInt32 mcvColorNV12toYUYVu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/**************************************
* Convert YUYV to NV21 .
* pSrc  : Input YUYV format image.
* pDst  : Output NV21 format image.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvColorYUYVtoNV21u8(MByte *pSrc,MByte *pDst,MInt32 lWidth,MInt32 lHeight);


/**************************************
* Convert YUYV to NV12 .
* pSrc  : Input YUYV format image.
* pDst  : Output NV12 format image.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvColorYUYVtoNV12u8(MByte *pSrc,MByte *pDst,MInt32 lWidth,MInt32 lHeight);

/***************************************
*function:mcvColorYUYVtoNV12u8_v2
* Convert YUYV to NV12 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_YUYV
*						ASVL_PAF_NV12
****************************************/
MInt32 mcvColorYUYVtoNV12u8_v2(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);
/**************************************
* Convert YUYV to RGB888 .
* srcImg  : Input YUYV format image.
* dstImg  : Output RGB888 format image.
**************************************/
MInt32 mcvColorYUYVtoRGB888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorYUYVtoBGR888u8
* Convert YUYV to BGR888 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_YUYV
*						ASVL_PAF_RGB24_B8G8R8
****************************************/
MInt32 mcvColorYUYVtoBGR888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorYUYVtoRGBA8888u8
* Convert YUYV to RGBA8888 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_YUYV
*						ASVL_PAF_RGB32_R8G8B8A8
****************************************/
MInt32 mcvColorYUYVtoRGBA8888u8(LPASVLOFFSCREEN srcImg, LPASVLOFFSCREEN dstImg, MUInt8 alpha);

/**************************************
* Convert BGR888 to HSL888 .
* bgr  : Input BGR888 format image.
* hsl  : Output HSL888 format image.
* lWidth : Image width.
* lHeight: Image Height.
* lineBytesBgr : Input BGR888 line step.
* lineBytesHsl : Output HSL888 line step.
**************************************/
MInt32 mcvColorBGR888toHSL888u8(MUInt8 *bgr, MUInt8 *hsl, MUInt32 lHeight,
       MUInt32 lWidth, MUInt32 lineBytesBgr, MUInt32 lineBytesHsl);

/**************************************
* Convert I420 to HSL888 .
* pY  : Input I420 format image Y frame.
* pCb  : Input YUYV format image Cb frame.
* pCr  : Input YUYV format image Cr frame.
* lWidth : Image width.
* lHeight: Image Height.
* pHSL  :  Output HSL888 format image.
* y_step1 : Input Y frame line step.
* cbcr_step1 :  Input Cb and Cr frame line step.
* hsl_step1 : Output HSL888 line step.
**************************************/
MInt32 mcvColorI420toHSL888u8(MUInt8 *pY,MUInt8 *pCb, MUInt8 *pCr, 
        MUInt32 lWidth, MUInt32 lHeight, MUInt8 *pHSL,
        MUInt32 y_step1,MUInt32 cbcr_step1,MUInt32 hsl_step1);


/**************************************
* Convert BGR565 to HSL888 .
* pBgr565  : Input BGR565 format image.
* pHsl  : Output HSL888 format image.
* lWidth : Image width.
* lHeight: Image Height.
* lineBytesBgr : Input BGR565 line step.
* lineBytesHsl : Output HSL888 line step.
**************************************/
MInt32  mcvColorBGR565toHSL888u8(MUInt8 *pBgr565, MUInt8 *pHsl, 
        MUInt32 lHeight,MUInt32 lWidth, 
        MUInt32 lineBytesBgr, MUInt32 lineBytesHsl);


/**************************************
* Convert RGB565 to HSL888 .
* pBgr565  : Input RGB565 format image.
* pHsl  : Output HSL888 format image.
* lWidth : Image width.
* lHeight: Image Height.
* lineBytesBgr : Input RGB565 line step.
* lineBytesHsl : Output HSL888 line step.
**************************************/
MInt32  mcvColorRGB565toHSL888u8(MUInt8 *pBgr565, MUInt8 *pHsl, 
        MUInt32 lHeight,MUInt32 lWidth, 
        MUInt32 lineBytesBgr, MUInt32 lineBytesHsl);


/**************************************
* Convert I420 to RGB888 .
* srcImg  : Input I420 format image.
* dstImg  : Output RGB888 format image.
* NOTE:
*   image width\height must be a multiple of 2 and >= 2;
**************************************/
MInt32 mcvColorI420toRGB888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI420toBGR888u8
* Convert I420 to BGR888 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I420
*						ASVL_PAF_RGB24_B8G8R8
****************************************/
MInt32 mcvColorI420toBGR888u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorRGB888toI420u8
* Convert RGB888 to I420 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_R8G8B8
*						ASVL_PAF_I420
****************************************/
MInt32 mcvColorRGB888toI420u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorBGR888toI420u8
* Convert BGR888 to I420 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_RGB24_B8G8R8
*						ASVL_PAF_I420
****************************************/
MInt32 mcvColorBGR888toI420u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/**************************************
* Convert I420 to YUYV .
* pSrc  : Input I420 format image.
* pDst  : Output YUYV format image.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvColorI420toYUYVu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/**************************************
* Convert I420(YUV420) to NV21 .
* pSrc  : Input I420 format image.
* pDst  : Output NV21 format image.
* srcImg : input Image .
* dstImg : output Image .
* NOTE :  
*   width and height must be a multiple of 2 and >2.
**************************************/
MInt32 mcvColorI420toNV21u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);


/**************************************
* Convert NV12 to I420 .
* srcImg  : Input NV12 format image.
* dstImg  : Output I420 format image.
* NOTE:
*   image width and height must be a multiple of 2 and >= 2.
**************************************/
MInt32 mcvColorNV12toI420u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/**************************************
* Convert NV12 to NV21 .
* srcImg  : Input NV12 format image.
* dstImg  : Output NV21 format image.
* NOTE:
*   image width and height must be a multiple of 2 and >= 2.
**************************************/
MInt32 mcvColorNV12toNV21u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/**************************************
* Convert YV12 to RGB888 .
* srcImg  : Input YV12 format image.
* dstImg  : Output RGB888 format image.
**************************************/
MInt32 mcvColorYV12toRGB888u8(LPASVLOFFSCREEN srcImg, LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorYV12toBGR888u8
* Convert YV12 to BGR888 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_YV12
*						ASVL_PAF_RGB24_B8G8R8
****************************************/
MInt32 mcvColorYV12toBGR888u8(LPASVLOFFSCREEN srcImg, LPASVLOFFSCREEN dstImg);
/**************************************
* Convert YV12 to NV21 .
* pSrc  : Input YV12 format image.
* pDst  : Output NV21 format image.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvColorYV12toNV21u8(MByte* pSrc, MByte *pDst,MLong width,MLong height);
MInt32 mcvColorYV12toNV21u8_2(LPASVLOFFSCREEN srcImg, LPASVLOFFSCREEN dstImg);
/*********** I422H to other formats******************/
/***************************************
*function:mcvColorI422HtoI420u8
* Convert I422H to I420 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I422H
*						ASVL_PAF_I420
****************************************/
MInt32 mcvColorI422HtoI420u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI422HtoNV21u8
* Convert I422H to NV21 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I422H
*						ASVL_PAF_NV21
****************************************/
MInt32 mcvColorI422HtoNV21u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI422HtoNV12u8
* Convert I422H to NV12 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I422H
*						ASVL_PAF_NV12
****************************************/
MInt32 mcvColorI422HtoNV12u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI422HtoYUYVu8
* Convert I422H to YUYV .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I422H
*						ASVL_PAF_YUYV
****************************************/
MInt32 mcvColorI422HtoYUYVu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI422HtoLPI422Hu8
* Convert I422H to LPI422H .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I422H
*						ASVL_PAF_LPI422H
****************************************/
MInt32 mcvColorI422HtoLPI422Hu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);
/*********** End of I422H to other formats******************/


/*********** I422V to other formats******************/
/***************************************
*function:mcvColorI422VtoI420u8
* Convert I422V to I420 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I422V
*						ASVL_PAF_I420
****************************************/
MInt32 mcvColorI422VtoI420u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI422VtoNV21u8
* Convert I422V to NV21 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I422V
*						ASVL_PAF_NV21
****************************************/
MInt32 mcvColorI422VtoNV21u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI422VtoNV12u8
* Convert I422V to NV12 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I422V
*						ASVL_PAF_NV12
****************************************/
MInt32 mcvColorI422VtoNV12u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI422VtoYUYVu8
* Convert I422V to YUYV .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I422V
*						ASVL_PAF_YUYV
****************************************/
MInt32 mcvColorI422VtoYUYVu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI422VtoLPI422Hu8
* Convert I422V to LPI422H .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I422V
*						ASVL_PAF_LPI422H
****************************************/
MInt32 mcvColorI422VtoLPI422Hu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);
/*********** End of I422V to other formats******************/

/*********** I444 to other formats******************/
/***************************************
*function:mcvColorI444toI420u8
* Convert I444 to I420 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I444
*						ASVL_PAF_I420
****************************************/
MInt32 mcvColorI444toI420u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI444toNV21u8
* Convert I444 to NV21 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I444
*						ASVL_PAF_NV21
****************************************/
MInt32 mcvColorI444toNV21u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI444toNV12u8
* Convert I444 to NV12 .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I444
*						ASVL_PAF_NV12
****************************************/
MInt32 mcvColorI444toNV12u8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI444toYUYVu8
* Convert I444 toYUYV .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I444
*						ASVL_PAF_YUYV
****************************************/
MInt32 mcvColorI444toYUYVu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);

/***************************************
*function:mcvColorI444toLPI422Hu8
* Convert I444 to LPI422H .
*input:
*   		srcImg     		src image, should not be NULL.
*output:
*   		dstImg 			dst image, should not be NULL.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*           -2    ---  invalid input param       (MCV_INVALID_PARAM)
*note:
*		Support Format 	ASVL_PAF_I444
*						ASVL_PAF_LPI422H
****************************************/
MInt32 mcvColorI444toLPI422Hu8(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg);
/*********** End of I444 to other formats******************/

/**** Color Convert engine for single threads ****/
/**************************************
* Interface of color convert .
* srcImg   : Input image.
* dstImg   : Output image.
* Param    : input parameter, like alpha.
* NOTE : 
*        Image width and height should be larger than 2.
*        if the height is odd, the size should be add 1 when alloc the memory.
**************************************/
MInt32 mcvColorCvt(LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg, mcvColorExtParam Param);
/**** End of Color Convert engine for single threads****/


/**** Color Convert for big image engine****/
/*return a handle :mcvColorCvtHandle*/
MHandle mcvColorCvtInit_MultiThreads(MHandle hMemMgr);
MInt32 mcvColorCvtProcess_MultiThreads(MHandle mcvColorCvtHandle,LPASVLOFFSCREEN srcImg,LPASVLOFFSCREEN dstImg, MVoid *extParam);
MInt32 mcvColorCvtUnInit_MultiThreads(MHandle hMemMgr, MHandle mcvColorCvtHandle);

/**** End of Color Convert for big image engine****/


/**************************************
* Computes the per-element absolute difference between two uint32 image.
* pSrc1  : First input image.
* pSrc2  : Second input image, must has the same size as pSrc1.
* pDst   : Output image, must has the same size as pSrc1.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
* lLineBytes: Image length of a row in bytes.
		  NOTE :  must be a multiple of 2.	
**************************************/
MInt32 mcvAbsDiffu32(MUInt32* pSrc1, MUInt32* pSrc2, MUInt32* pDst, MInt32 lWidth, MInt32 lHeight, MInt32 lLineBytes);

/**************************************
* Computes the per-element absolute difference between two int32 image.
* pSrc1  : First input image.
* pSrc2  : Second input image, must has the same size as pSrc1.
* pDst   : Output image, must has the same size as pSrc1.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
* lLineBytes: Image length of a row in bytes.
		  NOTE :  must be a multiple of 2.	
**************************************/
MInt32 mcvAbsDiffs32(MInt32* pSrc1, MInt32* pSrc2, MInt32* pDst,MInt32 lWidth, MInt32 lHeight, MInt32 lLineBytes);

/**************************************
* Computes the per-element absolute difference between two byte image.
* pSrc1  : First input image.
* pSrc2  : Second input image, must has the same size as pSrc1.
* pDst   : Output image, must has the same size as pSrc1.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
* lLineBytes: Image length of a row in bytes.
		  NOTE :  must be a multiple of 2.	
**************************************/
MInt32 mcvAbsDiffu8(MByte* pSrc1, MByte* pSrc2,MByte* pDst, MInt32 lWidth, MInt32 lHeight, MInt32 lLineBytes);

/**************************************
* Computes the per-element absolute difference between one int32 image and one value.
* pSrc  : Input image.
* lValue: Input Value
* pDst  : Output image, must has the same size as pSrc.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
* lLineBytes: Image length of a row in bytes.
		  NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvAbsDiffVs32(MInt32* pSrc, MInt32 lValue, MInt32* pDst, MInt32 lWidth, MInt32 lHeight, MInt32 lLineBytes);

/**************************************
* Computes the per-element absolute difference between one float image and one value.
* pSrc  : Input image.
* lValue: Input Value
* pDst  : Output image, must has the same size as pSrc.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
* lLineBytes: Image length of a row in bytes.
		  NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvAbsDiffVf32(MFloat* pSrc, MFloat lValue, MFloat* pDst, MInt32 lWidth, MInt32 lHeight, MInt32 lLineBytes);

/**************************************
* Binarizes a grayscale image based on a threshold value. 
* Sets the pixel to max(255) if it's value is greater than the threshold; else, set the pixel to min(0).
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* lThreshold  : Input threshold.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
* lLineBytes: Image length of a row in bytes.
		  NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvFilterThresholdu8(MByte* pSrc, MByte* pDst,MInt32 lThreshold,MInt32 lWidth, MInt32 lHeight,MInt32 lLineBytes);

/**************************************
* Convolution an 32x32 matrix  by 5x5 filter matrix.
* input   : Input 32x32 matrix.
* output  : output 28x28 matrix.
* filter  : 5x5 filter matrix.
**************************************/
MUInt32 mcvConv_32_5_i32(MInt32 input[32][32], MInt32 output[28][28], MInt32 filter[5][5]);

/**************************************
* Convolution an 14x14 matrix  by 5x5 filter matrix.
* input   : Input 14x14 matrix.
* output  : output 10x10 matrix.
* filter  : 5x5 filter matrix.
**************************************/
MUInt32 mcvConv_14_5_i32(MInt32 input[14][14], MInt32 output[10][10], MInt32 filter[5][5]);

/**************************************
* Dilate a grayscale image by taking the local maxima of 3x3 neighborhood window.
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* pTmp  : Temporary buffer for use, must has the same size as pSrc.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvFilterDilate3x3u8(MByte* pSrc, MByte* pDst, MByte* pTmp, MInt32 lWidth, MInt32 lHeight);

/**************************************
* Erode a grayscale image by taking the local minima of 3x3 neighborhood window.
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* pTmp  : Temporary buffer for use, must has the same size as pSrc.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvFilterErode3x3u8(MByte* pSrc, MByte* pDst, MByte* pTmp, MInt32 lWidth, MInt32 lHeight);

/**************************************
* Blurs an image with 3x3 median filter.
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* lWidth : Image width.
		NOTE :  must be a multiple of 2.
* lHeight: Image Height.
		NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvFilterMedian3x3u8(MByte* pSrc, MByte* pDst, MInt32 lWidth, MInt32 lHeight);


/**************************************
* Blur with 5x5 Gaussian filter.
* Convolution with 5x5 Gaussian kernel: 
* 1 4 6 4 1 
* 4 16 24 16 4 
* 6 24 36 24 6 
* 4 16 24 16 4 
* 1 4 6 4 1
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* pTmp  : Temporary buffer for use, must has the same size as pSrc.
* lWidth : Image width.
          NOTE :  must be a multiple of 2.
* lHeight: Image Height.
          NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvFilterGaussian5x5u8(MByte* pSrc, MByte* pDst, MByte* pTmp, MInt32 lWidth, MInt32 lHeight);

/**************************************
* Blur a greyScale image with 7x7 Gaussian filter.
* Convolution with 7x7 Gaussian kernel in both direction: 
* 0.0126, 0.0788, 0.2373, 0.3426, 0.2373, 0.0788, 0.0126
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* pTmp  : Temporary buffer for use, must has the same size as pSrc.
* lWidth : Image width.
          NOTE :  must be >=  6.
* lHeight: Image Height.
          NOTE :  must be >= 6.
**************************************/
MInt32 mcvFilterGaussian7x7f32(MFloat* pSrc, MFloat* pDst, MFloat* pTmp, MInt32 lWidth, MInt32 lHeight);

/**************************************
* Blur a greyScale image with 7x7 Gaussian filter.
* Convolution with 7x7 Gaussian kernel in both direction: 
* 0.0126, 0.0788, 0.2373, 0.3426, 0.2373, 0.0788, 0.0126
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* pTmp  : Temporary buffer for use, must has the same size as pSrc.
* lWidth : Image width.
          NOTE :  must be >=  6.
* lHeight: Image Height.
          NOTE :  must be >= 6.
**************************************/
MInt32 mcvFilterGaussian7x7u16(MUInt16* pSrc, MUInt16* pDst, MUInt16* pTmp, MInt32 lWidth, MInt32 lHeight);

/**************************************
* Blur a greyScale image with 7x7 Gaussian filter.
* Convolution with 7x7 Gaussian kernel : 
0.000158,   0.000990,   0.002980,    0.004304,    0.002980,    0.000990,   0.000158,
0.000990,   0.006214,   0.018706,    0.027009,    0.018706,    0.006214,   0.000990,
0.002980,   0.018706,   0.056309,    0.081305,    0.056309,    0.018706,   0.002980,
0.004304,   0.027009,   0.081305,    0.117396,    0.081305,    0.027009,   0.004304,
0.002980,   0.018706,   0.056309,    0.081305,    0.056309,    0.018706,   0.002980,
0.000990,   0.006214,   0.018706,    0.027009,    0.018706,    0.006214,   0.000990,
0.000158,   0.000990,   0.002980,    0.004304,    0.002980,    0.000990,   0.000158
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* lWidth : Image width.
          NOTE :  must be >=  6.
* lHeight: Image Height.
          NOTE :  must be >= 6.
**************************************/
MInt32 mcvFilterGaussian7x7f32_2D(MFloat* pSrc, MFloat* pDst, MInt32 lWidth, MInt32 lHeight);

/**************************************
* Blur a greyScale image with 7x7 Gaussian filter.
* Convolution with 7x7 Gaussian kernel : 
0.000158,   0.000990,   0.002980,    0.004304,    0.002980,    0.000990,   0.000158,
0.000990,   0.006214,   0.018706,    0.027009,    0.018706,    0.006214,   0.000990,
0.002980,   0.018706,   0.056309,    0.081305,    0.056309,    0.018706,   0.002980,
0.004304,   0.027009,   0.081305,    0.117396,    0.081305,    0.027009,   0.004304,
0.002980,   0.018706,   0.056309,    0.081305,    0.056309,    0.018706,   0.002980,
0.000990,   0.006214,   0.018706,    0.027009,    0.018706,    0.006214,   0.000990,
0.000158,   0.000990,   0.002980,    0.004304,    0.002980,    0.000990,   0.000158
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* lWidth : Image width.
          NOTE :  must be >=  6.
* lHeight: Image Height.
          NOTE :  must be >= 6.
**************************************/
MInt32 mcvFilterGaussian7x7u16_2D(MUInt16* pSrc, MUInt16* pDst, MInt32 lWidth, MInt32 lHeight);


/**************************************
This function calculates an image derivative by convolving the image with an appropriate 3x3 kernel.
Border values are ignored.
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* lWidth : Image width.
		NOTE :  must be a multiple of 2.
* lHeight: Image Height.
		NOTE :  must be a multiple of 2.
**************************************/
MInt32 mcvFilterSobel3x3u8(MByte* pSrc, MByte* pDst, int lWidth, int lHeight);

/**************************************
Smooth a uint8_t image with a 3x3 box filter.
smooth with 3x3 box kernel and normalize: 
([ 1 1 1 ]/3 + [ 1 1 1 ]/3 + [ 1 1 1 ]/3)/3
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* pTmp  : Temporary buffer for use, must has the same size as pSrc.
* lWidth : Image width.
		NOTE :  must be > 2.
* lHeight: Image Height.
		NOTE :  must be > 2.
**************************************/
MInt32 mcvFilterBox3x3u8(MByte* pSrc, MByte* pDst, MByte* pTmp, MInt32 lWidth, MInt32 lHeight);

/**************************************
Smooth a uint8_t image with a 3x3 box filter.
smooth with 3x3 box kernel and normalize: 
[ 1 1 1 
1 1 1 
1 1 1 ]/9
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* lWidth : Image width.
		NOTE :  must be > 2.
* lHeight: Image Height.
		NOTE :  must be > 2.
* lStrideSrc: src Image line stride in [BYTE] unit.
		NOTE :  must be >= lWidth.
* lStrideDst: dst Image line stride in [BYTE] unit.
		NOTE :  must be >= lWidth.		
**************************************/
MInt32 mcvFilterBox3x3u8_2D(MByte* pSrc, MByte* pDst,MInt32 lWidth,MInt32 lHeight,MInt32 lStrideSrc,MInt32 lStrideDst);

/*
    * Filter an image with box filter. the size of the box is variable.
    * pSrc : input image;
    * pDst : output image;  (can be the same as pSrc)
    * pRowBuf: tmp buffer for internal usage.
   
    * kernelSize: box filter size(1D),say, 3 if 3x3 filter,5 if 5x5 filter
    * lWidth:image width
    * lHeight:image height
    * lStride:image buffer line stride
    *NOTE:
            1.  pRowBuf:lWidth*sizeof(MUInt32) + kernelSize *lWidth*sizeof(MUInt16) and must be 4bytes alligned
            2. pSrc == pDst supported.
            3. kernelSize must >= 3 and be odd.
            4. lWidth and lHeight >= 4.
*/
MInt32 mcvFilterBoxu8(MByte* pSrc,MByte* pDst,MUInt16* pRowBuf,MUInt32 kernelSize,MUInt32 lWidth,MUInt32 lHeight,MUInt32 lStride);


MInt32 mcvBoxFilterU8(MHandle hParEngine, MByte *pSrc, MByte *pDst,
    MInt32 lWidth, MInt32 lHeight, MInt32 lSrcLineStep, MInt32 lDstLineStep, MInt32 lRadius);
/**************************************
Smooth a YUYV image with a (1 << lSmoothLenShift * 1 << lSmoothLenShift) box using integral
smooth with (1 << lSmoothLenShift * 1 << lSmoothLenShift) box kernel and normalize: 
* pu8YUYV  : Input image & output smooth image.
* pYSum   : Temporary buffer for use of calculating Y integral.
		NOTE :  buffer size of pYSum should be ceil(lWidth+1) * (lHeight+1).
* pCbSum  : Temporary buffer for use of calculating U integral.
		NOTE :  buffer size of pCbSum should be ceil(lWidth/2+1) * (lHeight+1).
* pCrSum  : Temporary buffer for use of calculating V integral.
		NOTE :  buffer size of pCrSum should be ceil(lWidth/2+1) * (lHeight+1).
* lWidth : Image width.
		NOTE :  must be a multiple of 2.
* lHeight: Image Height.
		NOTE :  must be a multiple of 2.
* lPitch: Image pitch.
* lSmoothLenShift: smooth box length(box length is 1 << lSmoothLenShift).
**************************************/

MInt32	mcvFilterBoxYUYV(MUInt8* pu8YUYV, MUInt32 *pYSum, MUInt32 *pCbSum, MUInt32 *pCrSum, 
								MInt32 lPitch, MInt32 lWidth, MInt32 lHeight, MInt32 lSmoothLenShift);

/**************************************
Smooth the Y component of a YUYV image with a (kernelSize * kernelSize) box filter
* pSrc          : Input & output YUYV image
* pRowBuf    : Temporary row buffer used inside.
* kernelSize  : box filter size.
* lWidth       : Image width.
* lHeight      : Image Height.
* lStride       : input Image buffer line stride (normally >= lWidth*2).
* NOTE :  
*   1. buffer size of pRowBuf  >= (kernelSize ) * lWidth * sizeof(MUInt16) +  lWidth * sizeof(MUInt32).
*   2. lWidth must be a multiple of 2;
*   3. lWidth and lHeight  >= 4.
*   4. kernelSize must be odd; kernelSize >= 3;
**************************************/
MInt32 mcvFilterBoxYUYVInplaceLuma(MByte* pSrc,MUInt16* pRowBuf,MUInt32 kernelSize,MUInt32 lWidth,MUInt32 lHeight,MUInt32 lStride);

/**************************************
* pyrdown using guass 5x5.
* pTmp  : tmp buffer, size: srcStep * 5 * sizeof(MByte).
* pSrc  : Input image.
* srcWidth : src Image width.
		NOTE :  must be >= 5.
* srcHeight: src Image Height.
		NOTE :  must be >= 5.
* srcStep: src Image step.
* pDst  : Output image, valid  area is ((srcWidth + 1)/2) * ((srcHeight + 1)/2),buffer size is (dstStep) * ((srcHeight + 1)/2) * sizeof(MByte).
* dstStep: dst Image step.
* Note: 
*       1/16[1  4  6  4  1] for every direction
**************************************/
MInt32 mcvPyrDownGauss5x5u8c1(MByte *pTmp,MByte *pSrc,MInt32 srcWidth,MInt32 srcHeight,MInt32 srcStep,
                                        MByte *pDst,MInt32 dstStep);


/*
*rotate Y channel by restric angle
* lDegree ： rotate angle,should be 90 or 180 or 270. 
* pSrcY  : Input Y channel.
* lSrcLineStep : Input Y channel line step.
* lSrcImgWidth : Input Y channel width.
* lSrcImgHeight: Input Y channel height.
* pDstY  : output Y channel.
* lDstLineStep : output Y channel line step.
* lDstImgWidth : output Y channel width.
* lDstImgHeight: output Y channel height.
* Note:no neon version.
*/
MVoid mcvRotateYRestrictAngle(MLong lDegree,
   MUInt8 *pSrcY, MLong lSrcLineStep, MLong lSrcImgWidth, MLong lSrcImgHeight, 
   MUInt8 *pDstY, MLong lDstLineStep, MLong lDstImgWidth, MLong lDstImgHeight);


/*
*rotate UV channel by restric angle
* lDegree ： rotate angle,should be 90 or 180 or 270. 
* pSrcY  : Input UV channel.
* lSrcLineStep : Input UV channel line step.
* lSrcImgWidth : Input UV channel width.
* lSrcImgHeight: Input UV channel height.
* pDstY  : output UV channel.
* lDstLineStep : output UV channel line step.
* lDstImgWidth : output UV channel width.
* lDstImgHeight: output UV channel height.
* Note:no neon version.
*/
MVoid mcvRotateUVRestrictAngle(MLong lDegree,
   MUInt8 *pSrcUV, MLong lSrcLineStep, MLong lSrcImgWidth, MLong lSrcImgHeight, 
   MUInt8 *pDstUV, MLong lDstLineStep, MLong lDstImgWidth, MLong lDstImgHeight);



/**************************************
* Sets every element of an int32_t array to a given value.
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* lWidth : Image width.
		NOTE :  must be a multiple of 2.
* lHeight: Image Height.
		NOTE :  must be a multiple of 2.
* lChannel: Channel of one pixel, such as RGB has 3 channel.
* lValue: The input int32_t value.
**************************************/
MInt32 mcvSetElementss32(MInt32* pSrc, MInt32 lWidth, MInt32 lHeight, MInt32 lChannel, MInt32 lValue);

/**************************************
* Sets every element of an uint8_t array to a given value.
* pSrc  : Input image.
* pDst  : Output image, must has the same size as pSrc.
* lWidth : Image width.
		NOTE :  must be a multiple of 2.
* lHeight: Image Height.
		NOTE :  must be a multiple of 2.
* lChannel: Channel of one pixel, such as RGB has 3 channel.
* lValue: The input uint8_t value.
**************************************/
MInt32 mcvSetElementsu8(MByte* pSrc, MInt32 lWidth, MInt32 lHeight, MInt32 lChannel, MByte lValue);



/**************************************
*Calculate Surf Integral Image for 8 channels.
* pGraySrc  : Input gray image.
* lWidth : Image width.
		NOTE :  must be a multiple of 2.
* lHeight: Image Height.
		NOTE :  must be a multiple of 2.
* surf_int_image: The Integral Image.
* pRect: The ROI rect.
**************************************/
MInt32 mcvCalcSurfIntegralImage_Detect_Surf(MByte *pGraySrc,MInt32 lWidth,MInt32 lHeight,
									MVoid* surf_int_image,MRECT *pRect);



/**************************************
* Computes the Hamming distance between the two supplied arbitrary length vectors.
* pA  : Pointer to vector to compute distance.
* pB  : Pointer to vector to compute distance.
* lLength : Length in bits of each of the vectors.
* pDistance: Pointer to the distance.
**************************************/
MInt32 mcvHammingDistanceu8(MByte* pA, MByte* pB, int lLength, int* pDistance);

/**************mcv parallel engine begin*****************/
/********************************************
Step 1:Call mcvParallelInit  at the very beginning of your program(call it only once):
        MHandle mcvParallelMonitor = mcvParallelInit(MemHandle,1000)
	if(mcvParallelMonitor == 0)
	{
		printf("Failed to start parallel engine!!\n");
	}
	
Step 2:Package your function like this :

	MVoid yourFunction1(MVoid *param)
	{
		if(param != MNull)
		{
			int *p = (int *)param;
			unsigned char *pSrc = (unsigned char *)(*p);p++;
			int width = *p;p++;
			int height = *p;p++;
			....

			resize(pSrc,width,height,...);//this is what you will actually do
		}
	}

Step 3: Execute tasks:
	task 1:
		MVoid *param1 = (MVoid *)malloc(...);
		int *p = (int *)pParam1;
		MInt32 taskId1,taskId2,...;

		*p = (int)pSrc;p++;
		*p = (int)lSrcWidth;p++;
		*p = (int)lSrcHeight;p++;
		....

		taskId1 = mcvAddTask(yourFunction1,param1);
	task 2:
		...
		taskId2 = mcvAddTask(yourFunction2,param2);
		...
	task 3:
		...

Step 4: Wait tasks to finish:
	mcvWaitTask(taskId1);
	mcvWaitTask(taskId2);
	....
	

Step 5: Shutdown parallel engine when your program is coming to an end(call it only once):
	if(mcvParallelUninit(mcvParallelMonitor) < 0)
	{
		printf("Failed to shut down parallel engine!!\n");
	}

**********************************************/

/* Function:
  * 		mcvParallelInit
  * Description:
  * 		Initialize parallel engine;
  * Input:
  *		hContext   handle returned by MMemMgrCreate. Use mpbase lib to allocate memory inside or malloc
  *                         hContext is NULL, use malloc.
  * 		iCoreNumHint		number of  cores(threads) to be used;
  *						range [0,0xffffffff],if iCoreNumHint == 0, use default cpu number.
  *					this value is just a hint,actual used core number is limited by the platform.
  * Return:
  * 		A handle of internal struct,valid only when non-zero. Users should not care about this.
  * Notes:
  * 		1.Call only once.
  *          2. About 2K Bytes will be allocated inside;
  */
MHandle mcvParallelInit(MHandle hContext,MUInt32 iCoreNumHint);


/* Function:
  * 		mcvAddTask
  * Description:
  * 		Add a task ,after this function returns, the task will be exec when there is a free thread;
  * Input:
  *         mcvParallelMonitor handle returned by mcvParallelInit.
  * 		process  function name, should be  void *(f)(void *);
  * 		arg         function parameter;
  * Return:
  * 		return a taskId, which can be used in function "mcvWaitTask";
  *		Failed to add task when taskId is negative.
  */
MInt32 mcvAddTask(MHandle mcvParallelMonitor, MPVoidProc process, MVoid *arg);


/* Function:
  * 		mcvWaitTask
  * Description:
  * 		wait a task to finish;
  * Input:
  *         mcvParallelMonitor handle returned by mcvParallelInit.
  * 		taskId  return value of "mcvAddTask";
  * Return:
  * 		MCV_OK;
  *		MCV_NULL_POINTER.
  * Notes:
  * 		a.This function will block;
  *		b.The times you call this function shall be the same as calling 'mcvAddTask'.
  */
MInt32 mcvWaitTask(MHandle mcvParallelMonitor,MInt32 taskId);


/* Function:
  * 		mcvParallelUninit
  * Description:
  * 		Destroy parallel engine;
  * Input:
  *         mcvParallelMonitor handle returned by mcvParallelInit.
  * Return:
  * 		MCV_OK : ok;  MCV_INVALID_CALL : failed;
  * Note:
  * 		Call only once.
  */
MInt32 mcvParallelUninit(MHandle mcvParallelMonitor);

/**************mcv parallel engine end*****************/


/**************************************
* Detect motion of rectangle region of 3 images with grey data.
* pPrevPrevData  :  Source grey image.
* pPrevData		 : 	Source grey image.
* pCurrData		 : 	Source grey image.
* lPrevPrevStep  :	Line step of PrevPrevData.
* lPrevStep		 :	Line step of PrevData.
* lCurrStep		 :	Line step of CurrData.
* lWidth		 :	Width of three source images.
		Note : Width of three source images should be equal.
* lHeight		 :	Height of three source images.
		Note : Height of three source images should be equal.
* pDstData		 :	Dest image.
* lDstStep		 :	Line step of dest image.
		Note : lPrevPrevStep,lPrevStep,lCurrStep,lDstStep should be equal.
* roi			 : 	The detected rectangle region.
		Note : If roi is null rectangle is lWidth * lHeight.
**************************************/
MInt32 mcvDetectMotion3FrameDiffYWithRect(MByte *pPrevPrevData, MLong lPrevPrevStep, MByte *pPrevData, MLong lPrevStep,
	MByte *pCurrData, MLong lCurrStep, MLong lWidth, MLong lHeight, MByte* pDstData, MLong lDstStep, MRECT *roi);


/**************************************
* Detect motion of 3 images with grey data.
* pPrevPrevData  :  Source grey image.
* pPrevData		 : 	Source grey image.
* pCurrData		 : 	Source grey image.
* lPrevPrevStep  :	Line step of PrevPrevData.
* lPrevStep		 :	Line step of PrevData.
* lCurrStep		 :	Line step of CurrData.
* lWidth		 :	Width of three source images.
		Note : Width of three source images should be equal.
* lHeight		 :	Height of three source images.
		Note : Height of three source images should be equal.
* pDstData		 :	Dest image.
* lDstStep		 :	Line step of dest image.
		Note : lPrevPrevStep,lPrevStep,lCurrStep,lDstStep should be equal.
**************************************/
MInt32 mcvDetectMotion3FrameDiffY(MByte *pPrevPrevData, MLong lPrevPrevStep, MByte *pPrevData, MLong lPrevStep,
	 MByte *pCurrData, MLong lCurrStep, MLong lWidth, MLong lHeight, MByte* pDstData, MLong lDstStep);


/**************************************
* Map histogram of rectangle region after weighting the Y U V channel data.
* pOffScreen : Data structure stores source and dest image.
	Note : Source image format should be ASVL_PAF_I422H.
* table		 : The table stores historam mapping relation.
* roi		 : The rectangle region to calculate histogram.
**************************************/
MInt32 mcvCalcHistBackProject_I422HWithRect(LPASVLOFFSCREEN pOffScreen, MLong* table, MRECT *roi);


/**************************************
* Map histogram of whole image after weighting the Y U V channel data.
* pOffScreen : Data structure stores source and dest image.
	Note : Source image format should be ASVL_PAF_I422H.
	Note : Width and height of image should be a multiple of 2.
* table		 : The table stores historam mapping relation.
**************************************/
MInt32 mcvCalcHistBackProject_I422H(LPASVLOFFSCREEN pOffScreen, MLong* table);


/******************* OPTICAL FLOW *********************/
typedef struct
{
    float x;
    float y;
} mcvPoint2D32f;
typedef struct
{
    int width;
    int height;
} mcvSize;
typedef struct
{
    int		type;  /* may be MCV_CM_TERMCRIT_ITER | MCV_CM_TERMCRIT_EPS */
    int		max_iter;
    float	epsilon;
} mcvTermCriteria;

#define  MCV_CM_LKFLOW_PYR_A_READY       (1)
#define  MCV_CM_LKFLOW_PYR_B_READY       (2)
#define  MCV_CM_LKFLOW_INITIAL_GUESSES   (4)
#define  MCV_CM_LKFLOW_MT                (8)

#define MCV_CM_TERMCRIT_ITER    1
#define MCV_CM_TERMCRIT_EPS     2

/***************/
MInt32 mcvIcmCalcOpticalFlowPyrLK_u8c1( const MByte* imgA, const MByte* imgB,
                      int imgStep, mcvSize imgSize, MByte* pyrA, MByte* pyrB, 
					  const mcvPoint2D32f* featuresA, mcvPoint2D32f* featuresB, 
					  int count, mcvSize winSize, int level, char* status, float* error, 
					  mcvTermCriteria criteria, int flags,MHandle hMemMgr);


/******************* END OF OPTICAL FLOW *********************/



/********** MTHREAD API ****************/
MHandle		mcvEventCreate(MBool bAutoReset);
MRESULT		mcvEventDestroy(MHandle hEvent);
MRESULT		mcvEventWait(MHandle hEvent, MDWord dwTimeOut);
MRESULT		mcvEventSignal(MHandle hEvent);
MHandle     mcvThreadCreate(MPVoidProc proc, MVoid* pParam);
MRESULT		mcvThreadExit(MHandle hThread);
MRESULT		mcvThreadDestory(MHandle hThread);
/********** END of MTHREAD API ****************/



/************************ OPENCL API***********************/

/*** COMMON OPENCL API*********/
/**************************************
*function:mcvOCLInit
*		Init an opencl handle .
*input:
*   		hContext     	Mem  handle
*return:
*   		hContext		handle returned by mcvOCLInit
**************************************/
MHandle mcvOCLInit(MHandle hContext);

/**************************************
*function:mcvOCLUnInit
*		InInit an opencl handle .
*input:
*   		hContext     	OCL handle
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
**************************************/
MInt32 mcvOCLUnInit(MHandle mcvOCLHandle);

/**************************************
*function:mcvOCLWaitGpu
*		Wait for opencl finishing  .
*input:
*   		hContext     	OCL handle
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
**************************************/
MInt32 mcvOCLWaitGpu(MHandle mcvOCLHandle);

/*** END OF COMMON OPENCL API**/


/*** MATRIX OPERATION OPENCL API*********/
/**************************
m1Rows,m1Cols describe the first matrix;
m1Cols,m1Cols describe the second matrix;
RowColMajor: 0 : RowMajor  ; 1 : ColMajor:CURRENTLY SUPPORT ONLY ROW MAJOR 
****************************/
MHandle mcvOCLMatrixMulInit(MHandle mcvOCLHandle, MInt32 m1Rows,  MInt32 m1Cols, MInt32 m2Cols);
MInt32 mcvOCLMatrixMulUnInit(MHandle mcvOCLMatrixHandle);
MInt32 mcvOCLMatrixMul_RowMajor_f32_begin(MHandle mcvOCLMatrixHandle, MFloat *M_out,MFloat *M1_in,MFloat *M2_in,MInt32 m1Rows,  MInt32 m1Cols, MInt32 m2Cols);
MInt32 mcvOCLMatrixMul_RowMajor_f32_end(MHandle mcvOCLMatrixHandle);
/*** END OF MATRIX OPERATION OPENCL API**/

/**************** PYRAMID BLEND OPENCL API ******************/
MHandle mcvOCLPyrUpDownInit(MHandle mcvOCLHandle,MInt32 iWidth,MInt32 iHeight,MInt32 ilayer);
MInt32 mcvOCLPyrUpDownBeginPyr1(MHandle pyrUpDownHandle,MInt16 **pyramid);
MInt32 mcvOCLPyrUpDownBeginPyr1_NVXX_UV(MHandle pyrUpDownHandle,MInt16 **pyramid);
MInt32 mcvOCLPyrUpDownBeginPyr2(MHandle pyrUpDownHandle,MInt16 **pyramid);
MInt32 mcvOCLPyrUpDownBeginPyr2_NVXX_UV(MHandle pyrUpDownHandle,MInt16 **pyramid);
MInt32 mcvOCLPyrDownWeightTableBegin(MHandle pyrUpDownHandle,MInt16 **weightedTable);
MInt32 mcvOCLPyrDownWeightedAddBeginS16(MHandle pyrUpDownHandle,MInt32 **pyrResult);
MInt32 mcvOCLPyrDownWeightedAddBeginS16_NVXX_UV(MHandle pyrUpDownHandle,MInt32 **pyrResult);
MInt32 mcvOCLPyrUpDownUpdateParam(MHandle pyrUpDownHandle,MInt32 iWidth,MInt32 iHeight);
MInt32 mcvOCLPyrUpDownWaitGpu(MHandle pyrUpDownHandle);
MInt32 mcvOCLPyrUpDownUnInit(MHandle pyrUpDownHandle);
/************************ END OF PYRAMID BLEND OPENCL API***********************/

/*************IMAGE WARPAFFINE OPENCL API**********************************************/
/**********************************************
Function:    mcvWarpAffineInit.
Description:
	Init warp affine opencl gpu running environment.
Input:
    mcvOCLHandle:
	    OpenCL run environment manager(mcvOCLMgr_t) returned by mcvOCLInit.
	pSrcImg:
		Basic info of srcImg.Its ppuPlane can not be allocated. 
	pDstImg: 
		Basic info of output Image.Its ppuPlane can not be allocated.
	flag:
		Interpolation mode.
Return:
	A Handle.
	if the handle is NULL, init failed.
Note: 
	1.  MCV_INTERPOLATION_MODE_YBILINEAR_UVNEAREST supported only.
	2.  NV21 image format supported only.
**********************************************/
MHandle mcvOCLWarpAffineInit(MHandle mcvOCLHandle,LPASVLOFFSCREEN pSrcImg,LPASVLOFFSCREEN pDstImg,mcvInterpolationMode flag);
/**********************************************
Function:    mcvOCLWarpAffine.
Description:
	process.
Input:
    mcvOCLWarpAffineHandle:
	    a warpAffine handle returned by mcvOCLWarpAffineInit.
	pSrcImg:
		Source Image that should be processed.Will not be modified. 
	pfM: 
		Forward transformation matrix.
		[1,0,0 // first row
		 0,1,0]// second row
Output:
	pOutImg:
	    WarpAffine result of source Image.
Return:
    MCV_NULL_POINTER   At least one of handle\pSrcImg\pDstImg\pfM is NULL.
	MCV_INVALID_PARAM  Invalid PAF that supported.or pSrcImg /pDstImg larger than initial size.or mcvOCLMgr_t is Null.
	MCV_CL_FAIL        OpenCL aspects failure.
	MCV_OK             Successful.
**********************************************/
MInt32  mcvOCLWarpAffine(MHandle mcvOCLWarpAffineHandle,LPASVLOFFSCREEN pSrcImg,LPASVLOFFSCREEN pDstImg,MFloat* pfM);
/**********************************************
Function:    mcvWarpAffineUnInit.
Description:
	Release wareAffine on GPU. Should be called before mcvOCLUninit.
Input:
    mcvOCLWarpHandle:
	    A warpAffine handle returned by mcvOCLWarpAffineInit.
Return:
	MCV_OK             Successful.
**********************************************/
MInt32  mcvOCLWarpAffineUnInit(MHandle mcvOCLWarpHandle);
MInt32 mcvOCLWarpAffineSimple(MHandle handle,LPASVLOFFSCREEN _src,LPASVLOFFSCREEN _dst,MFloat dW,MFloat dH,MFloat scale,MFloat scale2,MInt32 row_begin,MInt32 row_end);
MInt32 mcvOCLWarpAffineSimpleDelight(MHandle pHandle,LPASVLOFFSCREEN _src,LPASVLOFFSCREEN _dst, MFloat dW,MFloat dH,MFloat scale,MFloat scale2,MInt32 row_beign,MInt32 row_end,float* bezierMap,float *yscaleMap);
/*************END OF IMAGE WARPAFFINE OPENCL API***************************************/

/************IMAGE  RESIZE OPENCL API********************************************/
/**********************************************
Function:    mcvResizeInit.
Description:
	Init Resize opencl running environment.
Input:
    mcvOCLHandle:
	    OpenCL run environment manager(mcvOCLMgr_t) returned by mcvOCLInit.
	pSrcImg:
		Basic info of srcImg.Its ppuPlane can not be allocated. 
	pDstImg: 
		Basic info of output Image.Its ppuPlane can not be allocated either.
	flag:
		Interpolation mode.
Return:
	mcvResizeHandle_t.
	if NULL, init failed.
Note: 
	1.  MCV_INTERPOLATION_MODE_YBILINEAR_UVNEAREST supported only.
	2.  NV21 image format supported only.
**********************************************/
MHandle mcvOCLResizeInit(MHandle mcvOCLHandle,LPASVLOFFSCREEN pSrcImg,LPASVLOFFSCREEN pDstImg,mcvInterpolationMode flag);
/**********************************************
Function:    mcvOCLResize.
Description:
	process.
Input:
    handle:
	    mcvOCLResizeHandle_t returned by mcvResizeInit.
	pSrcImg:
		Source Image that should be processed.Will not be modified. 
    fX:
	    Scale factor along the horizontal axis.
	fY: 
	    Scale factor along the vertical axis.
	Note:
	    If fX or fY is zero.fX is computed as:
		(float)pDstImg.i32Width/pSrcImg.i32Width.
		fY is computed as:
		(float)pDstImg.i32Height/pSrcImg.i32Height.
Output:
	pOutImg:
	    resize result of source Image.
Return:
    MCV_NULL_POINTER   At least one of handle\pSrcImg\pDstImg is MNull.
	MCV_INVALID_PARAM  Invalid PAF that supported.or pSrcImg /pDstImg larger than initial size.or mcvOCLMgr_t is Null.
	MCV_CL_FAIL        OpenCL aspects failure.
	MCV_OK             Successful.
**********************************************/
MInt32  mcvOCLResize(MHandle handle,LPASVLOFFSCREEN pSrcImg,LPASVLOFFSCREEN pDstImg,MFloat fX,MFloat fY);
/**********************************************
Function:    mcvResizeUnInit.
Description:
	Release mcvOCLResizeHandle_T. Should be called before mcvOCLUninit.
Input:
    mcvOCLResizeHandle:
	    A Resize handle returned by mcvOCLResizeInit.
Return:
	MCV_OK             Successful.
**********************************************/
MInt32  mcvOCLResizeUnInit(MHandle mcvOCLResizeHandle);
/************END of  RESIZE OPENCL API************************************/

/************ColorCvt OPENCL API***************/
/**********************************************
Function:    mcvOCLColorCvtInit.
Description:
	Init color convert opencl running environment.
Input:
    mcvOCLHandle:
	    OpenCL run environment manager(mcvOCLMgr_t) returned by mcvOCLInit.
	pSrcImg:
		Basic info of srcImg.Its ppuPlane can not be allocated. 
	pDstImg: 
		Basic info of output Image.Its ppuPlane can not be allocated either.
Return:
	mcvColorCvtHandle_t.
	if NULL, init failed.
**********************************************/
MHandle mcvOCLColorCvtInit(MHandle mcvOCLHandle, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg);

/**********************************************
Function:    mcvOCLColorCvt.
Description:
	process.
Input:
    handle:
	    mcvOCLColorCvtHandle_t returned by mcvOCLColorCvtInit.
	pSrcImg:
		Source Image that should be processed.Will not be modified. 
Output:
	pDstImg:
	     result of source Image.
Return:
    MCV_NULL_POINTER   At least one of handle\pSrcImg\pDstImg is MNull.
	MCV_INVALID_PARAM  mcvOCLMgr_t is Null.
	MCV_CL_FAIL        OpenCL aspects failure.
	MCV_OK             Successful.
**********************************************/
MInt32 mcvOCLColorCvt(MHandle Handle, LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MUInt32 alpha);

/**********************************************
Function:    mcvOCLColorCvtUnInit.
Description:
	Release mcvOCLColorCvtHandle_T. Should be called before mcvOCLUninit.
Input:
    mcvOCLHandle:
	    A handle returned by mcvOCLColorCvtInit.
Return:
	MCV_OK             Successful.
**********************************************/
MInt32 mcvOCLColorCvtUnInit(MHandle mcvOCLHandle);
/************END OF ColorCvt OPENCL API***************/

/************************ MULTITHREAD FUNCTIONS API***********************/
/**************************************
*function:mcvResizeMultiThreadsInit
*		Init an Resize multithread handle .
*input:
*   		hContext     	mcvParallelInit  handle, if handle is NULL ,  a new parallel handle will be created internally, if handle is yes,  pls input the mcvParallelInit handle.
*return:
*   		hContext		handle returned by mcvResizeMultiThreadsInit
**************************************/
MHandle mcvResizeMultiThreadsInit(MHandle handle);

/**************************************
*function:mcvResizeMultiThreadsProcess
*		Resize the img  .
*input:
*   		hContext     		Mem  handle
*   		pSrcImg 			src image, should not be NULL.
*   		mcvInterMode        mcvInterpolationMode, default be MCV_INTERPOLATION_MODE_YBILINEAR_UVNEAREST.
*output:
*   		pDstImg              	image to be resized.
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*note:
*		Support Format 	ResizeNV21BilinearUVNearest
*						ResizeNV21Nearest
*						ResizeSingleComponentBilinear
**************************************/
MInt32 mcvResizeMultiThreadsProcess(MHandle mcvResizeHandle,LPASVLOFFSCREEN pSrcImg,LPASVLOFFSCREEN pDstImg, MUInt32 mcvInterMode);

/**************************************
*function:mcvResizeMultiThreadsUninit
*		UnInit an Resize handle .
*input:
*   		handle     			mcvParallelInit  handle, if handle is NULL,  the parallel handle will be destroyed internally, if handle is yes,  pls destroy parallel handle outside.
*   		mcvResizeHandle     	mcvResizeMultiThreadsInit  handle
*return:
*           0      ---  success                      			(MCV_OK)
*           -1    ---  invalid input param       			(MCV_NULL_POINTER)
*           -6    ---  Parallel engine UnInit failed      	(MCV_UNEXPECTED_ERR)
**************************************/
MInt32 mcvResizeMultiThreadsUninit(MHandle handle,MHandle mcvResizeHandle);

/**************************************
*function:mcvMatrixMultiThreadsInit
*		Init an Matrix multithread handle .
*input:
*   		hContext     	mcvParallelInit  handle, if handle is NULL ,  a new parallel handle will be created internally, if handle is yes,  pls input the mcvParallelInit handle.
*return:
*   		hContext		handle returned by mcvMatrixMultiThreadsInit
**************************************/
MHandle mcvMatrixMultiThreadsInit(MHandle handle);

/**************************************
*function:mcvMatrixMultiThreadsProcess
*		Matrix mul Matrix  .
*input:
*   		hContext     		Mem  handle
*   		matrix1 			input matrix1
*   		matrix2             input matrix2
*           row1                the row of matrix1
*           col1                the col of matrix1
*           col2                the col of matrix2
*output:
*   		matrixOut           output matrix
*return:
*           0      ---  success                       (MCV_OK)
*           -1    ---  invalid input param       (MCV_NULL_POINTER)
*note:
*		Support Format 	ResizeNV21BilinearUVNearest
*						ResizeNV21Nearest
*						ResizeSingleComponentBilinear
**************************************/
MInt32 mcvMatrixMultiThreadsProcess_f32(MHandle mcvMatrixHandle, MFloat *matrix1, MFloat *matrix2, MFloat *matrixOut, MInt32 row1, MInt32 col1, MInt32 col2);
MInt32 mcvMatrixMultiThreadsProcess_s32(MHandle mcvMatrixHandle, MInt32 *matrix1, MInt32 *matrix2, MInt32 *matrixOut, MInt32 row1, MInt32 col1, MInt32 col2);
/**************************************
*function:mcvMatrixMultiThreadsUninit
*		UnInit an Matrix handle .
*input:
*   		handle     			mcvParallelInit  handle, if handle is NULL,  the parallel handle will be destroyed internally, if handle is yes,  pls destroy parallel handle outside.
*   		mcvMatrixHandle     	mcvResizeMultiThreadsInit  handle
*return:
*           0      ---  success                      			(MCV_OK)
*           -1    ---  invalid input param       			(MCV_NULL_POINTER)
*           -6    ---  Parallel engine UnInit failed      	(MCV_UNEXPECTED_ERR)
**************************************/
MInt32 mcvMatrixMultiThreadsUninit(MHandle handle,MHandle mcvMatrixHandle);

/************************ END OF MULTITHREAD FUNCTIONS API***********************/

typedef struct tagAfImage
{
	MInt32              size;
	MInt32              width;
	MInt32              height;
	MInt32              depth;
	MInt32              channel;
	MInt32              linebytes;
	MByte      *data;
}AfImage, *PAfImage;
MRESULT afCreatePreMask(PAfImage img,PAfImage sumImg ,MDWord * bitMask,MLong curWd,MLong curHt);

MVoid mcvComputeimgpyramidbasedonIntegral(PAfImage dstimg,PAfImage sumshort,MInt32 wd,MInt32 ht,MInt32 scalei,MInt32 startx,MInt32 starty);
MRESULT mcvCreatePreMask(PAfImage img,PAfImage sumImg ,MDWord * bitMask,MLong curWd,MLong curHt);
enum {
	//_win_
	//mmx
	MCV_SIMD_MMX,       // 0, mmx
	MCV_SIMD_MMX_NUM, 
	//sse
	MCV_SIMD_SSE_1,     //2, sse
	MCV_SIMD_SSE_2,     //3, sse2
	MCV_SIMD_SSE_3,     //4, sse3
	MCV_SIMD_SSE_3S,    //5, ssse3
	MCV_SIMD_SSE_41,    //6, sse4.1
	MCV_SIMD_SSE_42,    //7, sse4.2
	MCV_SIMD_SSE_NUM,
    //avx
	MCV_SIMD_AVX,       //9, avx
	MCV_SIMD_AVX_2,     //10, avx2
	MCV_SIMD_AVX_NUM,
	//opencl,for win too tedious to implement
	MCV_GPU_OPENCL_AMD,     // 12, opencl amd
	MCV_GPU_OPENCL_NAVIDA,  // 13, opencl navida
	MCV_CPU_OPENCL_INTEL,   // 14, opencl(cpu) intel
	MCV_OPENCL_NUM,
	//_android_
	//arm features (both arm32 and arm64)
	MCV_ARM_FEATURE_VFPv2,       //16
	MCV_ARM_FEATURE_VFPv3,       //17
	MCV_ARM_FEATURE_NEON,        //18
	MCV_ARM_FEATURE_IDIV_ARM,    //19
	MCV_ARM_FEATURE_IDIV_THUMB2, //20
	//arm32
	MCV_ARM_FEATURE_LDREX_STREX, //21
	MCV_ARM_FEATURE_ARMv7,       //22
	MCV_ARM_FEATURE_VFP_D32,     //23
	MCV_ARM_FEATURE_VFP_FP16,    //24
	MCV_ARM_FEATURE_VFP_FMA,     //25
	MCV_ARM_FEATURE_NEON_FMA,    //26
	MCV_ARM_FEATURE_iWMMXt,      //27
	MCV_ARM_FEATURE_NUM,         
	//arm64
	MCV_ARM64_FEATURE_FP,        //29
	MCV_ARM64_FEATURE_ASIMD,     //30
	MCV_ARM64_FEATURE_AES,       //31
	MCV_ARM64_FEATURE_PMULL,     //32
	MCV_ARM64_FEATURE_SHA1,      //33
	MCV_ARM64_FEATURE_SHA2,      //34
	MCV_ARM64_FEATURE_CRC32,     //35
	MCV_ARM64_FEATURE_NUM,       
	//x86
	MCV_X86_FEATURE_SSSE3,       //37
	MCV_X86_FEATURE_POPCNT,      //38
	MCV_X86_FEATURE_MOVBE,       //39
	MCV_X86_FEATURE_NUM,      
	//opencl
	MCV_ANDROID_GPU_OPENCL,      //41
	MCV_FEATURE_NUM
};

typedef enum{
	CPU_ARCH_UNKNOWN = 0,
	//android
	ANDROID_CPU_ARM,
	ANDROID_CPU_X86,
	ANDROID_CPU_MIPS,
	ANDROID_CPU_ARM64,
	ANDROID_CPU_X86_64,
	ANDROID_CPU_MIPS64,
	//windows
	WIN_CPU_AMD64,
	WIN_CPU_ARM,
	WIN_CPU_IA64,
	WIN_CPU_INTEL,
	CPU_ARCH_NUM
}CpuArch;
typedef struct _CPU_INFO{
	MChar vendor[64];
	MChar brand[64];
	MChar count;
	CpuArch arch;
} CPUINFO;
typedef struct _GPU_INFO{
	MChar* dynOclLibName;
	MVoid* oclLibHandle;
}GPUINFO;
typedef struct{
	MChar* featureName;
	MBool isSupported;
}SupportedFeature;
typedef struct _PARALLEL_TECH{
	CPUINFO cpuInfo;
	GPUINFO gpuInfo;
	SupportedFeature featureTech[MCV_FEATURE_NUM];
}FEATURE_TECH,*PFEATURE_TECH;

MRESULT mcvGetFeatureSupportInfo(FEATURE_TECH* ppTech);

MInt32 mcvResizeMeanX16(LPASVLOFFSCREEN pSrcImg,LPASVLOFFSCREEN pDstImg,  MUInt32 mcvMode);


typedef struct
{
	    MLong lCodebase;             	// Codebase version number 
	    MLong lMajor;                		// major version number 
	    MLong lMinor;                		// minor version number
	    MLong lBuild;                		// Build version number, increasable only
	    const MChar *Version;        	// version in string form
	    const MChar *BuildDate;      	// latest build Date
	    const MChar *CopyRight;      	// copyright 
}MCV_Version;

/************************************************************************
 * The function used to get version information of mcv library. 
 ************************************************************************/

const MCV_Version *MCV_GetVersion();

#ifdef __cplusplus
}
#endif

#endif //_MOBILECV_H_
