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
    lTmpW = (int)(pInvMap[lDif]);                                                    \
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
	int dst_x = get_global_id(0);//dst_width
	int dst_y = get_global_id(1);

	int src_x = dst_x << 1;
	int src_x_pre = max(0, src_x - 1);
	int src_x_next = min(src_x + 1,Src_Width - 1);
	int src_y = dst_y << 1;
	int src_y_pre = max(0, src_y - 1);
	int src_y_next = min(src_y + 1,Src_Height - 1);


	__global uchar* src0 = (__global uchar*)(pSrc + src_y_pre * Src_Pitch);
	__global uchar* src1 = (__global uchar*)(pSrc + src_y * Src_Pitch);
	__global uchar* src2 = (__global uchar*)(pSrc + src_y_next * Src_Pitch);
	__global uchar* dst = (__global uchar*)(pDst + dst_y * Dst_Pitch);


	int r0 = GAUSS_121(src0[src_x_pre], src0[src_x], src0[src_x_next]);
	int r1 = GAUSS_121(src1[src_x_pre], src1[src_x], src1[src_x_next]);
	int r2 = GAUSS_121(src2[src_x_pre], src2[src_x], src2[src_x_next]);

	int out = GAUSS_121(r0, r1, r2);
	out = (out+8)/16;
	out = min(255, out);

	dst[dst_x] = out;

	return;
}


///// 会往左上角片0.5个像素值
//constant ushort8 vValue8 = {8,8,8,8,8,8,8,8};
//kernel void PyramidDown
//(
//	global uchar* pSrc,
//	int Src_Pitch,
//	int Src_Width,
//	int Src_Height,
//	global uchar* pDst,
//	int Dst_Pitch,
//	int Dst_Width,
//	int Dst_Height
//)
//{
//	int dst_x = get_global_id(0) * 8;//dst_width
//	int dst_y = get_global_id(1);
//
//
//	int src_y = dst_y << 1;
//	int src_y_pre = max(0, src_y - 1);
//	int src_y_next = min(src_y + 1, Src_Height - 1);
//	__global uchar* pSrc0 = (__global uchar*)(pSrc + src_y_pre * Src_Pitch);
//	__global uchar* pSrc1 = (__global uchar*)(pSrc + src_y * Src_Pitch);
//	__global uchar* pSrc2 = (__global uchar*)(pSrc + src_y_next * Src_Pitch);
//
//
//    uchar8 s0[3], s1[3], s2[3];
//    if(dst_x == 0)
//    {
//        s0[1] = vload8(0, pSrc0);
//        s1[1] = vload8(0, pSrc1);
//        s2[1] = vload8(0, pSrc2);
//
//
//        s0[0] = (uchar8)(s0[1].s0, s0[1].s0123, s0[1].s456);
//        s1[0] = (uchar8)(s1[1].s0, s1[1].s0123, s1[1].s456);
//        s2[0] = (uchar8)(s2[1].s0, s2[1].s0123, s2[1].s456);
//
//
//        s0[2] = (uchar8)(s0[1].s1234, s0[1].s567, pSrc0[8]);
//        s1[2] = (uchar8)(s1[1].s1234, s0[1].s567, pSrc1[8]);
//        s2[2] = (uchar8)(s2[1].s1234, s0[1].s567, pSrc2[8]);
//
//    } else if(dst_x == Dst_Width - 1) {
//
//
//    } else {
//        int src_x = dst_x << 1;
//        uchar2 tmp;
//
//        pSrc0 += src_x - 1;
//        s0[0] = vload8(0, pSrc0);
//        tmp = vload2(0, pSrc0 + 8);
//        s0[1] = (uchar8)(s0[0].s1234, s0[0].s567, tmp.s0);
//        s0[2] = (uchar8)(s0[0].s2345, s0[0].s67, tmp.s01);
//
//
//        pSrc1 += src_x - 1;
//        s1[0] = vload8(0, pSrc1);
//        tmp = vload2(0, pSrc1 + 8);
//        s1[1] = (uchar8)(s1[0].s1234, s1[0].s567, tmp.s0);
//        s1[2] = (uchar8)(s1[0].s2345, s1[0].s67, tmp.s01);
//
//
//        pSrc2 += src_x - 1;
//        s2[0] = vload8(0, pSrc2);
//        tmp = vload2(0, pSrc2 + 8);
//        s2[1] = (uchar8)(s2[0].s1234, s2[0].s567, tmp.s0);
//        s2[2] = (uchar8)(s2[0].s2345, s2[0].s67, tmp.s01);
//    }
//
//
//    ushort8 r0 = convert_ushort8(s0[0]) + (convert_ushort8(s0[1]) << 1) + convert_ushort8(s0[2]);
//    ushort8 r1 = convert_ushort8(s1[0]) + (convert_ushort8(s1[1]) << 1) + convert_ushort8(s1[2]);
//    ushort8 r2 = convert_ushort8(s2[0]) + (convert_ushort8(s2[1]) << 1) + convert_ushort8(s2[2]);
//
//    ushort8 out = r0 + (r1 << 1) + r2;
//    out = (out + vValue8) >> 4;
//	out = clamp(out, 0, 255);
//
//    __global uchar* dst = (__global uchar*)(pDst + dst_y * Dst_Pitch);
//	vstore8(convert_uchar8(out), 0, dst + dst_x);
//}



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

