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
                            global uchar *pDstLine,
                            const global int *pMap,
                            int lPitch,
                            const global int *pInvMap
                            )
    {
        // 权重表清零
		int lSumWei[17] = {0};

        // 当前块, 权重设置为最大值256
        AddBlockSum(pCurLine, lPitch, lSumWei, 256);

        // 领域8个位置
        AddBlockSumByNei(pCurLine, pCurLine - 1, lPitch, lSumWei, pMap);
        AddBlockSumByNei(pCurLine, pCurLine + 1, lPitch, lSumWei, pMap);

        AddBlockSumByNei(pCurLine, pCurLine - lPitch - 1, lPitch, lSumWei, pMap);
        AddBlockSumByNei(pCurLine, pCurLine - lPitch, lPitch, lSumWei, pMap);
        AddBlockSumByNei(pCurLine, pCurLine - lPitch + 1, lPitch, lSumWei, pMap);

        AddBlockSumByNei(pCurLine, pCurLine + lPitch - 1, lPitch, lSumWei, pMap);
        AddBlockSumByNei(pCurLine, pCurLine + lPitch, lPitch, lSumWei, pMap);
        AddBlockSumByNei(pCurLine, pCurLine + lPitch + 1, lPitch, lSumWei, pMap);

        // average / sweight
        GetBlockResult(pDstLine, lPitch, lSumWei, pInvMap);
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



   
     if (y == 1) //最上面一行
     {
        const global uchar *pCurLine = pSrc + src_step * (y - 1) + x;
        global uchar *pDstLine = pDst + dst_step * (y - 1) + x;
     	ProcessLinesBroundMain(pCurLine, pCurLine + src_step, pDstLine, src_cols, pMap, pInvMap);
     }

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

	if (y >= 1 && y < src_rows - 4)
    {
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
		
		if (x >=0 && x < src_cols - 4)
		{
			const global uchar *pCurLine = pSrc + src_step * y + x + 1;
    		global uchar *pDstLine = pDst + dst_step * y + x + 1;
		    ProcessBlock4x4(pCurLine,
                        	pDstLine,
                        	pMap,
                        	src_step,
                        	pInvMap);
		}
		
	}

	
}