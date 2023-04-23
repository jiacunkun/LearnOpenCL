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
#include "merror.h"
#include "defcompilesetting.h"
#include "asvloffscreen.h"

#ifdef _DEBUG
// #define  _WIN32_DEBUG_
// #define  _OUTPUT_LOG_
#endif


#ifdef __ARM_NEON__
#include "arm_neon.h"
#endif

#ifndef TRIMBYTE
#define TRIMBYTE(x)	(MByte)((x)&(~255)?((-(x))>>31):(x))
#endif

#ifndef MAX
#define MAX(a,b)	((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b)	((a)<(b)?(a):(b))
#endif

#ifndef	ABS
#define ABS(x)		((x) > 0 ? (x) : -(x))
#endif

#ifndef	SWAP
#define SWAP(a,b,t)	((t) = (a), (a) = (b), (b) = (t))
#endif


#undef SALFDELETE
#define SALFDELETE(x) if((x)!=nullptr){ delete (x); (x)=nullptr; }

#undef SAFE_DELETE
#define SAFE_DELETE(x) if((x)!=nullptr){ delete (x); (x)=nullptr; }

#undef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) if((x)!=nullptr){ delete[] (x); (x)=nullptr; }

#ifndef SAFE_FREE
#define SAFE_FREE(x) { if (x) free(x); (x) = nullptr; }    //定义FREE释放函数
#endif

#ifndef SAFE_FCLOSE
#define SAFE_FCLOSE(x) { if (x) fclose(x); (x) = nullptr; }    //定义FREE释放函数
#endif


#if defined(MULTI_THREAD)|| defined(QUAD_MULTI_THREAD)
#include "mthread.h"
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

// image resize
#define FLT_TO_FIX(x,n)		(MLong)((x)*(1<<(n))+0.5f)
#define DESCALE(x,n)		(((x) + (1 << ((n)-1))) >> (n))
#define WARP_SHIFT			7
#define POSIT_BLOCK_NUM		64
#define WARP_MUL_ONE_8U(x)  ((x) << WARP_SHIFT)
#define WARP_DESCALE_8U(x)  DESCALE((x), (WARP_SHIFT<<1))
#define WARP_DESCALE2_8U(x)	DESCALE((x), WARP_SHIFT)



#endif // _IMAGE_BASE_H_
