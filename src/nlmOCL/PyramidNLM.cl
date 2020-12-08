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


inline int GetBlockDiff(const global uchar *pCurBlock, const global uchar *pNeiBlock, int lPitch)
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
    const global int *pMap
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
                            int *lSumWei,
                            const global int *pInvMap)
    {
        int lSW = lSumWei[ 0 ];
        int lInvW = pInvMap[ lSW ];
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
                            const global int *pMap,
                            int lPitch,
                            int *lSumWei,
                            const global int *pInvMap
                            )
    {
        // 权重表清零
		for (int i = 0; i < 17; i++)
		{
			lSumWei[ i ] = 0;
		}

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
        GetBlockResult(pDstLine, lPitch, lSumWei, pInvMap);
    }


/// yz123  0
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
//	ProcessBlock4x4(pCurLine,
//                    pPreLine,
//                    pNexLine,
//                    pDstLine,
//                    pMap,
//                    src_step,
//                    lSumWei,
//                    pInvMap);
//
//
//
////    pDstLine[0] = 0;
////    pDstLine[1] = 0;
////    pDstLine[2] = 0;
////    pDstLine[3] = 0;
////
////    pDstLine += dst_step;
////    pDstLine[0] = 0;
////    pDstLine[1] = 0;
////    pDstLine[2] = 0;
////    pDstLine[3] = 0;
////
////    pDstLine += dst_step;
////    pDstLine[0] = 0;
////    pDstLine[1] = 0;
////    pDstLine[2] = 0;
////    pDstLine[3] = 0;
////
////    pDstLine += dst_step;
////    pDstLine[0] = 0;
////    pDstLine[1] = 0;
////    pDstLine[2] = 0;
////    pDstLine[3] = 0;
//}



/// yz123  1
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
//
//	const global uchar *pCurLine = pSrc + src_step * y + x;
//    const global uchar *pPreLine = pCurLine - src_step;
//    const global uchar *pNexLine = pCurLine + src_step;
//    global uchar *pDstLine = pDst + dst_step * y + x;
//
//
//    int lSumWei[17] = {0};
//
//    // 当前块, 权重设置为最大值256
//    AddBlockSum(pCurLine, src_step, lSumWei, 256);
//
//    // 领域8个位置
//    AddBlockSumByNei(pCurLine, pCurLine - 1, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pCurLine + 1, src_step, lSumWei, pMap);
//
//    AddBlockSumByNei(pCurLine, pPreLine - 1, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pPreLine, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pPreLine + 1, src_step, lSumWei, pMap);
//
//    AddBlockSumByNei(pCurLine, pNexLine - 1, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pNexLine, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pNexLine + 1, src_step, lSumWei, pMap);
//
//    // average / sweight
//    GetBlockResult(pDstLine, src_step, lSumWei, pInvMap);
//
//}


/// yz123  2
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
//
//	const global uchar *pCurLine = pSrc + src_step * y + x;
//    const global uchar *pPreLine = pCurLine - src_step;
//    const global uchar *pNexLine = pCurLine + src_step;
//    global uchar *pDstLine = pDst + dst_step * y + x;
//
//
//    /// =============================================
//    // 当前块, 权重设置为最大值256
//    int lSumWei[17];
//    lSumWei[ 0 ] = 256;
//
//    lSumWei[ 1 ] = pCurLine[ 0 ] * 256;
//    lSumWei[ 2 ] = pCurLine[ 1 ] * 256;
//    lSumWei[ 3 ] = pCurLine[ 2 ] * 256;
//    lSumWei[ 4 ] = pCurLine[ 3 ] * 256;
//    pCurLine += src_step;
//
//    lSumWei[ 5 ] = pCurLine[ 0 ] * 256;
//    lSumWei[ 6 ] = pCurLine[ 1 ] * 256;
//    lSumWei[ 7 ] = pCurLine[ 2 ] * 256;
//    lSumWei[ 8 ] = pCurLine[ 3 ] * 256;
//    pCurLine += src_step;
//
//    lSumWei[ 9 ] = pCurLine[ 0 ] * 256;
//    lSumWei[ 10 ] = pCurLine[ 1 ] * 256;
//    lSumWei[ 11 ] = pCurLine[ 2 ] * 256;
//    lSumWei[ 12 ] = pCurLine[ 3 ] * 256;
//    pCurLine += src_step;
//
//    lSumWei[ 13 ] = pCurLine[ 0 ] * 256;
//    lSumWei[ 14 ] = pCurLine[ 1 ] * 256;
//    lSumWei[ 15 ] = pCurLine[ 2 ] * 256;
//    lSumWei[ 16 ] = pCurLine[ 3 ] * 256;
//
//
//
//    // 领域8个位置
//    AddBlockSumByNei(pCurLine, pCurLine - 1, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pCurLine + 1, src_step, lSumWei, pMap);
//
//    AddBlockSumByNei(pCurLine, pPreLine - 1, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pPreLine, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pPreLine + 1, src_step, lSumWei, pMap);
//
//    AddBlockSumByNei(pCurLine, pNexLine - 1, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pNexLine, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pNexLine + 1, src_step, lSumWei, pMap);
//
//    // average / sweight
//    GetBlockResult(pDstLine, src_step, lSumWei, pInvMap);
//
//}



