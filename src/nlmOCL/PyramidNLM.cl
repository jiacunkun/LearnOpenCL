#ifndef	ABS
#define ABS(x)		((x) > 0 ? (x) : -(x))
#endif

#ifndef	MAX
#define MAX(min, x)		((x) > min ? (x) : min)
#endif

#ifndef	MIN
#define MIN(max, x)		((x) < max ? (x) : max)
#endif

#ifndef CLAMP
#define CLAMP(x, min, max)          \
    {                               \
        ((x) = (x) > min ? (x) : min);       \
        ((x) = (x) < max ? (x) : max);       \
    }
#endif

//用于NLM
#define ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pInvMap)        \
{                                                                            \
    lDif = ABS(lCVal - lNVal);                                                \
    lDif = MIN(49,lDif);                                                    \
    lDif = lDif * 9;                                                        \
    lTmpW = pInvMap[lDif];                                                    \
    lTmpW >>= 1;                                                            \
    lWSum += lTmpW;                                                            \
    lSumW += lNVal * lTmpW;                                                    \
}

////////////////////////////////////////////////////////////////////////////////
// 编写kernel
////////////////////////////////////////////////////////////////////////////////

#define GAUSS_121(a0,a1,a2) ((a0)+(2 * a1) + (a2))
/*Pyr3x3s2Down_f32in_f32out_kernel*/
kernel void PyramidDown
(
	global uchar* pSrc,
	int Src_Pitch,
	int Src_Width,
	int Src_Height,
	global uchar* pDst,
	int Dst_Pitch,
	int Dst_Width,
	int Dst_Height
)
{
	int idx = get_global_id(0);//dst_width
	int idy = get_global_id(1);

	int idx_x2 = idx << 1;
	int idy_x2 = idy << 1;

	int y_pre = max(0,idy_x2-1);//(idy_x2 - 1) < 0 ? 0 : (idy_x2 - 1);
	int y_cur = idy_x2;
	int y_next = min(idy_x2+1,Src_Height-1);//(idy_x2 + 1) > (Src_Height - 1) ? (Src_Height - 1) : (idy_x2 + 1);

	int x_pre = max(0,idx_x2-1);//(idx_x2 - 1) < 0 ? 0 : (idx_x2 - 1);
	int x_cur = idx_x2;
	int x_next = min(idx_x2+1,Src_Width-1);//(idx_x2 + 1) > (Src_Width - 1) ? (Src_Width - 1) : (idx_x2 + 1);

	__global uchar* src0 = (__global uchar*)(pSrc + y_pre * Src_Pitch);
	__global uchar* src1 = (__global uchar*)(pSrc + y_cur * Src_Pitch);
	__global uchar* src2 = (__global uchar*)(pSrc + y_next * Src_Pitch);

	__global uchar* dst = (__global uchar*)(pDst + idy * Dst_Pitch);

	int r0 = GAUSS_121(src0[x_pre], src0[x_cur], src0[x_next]);
	int r1 = GAUSS_121(src1[x_pre], src1[x_cur], src1[x_next]);
	int r2 = GAUSS_121(src2[x_pre], src2[x_cur], src2[x_next]);

	int out = GAUSS_121(r0, r1, r2);
	out = (out+8)/16;
	out = MIN(255, out);

	dst[idx] = out;

	return;
}

// kernel void PyramidUp
// (
// 	const global uchar *src_ptr,
//     const int src_step,
//     const int src_cols,
//     const int src_rows,
//     global uchar *dst_ptr,
//     const int dst_step,
//     const int dst_cols,
//     const int dst_rows
// )
// {
// 	int x = get_global_id(0);
// 	int y = get_global_id(1);

// 	int x_src = x >> 1;
// 	int y_src = y >> 1;

// 	if (x%2 == 0 && y%2 == 0) // 偶数行列直接赋值
// 	{
// 		dst_ptr[y*dst_step + x] = src_ptr[y_src*src_step + x_src];
// 		return;
// 	}
	
// 	if (x%2 == 1 && y%2 == 0)
// 	{
// 		int left = max(x_src - 1, 0);
// 		int right = min(x_src + 1, src_cols - 1);

// 		dst_ptr[y*dst_step + x] = (src_ptr[y_src*src_step + left] + src_ptr[y_src*src_step + right] + 1) >> 1;
// 		return;
// 	}