///*BilinearResize_kernel*/
//kernel void PyramidUp(global uchar *src_ptr,
//                           const int src_step,
//                           const int src_cols,
//                           const int src_rows,
//                           global uchar *dst_ptr,
//                           const int dst_step,
//                           const int dst_cols,
//                           const int dst_rows,
//                           global uchar *pOut,
//                           int out_step)
//{
//	const int x = get_global_id(0);
//	const int y = get_global_id(1);
//
//	float hor_scale = (float)src_cols / (float)dst_cols;
//	float ver_scale = (float)src_rows / (float)dst_rows;
//
//	float pos = ((float)x + 1.0f) * hor_scale; // 修改此处控制图像偏移
//	int left_pos = (int)fmax(pos - 0.5f, 0.0f);
//	int right_pos = (int)fmin(pos + 0.5f, (float)src_cols - 1.0f);
//	float hor_weight = fabs(pos - 0.5f - (float)left_pos);
//
//	pos = ((float)y + 1.0f) * ver_scale; // 修改此处控制图像偏移
//	int top_pos = (int)fmax(pos - 0.5f, 0.0f);
//	int bottom_pos = (int)fmin(pos + 0.5f, (float)src_rows - 1.0f);
//	float ver_weight = fabs(pos - 0.5f - (float)top_pos);
//
//	float data00 = (float)src_ptr[mad24(src_step, top_pos, left_pos)];
//	float data01 = (float)src_ptr[mad24(src_step, top_pos, right_pos)];
//	float data10 = (float)src_ptr[mad24(src_step, bottom_pos, left_pos)];
//	float data11 = (float)src_ptr[mad24(src_step, bottom_pos, right_pos)];
//
//	float tmp0 = data00 + (data01 - data00) * hor_weight;
//	float tmp1 = data10 + (data11 - data10) * hor_weight;
//	float res = tmp0 + (tmp1 - tmp0) * ver_weight + 0.5;
//	uchar dst = (unsigned char)clamp(res, 0.0f, 255.0f);
//	int dst_index = mad24(dst_step, y, x);
//	dst_ptr[dst_index] = dst;
//
//
//
////    dst_index = mad24(out_step, y, x);
////    int out = pOut[dst_index];
////    out = out + (int)(dst) - 128;
////    out = clamp(out, 0, 255);
////    pOut[dst_index] = out;
//}


#define SetOut(secDst, src) \
    out = secDst; \
    out = out + (int)(src) - 128; \
    out = clamp(out, 0, 255);\
    secDst = out;