/////// yz123  3
//constant int4 uchar256 = {255, 255, 255, 255};
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
//
//	const global uchar *pCurLine = pSrc + src_step * y + x;
//    const global uchar *pPreLine = pCurLine - src_step;
//    const global uchar *pNexLine = pCurLine + src_step;
//    global uchar *pDstLine = pDst + dst_step * y + x;
//
//
//    /// =============================================
//    // 当前块, 权重设置为最大值256
//    int lSumWei[17];
//    lSumWei[ 0 ] = 256;
//
//
//    int4 SumWei0 = convert_int4(vload4(0, pCurLine)) * uchar256;
//    pCurLine += src_step;
//
//    int4 SumWei1 = convert_int4(vload4(0, pCurLine)) * uchar256;
//    pCurLine += src_step;
//
//    int4 SumWei2 = convert_int4(vload4(0, pCurLine)) * uchar256;
//    pCurLine += src_step;
//
//    int4 SumWei3 = convert_int4(vload4(0, pCurLine)) * uchar256;
//    pCurLine += src_step;
//
//
//    lSumWei[1] = SumWei0.s0;
//    lSumWei[2] = SumWei0.s1;
//    lSumWei[3] = SumWei0.s2;
//    lSumWei[4] = SumWei0.s3;
//
//    lSumWei[4] = SumWei1.s0;
//    lSumWei[5] = SumWei1.s1;
//    lSumWei[6] = SumWei1.s2;
//    lSumWei[7] = SumWei1.s3;
//
//    lSumWei[8] = SumWei2.s0;
//    lSumWei[9] = SumWei2.s1;
//    lSumWei[10] = SumWei2.s2;
//    lSumWei[11] = SumWei2.s3;
//
//    lSumWei[12] = SumWei3.s0;
//    lSumWei[13] = SumWei3.s1;
//    lSumWei[14] = SumWei3.s2;
//    lSumWei[15] = SumWei3.s3;
//
//
//
//    // 领域8个位置
//    AddBlockSumByNei(pCurLine, pCurLine - 1, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pCurLine + 1, src_step, lSumWei, pMap);
//
//    AddBlockSumByNei(pCurLine, pPreLine - 1, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pPreLine, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pPreLine + 1, src_step, lSumWei, pMap);
//
//    AddBlockSumByNei(pCurLine, pNexLine - 1, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pNexLine, src_step, lSumWei, pMap);
//    AddBlockSumByNei(pCurLine, pNexLine + 1, src_step, lSumWei, pMap);
//
//    // average / sweight
//    GetBlockResult(pDstLine, src_step, lSumWei, pInvMap);
//
//}


/// yz123   4