// 	if (x%2 == 0 && y%2 == 1)
// 	{
// 		int top = max(y_src - 1, 0);
// 		int bottom = min(y_src + 1, dst_cols - 1);

// 		dst_ptr[y*dst_step + x] = (src_ptr[top*src_step + x_src] + src_ptr[bottom*src_step + x_src] + 1) >> 1;
// 		return;
// 	}

// 	if (x%2 == 1 && y%2 == 1)
// 	{
// 		int top = max(y_src - 1, 0);
// 		int bottom = min(y_src + 1, dst_cols - 1);
// 		int left = max(x_src - 1, 0);
// 		int right = min(x_src + 1, src_cols - 1);


// 		dst_ptr[y*dst_step + x] = (src_ptr[top*src_step + left] + src_ptr[bottom*src_step + right] + src_ptr[top*src_step + right] + src_ptr[bottom*src_step + left] + 2) >> 2;
// 		return;
// 	}

// 	return;
// }

/*BilinearResize_kernel*/
kernel void PyramidUp(global uchar *src_ptr,
                           const int src_step,
                           const int src_cols,
                           const int src_rows,
                           global uchar *dst_ptr,
                           const int dst_step,
                           const int dst_cols,
                           const int dst_rows)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);

	float hor_scale = (float)src_cols / (float)dst_cols;
	float ver_scale = (float)src_rows / (float)dst_rows;

	float pos = ((float)x + 1.0f) * hor_scale; // 修改此处控制图像偏移
	int left_pos = (int)fmax(pos - 0.5f, 0.0f);
	int right_pos = (int)fmin(pos + 0.5f, (float)src_cols - 1.0f);
	float hor_weight = fabs(pos - 0.5f - (float)left_pos);

	pos = ((float)y + 1.0f) * ver_scale; // 修改此处控制图像偏移
	int top_pos = (int)fmax(pos - 0.5f, 0.0f);
	int bottom_pos = (int)fmin(pos + 0.5f, (float)src_rows - 1.0f);
	float ver_weight = fabs(pos - 0.5f - (float)top_pos);

	float data00 = (float)src_ptr[mad24(src_step, top_pos, left_pos)];
	float data01 = (float)src_ptr[mad24(src_step, top_pos, right_pos)];
	float data10 = (float)src_ptr[mad24(src_step, bottom_pos, left_pos)];
	float data11 = (float)src_ptr[mad24(src_step, bottom_pos, right_pos)];

	float tmp0 = data00 + (data01 - data00) * hor_weight;
	float tmp1 = data10 + (data11 - data10) * hor_weight;
	float res = tmp0 + (tmp1 - tmp0) * ver_weight + 0.5;
	dst_ptr[mad24(dst_step, y, x)] = (unsigned char)clamp(res, 0.0f, 255.0f);

	return;
}

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
	dst[y*dst_step + x] = (uchar)(a0 + (a1 - a0) * beta + 0.5);
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
	uvDst[y*dst_step + x*2 + 1] = vSrc[y*src_step + x];
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