/*BilinearResize_kernel*/
kernel void PyramidUp(global uchar *src_ptr,
                           const int src_step,
                           const int src_cols,
                           const int src_rows,
                           global uchar *dst_ptr,
                           const int dst_step)
{
	const int src_x = get_global_id(0);
	const int src_y = get_global_id(1);

	int dst_x = src_x << 1;
	int dst_y = src_y << 1;


	__global uchar* pSrc0 = src_ptr + src_step * src_y;
	__global uchar* pSrc1 = src_ptr + src_step * min(src_y + 1, src_rows - 1);
    int src_index = min(src_x + 1, src_cols - 1);
	uchar data00 = pSrc0[src_x];
	uchar data01 = pSrc0[src_index];
	uchar data10 = pSrc1[src_x];
	uchar data11 = pSrc1[src_index];


	uchar out00 = data00;
	uchar out01 = ((ushort)data00 + data01 + 1) >> 1;
	uchar out10 = ((ushort)data00 + data10 + 1) >> 1;
	uchar out11 = ((ushort)data01 + data11 + 1) >> 1;


    __global uchar* pOut0 = dst_ptr + dst_step * dst_y;
    __global uchar* pOut1 = dst_ptr + dst_step * (dst_y + 1);
    int out;
    SetOut(pOut0[dst_x],     out00)
    SetOut(pOut0[dst_x + 1], out01)
    SetOut(pOut1[dst_x],     out10)
    SetOut(pOut1[dst_x + 1], out11)
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






#define AddBlockSumByNei_Define() \
    vDiff = abs_diff(vCur[0], neiBlock0);       \
    vDiff = min(vDiff, v49);                    \
    vDiffSum = vDiff;                           \
                                                \
    vDiff = abs_diff(vCur[1], neiBlock1);       \
    vDiff = min(vDiff, v49);                    \
    vDiffSum += vDiff;                          \
                                                \
    vDiff = abs_diff(vCur[2], neiBlock2);       \
    vDiff = min(vDiff, v49);                    \
    vDiffSum += vDiff;                          \
                                                \
    vDiff = abs_diff(vCur[3], neiBlock3);       \
    vDiff = min(vDiff, v49);                    \
    vDiffSum += vDiff;                          \
                                                \
    lDif = vDiffSum.s0 + vDiffSum.s1 + vDiffSum.s2 + vDiffSum.s3;    \
    lW = (int)(pMap[lDif]);                                          \
    lW >>= 1;                                                        \
                                                                     \
    vWeight = (int4)(lW, lW, lW, lW);                                \
    vSumWei[0] += convert_int4(neiBlock0) * vWeight;                 \
    vSumWei[1] += convert_int4(neiBlock1) * vWeight;                 \
    vSumWei[2] += convert_int4(neiBlock2) * vWeight;                 \
    vSumWei[3] += convert_int4(neiBlock3) * vWeight;                 \
    lSumWei += lW;


#define GetBlockResult_Define(vSumWei_i)                     \
    out = (vSumWei_i * vInvW + ( 1 << 19 )) >> 20;                   \
    out_char4 = convert_uchar4(out);                                \
    vstore4(out_char4, 0, pDstLine);                                \
    pDstLine += lPitch;                                             \


#define GetBlockResult_Define2(vSumWei_i, vCur_i)                     \
    out = (vSumWei_i * vInvW + ( 1 << 19 )) >> 20;                   \
    out_char4 = convert_uchar4(out);                                \
    out = convert_int4(out_char4) - convert_int4(vCur_i) + v128; \
    out = clamp(out, vValue0, v256);  \
    out_char4 = convert_uchar4(out);  \
    vstore4(out_char4, 0, pDstLine);                                \
    pDstLine += lPitch;




/// new
constant int4 vValue0 = {0, 0, 0, 0};
constant int4 v256 = {255, 255, 255, 255};
constant int4 v128 = {128, 128, 128, 128};
inline void ProcessBlock4x4(const global uchar *pCurLine,
                            const global uchar *pPreLine,
                            const global uchar *pNexLine,
                            global uchar *pDstLine,
                            const global uchar *pMap,
                            int lPitch,
                            const global int *pInvMap,
                            int isSubImage
                            )
{
    uchar8 data[6];
    const global uchar *pNeiBlock = pCurLine - lPitch - 1;
    data[0] = vload8(0, pNeiBlock);   pNeiBlock += lPitch;
    data[1] = vload8(0, pNeiBlock);   pNeiBlock += lPitch;
    data[2] = vload8(0, pNeiBlock);   pNeiBlock += lPitch;
    data[3] = vload8(0, pNeiBlock);   pNeiBlock += lPitch;
    data[4] = vload8(0, pNeiBlock);   pNeiBlock += lPitch;
    data[5] = vload8(0, pNeiBlock);   pNeiBlock += lPitch;


    /// ========================================================
    /// 中心
    /// ========================================================
    short4 vCur[4];
    vCur[0] = convert_short4((uchar4)(data[1].s1234));
    vCur[1] = convert_short4((uchar4)(data[2].s1234));
    vCur[2] = convert_short4((uchar4)(data[3].s1234));
    vCur[3] = convert_short4((uchar4)(data[4].s1234));


    int lSumWei = 256;
    int4 vSumWei[4];
    vSumWei[0] = convert_int4(vCur[0]) << 8;
    vSumWei[1] = convert_int4(vCur[1]) << 8;
    vSumWei[2] = convert_int4(vCur[2]) << 8;
    vSumWei[3] = convert_int4(vCur[3]) << 8;


    /// ========================================================
    /// 中, 左
    /// ========================================================
    short4 neiBlock0, neiBlock1, neiBlock2, neiBlock3;
    ushort4 vDiffSum;
    ushort4 vDiff;
    ushort lDif;
    int lW;
    int4 vWeight;



    /// ========================================================
    /// 中, 左
    /// ========================================================
    neiBlock0 = convert_short4((uchar4)(data[1].s0123));
    neiBlock1 = convert_short4((uchar4)(data[2].s0123));
    neiBlock2 = convert_short4((uchar4)(data[3].s0123));
    neiBlock3 = convert_short4((uchar4)(data[4].s0123));

    AddBlockSumByNei_Define()


    /// ========================================================
    /// 中, 右
    /// ========================================================
    neiBlock0 = convert_short4((uchar4)(data[1].s2345));
    neiBlock1 = convert_short4((uchar4)(data[2].s2345));
    neiBlock2 = convert_short4((uchar4)(data[3].s2345));
    neiBlock3 = convert_short4((uchar4)(data[4].s2345));

    AddBlockSumByNei_Define()


    /// ========================================================
    /// 上, 左
    /// ========================================================
    neiBlock0 = convert_short4((uchar4)(data[0].s0123));
    neiBlock1 = convert_short4((uchar4)(data[1].s0123));
    neiBlock2 = convert_short4((uchar4)(data[2].s0123));
    neiBlock3 = convert_short4((uchar4)(data[3].s0123));

    AddBlockSumByNei_Define()


    /// ========================================================
    /// 上, 中
    /// ========================================================
    neiBlock0 = convert_short4((uchar4)(data[0].s1234));
    neiBlock1 = convert_short4((uchar4)(data[1].s1234));
    neiBlock2 = convert_short4((uchar4)(data[2].s1234));
    neiBlock3 = convert_short4((uchar4)(data[3].s1234));

    AddBlockSumByNei_Define()


    /// ========================================================
    /// 上, 左
    /// ========================================================
    neiBlock0 = convert_short4((uchar4)(data[0].s2345));
    neiBlock1 = convert_short4((uchar4)(data[1].s2345));
    neiBlock2 = convert_short4((uchar4)(data[2].s2345));
    neiBlock3 = convert_short4((uchar4)(data[3].s2345));

    AddBlockSumByNei_Define()


    /// ========================================================
    /// 下, 左
    /// ========================================================
    neiBlock0 = convert_short4((uchar4)(data[2].s0123));
    neiBlock1 = convert_short4((uchar4)(data[3].s0123));
    neiBlock2 = convert_short4((uchar4)(data[4].s0123));
    neiBlock3 = convert_short4((uchar4)(data[5].s0123));

    AddBlockSumByNei_Define()


    /// ========================================================
    /// 下, 中
    /// ========================================================
    neiBlock0 = convert_short4((uchar4)(data[2].s1234));
    neiBlock1 = convert_short4((uchar4)(data[3].s1234));
    neiBlock2 = convert_short4((uchar4)(data[4].s1234));
    neiBlock3 = convert_short4((uchar4)(data[5].s1234));

    AddBlockSumByNei_Define()


    /// ========================================================
    /// 下, 左
    /// ========================================================
    neiBlock0 = convert_short4((uchar4)(data[2].s2345));
    neiBlock1 = convert_short4((uchar4)(data[3].s2345));
    neiBlock2 = convert_short4((uchar4)(data[4].s2345));
    neiBlock3 = convert_short4((uchar4)(data[5].s2345));

    AddBlockSumByNei_Define()




    /// ========================================================
    ///  average / sweight
    /// ========================================================
//    int lInvW = pInvMap[ lSumWei ];
    int lInvW = (1 << 20) / lSumWei;
    int4 vInvW = {lInvW, lInvW, lInvW, lInvW};

    int4 out;
    uchar4 out_char4;

    if(isSubImage > 0)
    {
        GetBlockResult_Define2(vSumWei[0], vCur[0])
        GetBlockResult_Define2(vSumWei[1], vCur[1])
        GetBlockResult_Define2(vSumWei[2], vCur[2])
        GetBlockResult_Define2(vSumWei[3], vCur[3])
    } else {
        GetBlockResult_Define(vSumWei[0])
        GetBlockResult_Define(vSumWei[1])
        GetBlockResult_Define(vSumWei[2])
        GetBlockResult_Define(vSumWei[3])
    }

}



// 单行降噪
inline void ProcessLinesBroundMain(const global uchar *pCurLine,
                                          const global uchar *pPreLine, 
                                           global uchar *pDstLine,
                                          int lWidth,
                                          const global uchar *pMap,
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
                                const global uchar *pMap,
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
                         const global uchar *pMap,
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
                              const global uchar *pMap,
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
//	const global uchar *pMap,
//	const global int *pInvMap,
//	int isSubImage
//)
//{
//    //获取当前图像行和列
//    int x = get_global_id(0)*4;
//	int y = get_global_id(1)*4 + 1;
//
//	// 第一行
//     if (y == 1)
//     {
//        const global uchar *pCurLine = pSrc + src_step * (y - 1) + x;
//        global uchar *pDstLine = pDst + dst_step * (y - 1) + x;
//     	ProcessLinesBroundMain(pCurLine, pCurLine + src_step, pDstLine, src_cols, pMap, pInvMap);
//     }
//
//	// 最后几行
//	 if (src_rows - y <= 4)
//     {
//     	int diff = src_rows - y;
//        for (int i = 0; i < diff-1; i++)
//        {
//            const global uchar *pCurLine = pSrc + src_step * (y + i) + x;
//            global uchar *pDstLine = pDst + dst_step * (y + i) + x;
//	   	    ProcessLines1Main(pCurLine, pCurLine - src_step, pCurLine + src_step, pDstLine, src_cols, pMap, pInvMap);
//	    }
//		{
//			const global uchar *pCurLine = pSrc + src_step * (y + diff - 1) + x;
//        	global uchar *pDstLine = pDst + dst_step * (y + diff - 1) + x;
//     		ProcessLinesBroundMain(pCurLine, pCurLine - src_step, pDstLine, src_cols, pMap, pInvMap);
//		}
//	 }
//
//	// 中间行
//	if (y >= 1 && y < src_rows - 4)
//    {
//		// 最左边的点
//		if (x == 0)
//		{
//			const global uchar *pCurLine = pSrc + src_step * y + x;
//    		global uchar *pDstLine = pDst + dst_step * y + x;
//			int lShift = 0;
//            ProcessPointBround(pCurLine, pCurLine - src_step, pCurLine + src_step, pDstLine, pMap, pInvMap, 1);
//            lShift += src_step;
//            ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, 1);
//            lShift += src_step;
//            ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, 1);
//            lShift += src_step;
//            ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, 1);
//            lShift += src_step;
//		}
//		// 最右边的几个点
//		else if (src_cols - x <= 4)
//		{
//			int diff = src_cols - x;
//			for (int i = 0; i < diff-1; i++)
//			{
//			    const global uchar *pCurLine = pSrc + src_step * y + x + i;
//    		    global uchar *pDstLine = pDst + dst_step * y + x + i;
//			    int lShift = 0;
//			    ProcessPoint(pCurLine, pCurLine - src_step, pCurLine + src_step, pDstLine, pMap, pInvMap);
//                lShift += src_step;
//                ProcessPoint(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap);
//                lShift += src_step;
//                ProcessPoint(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap);
//                lShift += src_step;
//                ProcessPoint(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap);
//                lShift += src_step;
//			}
//
//			//最右边的点
//			{
//			    const global uchar *pCurLine = pSrc + src_step * y + x + diff-1;
//    		    global uchar *pDstLine = pDst + dst_step * y + x + diff-1;
//			    int lShift = 0;
//                ProcessPointBround(pCurLine, pCurLine - src_step, pCurLine + src_step, pDstLine, pMap, pInvMap, -1);
//                lShift += src_step;
//                ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, -1);
//                lShift += src_step;
//                ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, -1);
//                lShift += src_step;
//                ProcessPointBround(pCurLine + lShift, pCurLine - src_step + lShift, pCurLine + src_step + lShift, pDstLine + lShift, pMap, pInvMap, -1);
//                lShift += src_step;
//			}
//
//		}
//
//		// 中间点
//		if (x >=0 && x < src_cols - 4)
//		{
//			const global uchar *pCurLine = pSrc + src_step * y + x + 1;
//            const global uchar *pPreLine = pCurLine - src_step;
//            const global uchar *pNexLine = pCurLine + src_step;
//    		global uchar *pDstLine = pDst + dst_step * y + x + 1;
//
//			ProcessBlock4x4(pCurLine,
//                            pPreLine,
//                            pNexLine,
//                        	pDstLine,
//                        	pMap,
//                        	src_step,
//                        	pInvMap,
//                        	isSubImage);
//		}
//	}
//}


/// test 边缘不做处理
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
	const global uchar *pMap,
	const global int *pInvMap,
	int isSubImage
)
{
    //获取当前图像行和列
    int x = get_global_id(0)*4;
	int y = get_global_id(1)*4 + 1;

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
                    pInvMap,
                    isSubImage);
}