constant ushort4 v49 = {49, 49, 49, 49};
inline void AddBlockSumByNei4
 (
    const global uchar *pCurBlock,
    const global uchar *pNeiBlock,
    int lPitch,
    int *lSumWei,
    const global int *pMap
 )
{
    ushort4 vDiffSum;
    short4 neiBlock0 = convert_short4(vload4(0, pNeiBlock));
    ushort4 vDiff = abs_diff(convert_short4(vload4(0, pCurBlock)), neiBlock0);
    vDiff = min(vDiff, v49);
    vDiffSum = vDiff;
    pCurBlock += lPitch;
    pNeiBlock += lPitch;

    neiBlock1 = convert_short4(vload4(0, pNeiBlock));
    vDiff = abs_diff(convert_short4(vload4(0, pCurBlock)), neiBlock1);
    vDiff = min(vDiff, v49);
    vDiffSum += vDiff;
    pCurBlock += lPitch;
    pNeiBlock += lPitch;

    neiBlock2 = convert_short4(vload4(0, pNeiBlock));
    vDiff = abs_diff(convert_short4(vload4(0, pCurBlock)), neiBlock2);
    vDiff = min(vDiff, v49);
    vDiffSum += vDiff;
    pCurBlock += lPitch;
    pNeiBlock += lPitch;

    neiBlock3 = convert_short4(vload4(0, pNeiBlock));
    vDiff = abs_diff(convert_short4(vload4(0, pCurBlock)), neiBlock3);
    vDiff = min(vDiff, v49);
    vDiffSum += vDiff;


    ushort lDif = vDiffSum.so + vDiffSum.s1 + vDiffSum.s2 + vDiffSum.s3;
    int lW = pMap[lDif]; // 获取权重, 查表法, exp(-d/(h^2))
    lW >>= 1;  // ？？？
    AddBlockSum(pNeiBlock, lPitch, lSumWei, lW);


//inline void AddBlockSum(const global uchar *pNeiBlock, int lPitch, int *lSumWei, int lW)
//{
//    lSumWei[ 0 ] += lW;
//
//    lSumWei[ 1 ] += pNeiBlock[ 0 ] * lW;
//    lSumWei[ 2 ] += pNeiBlock[ 1 ] * lW;
//    lSumWei[ 3 ] += pNeiBlock[ 2 ] * lW;
//    lSumWei[ 4 ] += pNeiBlock[ 3 ] * lW;
//    pNeiBlock += lPitch;
//
//    lSumWei[ 5 ] += pNeiBlock[ 0 ] * lW;
//    lSumWei[ 6 ] += pNeiBlock[ 1 ] * lW;
//    lSumWei[ 7 ] += pNeiBlock[ 2 ] * lW;
//    lSumWei[ 8 ] += pNeiBlock[ 3 ] * lW;
//    pNeiBlock += lPitch;
//
//    lSumWei[ 9 ] += pNeiBlock[ 0 ] * lW;
//    lSumWei[ 10 ] += pNeiBlock[ 1 ] * lW;
//    lSumWei[ 11 ] += pNeiBlock[ 2 ] * lW;
//    lSumWei[ 12 ] += pNeiBlock[ 3 ] * lW;
//    pNeiBlock += lPitch;
//
//    lSumWei[ 13 ] += pNeiBlock[ 0 ] * lW;
//    lSumWei[ 14 ] += pNeiBlock[ 1 ] * lW;
//    lSumWei[ 15 ] += pNeiBlock[ 2 ] * lW;
//    lSumWei[ 16 ] += pNeiBlock[ 3 ] * lW;
//}
}


constant int4 uchar256 = {255, 255, 255, 255};
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
    int x = get_global_id(0)*4 + 1;
	int y = get_global_id(1)*4 + 1;


	const global uchar *pCurLine = pSrc + src_step * y + x;
    const global uchar *pPreLine = pCurLine - src_step;
    const global uchar *pNexLine = pCurLine + src_step;
    global uchar *pDstLine = pDst + dst_step * y + x;


    /// =============================================
    // 当前块, 权重设置为最大值256
    int lSumWei[17];
    lSumWei[ 0 ] = 256;


    int4 SumWei0 = convert_int4(vload4(0, pCurLine)) * uchar256;
    pCurLine += src_step;

    int4 SumWei1 = convert_int4(vload4(0, pCurLine)) * uchar256;
    pCurLine += src_step;

    int4 SumWei2 = convert_int4(vload4(0, pCurLine)) * uchar256;
    pCurLine += src_step;

    int4 SumWei3 = convert_int4(vload4(0, pCurLine)) * uchar256;
    pCurLine += src_step;


    lSumWei[1] = SumWei0.s0;
    lSumWei[2] = SumWei0.s1;
    lSumWei[3] = SumWei0.s2;
    lSumWei[4] = SumWei0.s3;

    lSumWei[4] = SumWei1.s0;
    lSumWei[5] = SumWei1.s1;
    lSumWei[6] = SumWei1.s2;
    lSumWei[7] = SumWei1.s3;

    lSumWei[8] = SumWei2.s0;
    lSumWei[9] = SumWei2.s1;
    lSumWei[10] = SumWei2.s2;
    lSumWei[11] = SumWei2.s3;

    lSumWei[12] = SumWei3.s0;
    lSumWei[13] = SumWei3.s1;
    lSumWei[14] = SumWei3.s2;
    lSumWei[15] = SumWei3.s3;



    // 领域8个位置
    AddBlockSumByNei(pCurLine, pCurLine - 1, src_step, lSumWei, pMap);
    AddBlockSumByNei(pCurLine, pCurLine + 1, src_step, lSumWei, pMap);

    AddBlockSumByNei(pCurLine, pPreLine - 1, src_step, lSumWei, pMap);
    AddBlockSumByNei(pCurLine, pPreLine, src_step, lSumWei, pMap);
    AddBlockSumByNei(pCurLine, pPreLine + 1, src_step, lSumWei, pMap);

    AddBlockSumByNei(pCurLine, pNexLine - 1, src_step, lSumWei, pMap);
    AddBlockSumByNei(pCurLine, pNexLine, src_step, lSumWei, pMap);
    AddBlockSumByNei(pCurLine, pNexLine + 1, src_step, lSumWei, pMap);

    // average / sweight
    GetBlockResult(pDstLine, src_step, lSumWei, pInvMap);

}