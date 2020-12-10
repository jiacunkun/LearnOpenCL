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
#if 0
   int idx = get_global_id(0);//dst_width
	int idy = get_global_id(1);

	int idx_x2 = idx << 1;
	int idy_x2 = idy << 1;

	int y_pre = max(0,idy_x2-1);//(idy_x2 - 1) < 0 ? 0 : (idy_x2 - 1);
	int y_cur = idy_x2;
	int y_next = min(idy_x2+1,Src_Height-1);//(idy_x2 + 1) > (Src_Height - 1) ? (Src_Height - 1) : (idy_x2 + 1);

	int x_pre = max(0,idx_x2-1);//(idx_x2 - 1) < 0 ? 0 : (idx_x2 - 1);
	int x_cur = idx_x2;
	int x_next = min(idx_x2+1,Src_Width-1);//(idx_x2 + 1) > (Src_Width - 1) ? 

    global uchar* src0 = (pSrc + x_pre + y_pre * Src_Pitch);
	global uchar* src1 = (pSrc + x_pre + y_cur * Src_Pitch);
	global uchar* src2 = (pSrc + x_pre + y_next * Src_Pitch);

    short8 pre = convert_short8(vload8(0, pSrcPre));
    short8 cur = convert_short8(vload8(0, pSrcCur));
    short8 nex = convert_short8(vload8(0, pSrcnex));

    short8 sum_ver = pre + cur * 2 + nex;


    global uchar * pDst = pDst + mad24(y, dst_step, x);


#else
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

	global uchar* src0 = (pSrc + y_pre * Src_Pitch);
	global uchar* src1 = (pSrc + y_cur * Src_Pitch);
	global uchar* src2 = (pSrc + y_next * Src_Pitch);

	global uchar* dst = (__global uchar*)(pDst + idy * Dst_Pitch);

    int3 c0 = (int3)(src0[x_pre], src1[x_pre], src2[x_pre]);
    int3 c1 = (int3)(src0[x_cur], src1[x_cur], src2[x_cur]);
    int3 c2 = (int3)(src0[x_next], src1[x_next], src2[x_next]);
    int3 c = c0 + c1*2 + c2;
    int out = c.s0 + (c.s1 << 1) + c.s2;

	out = (out + 8) >> 4;
	out = min(255, out);

	dst[idx] = out;
#endif
	return;
}


/*BilinearResize_kernel*/
// kernel void PyramidUp(global uchar *src_ptr,
//                            const int src_step,
//                            const int src_cols,
//                            const int src_rows,
//                            global uchar *dst_ptr,
//                            const int dst_step,
//                            const int dst_cols,
//                            const int dst_rows)
// {
// 	const int x = get_global_id(0);
// 	const int y = get_global_id(1);

// 	float hor_scale = (float)src_cols / (float)dst_cols;
// 	float ver_scale = (float)src_rows / (float)dst_rows;

// 	float pos = ((float)x + 1.0f) * hor_scale; // 修改此处控制图像偏移
// 	int left_pos = (int)fmax(pos - 0.5f, 0.0f);
// 	int right_pos = (int)fmin(pos + 0.5f, (float)src_cols - 1.0f);
// 	float hor_weight = fabs(pos - 0.5f - (float)left_pos);

// 	pos = ((float)y + 1.0f) * ver_scale; // 修改此处控制图像偏移
// 	int top_pos = (int)fmax(pos - 0.5f, 0.0f);
// 	int bottom_pos = (int)fmin(pos + 0.5f, (float)src_rows - 1.0f);
// 	float ver_weight = fabs(pos - 0.5f - (float)top_pos);

// 	float data00 = (float)src_ptr[mad24(src_step, top_pos, left_pos)];
// 	float data01 = (float)src_ptr[mad24(src_step, top_pos, right_pos)];
// 	float data10 = (float)src_ptr[mad24(src_step, bottom_pos, left_pos)];
// 	float data11 = (float)src_ptr[mad24(src_step, bottom_pos, right_pos)];

// 	float tmp0 = data00 + (data01 - data00) * hor_weight;
// 	float tmp1 = data10 + (data11 - data10) * hor_weight;
// 	float res = tmp0 + (tmp1 - tmp0) * ver_weight + 0.5;
// 	dst_ptr[mad24(dst_step, y, x)] = (unsigned char)clamp(res, 0.0f, 255.0f);

// 	return;
// }


