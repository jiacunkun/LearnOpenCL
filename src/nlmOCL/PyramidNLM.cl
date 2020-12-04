#ifndef	ABS
#define ABS(x)		((x) > 0 ? (x) : -(x))
#endif

#ifndef	MAX
#define MAX(min, x)		((x) > min ? (x) : min)
#endif

#ifndef	MIN
#define MIN(max, x)		((x) < max ? (x) : max)
#endif

kernel void Resize
(
	const global uchar *src,
	int src_step,
	int src_cols,
	int src_rows,
	global uchar *dst,
	int dst_step,
	int dst_cols,
	int dst_rows
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

    const float ratiox = ((float)src_cols) / ((float)dst_cols);
    const float ratioy =  ((float)src_rows) / ((float)dst_rows);

	///
	float src_y = ((float)y + 0.5f) * ratioy - 0.5f;
	int src_y_floor = (int)floor(src_y);
    src_y_floor = clamp(src_y_floor, 0, src_rows-2);
	float beta = src_y - (float)src_y_floor;
    beta = clamp(beta,0.0f, 1.0f);
    int src_y_floor_1 = src_y_floor + 1;


	/////
	float src_x = ((float)x + 0.5f) * ratiox - 0.5f;
	int src_x_floor = (int)floor(src_x);
	src_x_floor = clamp(src_x_floor,0, src_cols-2);
	float alpha = src_x - (float)src_x_floor;
    alpha = clamp(alpha, 0.0f, 1.0f);
    int src_x_floor_1 = src_x_floor + 1;

	// Adreno(TM) 612:  38 ms - 42ms
    /*float A00 = (float)src[src_y_floor*src_step + src_x_floor];
	float A01 = (float)src[src_y_floor*src_step + src_x_floor_1];
	float A10 = (float)src[src_y_floor_1*src_step + src_x_floor];
	float A11 = (float)src[src_y_floor_1*src_step + src_x_floor_1];


	float A00f = A00 + (A01 - A00) * alpha;
	float A10f = A10 + (A11 - A10) * alpha;
	dst[y*dst_step + x] = (unsigned char)(A00f + (A10f - A00f) * beta);*/

	// Adreno(TM) 612:  25 ms - 30ms
	float2 A00 = convert_float2(vload2(0, src + mad24(src_y_floor, src_step, src_x_floor)));
    float2 A10 = convert_float2(vload2(0, src + mad24(src_y_floor_1, src_step , src_x_floor)));
	float2 v_alpha = (float2)(1.0f - alpha, alpha );
	float a0 = dot(A00, v_alpha);
	float a1 = dot(A10, v_alpha);
	dst[y*dst_step + x] = (uchar)(a0 + (a1 - a0) * beta);
}

kernel void SplitNV21Channel
(
	const global uchar *uvSrc,
	int src_step,
	int src_cols,
	int src_rows,
	global uchar *uDst,
	global uchar *vDst,
	int dst_step,
	int dst_cols,
	int dst_rows
)
{
    //获取当前图像行和列
    int x = get_global_id(0);
	int y = get_global_id(1);

	uDst[y*dst_step + x] = uvSrc[y*src_step + 2*x];
	vDst[y*dst_step + x] = uvSrc[y*src_step + 2*x + 1];
}


kernel void MergeNV21Channel
(
	global const uchar *uSrc, 
	global const uchar *vSrc, 
	int src_step,
	int src_cols,
	int src_rows,
	global uchar *uvDst, 
	int dst_step,
	int dst_cols,
	int dst_rows
)
{
    //获取当前图像行和列
   	int x = get_global_id(0);
	int y = get_global_id(1);

	uvDst[y*dst_step + x*2] = uSrc[y*src_step + x];
	uvDst[y*dst_step + x*2 + 1] = uSrc[y*src_step + x];
}


kernel void ImageSubImage
(
	global uchar *srcDst, 
	int src_step,
	int src_cols,
	int src_rows,
	global uchar *src, 
	int dst_step,
	int dst_cols,
	int dst_rows
)
{
    //获取当前图像行和列
   	int x = get_global_id(0);
	int y = get_global_id(1);

	int srcDstVal = srcDst[y*src_step + x];
	int srcVal = src[y*dst_step + x];
	srcDstVal = srcDstVal - srcVal + 128;
	srcDstVal = srcDstVal > 255 ? 255 : srcDstVal;
	srcDstVal = srcDstVal < 0 ? 0 : srcDstVal;
	srcDst[y*src_step + x] = srcDstVal;
}

kernel void ImageAddImage
(
	global uchar *srcDst, 
	int src_step,
	int src_cols,
	int src_rows,
	global uchar *src, 
	int dst_step,
	int dst_cols,
	int dst_rows
)
{
    //获取当前图像行和列
   	int x = get_global_id(0);
	int y = get_global_id(1);

	int srcDstVal = srcDst[y*src_step + x];
	int srcVal = src[y*dst_step + x];
	srcDstVal = srcDstVal + srcVal - 128;
	srcDstVal = srcDstVal > 255 ? 255 : srcDstVal;
	srcDstVal = srcDstVal < 0 ? 0 : srcDstVal;
	srcDst[y*src_step + x] = srcDstVal;
}


// inline int  GetBlockDiff(local uchar *pCurBlock, local uchar *pNeiBlock, int lPitch)
//     {
//         int lDif = 0;
//         int lPDif = 0;

//         lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         pCurBlock += lPitch;
//         pNeiBlock += lPitch;

//         lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         pCurBlock += lPitch;
//         pNeiBlock += lPitch;

//         lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         pCurBlock += lPitch;
//         pNeiBlock += lPitch;

//         lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;
//         lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
//         lPDif = MIN(49, lPDif);
//         lDif += lPDif;

//         return lDif;
//     }