inline int  GetBlockDiff(const global uchar *pCurBlock, const global uchar *pNeiBlock, int lPitch)
{
    int lDif = 0;
    int lPDif = 0;

    lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    pCurBlock += lPitch;
    pNeiBlock += lPitch;

    lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    pCurBlock += lPitch;
    pNeiBlock += lPitch;

    lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    pCurBlock += lPitch;
    pNeiBlock += lPitch;

    lPDif = ABS(pCurBlock[ 0 ] - pNeiBlock[ 0 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 1 ] - pNeiBlock[ 1 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 2 ] - pNeiBlock[ 2 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;
    lPDif = ABS(pCurBlock[ 3 ] - pNeiBlock[ 3 ]);
    lPDif = MIN(49, lPDif);
    lDif += lPDif;

    return lDif;
}

inline void AddBlockSum(const global uchar *pNeiBlock, int lPitch, int *lSumWei, int lW)
{
    lSumWei[ 0 ] += pNeiBlock[ 0 ] * lW;
    lSumWei[ 1 ] += pNeiBlock[ 1 ] * lW;
    lSumWei[ 2 ] += pNeiBlock[ 2 ] * lW;
    lSumWei[ 3 ] += pNeiBlock[ 3 ] * lW;
    pNeiBlock += lPitch;

    lSumWei[ 4 ] += pNeiBlock[ 0 ] * lW;
    lSumWei[ 5 ] += pNeiBlock[ 1 ] * lW;
    lSumWei[ 6 ] += pNeiBlock[ 2 ] * lW;
    lSumWei[ 7 ] += pNeiBlock[ 3 ] * lW;
    pNeiBlock += lPitch;

    lSumWei[ 8 ] += pNeiBlock[ 0 ] * lW;
    lSumWei[ 9 ] += pNeiBlock[ 1 ] * lW;
    lSumWei[ 10 ] += pNeiBlock[ 2 ] * lW;
    lSumWei[ 11 ] += pNeiBlock[ 3 ] * lW;
    pNeiBlock += lPitch;

    lSumWei[ 12 ] += pNeiBlock[ 0 ] * lW;
    lSumWei[ 13 ] += pNeiBlock[ 1 ] * lW;
    lSumWei[ 14 ] += pNeiBlock[ 2 ] * lW;
    lSumWei[ 15 ] += pNeiBlock[ 3 ] * lW;

    lSumWei[ 16 ] += lW;
}

inline void AddBlockSumByNei
 (
    const global uchar *pCurBlock,
    const global uchar *pNeiBlock,
    int lPitch,
    int *lSumWei,
    const global int *pMap
 )
{
    int lBDif = GetBlockDiff(pCurBlock, pNeiBlock, lPitch); // 计算块匹配值d,
    CLAMP(lBDif, 1, 799);
    int lW = pMap[lBDif]; // 获取权重, 查表法, exp(-d/(h^2))
    lW >>= 1;  // ？？？
    AddBlockSum(pNeiBlock, lPitch, lSumWei, lW);
}


kernel void subImage
(
	global uchar *srcDst,
	global uchar *src
)
{
//	int srcDstVal = srcDst[0];
//	int srcVal = src[0];
//	srcDstVal = srcDstVal - srcVal + 128;
//	srcDstVal = srcDstVal > 255 ? 255 : srcDstVal;
//	srcDstVal = srcDstVal < 0 ? 0 : srcDstVal;
//	srcDst[0] = srcDstVal;
//
//	srcDstVal = srcDst[1];
//    srcVal = src[1];
//    srcDstVal = srcDstVal - srcVal + 128;
//    srcDstVal = srcDstVal > 255 ? 255 : srcDstVal;
//    srcDstVal = srcDstVal < 0 ? 0 : srcDstVal;
//    srcDst[1] = srcDstVal;
//
//    srcDstVal = srcDst[2];
//    srcVal = src[2];
//    srcDstVal = srcDstVal - srcVal + 128;
//    srcDstVal = srcDstVal > 255 ? 255 : srcDstVal;
//    srcDstVal = srcDstVal < 0 ? 0 : srcDstVal;
//    srcDst[2] = srcDstVal;
//
//    srcDstVal = srcDst[3];
//    srcVal = src[3];
//    srcDstVal = srcDstVal - srcVal + 128;
//    srcDstVal = srcDstVal > 255 ? 255 : srcDstVal;
//    srcDstVal = srcDstVal < 0 ? 0 : srcDstVal;
//    srcDst[3] = srcDstVal;
}


inline void GetBlockResult(const global uchar *pCurLine,
                           global uchar *pDstBlock,
                           int lPitch,
                           int *lSumWei,
                           const global int *pInvMap)
{
    int lSW = lSumWei[ 16 ];
    int lInvW = pInvMap[ lSW ];

    pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
    subImage(pDstBlock, pCurLine);

    pDstBlock += lPitch;
    pCurLine += lPitch;
    lSumWei += 4;




    pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
    subImage(pDstBlock, pCurLine);

    pDstBlock += lPitch;
    pCurLine += lPitch;
    lSumWei += 4;



    pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
    subImage(pDstBlock, pCurLine);
    pDstBlock += lPitch;
    pCurLine += lPitch;
    lSumWei += 4;



    pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
    subImage(pDstBlock, pCurLine);

}




constant ushort4 v49 = {49, 49, 49, 49};
inline void AddBlockSumByNei4
 (
     short4* vCur,
    const global uchar *pNeiBlock,
    int lPitch,
    int4 *vSumWei,
    int *lSumWei,
    const global int *pMap
 )
{

    short4 neiBlock0 = convert_short4(vload4(0, pNeiBlock));   pNeiBlock += lPitch;
    short4 neiBlock1 = convert_short4(vload4(0, pNeiBlock));   pNeiBlock += lPitch;
    short4 neiBlock2 = convert_short4(vload4(0, pNeiBlock));   pNeiBlock += lPitch;
    short4 neiBlock3 = convert_short4(vload4(0, pNeiBlock));




    ushort4 vDiffSum;
    ushort4 vDiff = abs_diff(vCur[0], neiBlock0);
    vDiff = min(vDiff, v49);
    vDiffSum = vDiff;


    vDiff = abs_diff(vCur[1], neiBlock1);
    vDiff = min(vDiff, v49);
    vDiffSum += vDiff;


    vDiff = abs_diff(vCur[2], neiBlock2);
    vDiff = min(vDiff, v49);
    vDiffSum += vDiff;


    vDiff = abs_diff(vCur[3], neiBlock3);
    vDiff = min(vDiff, v49);
    vDiffSum += vDiff;



    /// ==========================
    ushort lDif = vDiffSum.s0 + vDiffSum.s1 + vDiffSum.s2 + vDiffSum.s3;
    int lW = pMap[lDif]; // 获取权重, 查表法, exp(-d/(h^2))
    lW >>= 1;  // ？？？


    int4 vWeight = {lW, lW, lW, lW};
//    mad(convert_int4(neiBlock0), vWeight, vSumWei[0]);   pNeiBlock += lPitch;
//    mad(convert_int4(neiBlock1), vWeight, vSumWei[1]);   pNeiBlock += lPitch;
//    mad(convert_int4(neiBlock2), vWeight, vSumWei[2]);   pNeiBlock += lPitch;
//    mad(convert_int4(neiBlock3), vWeight, vSumWei[3]);   pNeiBlock += lPitch;

    vSumWei[0] += convert_int4(neiBlock0) * vWeight;
    vSumWei[1] += convert_int4(neiBlock1) * vWeight;
    vSumWei[2] += convert_int4(neiBlock2) * vWeight;
    vSumWei[3] += convert_int4(neiBlock3) * vWeight;
    *lSumWei += lW;
}





constant int4 vValue0 = {0, 0, 0, 0};
constant int4 v256 = {255, 255, 255, 255};
constant int4 v128 = {128, 128, 128, 128};
inline void ProcessBlock4x4(const global uchar *pCurLine,
                            const global uchar *pPreLine,
                            const global uchar *pNexLine,
                            global uchar *pDstLine,
                            const global int *pMap,
                            int lPitch,
                            const global int *pInvMap
                            )
{
    // 当前块, 权重设置为最大值256
//    AddBlockSum(pCurLine, lPitch, lSumWei, 256);

    short4 vCur[4];
    const global uchar *pNeiBlock = pCurLine;
    vCur[0] = convert_short4(vload4(0, pNeiBlock));    pNeiBlock += lPitch;
    vCur[1] = convert_short4(vload4(0, pNeiBlock));    pNeiBlock += lPitch;
    vCur[2] = convert_short4(vload4(0, pNeiBlock));    pNeiBlock += lPitch;
    vCur[3] = convert_short4(vload4(0, pNeiBlock));


    int lSumWei = 256;
    int4 vSumWei[4];
    vSumWei[0] = convert_int4(vCur[0]) << 8;
    vSumWei[1] = convert_int4(vCur[1]) << 8;
    vSumWei[2] = convert_int4(vCur[2]) << 8;
    vSumWei[3] = convert_int4(vCur[3]) << 8;




    // 领域8个位置
    AddBlockSumByNei4(vCur, pCurLine - 1, lPitch, vSumWei, &lSumWei, pMap);
    AddBlockSumByNei4(vCur, pCurLine + 1, lPitch, vSumWei, &lSumWei, pMap);

    AddBlockSumByNei4(vCur, pPreLine - 1, lPitch, vSumWei, &lSumWei, pMap);
    AddBlockSumByNei4(vCur, pPreLine, lPitch, vSumWei, &lSumWei, pMap);
    AddBlockSumByNei4(vCur, pPreLine + 1, lPitch, vSumWei, &lSumWei, pMap);

    AddBlockSumByNei4(vCur, pNexLine - 1, lPitch, vSumWei, &lSumWei, pMap);
    AddBlockSumByNei4(vCur, pNexLine, lPitch, vSumWei, &lSumWei, pMap);
    AddBlockSumByNei4(vCur, pNexLine + 1, lPitch, vSumWei, &lSumWei, pMap);



    // average / sweight
//    GetBlockResult(pCurLine, pDstLine, lPitch, lSumWei, pInvMap);

    int lSW = lSumWei;
    int lInvW = pInvMap[ lSW ];
    int4 vInvW = {lInvW, lInvW, lInvW, lInvW};

    int4 out;
    uchar4 out_char4;
    out = (vSumWei[0] * vInvW + ( 1 << 19 )) >> 20;
    out_char4 = convert_uchar4(out);
    vstore4(out_char4, 0, pDstLine);
    pDstLine += lPitch;

    out = (vSumWei[1] * vInvW + ( 1 << 19 )) >> 20;
    out_char4 = convert_uchar4(out);
    vstore4(out_char4, 0, pDstLine);
    pDstLine += lPitch;

    out = (vSumWei[2] * vInvW + ( 1 << 19 )) >> 20;
    out_char4 = convert_uchar4(out);
    vstore4(out_char4, 0, pDstLine);
    pDstLine += lPitch;

    out = (vSumWei[3] * vInvW + ( 1 << 19 )) >> 20;
    out_char4 = convert_uchar4(out);
    vstore4(out_char4, 0, pDstLine);
    pDstLine += lPitch;


//    int4 out;
//    uchar4 out_char4;
//    out = (vSumWei[0] * vInvW + ( 1 << 19 )) >> 20;
//    out = out - convert_int4(vCur[0]) + v128;
//    out = clamp(out, vValue0, v256);
//    vstore4(convert_uchar4(out), 0, pDstLine);
//    pDstLine += lPitch;
//
//    out = (vSumWei[1] * vInvW + ( 1 << 19 )) >> 20;
//    out = out - convert_int4(vCur[1]) + v128;
//    out = clamp(out, vValue0, v256);
//    vstore4(convert_uchar4(out), 0, pDstLine);
//    pDstLine += lPitch;
//
//    out = (vSumWei[2] * vInvW + ( 1 << 19 )) >> 20;
//    out = out - convert_int4(vCur[2]) + v128;
//    out = clamp(out, vValue0, v256);
//    vstore4(convert_uchar4(out), 0, pDstLine);
//    pDstLine += lPitch;
//
//    out = (vSumWei[3] * vInvW + ( 1 << 19 )) >> 20;
//    out = out - convert_int4(vCur[3]) + v128;
//    out = clamp(out, vValue0, v256);
//    vstore4(convert_uchar4(out), 0, pDstLine);
//    pDstLine += lPitch;
}



// 单行降噪
inline void ProcessLinesBroundMain(const global uchar *pCurLine,
                                          const global uchar *pPreLine, 
                                           global uchar *pDstLine,
                                          int lWidth,
                                          const global int *pMap,
                                          const global int *pInvMap)
{
    int lWSum = 0;
    int lSumW = 0;
    int lCVal = 0, lNVal = 0;
    int lDVal = 0;
    int lTmpW = 0;
    int lInvW = 0;
    int lDif = 0;
    int x = 0;
    //left point
    lCVal = pCurLine[ 0 ];
    lWSum = 256;
    lSumW = 256 * lCVal;
    lNVal = pCurLine[ 1 ];

    ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
    lNVal = pPreLine[ 0 ];
    ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
    lNVal = pPreLine[ 1 ];
    ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

    lInvW = pInvMap[ lWSum ];
    lDVal = ( lSumW * lInvW + ( 1 << 19 )) >> 20;
    //pDstLine[0] = (lDVal - lCVal >> 1) + 128;
    pDstLine[ 0 ] = lDVal;

    //med point
    for(x = 1; x < lWidth - 1; x++)
    {
        lCVal = pCurLine[ x ];
        lWSum = 256;
        lSumW = 256 * lCVal;

        lNVal = pCurLine[ x - 1 ];
        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
        lNVal = pCurLine[ x + 1 ];
        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

        lNVal = pPreLine[ x - 1 ];
        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
        lNVal = pPreLine[ x ];
        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
        lNVal = pPreLine[ x + 1 ];
        ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

        lInvW = pInvMap[ lWSum ];
        lDVal = ( lSumW * lInvW + +( 1 << 19 )) >> 20;
        pDstLine[ x ] = lDVal;
        //pDstLine[x] = (lDVal - lCVal >> 1) + 128;
    }

    //right point
    lCVal = pCurLine[ lWidth - 1 ];
    lWSum = 256;
    lSumW = 256 * lCVal;

    lNVal = pCurLine[ lWidth - 2 ];
    ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
    lNVal = pPreLine[ lWidth - 2 ];
    ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);
    lNVal = pPreLine[ lWidth - 1 ];
    ADD_POINT_WEI(lNVal, lCVal, lDif, lWSum, lSumW, lTmpW, pMap);

    lInvW = pInvMap[ lWSum ];
    lDVal = ( lSumW * lInvW + ( 1 << 19 )) >> 20;
    pDstLine[ lWidth - 1 ] = lDVal;
}

 inline void ProcessPointBround(const global uchar *pCurPoint,
                                const global uchar *pPrePoint,
                                const global uchar *pNexPoint,
                                global uchar *pDstPoint,
                                const global int *pMap,
                                const global int *pInvMap,
                                int l_add)
 {
     int lNVal = 0;
     int lDif = 0, lTmpW = 0;
     int lInvW = 0, lDVal = 0;

     int nCurrentVal = pCurPoint[ 0 ];
     int nWeight = 256;
     int nAverage = nCurrentVal * 256;

     lNVal = pCurPoint[ l_add ];
     ADD_POINT_WEI(lNVal, nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

     lNVal = pPrePoint[ 0 ];
     ADD_POINT_WEI(lNVal, nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
     lNVal = pPrePoint[ l_add ];
     ADD_POINT_WEI(lNVal, nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

     lNVal = pNexPoint[ 0 ];
     ADD_POINT_WEI(lNVal, nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
     lNVal = pNexPoint[ l_add ];
     ADD_POINT_WEI(lNVal, nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

     lInvW = pInvMap[ nWeight ];
     lDVal = ( nAverage * lInvW + ( 1 << 19 )) >> 20;
     pDstPoint[ 0 ] = lDVal;
 }

inline void ProcessPoint(const global uchar *pCurPoint,
                         const global uchar *pPrePoint,
                         const global uchar *pNexPoint,
                         global uchar *pDstPoint,
                         const global int *pMap,
                         const global int *pInvMap)
{
    int lDif = 0;
    int lTmpW = 0;

    int nCurrentVal = pCurPoint[ 0 ];
    int nWeight = 256;
    int nAverage = nCurrentVal * nWeight;

    ADD_POINT_WEI(pCurPoint[ -1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
    ADD_POINT_WEI(pCurPoint[ 1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

    ADD_POINT_WEI(pPrePoint[ -1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
    ADD_POINT_WEI(pPrePoint[ 0 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
    ADD_POINT_WEI(pPrePoint[ 1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

    ADD_POINT_WEI(pNexPoint[ -1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
    ADD_POINT_WEI(pNexPoint[ 0 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);
    ADD_POINT_WEI(pNexPoint[ 1 ], nCurrentVal, lDif, nWeight, nAverage, lTmpW, pMap);

    int lInvW = pInvMap[ nWeight ];
    int lDVal = ( nAverage * lInvW + ( 1 << 19 )) >> 20;
    pDstPoint[ 0 ] = lDVal;
}

inline void ProcessLines1Main(const global uchar *pCurLine,
                              const global uchar *pPreLine,
                              const global uchar *pNexLine,
                              global uchar *pDstLine,
                              int lWidth,
                              const global int *pMap,
                              const global int *pInvMap)
{
    //left point
    ProcessPointBround(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, 1);

    //med point
    pCurLine++;
    pPreLine++;
    pNexLine++;
    pDstLine++;
    for(int x = 1; x <= 4; x++)
    {
        ProcessPoint(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap);
        pCurLine++;
        pPreLine++;
        pNexLine++;
        pDstLine++;
    }

    //right point
    ProcessPointBround(pCurLine, pPreLine, pNexLine, pDstLine, pMap, pInvMap, -1);
}


kernel void NLMDenoise
(
	const global uchar *pSrc,
	int src_step,
	int src_cols,
	int src_rows,
	global uchar *pDst,
	int dst_step,
	int dst_cols,
	int dst_rows,
	const global int *pMap,
	const global int *pInvMap
)
{
    //获取当前图像行和列
    int x = get_global_id(0)*4;
	int y = get_global_id(1)*4 + 1;

	// 第一行
     if (y == 1)
     {
        const global uchar *pCurLine = pSrc + src_step * (y - 1) + x;
        global uchar *pDstLine = pDst + dst_step * (y - 1) + x;
     	ProcessLinesBroundMain(pCurLine, pCurLine + src_step, pDstLine, src_cols, pMap, pInvMap);
     }

	// 最后几行
	 if (src_rows - y <= 4)
     {
     	int diff = src_rows - y;
        for (int i = 0; i < diff-1; i++)
        {
            const global uchar *pCurLine = pSrc + src_step * (y + i) + x;
            global uchar *pDstLine = pDst + dst_step * (y + i) + x;
	   	    ProcessLines1Main(pCurLine, pCurLine - src_step, pCurLine + src_step, pDstLine, src_cols, pMap, pInvMap);
	    }
		{
			const global uchar *pCurLine = pSrc + src_step * (y + diff - 1) + x;
        	global uchar *pDstLine = pDst + dst_step * (y + diff - 1) + x;
     		ProcessLinesBroundMain(pCurLine, pCurLine - src_step, pDstLine, src_cols, pMap, pInvMap);
		}
	 }

	// 中间行
	if (y >= 1 && y < src_rows - 4)
    {
		// 最左边的点
		if (x == 0)
		{
			const global uchar *pCurLine = pSrc + src_step * y + x;
    		global uchar *pDstLine = pDst + dst_step * y + x;
			int lShift = 0;
            ProcessPointBround(pCurLine, pCurLine - src_step, pCurLine + src_step, pDstLine, pMap, pInvMap, 1);
            lShift += src_step;
            ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, 1);
            lShift += src_step;
            ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, 1);
            lShift += src_step;
            ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, 1);
            lShift += src_step;
		}
		// 最右边的几个点
		else if (src_cols - x <= 4)
		{
			int diff = src_cols - x;
			for (int i = 0; i < diff-1; i++)
			{
			    const global uchar *pCurLine = pSrc + src_step * y + x + i;
    		    global uchar *pDstLine = pDst + dst_step * y + x + i;
			    int lShift = 0;
			    ProcessPoint(pCurLine, pCurLine - src_step, pCurLine + src_step, pDstLine, pMap, pInvMap);
                lShift += src_step;
                ProcessPoint(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap);
                lShift += src_step;
                ProcessPoint(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap);
                lShift += src_step;
                ProcessPoint(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap);
                lShift += src_step;
			}

			//最右边的点
			{
			    const global uchar *pCurLine = pSrc + src_step * y + x + diff-1;
    		    global uchar *pDstLine = pDst + dst_step * y + x + diff-1;
			    int lShift = 0;
                ProcessPointBround(pCurLine, pCurLine - src_step, pCurLine + src_step, pDstLine, pMap, pInvMap, -1);
                lShift += src_step;
                ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, -1);
                lShift += src_step;
                ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, -1);
                lShift += src_step;
                ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, -1);
                lShift += src_step;
			}

		}

		// 中间点
		if (x >=0 && x < src_cols - 4)
		{
			const global uchar *pCurLine = pSrc + src_step * y + x + 1;
            const global uchar *pPreLine = pCurLine - src_step;
            const global uchar *pNexLine = pCurLine + src_step;
    		global uchar *pDstLine = pDst + dst_step * y + x + 1;

			ProcessBlock4x4(pCurLine,
                            pPreLine,
                            pNexLine,
                        	pDstLine,
                        	pMap,
                        	src_step,
                        	pInvMap);
		}
	}
}




///// yz123  0
//kernel void NLMDenoise
//(
//	const global uchar *pSrc,
//	int src_step,
//	int src_cols,
//	int src_rows,
//	global uchar *pDst,
//	int dst_step,
//	int dst_cols,
//	int dst_rows,
//	const global int *pMap,
//	const global int *pInvMap
//)
//{
//    //获取当前图像行和列
//    int x = get_global_id(0)*4 + 1;
//	int y = get_global_id(1)*4 + 1;
//
//	int lSumWei[17] = {0};
//
//	const global uchar *pCurLine = pSrc + src_step * y + x;
//    const global uchar *pPreLine = pCurLine - src_step;
//    const global uchar *pNexLine = pCurLine + src_step;
//    global uchar *pDstLine = pDst + dst_step * y + x;
//
//    if (y >= 1 && y < src_rows - 4 && x >=1 && x < src_cols - 4) {
//        ProcessBlock4x4(pCurLine,
//                        pPreLine,
//                        pNexLine,
//                        pDstLine,
//                        pMap,
//                        src_step,
//                        lSumWei,
//                        pInvMap);
//     }
//}