// 将上采样和加法一起做
#define SetOut(secDst, src) \
    out = secDst; \
    out = out + (int)(src) - 128; \
    out = clamp(out, 0, 255);\
    secDst = out;

kernel void PyramidUp(global uchar *src_ptr,
                           const int src_step,
                           const int src_cols,
                           const int src_rows,
                           global uchar *dst_ptr,
                           const int dst_step,
                           const int dst_cols,
                           const int dst_rows)
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


__kernel void MeanDown2x2_u8(
__global uchar* pSrc,
__global uchar* pDst,
int Src_Width,
int Src_Height,
int Src_Pitch,
int Dst_Width,
int Dst_Height,
int Dst_Pitch)
{
	int idx = get_global_id(0);
	int idy = get_global_id(1);

	int idx_x2 = idx << 1;
	int idy_x2 = idy << 1;

	int y1 = min(idy_x2 + 1, Src_Height - 1);

	__global uchar* src0 = (__global uchar*)(pSrc + idy_x2*Src_Pitch);
	__global uchar* src1 = (__global uchar*)(pSrc + y1*Src_Pitch);
	__global uchar* dst = (__global uchar*)(pDst + idy*Dst_Pitch);

	int2 in0 = convert_int2(vload2(0, src0 + idx_x2));
	int2 in1 = convert_int2(vload2(0, src1 + idx_x2));

	int sum = in0.s0 + in0.s1 + in1.s0 + in1.s1;
	sum += 2;
	sum >>= 2;

	dst[idx] = convert_uchar(sum);

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


kernel void MakeWeightMap
(
	global int *pTable,
    const float fVar,
    const int lMaxNum 
)
{
    int x = get_global_id(0);
	int y = get_global_id(1);

    float SumVar = fVar * 512;
    int lVal = x * x; 
    lVal = (int) ( 255.0 * exp(-lVal / SumVar) + 0.5f );
    pTable[x] = lVal;
}

kernel void CopyAndPaddingImage(global uchar *src_ptr,
                                const int src_step,
                                const int src_cols,
                                const int src_rows,
                                global uchar *dst_ptr,
                                const int dst_step,
                                const int dst_cols,
                                const int dst_rows,
                                const int lExpandSize)
{
	int x = get_global_id(0)*4;
	int y = get_global_id(1);

    int idx = min(max(x - lExpandSize, 0), src_cols - 1);
    int idy = min(max(y - lExpandSize, 0), src_rows - 1);

    const global uchar *pSrc = src_ptr + mad24(src_step, idy, idx);
    global uchar *pDst = dst_ptr + mad24(dst_step, y, x);

    //dst_ptr[mad24(dst_step, y, x)] = src_ptr[mad24(src_step, idy, idx)];

    if (dst_cols - x < 5)
    {
        int diff = dst_cols -x;
        for (int i = 0; i < diff; i++)
        {
            pDst[i] = pSrc[0];
        }
    }
    else if (x == 0)
    {
        vstore4((uchar4)(pSrc[0]), 0, pDst);
    }
    else
    {
        vstore4(vload4(0, pSrc), 0, pDst);
    }

	return;
}


kernel void CopyAndDePaddingImage(const global uchar *src_ptr,
                                const int src_step,
                                const int src_cols,
                                const int src_rows,
                                global uchar *dst_ptr,
                                global uchar *sub_ptr,
                                const int dst_step,
                                const int dst_cols,
                                const int dst_rows,
                                const int lExpandSize, 
                                const int isSubImage)
{
	int x = get_global_id(0)*8;
	int y = get_global_id(1);

    const global uchar *pSrc = src_ptr + mad24(src_step, (y + lExpandSize), x + lExpandSize);
    global uchar *pDst = dst_ptr + mad24(dst_step, y, x);
    global uchar *pSub = sub_ptr + mad24(dst_step, y, x);
    if (dst_cols - x < 8)
    {
        int diff = dst_cols -x;
        for (int i = 0; i < diff; i++)
        {
            if (isSubImage > 0)
            {
                pDst[i] = 128 + pSrc[i] - pSub[i];
            }
            else
            {
                pDst[i] = pSrc[i];
            }
           
        }
    }
    else
    {
         if (isSubImage > 0)
        {
            short8 val = convert_short8(vload8(0, pSrc)) - convert_short8(vload8(0, pSub)) + (short8)(128);
            val = clamp(val, 0, 255);
            vstore8(convert_uchar8(val), 0, pDst);
        }
        else
        {
            vstore8(vload8(0, pSrc), 0, pDst);
        }
    
    }

	return;
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
    // int x = get_global_id(0);
	// int y = get_global_id(1);

	// uDst[y*dst_step + x] = uvSrc[y*src_step + 2*x];
	// vDst[y*dst_step + x] = uvSrc[y*src_step + 2*x + 1];

    int row = get_global_id(1);
	int col = get_global_id(0) * 8;

    int srcCol = col * 2;
    global uchar *pVDst = vDst + mad24(row, dst_step, col);
    global uchar *pUDst = uDst + mad24(row, dst_step, col);
    global const uchar *pUVSrc = uvSrc + mad24(row, src_step, srcCol);
    uchar16 uv_val = vload16(0, pUVSrc);
    vstore8(uv_val.s02468ace, 0, pUDst);
    vstore8(uv_val.s13579bdf, 0, pVDst);
    
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
    int row = get_global_id(1);
	int col = get_global_id(0) * 8;

    int dstCol = col * 2;
    global uchar * pUVDst = uvDst + mad24(row, dst_step, dstCol);
    global const uchar * pVSrc = vSrc + mad24(row, src_step, col);
    global const uchar * pUSrc = uSrc + mad24(row, src_step, col);
    uchar8 v_val = vload8(0, pVSrc);
    uchar8 u_val = vload8(0, pUSrc);
    uchar16 uv_val;
    uv_val.s02468ace = u_val;
    uv_val.s13579bdf = v_val;
    vstore16(uv_val, 0, pUVDst);
}


kernel void ImageSubImage
(
	global uchar *srcDst, 
	int src_step,
	int src_cols,
	int src_rows,
	const global uchar *src, 
	int dst_step,
	int dst_cols,
	int dst_rows
)
{
#if 1
   	int x = get_global_id(0)*4;
	int y = get_global_id(1);


    global uchar * pSrcDst = srcDst + mad24(y, src_step, x);
    global uchar * pSrc = src + mad24(y, dst_step, x);

    if (dst_cols -x < 4)
    {
        int diff = dst_cols -x;
        for (int i = 0; i < diff; i++)
        {
            short srcDstVal = pSrcDst[i];
	        short srcVal = pSrc[i];
	        srcDstVal = srcDstVal - srcVal + 128;
	        srcDstVal = srcDstVal > 255 ? 255 : srcDstVal;
	        srcDstVal = srcDstVal < 0 ? 0 : srcDstVal;
	        srcDst[y*src_step + x + i] = srcDstVal;
        }
    
    }
    else
    {
        short4 srcDstVal = convert_short4(vload4(0, pSrcDst));
        short4 srcVal = convert_short4(vload4(0, pSrc));
        srcDstVal = srcDstVal + (short4)(128) - srcVal;
        srcDstVal = clamp(srcDstVal, 0, 255);
        vstore4(convert_uchar4(srcDstVal), 0, pSrcDst);
    }

    
#else
    int x = get_global_id(0);
	int y = get_global_id(1);


	int srcDstVal = srcDst[y*src_step + x];
	int srcVal = src[y*dst_step + x];
	srcDstVal = srcDstVal - srcVal + 128;
	srcDstVal = srcDstVal > 255 ? 255 : srcDstVal;
	srcDstVal = srcDstVal < 0 ? 0 : srcDstVal;
	srcDst[y*src_step + x] = srcDstVal;
#endif
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
#if 1
    int x = get_global_id(0)*8;
	int y = get_global_id(1);


    global uchar * pSrcDst = srcDst + mad24(y, src_step, x);
    global uchar * pSrc = src + mad24(y, dst_step, x);

    int8 srcDstVal = convert_int8(vload8(0, pSrcDst));
    int8 srcVal = convert_int8(vload8(0, pSrc));
    srcDstVal = srcDstVal + srcVal - (int8)(128);
    srcDstVal = clamp(srcDstVal, 0, 255);
    vstore8(convert_uchar8(srcDstVal), 0, pSrcDst);
#else
   	int x = get_global_id(0);
	int y = get_global_id(1);

	int srcDstVal = srcDst[y*src_step + x];
	int srcVal = src[y*dst_step + x];
	srcDstVal = srcDstVal + srcVal - 128;
	srcDstVal = srcDstVal > 255 ? 255 : srcDstVal;
	srcDstVal = srcDstVal < 0 ? 0 : srcDstVal;
	srcDst[y*src_step + x] = srcDstVal;
#endif
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
    lSumWei[ 0 ] += lW;

    lSumWei[ 1 ] += pNeiBlock[ 0 ] * lW;
    lSumWei[ 2 ] += pNeiBlock[ 1 ] * lW;
    lSumWei[ 3 ] += pNeiBlock[ 2 ] * lW;
    lSumWei[ 4 ] += pNeiBlock[ 3 ] * lW;
    pNeiBlock += lPitch;

    lSumWei[ 5 ] += pNeiBlock[ 0 ] * lW;
    lSumWei[ 6 ] += pNeiBlock[ 1 ] * lW;
    lSumWei[ 7 ] += pNeiBlock[ 2 ] * lW;
    lSumWei[ 8 ] += pNeiBlock[ 3 ] * lW;
    pNeiBlock += lPitch;

    lSumWei[ 9 ] += pNeiBlock[ 0 ] * lW;
    lSumWei[ 10 ] += pNeiBlock[ 1 ] * lW;
    lSumWei[ 11 ] += pNeiBlock[ 2 ] * lW;
    lSumWei[ 12 ] += pNeiBlock[ 3 ] * lW;
    pNeiBlock += lPitch;

    lSumWei[ 13 ] += pNeiBlock[ 0 ] * lW;
    lSumWei[ 14 ] += pNeiBlock[ 1 ] * lW;
    lSumWei[ 15 ] += pNeiBlock[ 2 ] * lW;
    lSumWei[ 16 ] += pNeiBlock[ 3 ] * lW;
}


inline void AddBlockSumByNei
 (
    const global uchar *pCurBlock,
    const global uchar *pNeiBlock,
    int lPitch,
    int *lSumWei,
    constant int *pMap
 )
{
    int lBDif = GetBlockDiff(pCurBlock, pNeiBlock, lPitch); // 计算块匹配值d,
    CLAMP(lBDif, 1, 799);
    int lW = pMap[lBDif]; // 获取权重, 查表法, exp(-d/(h^2))
    lW >>= 1;  // ？？？
    AddBlockSum(pNeiBlock, lPitch, lSumWei, lW);
}

inline void GetBlockResult(global uchar *pDstBlock,
                           int lPitch,
                           int *lSumWei
                          )
{
    int lSW = lSumWei[ 0 ];
    int lInvW = (1 << 20) / lSW;
    lSumWei++;

    pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock += lPitch;
    lSumWei += 4;

    pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock += lPitch;
    lSumWei += 4;

    pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock += lPitch;
    lSumWei += 4;

    pDstBlock[ 0 ] = ( lSumWei[ 0 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 1 ] = ( lSumWei[ 1 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 2 ] = ( lSumWei[ 2 ] * lInvW + ( 1 << 19 )) >> 20;
    pDstBlock[ 3 ] = ( lSumWei[ 3 ] * lInvW + ( 1 << 19 )) >> 20;

}


inline void ProcessBlock4x4(const global uchar *pCurLine,
                            const global uchar *pPreLine,
                            const global uchar *pNexLine,
                            global uchar *pDstLine,
                            constant int *pMap,
                            int lPitch
                            )
{
    // 权重表清零
	int lSumWei[17] = {0};

    // 当前块, 权重设置为最大值256
    AddBlockSum(pCurLine, lPitch, lSumWei, 256);

    // 领域8个位置
    AddBlockSumByNei(pCurLine, pCurLine - 1, lPitch, lSumWei, pMap);
    AddBlockSumByNei(pCurLine, pCurLine + 1, lPitch, lSumWei, pMap);

    AddBlockSumByNei(pCurLine, pPreLine - 1, lPitch, lSumWei, pMap);
    AddBlockSumByNei(pCurLine, pPreLine, lPitch, lSumWei, pMap);
    AddBlockSumByNei(pCurLine, pPreLine + 1, lPitch, lSumWei, pMap);

    AddBlockSumByNei(pCurLine, pNexLine - 1, lPitch, lSumWei, pMap);
    AddBlockSumByNei(pCurLine, pNexLine, lPitch, lSumWei, pMap);
    AddBlockSumByNei(pCurLine, pNexLine + 1, lPitch, lSumWei, pMap);

    // average / sweight
    GetBlockResult(pDstLine, lPitch, lSumWei);
}

#if 0
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
	constant int *pMap
)
{
#if 0 //并行加速
    
   

#else
    int x = get_global_id(0)*4 + 1;
	int y = get_global_id(1)*4 + 1;
	
    const global uchar *pCurLine = pSrc + mad24(y, src_step, x);
    const global uchar *pPreLine = pCurLine - src_step;
    const global uchar *pNexLine = pCurLine + src_step;
    global uchar *pDstLine = pDst + mad24(y, dst_step, x);

    ProcessBlock4x4(pCurLine,
                    pPreLine,
                    pNexLine,
                    pDstLine,
                    pMap,
                    src_step
                    );
#endif
    
}

#else

inline int Block_WeiCom(short4 curr0, short4 curr1, short4 curr2, short4 curr3,
		short4 cent0, short4 cent1, short4 cent2, short4 cent3, constant int* pMap)
	{
		ushort4 ldif;
		ushort4 sumdif = (ushort4)(0,0,0,0);
		int lBDif = 0;

		ldif = abs_diff(curr0, cent0);
		ldif = min(ldif, 49);
		sumdif = sumdif + ldif;
		ldif = abs_diff(curr1, cent1);
		ldif = min(ldif, 49);
		sumdif = sumdif + ldif;
		ldif = abs_diff(curr2, cent2);
		ldif = min(ldif, 49);
		sumdif = sumdif + ldif;
		ldif = abs_diff(curr3, cent3);
		ldif = min(ldif, 49);
		sumdif = sumdif + ldif;
		lBDif = sumdif.s0 + sumdif.s1 + sumdif.s2 + sumdif.s3;
		lBDif = min(799, lBDif);		
		return pMap[lBDif];
	}



	inline void  Block_WeiAdd(int4* WeiSum0, int4* WeiSum1, int4* WeiSum2, int4* WeiSum3,
		short4 data0, short4 data1, short4 data2, short4 data3, int wei)
	{
		WeiSum0[0] = WeiSum0[0] + convert_int4(data0) * wei;
		WeiSum1[0] = WeiSum1[0] + convert_int4(data1) * wei;
		WeiSum2[0] = WeiSum2[0] + convert_int4(data2) * wei;
		WeiSum3[0] = WeiSum3[0] + convert_int4(data3) * wei;
		return;
	}


	//pSrc 和 //pDst 外面都有一圈padding
	//上下左右都padding 4， 为了数据对齐
	//领域块处理顺序
	//1 4 6
	//2 0 7
	//3 5 8
	kernel void NLMDenoise
	(
		const global uchar* pSrc,
		int src_step,
		int src_cols,
		int src_rows,
		global uchar* pDst,
		int dst_step,
		int dst_cols,
		int dst_rows,
		constant int* pMap
	)
	{
		//获取当前图像行和列
		int x = get_global_id(0) * 4 + 1;
		int y = get_global_id(1) * 4 + 1;
		
		const global uchar* pTmpSrc = pSrc + (y - 1) * src_step + (x - 1);
		global uchar* pTmpDst = pDst + y * dst_step + x;
		short8 Data0 = convert_short8(vload8(0, pTmpSrc));
		short8 Data1 = convert_short8(vload8(0, pTmpSrc + src_step));
		short8 Data2 = convert_short8(vload8(0, pTmpSrc + src_step * 2));
		short8 Data3 = convert_short8(vload8(0, pTmpSrc + src_step * 3));
		short8 Data4 = convert_short8(vload8(0, pTmpSrc + src_step * 4));
		short8 Data5 = convert_short8(vload8(0, pTmpSrc + src_step * 5));
		uint4  mask = (uint4)(1, 2, 3, 4);
		short4 cent0 = Data1.s1234;
		short4 cent1 = Data2.s1234;
		short4 cent2 = Data3.s1234;
		short4 cent3 = Data4.s1234;
		short4 curr0, curr1, curr2, curr3;
		int4 WeiSum0 = (int4)(0, 0, 0, 0);
		int4 WeiSum1 = (int4)(0, 0, 0, 0);
		int4 WeiSum2 = (int4)(0, 0, 0, 0);
		int4 WeiSum3 = (int4)(0, 0, 0, 0);
		int  SumWei = 0;
		int  lWei = 0;

		Block_WeiAdd(&WeiSum0, &WeiSum1, &WeiSum2, &WeiSum3, cent0, cent1, cent2, cent3, 256);
		SumWei = SumWei + 256;

		mask = (uint4)(0, 1, 2, 3);
		curr0 = Data0.s0123;
		curr1 = Data1.s0123;
		curr2 = Data2.s0123;
		curr3 = Data3.s0123;
		lWei = Block_WeiCom(curr0, curr1, curr2, curr3, cent0, cent1, cent2, cent3, pMap);
		Block_WeiAdd(&WeiSum0, &WeiSum1, &WeiSum2, &WeiSum3, curr0, curr1, curr2, curr3, lWei);
		SumWei = SumWei + lWei;

		curr0 = Data4.s0123;
		lWei = Block_WeiCom( curr1, curr2, curr3,curr0, cent0, cent1, cent2, cent3, pMap);
		Block_WeiAdd(&WeiSum0, &WeiSum1, &WeiSum2, &WeiSum3, curr1, curr2, curr3, curr0, lWei);
		SumWei = SumWei + lWei;

		curr1 = Data5.s0123;
		lWei = Block_WeiCom( curr2, curr3, curr0,curr1, cent0, cent1, cent2, cent3, pMap);
		Block_WeiAdd(&WeiSum0, &WeiSum1, &WeiSum2, &WeiSum3,  curr2, curr3, curr0, curr1, lWei);
		SumWei = SumWei + lWei;

		mask = (uint4)(1, 2, 3, 4);
		curr0 = Data0.s1234;
		lWei = Block_WeiCom(curr0, cent0, cent1, cent2, cent0, cent1, cent2, cent3, pMap);
		Block_WeiAdd(&WeiSum0, &WeiSum1, &WeiSum2, &WeiSum3, curr0, cent0, cent1, cent2, lWei);
		SumWei = SumWei + lWei;

		curr0 = Data5.s1234;
		lWei = Block_WeiCom(cent1, cent2, cent3, curr0, cent0, cent1, cent2, cent3, pMap);
		Block_WeiAdd(&WeiSum0, &WeiSum1, &WeiSum2, &WeiSum3, cent1, cent2, cent3, curr0, lWei);
		SumWei = SumWei + lWei;


		mask = (uint4)(2, 3, 4, 5);
		curr0 = Data0.s2345;
		curr1 = Data1.s2345;
		curr2 = Data2.s2345;
		curr3 = Data3.s2345;
		lWei = Block_WeiCom(curr0, curr1, curr2, curr3, cent0, cent1, cent2, cent3, pMap);
		Block_WeiAdd(&WeiSum0, &WeiSum1, &WeiSum2, &WeiSum3, curr0, curr1, curr2, curr3, lWei);
		SumWei = SumWei + lWei;

		curr0 = Data4.s2345;
		lWei = Block_WeiCom(curr1, curr2, curr3, curr0, cent0, cent1, cent2, cent3, pMap);
		Block_WeiAdd(&WeiSum0, &WeiSum1, &WeiSum2, &WeiSum3, curr1, curr2, curr3, curr0, lWei);
		SumWei = SumWei + lWei;

		curr1 = Data5.s2345;
		lWei = Block_WeiCom(curr2, curr3, curr0, curr1, cent0, cent1, cent2, cent3, pMap);
		Block_WeiAdd(&WeiSum0, &WeiSum1, &WeiSum2, &WeiSum3, curr2, curr3, curr0, curr1, lWei);
		SumWei = SumWei + lWei;

		//计算结果
		WeiSum0 = (WeiSum0 + (SumWei >> 1)) / SumWei;
        WeiSum1 = (WeiSum1 + (SumWei >> 1)) / SumWei;
        WeiSum2 = (WeiSum2 + (SumWei >> 1)) / SumWei;
        WeiSum3 = (WeiSum3 + (SumWei >> 1)) / SumWei;

       #if 0 // todo: 偏移不对
            //做减法
            global uchar* pTemSubSrc = pSubSrc + (y - 3) * (src_step - 8) + (x - 3);
            WeiSum0 = 128 + WeiSum0 - convert_int4(vload4(0, pTemSubSrc));
            WeiSum1 = 128 + WeiSum1 - convert_int4(vload4(0, pTemSubSrc + src_step));
            WeiSum2 = 128 + WeiSum2 - convert_int4(vload4(0, pTemSubSrc + src_step*2));
            WeiSum3 = 128 + WeiSum3 - convert_int4(vload4(0, pTemSubSrc + src_step*3));
        #endif

		vstore4(convert_uchar4(WeiSum0), 0, pTmpDst);
		vstore4(convert_uchar4(WeiSum1), 0, pTmpDst + dst_step);
		vstore4(convert_uchar4(WeiSum2), 0, pTmpDst + dst_step * 2);
		vstore4(convert_uchar4(WeiSum3), 0, pTmpDst + dst_step * 3);
		
		return;
	}



#endif