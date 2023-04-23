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
//#include "wavelet.h"
// mpbase
#include "merror.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "amcomdef.h"

#include "mobilecv.h"
#include "imagebase.h"
#include "single_image_enhancement_define.h"
#include "wavelet_LL.h"
NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
const int DENOISE_TASK_NUM = 16; //todo: 未初始化变量值，暂且初始化，后续再看 4.9 by jck

#ifdef MULTI_THREAD

#include "mthread.h"

#endif

#if defined(USE_NEON) || defined(__ARM_NEON__)
#if defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif


//#define DB3                        //1*6的小波基
#define DB2                        //1*4的小波基
#define LL_SHIFT_SIZE             (10)
#define LL_DOUBLE_SHIFT_SIZE      (20)
#define LL_HALF_SHIFT_SIZE        (5)

#ifdef DB3
#define LL_DB_FILTER_SIZE         (6)
#define LL_DB_FILTER_HALF_SIZE    (3)

//db_6小波核
MFloat db_decom_l[] = { 0.035226291882101, -0.085441273882241, -0.135011020010391, 0.459877502119331, 0.806891509313339, 0.332670552950957 };
MFloat db_recon_l[] = { 0.332670552950957, 0.806891509313339, 0.459877502119331, -0.135011020010391, -0.085441273882241, 0.035226291882101 };

//db_4小波核
//MFloat db_decom_l[] = { 0, -0.129409522550921, 0.224143868041857, 0.836516303737469, 0.482962913144690, 0 };
//MFloat db_recon_l[] = { 0, 0.482962913144690, 0.836516303737469, 0.224143868041857, -0.129409522550921, 0 };

//MFloat db_decom_l[] = { -0.015655728135465, -0.072732619512854, 0.384864846864203, 0.852572020212255, 0.337897662457809, -0.072732619512854};
//MFloat db_recon_l[] = { -0.072732619512854, 0.337897662457809, 0.852572020212255, 0.384864846864203, -0.072732619512854, -0.015655728135465 };

//MFloat db_decom_l[] = { -0.088388347648318, 0.088388347648318, 0.707106781186548, 0.707106781186548, 0.088388347648318, -0.088388347648318 };
//MFloat db_recon_l[] = { 0, 0, 0.707106781186548, 0.707106781186548, 0, 0 };

#endif

#ifdef DB2
#define LL_DB_FILTER_SIZE         (4)
#define LL_DB_FILTER_HALF_SIZE    (2)
MFloat db_decom_l[] = {-0.129409522550921, 0.224143868041857, 0.836516303737469, 0.482962913144690};
MFloat db_recon_l[] = {0.482962913144690, 0.836516303737469, 0.224143868041857, -0.129409522550921};
#endif

#if defined(MCV_MULTI_THREAD)
typedef struct _tag_WL_LL_DECOM
{
    MInt32 task_ID;
    MRESULT errCode;
    WAVELET_LPDATA waveletBuf;
    MInt32 *srcBuf32I;
    MByte *srcBuf8U;
    MInt32 *rowLF;
    MInt64 *rowLF64;
    MInt32 *low_filter;
    MInt32 w;
    MInt32 h;
    MInt32 sw;
    MInt32 lineSteps;
    MInt32 level;
    MInt32 startRow;
    MInt32 endRow;
} Wl_LL_Decom, *LP_Wl_LL_Decom;
#endif

#if defined(MCV_MULTI_THREAD)
typedef struct _tag_WL_LL_DECOM_8U
{
    MInt32 task_ID;
    MRESULT errCode;
    WAVELET_LPDATA_8U waveletBuf;
    MByte *srcBuf;
    MByte *rowLF;
    MInt32 *low_filter;
    MInt32 w;
    MInt32 h;
    MInt32 sw;
    MInt32 lineSteps;
    MInt32 level;
    MInt32 startRow;
    MInt32 endRow;
} Wl_LL_Decom_8U, *LP_Wl_LL_Decom_8U;
#endif

#if defined(MCV_MULTI_THREAD)
typedef struct _tag_WL_LL_RECON
{
    MInt32 task_ID;
    MRESULT errCode;
    WAVELET_LPDATA waveletBuf;
    WAVELET_LPDATA detailsBuf;
    LPASVLOFFSCREEN resImg;
    MInt32 *currRowBuf;
    MInt32 *nextRowBuf;
    MInt32 *resData;
    MInt32 *low_filter;
    MInt32 sw;
    MInt32 sh;
    MInt32 level;
    MInt32 startRow;
    MInt32 endRow;
} Wl_LL_Recon, *LP_Wl_LL_Recon;
#endif

#ifdef DB3
MVoid dwt_LL_decomposition(WAVELET_LPDATA waveletBuf, MInt32 *srcBuf32I, MByte *srcBuf8U, MInt32 *rowLF, MInt64 *rowLF64,
    MInt32 *low_filter, MInt32 w, MInt32 h, MInt32 sw, MInt32 lineSteps, MInt32 level, MInt32 startRow, MInt32 endRow)
{
    MInt32 x, y, m, n, i, j;
    MInt32 convRes;
    MInt64 convRes64;
    MInt32 *rowBuf32I[LL_DB_FILTER_SIZE] = { MNull };
    MByte  *rowBuf8U[LL_DB_FILTER_SIZE] = { MNull };
    MInt32 *llData = waveletBuf[level].LL_data;
    for (y = startRow, m = 2*startRow + 1; y < endRow; ++y, m += 2)
    {
        MInt32 *llBuf = llData + y*sw;
        MInt32 *tmpLF = rowLF + LL_DB_FILTER_HALF_SIZE;
        if (level == 0)
        {
            for (n = m - LL_DB_FILTER_HALF_SIZE, j = 0; n < (m + LL_DB_FILTER_HALF_SIZE); ++n, ++j)
            {
                MInt32 val = n;
                if (val < 0)
                    val = -val;
                else if (val >= h)
                    val = 2 * h - val - 2;
                rowBuf8U[j] = srcBuf8U + val*lineSteps;
            }
            //column direction convolution
            for (x = 0; x < w; ++x)
            {
                tmpLF[x] = (low_filter[0] * rowBuf8U[0][x] + low_filter[1] * rowBuf8U[1][x] + low_filter[2] * rowBuf8U[2][x]
                    + low_filter[3] * rowBuf8U[3][x] + low_filter[4] * rowBuf8U[4][x] + low_filter[5] * rowBuf8U[5][x]) >> LL_HALF_SHIFT_SIZE;
            }
        }
        else
        {
            for (n = m - LL_DB_FILTER_HALF_SIZE, j = 0; n < (m + LL_DB_FILTER_HALF_SIZE); ++n, ++j)
            {
                MInt32 val = n;
                if (val < 0)
                    val = -val;
                else if (val >= h)
                    val = 2 * h - val - 2;
                rowBuf32I[j] = srcBuf32I + val*lineSteps;
            }
            //column direction convolution
            for (x = 0; x < w; ++x)
            {
                tmpLF[x] = (low_filter[0] * rowBuf32I[0][x] + low_filter[1] * rowBuf32I[1][x] + low_filter[2] * rowBuf32I[2][x]
                    + low_filter[3] * rowBuf32I[3][x] + low_filter[4] * rowBuf32I[4][x] + low_filter[5] * rowBuf32I[5][x]) >> LL_HALF_SHIFT_SIZE;
            }
        }
        //copy border
        for (n = -LL_DB_FILTER_HALF_SIZE; n < 0; ++n)
        {
            tmpLF[n] = tmpLF[-n];
        }
        for (n = w; n < (w + LL_DB_FILTER_HALF_SIZE); ++n)
        {
            tmpLF[n] = tmpLF[2 * w - n - 2];
        }
        tmpLF -= LL_DB_FILTER_HALF_SIZE;
        //row direction convolution
        if (level == 0)
        {
            for (x = 0, j = 1; x < sw; ++x, j += 2)
            {
                convRes = tmpLF[j] * low_filter[0] + tmpLF[j + 1] * low_filter[1] + tmpLF[j + 2] * low_filter[2]
                    + tmpLF[j + 3] * low_filter[3] + tmpLF[j + 4] * low_filter[4] + tmpLF[j + 5] * low_filter[5];
                llBuf[x] = convRes >> LL_HALF_SHIFT_SIZE;
            }
        }
        else
        {
            //avoid convolution data overflow
            for (x = 0; x < (w + LL_DB_FILTER_SIZE); ++x)
            {
                rowLF64[x] = tmpLF[x];
            }
            for (x = 0, j = 1; x < sw; ++x, j += 2)
            {
                convRes64 = rowLF64[j] * low_filter[0] + rowLF64[j + 1] * low_filter[1] + rowLF64[j + 2] * low_filter[2]
                    + rowLF64[j + 3] * low_filter[3] + rowLF64[j + 4] * low_filter[4] + rowLF64[j + 5] * low_filter[5];
                llBuf[x] = (MInt32)(convRes64 >> (LL_SHIFT_SIZE + LL_HALF_SHIFT_SIZE));
            }
        }
    }
}

#if defined(MCV_MULTI_THREAD)
MVoid thread_wl_LL_decomposition(MVoid* pParam)
{
    Wl_LL_Decom *wl_d = (Wl_LL_Decom*)pParam;
    MInt32 task_ID = wl_d->task_ID;
    MInt32 lret = MOK;

    dwt_LL_decomposition(wl_d->waveletBuf, wl_d->srcBuf32I, wl_d->srcBuf8U, wl_d->rowLF, wl_d->rowLF64,
        wl_d->low_filter, wl_d->w, wl_d->h, wl_d->sw, wl_d->lineSteps, wl_d->level, wl_d->startRow, wl_d->endRow);

    wl_d->errCode = lret;
}
#endif

//just do LL frequency discrete wavelet transform
MLong dwt_LLNS(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN _src, WAVELET_LPDATA waveletBuf, MInt32 levels)
{
    MLong res = MOK;
    MInt32 m, n, i, j, x, y, l, lWidth = _src->i32Width, lHeight = _src->i32Height;
    MInt32 w = lWidth, h = lHeight, lSize;
    MFloat ratioTotal = 0, total;
    MInt32 *rowLF[SMALL_TASKS_NUM] = { MNull };
    MInt64 *rowLF64[SMALL_TASKS_NUM] = { MNull };
    MInt32 Low_D[LL_DB_FILTER_SIZE];
    MByte *_srcData = (MByte*)_src->ppu8Plane[0];
    MInt32 *lfBuf32I = MNull;

    //enlarge filter 1024 times
    for (i = 0; i < LL_DB_FILTER_SIZE; ++i)
    {
        Low_D[i] = (MInt32)((db_decom_l[i] * 1024));
        ratioTotal += db_decom_l[i];
    }
    total = (ratioTotal * 1024 + 0.5f);
    Low_D[4] = total - Low_D[0] - Low_D[1] - Low_D[2] - Low_D[3] - Low_D[5];

    for (l = 0; l < levels; ++l)
    {
        MInt32 sw = waveletBuf[l].w, sh = waveletBuf[l].h, lineSteps = w;
        MInt32 lTask_Num = SMALL_TASKS_NUM;
        rowLF[0] = (MInt32*)MMemAlloc(hMemMgr, SMALL_TASKS_NUM*(lineSteps + LL_DB_FILTER_SIZE)*sizeof(MInt32));
        if (rowLF[0] == MNull)
        {
            res = MERR_NO_MEMORY;
            goto exit;
        }
        if (l >= 1)
        {
            rowLF64[0] = (MInt64*)MMemAlloc(hMemMgr, SMALL_TASKS_NUM*(lineSteps + LL_DB_FILTER_SIZE)*sizeof(MInt64));
            if (rowLF64 == MNull)
            {
                res = MERR_NO_MEMORY;
                goto exit;
            }
            for (j = 1; j < SMALL_TASKS_NUM; ++j)
            {
                rowLF64[j] = rowLF64[0] + j*(lineSteps + LL_DB_FILTER_SIZE);
            }
        }

        for (j = 1; j < SMALL_TASKS_NUM; ++j)
        {
            rowLF[j] = rowLF[0] + j*(lineSteps + LL_DB_FILTER_SIZE);
        }
#ifdef MCV_MULTI_THREAD
        {
            MInt32 taskID[SMALL_TASKS_NUM] = { 0 };
            Wl_LL_Decom pParams[SMALL_TASKS_NUM] = { 0 };

            lSize = sh / lTask_Num;
            lSize = lSize >> 1 << 1;

            pParams[0].startRow = 0;
            pParams[0].endRow = lSize;
            for (i = 1; i < lTask_Num; i++)
            {
                pParams[i].startRow = i*lSize;
                pParams[i].endRow = (i + 1)*lSize;
            }
            pParams[i - 1].endRow = sh;

            for (i = 0; i < lTask_Num; i++)
            {
                pParams[i].task_ID = i;
                pParams[i].waveletBuf = waveletBuf;
                pParams[i].srcBuf32I = lfBuf32I;
                pParams[i].srcBuf8U = _srcData;
                pParams[i].rowLF = rowLF[i];
                pParams[i].rowLF64 = rowLF64[i];
                pParams[i].low_filter = Low_D;
                pParams[i].w = w;
                pParams[i].h = h;
                pParams[i].sw = sw;
                pParams[i].lineSteps = lineSteps;
                pParams[i].level = l;
            }

            for (i = 0; i < lTask_Num; i++)
            {
                taskID[i] = mcvAddTask(mcvParallelMonitor, thread_wl_LL_decomposition, (MVoid*)&pParams[i]);
            }
            for (i = 0; i < lTask_Num; i++)
            {
                mcvWaitTask(mcvParallelMonitor, taskID[i]);
            }
        }
#else
        dwt_LL_decomposition(waveletBuf, lfBuf, rowLF[0], rowLF64[0], Low_D, w, h, sw, lineSteps, l, 0, sh);
#endif

        if (rowLF[0])
        {
            MMemFree(hMemMgr, rowLF[0]);
            rowLF[0] = MNull;
        }
        lfBuf32I = waveletBuf[l].LL_data;
        w = w >> 1;
        h = h >> 1;
    }

exit:
    if (rowLF[0])
    {
        MMemFree(hMemMgr, rowLF[0]);
        rowLF[0] = MNull;
    }
    if (rowLF64[0])
    {
        MMemFree(hMemMgr, rowLF64[0]);
        rowLF64[0] = MNull;
    }
    return res;
}

/**********************************************************************************************************/

MVoid idwt_LL_reconstruct(WAVELET_LPDATA waveletBuf, MInt32 *currRowBuf, MInt32 *nextRowBuf, MInt32 *resData,
    MInt32 *low_filter, MInt32 sh, MInt32 sw, MInt32 level, MInt32 startRow, MInt32 endRow, LPASVLOFFSCREEN resImg)
{
    MInt32 *rowBufs[LL_DB_FILTER_SIZE];
    MInt32 convRes, halfWin = LL_DB_FILTER_HALF_SIZE >> 1;
    MInt32 x, y, m, n, k, curr_y, next_y, w = sw << 1;
    MInt32 *waveDecomData = MNull, *rowFilter = MNull, *colFilter = MNull;
    MByte *resPtr = MNull;
    MInt32 resLinePitch = 0;
    if (resImg != MNull)
    {
        resPtr = (MByte*)resImg->ppu8Plane[0];
        resLinePitch = resImg->pi32Pitch[0];
    }
    rowFilter = low_filter;
    colFilter = low_filter;
    waveDecomData = waveletBuf[level].LL_data;

    for (y = startRow; y < endRow; ++y)
    {
        curr_y = y << 1;
        next_y = curr_y + 1;
        MInt32 *currBuf = resData + curr_y*w;
        MInt32 *nextBuf = resData + next_y*w;
        MByte *currResImg = resPtr + curr_y*resLinePitch;
        MByte *nextResImg = resPtr + next_y*resLinePitch;

        for (m = y - halfWin, n = 0; m <= (y + halfWin); ++m, ++n)
        {
            MInt32 val = m;
            if (val < 0)
                val = -val;
            else if (val >= sh)
                val = 2 * sh - val - 2;

            rowBufs[n] = waveDecomData + val*sw;
        }
        MInt32 *currRowRecon = currRowBuf + halfWin;
        MInt32 *nextRowRecon = nextRowBuf + halfWin;
        MInt32 *buf0 = rowBufs[0], *buf1 = rowBufs[1], *buf2 = rowBufs[2];
        //column direction convolution
        for (x = 0; x < sw; ++x)
        {
            //even row column convolution
            currRowRecon[x] = (buf0[x] * colFilter[1] + buf1[x] * colFilter[3] + buf2[x] * colFilter[5]) >> (LL_SHIFT_SIZE);
            //odd row column convolution
            nextRowRecon[x] = (buf0[x] * colFilter[0] + buf1[x] * colFilter[2] + buf2[x] * colFilter[4]) >> (LL_SHIFT_SIZE);
        }
        //copy border
        for (m = -halfWin; m < 0; ++m)
        {
            currRowRecon[m] = currRowRecon[-m];
            nextRowRecon[m] = nextRowRecon[-m];
        }
        for (m = sw; m < (sw + halfWin); ++m)
        {
            currRowRecon[m] = currRowRecon[2 * sw - m - 2];
            nextRowRecon[m] = nextRowRecon[2 * sw - m - 2];
        }
        currRowRecon -= halfWin;
        nextRowRecon -= halfWin;
        //row direction convolution
        if (level == 0)
        {
            if (resImg == MNull)
            {
                for (x = 0; x < sw; ++x)
                {
                    convRes = currRowRecon[x] * rowFilter[1] + currRowRecon[x + 1] * rowFilter[3] + currRowRecon[x + 2] * rowFilter[5];
                    currBuf[2 * x] = (convRes >> (LL_DOUBLE_SHIFT_SIZE));

                    convRes = currRowRecon[x] * rowFilter[0] + currRowRecon[x + 1] * rowFilter[2] + currRowRecon[x + 2] * rowFilter[4];
                    currBuf[2 * x + 1] = (convRes >> (LL_DOUBLE_SHIFT_SIZE));

                    convRes = nextRowRecon[x] * rowFilter[1] + nextRowRecon[x + 1] * rowFilter[3] + nextRowRecon[x + 2] * rowFilter[5];
                    nextBuf[2 * x] = (convRes >> (LL_DOUBLE_SHIFT_SIZE));

                    convRes = nextRowRecon[x] * rowFilter[0] + nextRowRecon[x + 1] * rowFilter[2] + nextRowRecon[x + 2] * rowFilter[4];
                    nextBuf[2 * x + 1] = (convRes >> (LL_DOUBLE_SHIFT_SIZE));
                }
            }
            else
            {
                MInt32 tmpVal;
                for (x = 0; x < sw; ++x)
                {
                    convRes = currRowRecon[x] * rowFilter[1] + currRowRecon[x + 1] * rowFilter[3] + currRowRecon[x + 2] * rowFilter[5];
                    tmpVal = (MInt32)currResImg[2 * x] - (convRes >> (LL_DOUBLE_SHIFT_SIZE));
                    currResImg[2 * x] = TRIMBYTE(tmpVal);

                    convRes = currRowRecon[x] * rowFilter[0] + currRowRecon[x + 1] * rowFilter[2] + currRowRecon[x + 2] * rowFilter[4];
                    tmpVal = (MInt32)currResImg[2 * x + 1] - (convRes >> (LL_DOUBLE_SHIFT_SIZE));
                    currResImg[2 * x + 1] = TRIMBYTE(tmpVal);

                    convRes = nextRowRecon[x] * rowFilter[1] + nextRowRecon[x + 1] * rowFilter[3] + nextRowRecon[x + 2] * rowFilter[5];
                    tmpVal = (MInt32)nextResImg[2 * x] - (convRes >> (LL_DOUBLE_SHIFT_SIZE));
                    nextResImg[2 * x] = TRIMBYTE(tmpVal);

                    convRes = nextRowRecon[x] * rowFilter[0] + nextRowRecon[x + 1] * rowFilter[2] + nextRowRecon[x + 2] * rowFilter[4];
                    tmpVal = (MInt32)nextResImg[2 * x + 1] - (convRes >> (LL_DOUBLE_SHIFT_SIZE));
                    nextResImg[2 * x + 1] = TRIMBYTE(tmpVal);
                }
            }
        }
        else
        {
            for (x = 0; x < sw; ++x)
            {
                convRes = currRowRecon[x] * rowFilter[1] + currRowRecon[x + 1] * rowFilter[3] + currRowRecon[x + 2] * rowFilter[5];
                currBuf[2 * x] = (convRes >> (LL_SHIFT_SIZE));

                convRes = currRowRecon[x] * rowFilter[0] + currRowRecon[x + 1] * rowFilter[2] + currRowRecon[x + 2] * rowFilter[4];
                currBuf[2 * x + 1] = (convRes >> (LL_SHIFT_SIZE));

                convRes = nextRowRecon[x] * rowFilter[1] + nextRowRecon[x + 1] * rowFilter[3] + nextRowRecon[x + 2] * rowFilter[5];
                nextBuf[2 * x] = (convRes >> (LL_SHIFT_SIZE));

                convRes = nextRowRecon[x] * rowFilter[0] + nextRowRecon[x + 1] * rowFilter[2] + nextRowRecon[x + 2] * rowFilter[4];
                nextBuf[2 * x + 1] = (convRes >> (LL_SHIFT_SIZE));
            }
        }
    }
}

#if defined(MCV_MULTI_THREAD)
MVoid thread_wl_LL_reconstruct(MVoid* pParam)
{
    Wl_LL_Recon *wl_r = (Wl_LL_Recon*)pParam;
    MInt32 task_ID = wl_r->task_ID;
    MInt32 lret = MOK;

    idwt_LL_reconstruct(wl_r->waveletBuf, wl_r->currRowBuf, wl_r->nextRowBuf, wl_r->resData, wl_r->low_filter,
        wl_r->sh, wl_r->sw, wl_r->level, wl_r->startRow, wl_r->endRow, wl_r->resImg);

    wl_r->errCode = lret;

}
#endif

//just do low-frequency inverse discrete wavelet transform
MLong idwt_LLNS(MHandle hMemMgr, MHandle mcvParallelMonitor, WAVELET_LPDATA waveletBuf, MInt32 lHeight, MInt32 lWidth, MInt32 levels, LPASVLOFFSCREEN resImg)
{
    MLong res = MOK;
    MInt32 x, y, i, j, l, k;
    MInt32 sw, sh, w, h, numPxls, lSize;
    MInt32 *currRowBuf[SMALL_TASKS_NUM] = { MNull }, *nextRowBuf[SMALL_TASKS_NUM] = { MNull };
    MInt32 Low_R[LL_DB_FILTER_SIZE];
    MFloat ratioTotal = 0, total;
    MInt32 *reconLLBuf = MNull, *decomLLBuf = MNull;

    //enlarge filter 1024 times
    for (i = 0; i < LL_DB_FILTER_SIZE; ++i)
    {
        Low_R[i] = (MInt32)((db_recon_l[i] * 1024));
        ratioTotal += db_recon_l[i];
    }
    total = (ratioTotal * 1024 + 0.5f);
    Low_R[1] = total - Low_R[0] - Low_R[2] - Low_R[3] - Low_R[4] - Low_R[5];

    for (l = levels - 1; l >= 0; --l)
    {
        MInt32 lTask_Num = SMALL_TASKS_NUM;
        sw = waveletBuf[l].w;
        sh = waveletBuf[l].h;
        w = sw << 1;
        h = sh << 1;
        numPxls = w*h;
        currRowBuf[0] = (MInt32*)MMemAlloc(hMemMgr, SMALL_TASKS_NUM * (sw + 2 * (LL_DB_FILTER_HALF_SIZE >> 1))*sizeof(MInt32));
        nextRowBuf[0] = (MInt32*)MMemAlloc(hMemMgr, SMALL_TASKS_NUM * (sw + 2 * (LL_DB_FILTER_HALF_SIZE >> 1))*sizeof(MInt32));
        if (currRowBuf[0] == MNull || nextRowBuf[0] == MNull)
        {
            res = MERR_NO_MEMORY;
            goto exit;
        }
        for (j = 1; j < SMALL_TASKS_NUM; ++j)
        {
            currRowBuf[j] = currRowBuf[0] + j*(sw + 2 * (LL_DB_FILTER_HALF_SIZE >> 1));
            nextRowBuf[j] = nextRowBuf[0] + j*(sw + 2 * (LL_DB_FILTER_HALF_SIZE >> 1));
        }

        if (l > 0)
        {
            decomLLBuf = waveletBuf[l - 1].LL_data;
            reconLLBuf = (MInt32*)MMemAlloc(hMemMgr, numPxls*sizeof(MInt32));
            if (reconLLBuf == MNull)
            {
                res = MERR_NO_MEMORY;
                goto exit;
            }
        }

#ifdef MCV_MULTI_THREAD
        {
            MInt32 taskID[SMALL_TASKS_NUM] = { 0 };
            Wl_LL_Recon pParams[SMALL_TASKS_NUM] = { 0 };

            lSize = sh / lTask_Num;
            lSize = lSize >> 1 << 1;

            pParams[0].startRow = 0;
            pParams[0].endRow = lSize;
            for (i = 1; i < lTask_Num; i++)
            {
                pParams[i].startRow = i*lSize;
                pParams[i].endRow = (i + 1)*lSize;
            }
            pParams[i - 1].endRow = sh;

            for (i = 0; i < lTask_Num; i++)
            {
                pParams[i].task_ID = i;
                pParams[i].waveletBuf = waveletBuf;
                pParams[i].resImg = resImg;
                pParams[i].currRowBuf = currRowBuf[i];
                pParams[i].nextRowBuf = nextRowBuf[i];
                pParams[i].resData = reconLLBuf;
                pParams[i].low_filter = Low_R;
                pParams[i].sw = sw;
                pParams[i].sh = sh;
                pParams[i].level = l;
            }

            for (i = 0; i < lTask_Num; i++)
            {
                taskID[i] = mcvAddTask(mcvParallelMonitor, thread_wl_LL_reconstruct, (MVoid*)&pParams[i]);
            }
            for (i = 0; i < lTask_Num; i++)
            {
                mcvWaitTask(mcvParallelMonitor, taskID[i]);
            }
        }
#else
        idwt_LL_reconstruct(waveletBuf, currRowBuf[0], nextRowBuf[0], dstBuf, Low_R, sh, sw, l, 0, sh);
#endif

        if (l > 0)
        {
            MMemCpy(decomLLBuf, reconLLBuf, numPxls*sizeof(MInt32));
        }

        if (reconLLBuf)
        {
            MMemFree(hMemMgr, reconLLBuf);
            reconLLBuf = MNull;
        }
        if (currRowBuf[0])
        {
            MMemFree(hMemMgr, currRowBuf[0]);
            currRowBuf[0] = MNull;
        }
        if (nextRowBuf[0])
        {
            MMemFree(hMemMgr, nextRowBuf[0]);
            nextRowBuf[0] = MNull;
        }
    }

exit:
    if (reconLLBuf)
    {
        MMemFree(hMemMgr, reconLLBuf);
        reconLLBuf = MNull;
    }
    if (currRowBuf[0])
    {
        MMemFree(hMemMgr, currRowBuf[0]);
        currRowBuf[0] = MNull;
    }
    if (nextRowBuf[0])
    {
        MMemFree(hMemMgr, nextRowBuf[0]);
        nextRowBuf[0] = MNull;
    }
    return res;
}

/***********************************************************************************************************/
MVoid rough_dwt_LL_decomposition(WAVELET_LPDATA_8U waveletBuf, MByte *srcBuf, MByte *rowLF,
    MInt32 *low_filter, MInt32 w, MInt32 h, MInt32 sw, MInt32 lineSteps, MInt32 level, MInt32 startRow, MInt32 endRow)
{
    MInt32 x, y, m, n, i, j;
    MInt32 convRes;
    MByte *rowBuf[LL_DB_FILTER_SIZE];
    MByte *llData = waveletBuf[level].LL_data;
    for (y = startRow, m = 2 * startRow + 1; y < endRow; ++y, m += 2)
    {
        MByte *llBuf = llData + y*sw;
        for (n = m - LL_DB_FILTER_HALF_SIZE, j = 0; n < (m + LL_DB_FILTER_HALF_SIZE); ++n, ++j)
        {
            MInt32 val = n;
            if (val < 0)
                val = -val;
            else if (val >= h)
                val = 2 * h - val - 2;
            rowBuf[j] = srcBuf + val*lineSteps;
        }
        MByte *tmpLF = rowLF + LL_DB_FILTER_HALF_SIZE;
        MByte *buf0 = rowBuf[0], *buf1 = rowBuf[1], *buf2 = rowBuf[2];
        MByte *buf3 = rowBuf[3], *buf4 = rowBuf[4], *buf5 = rowBuf[5];
        //column direction convolution
        for (x = 0; x < w; ++x)
        {
             convRes = (low_filter[0] * buf0[x] + low_filter[1] * buf1[x] + low_filter[2] * buf2[x]
                + low_filter[3] * buf3[x] + low_filter[4] * buf4[x] + low_filter[5] * buf5[x]) >> LL_SHIFT_SIZE;
             tmpLF[x] = TRIMBYTE(convRes);
        }
        //copy border
        for (n = -LL_DB_FILTER_HALF_SIZE; n < 0; ++n)
        {
            tmpLF[n] = tmpLF[-n];
        }
        for (n = w; n < (w + LL_DB_FILTER_HALF_SIZE); ++n)
        {
            tmpLF[n] = tmpLF[2 * w - n - 2];
        }
        tmpLF -= LL_DB_FILTER_HALF_SIZE;
        //row direction convolution
        for (x = 0, j = 1; x < sw; ++x, j += 2)
        {
            convRes = (low_filter[0] * tmpLF[j] + low_filter[1] * tmpLF[j + 1] + low_filter[2] * tmpLF[j + 2]
                + low_filter[3] * tmpLF[j + 3] + low_filter[4] * tmpLF[j + 4] + low_filter[5] * tmpLF[j + 5]) >> LL_SHIFT_SIZE;
            llBuf[x] = TRIMBYTE(convRes);
        }
    }
}

#if defined(MCV_MULTI_THREAD)
MVoid thread_rough_wl_LL_decomposition(MVoid* pParam)
{
    Wl_LL_Decom_8U *wl_d = (Wl_LL_Decom_8U*)pParam;
    MInt32 task_ID = wl_d->task_ID;
    MInt32 lret = MOK;

    rough_dwt_LL_decomposition(wl_d->waveletBuf, wl_d->srcBuf, wl_d->rowLF,
        wl_d->low_filter, wl_d->w, wl_d->h, wl_d->sw, wl_d->lineSteps, wl_d->level, wl_d->startRow, wl_d->endRow);

    wl_d->errCode = lret;
}
#endif

//just do LL frequency discrete wavelet transform
MLong rough_dwt_LLNS(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN _src, WAVELET_LPDATA_8U waveletBuf, MInt32 levels)
{
    MLong res = MOK;
    MInt32 m, n, i, j, x, y, l, lWidth = _src->i32Width, lHeight = _src->i32Height;
    MInt32 w = lWidth, h = lHeight, lSize;
    MFloat ratioTotal = 0, scale;
    MInt32 Low_D[LL_DB_FILTER_SIZE];
    MByte *rowLF[SMALL_TASKS_NUM] = { MNull };
    MByte *_srcData = (MByte*)_src->ppu8Plane[0];
    MByte *lfBuf = _srcData;

    //normalize and enlarge filter to 1024
    for (i = 0; i < LL_DB_FILTER_SIZE; ++i)
        ratioTotal += db_decom_l[i];
    scale = 1024 / ratioTotal;

    for (i = 0; i < LL_DB_FILTER_SIZE; ++i)
    {
        Low_D[i] = db_decom_l[i] * scale + 0.5f;
    }
    Low_D[4] = 1024 - Low_D[0] - Low_D[1] - Low_D[2] - Low_D[3] - Low_D[5];

    for (l = 0; l < levels; ++l)
    {
        MInt32 lTask_Num = SMALL_TASKS_NUM;
        MInt32 sw = waveletBuf[l].w, sh = waveletBuf[l].h, lineSteps = w;
        rowLF[0] = (MByte*)MMemAlloc(hMemMgr, SMALL_TASKS_NUM*(lineSteps + LL_DB_FILTER_SIZE)*sizeof(MByte));
        if (rowLF[0] == MNull)
        {
            res = MERR_NO_MEMORY;
            goto exit;
        }

        for (j = 1; j < SMALL_TASKS_NUM; ++j)
        {
            rowLF[j] = rowLF[0] + j*(lineSteps + LL_DB_FILTER_SIZE);
        }
#ifdef MCV_MULTI_THREAD
        {
            MInt32 taskID[SMALL_TASKS_NUM] = { 0 };
            Wl_LL_Decom_8U pParams[SMALL_TASKS_NUM] = { 0 };

            lSize = sh / lTask_Num;
            lSize = lSize >> 1 << 1;

            pParams[0].startRow = 0;
            pParams[0].endRow = lSize;
            for (i = 1; i < lTask_Num; i++)
            {
                pParams[i].startRow = i*lSize;
                pParams[i].endRow = (i + 1)*lSize;
            }
            pParams[i - 1].endRow = sh;

            for (i = 0; i < lTask_Num; i++)
            {
                pParams[i].task_ID = i;
                pParams[i].waveletBuf = waveletBuf;
                pParams[i].srcBuf = lfBuf;
                pParams[i].rowLF = rowLF[i];
                pParams[i].low_filter = Low_D;
                pParams[i].w = w;
                pParams[i].h = h;
                pParams[i].sw = sw;
                pParams[i].lineSteps = lineSteps;
                pParams[i].level = l;
            }

            for (i = 0; i < lTask_Num; i++)
            {
                taskID[i] = mcvAddTask(mcvParallelMonitor, thread_rough_wl_LL_decomposition, (MVoid*)&pParams[i]);
            }
            for (i = 0; i < lTask_Num; i++)
            {
                mcvWaitTask(mcvParallelMonitor, taskID[i]);
            }
        }
#else
        rough_dwt_LL_decomposition(waveletBuf, lfBuf, rowLF[0], Low_D, w, h, sw, lineSteps, l, 0, sh);
#endif
        if (rowLF[0])
        {
            MMemFree(hMemMgr, rowLF[0]);
            rowLF[0] = MNull;
        }
        lfBuf = waveletBuf[l].LL_data;
        w = w >> 1;
        h = h >> 1;
    }

exit:
    return res;
}

#endif

#ifdef DB2
/***********************************************************************************************************/
//只做低频的部分的小波正变换
MVoid dwt_LL_decomposition(WAVELET_LPDATA waveletBuf, MInt32 *srcBuf32I, MByte *srcBuf8U, MInt32 *rowLF, MInt64 *rowLF64,
                           MInt32 *low_filter, MInt32 w, MInt32 h, MInt32 sw, MInt32 lineSteps, MInt32 level, MInt32 startRow, MInt32 endRow)
{
    MInt32 x, y, m, n, i, j;
    MInt32 convRes;
    MInt64 convRes64;
    MInt32 *rowBuf32I[LL_DB_FILTER_SIZE] = {MNull};
    MByte *rowBuf8U[LL_DB_FILTER_SIZE] = {MNull};
    MInt32 *llData = waveletBuf[ level ].LL_data;

#if defined(USE_NEON)
    int16x4_t vlo_d0_16x4 = vdup_n_s16(low_filter[0]);
    int16x4_t vlo_d1_16x4 = vdup_n_s16(low_filter[1]);
    int16x4_t vlo_d2_16x4 = vdup_n_s16(low_filter[2]);
    int16x4_t vlo_d3_16x4 = vdup_n_s16(low_filter[3]);

    int32x4_t vlo_d0_32x4 = vdupq_n_s32(low_filter[0]);
    int32x4_t vlo_d1_32x4 = vdupq_n_s32(low_filter[1]);
    int32x4_t vlo_d2_32x4 = vdupq_n_s32(low_filter[2]);
    int32x4_t vlo_d3_32x4 = vdupq_n_s32(low_filter[3]);
#endif
    for(y = startRow, m = 2 * startRow + 1; y < endRow; ++y, m += 2)
    {
        MInt32 *llBuf = llData + y * sw;
        MInt32 *tmpLF = rowLF + LL_DB_FILTER_HALF_SIZE;
        if( level == 0 )
        {
            for(n = m - LL_DB_FILTER_HALF_SIZE, j = 0; n < ( m + LL_DB_FILTER_HALF_SIZE); ++n, ++j)
            {
                MInt32 val = n;
                if( val < 0 )
                    val = -val;
                else if( val >= h )
                    val = 2 * h - val - 2;
                rowBuf8U[ j ] = srcBuf8U + val * lineSteps;
            }
            //column direction convolution
            x = 0;
#if defined(USE_NEON)
            {
                uint8x8_t tmpdata_8x8;
                int16x8_t tmpdata;
                int16x4_t Sdata00, Sdata01;
                int32x4_t sum00, sum01;
                int32x4_t res00, res01;

                for (x = 0; x < w - 7; x += 8)
                {
                    tmpdata_8x8 = vld1_u8(rowBuf8U[0] + x);
                    tmpdata = vreinterpretq_s16_u16(vmovl_u8(tmpdata_8x8));
                    Sdata00 = vget_low_s16(tmpdata);
                    Sdata01 = vget_high_s16(tmpdata);
                    sum00 = vmull_s16(Sdata00, vlo_d0_16x4);
                    sum01 = vmull_s16(Sdata01, vlo_d0_16x4);

                    tmpdata_8x8 = vld1_u8(rowBuf8U[1] + x);
                    tmpdata = vreinterpretq_s16_u16(vmovl_u8(tmpdata_8x8));
                    Sdata00 = vget_low_s16(tmpdata);
                    Sdata01 = vget_high_s16(tmpdata);
                    sum00 = vmlal_s16(sum00, Sdata00, vlo_d1_16x4);
                    sum01 = vmlal_s16(sum01, Sdata01, vlo_d1_16x4);

                    tmpdata_8x8 = vld1_u8(rowBuf8U[2] + x);
                    tmpdata = vreinterpretq_s16_u16(vmovl_u8(tmpdata_8x8));
                    Sdata00 = vget_low_s16(tmpdata);
                    Sdata01 = vget_high_s16(tmpdata);
                    sum00 = vmlal_s16(sum00, Sdata00, vlo_d2_16x4);
                    sum01 = vmlal_s16(sum01, Sdata01, vlo_d2_16x4);

                    tmpdata_8x8 = vld1_u8(rowBuf8U[3] + x);
                    tmpdata = vreinterpretq_s16_u16(vmovl_u8(tmpdata_8x8));
                    Sdata00 = vget_low_s16(tmpdata);
                    Sdata01 = vget_high_s16(tmpdata);
                    sum00 = vmlal_s16(sum00, Sdata00, vlo_d3_16x4);
                    sum01 = vmlal_s16(sum01, Sdata01, vlo_d3_16x4);

                    res00 = vshrq_n_s32(sum00, LL_HALF_SHIFT_SIZE);
                    res01 = vshrq_n_s32(sum01, LL_HALF_SHIFT_SIZE);
                    vst1q_s32(tmpLF + x, res00);
                    vst1q_s32(tmpLF + x + 4, res01);
                }
            }
#endif
            for(; x < w; ++x)
            {
                tmpLF[ x ] = ( low_filter[ 0 ] * rowBuf8U[ 0 ][ x ] + low_filter[ 1 ] * rowBuf8U[ 1 ][ x ] + low_filter[ 2 ] * rowBuf8U[ 2 ][ x ]
                               + low_filter[ 3 ] * rowBuf8U[ 3 ][ x ] ) >> LL_HALF_SHIFT_SIZE;
            }
        }
        else
        {
            for(n = m - LL_DB_FILTER_HALF_SIZE, j = 0; n < ( m + LL_DB_FILTER_HALF_SIZE); ++n, ++j)
            {
                MInt32 val = n;
                if( val < 0 )
                    val = -val;
                else if( val >= h )
                    val = 2 * h - val - 2;
                rowBuf32I[ j ] = srcBuf32I + val * lineSteps;
            }
            //column direction convolution
            x = 0;
#if defined(USE_NEON)
            {
                int32x4_t tmpdata_32x4;
                int32x4_t sum, Sdata;

                for (x = 0; x < w - 3; x += 4)
                {
                    tmpdata_32x4 = vld1q_s32(rowBuf32I[0] + x);
                    sum = vmulq_s32(tmpdata_32x4, vlo_d0_32x4);

                    tmpdata_32x4 = vld1q_s32(rowBuf32I[1] + x);
                    sum = vmlaq_s32(sum, tmpdata_32x4, vlo_d1_32x4);

                    tmpdata_32x4 = vld1q_s32(rowBuf32I[2] + x);
                    sum = vmlaq_s32(sum, tmpdata_32x4, vlo_d2_32x4);

                    tmpdata_32x4 = vld1q_s32(rowBuf32I[3] + x);
                    sum = vmlaq_s32(sum, tmpdata_32x4, vlo_d3_32x4);

                    Sdata = vshrq_n_s32(sum, LL_HALF_SHIFT_SIZE);
                    vst1q_s32(tmpLF + x, Sdata);
                }

            }
#endif
            for(; x < w; ++x)
            {
                tmpLF[ x ] = ( low_filter[ 0 ] * rowBuf32I[ 0 ][ x ] + low_filter[ 1 ] * rowBuf32I[ 1 ][ x ] + low_filter[ 2 ] * rowBuf32I[ 2 ][ x ]
                               + low_filter[ 3 ] * rowBuf32I[ 3 ][ x ] ) >> LL_HALF_SHIFT_SIZE;
            }
        }
        //copy border
        for(n = -LL_DB_FILTER_HALF_SIZE; n < 0; ++n)
        {
            tmpLF[ n ] = tmpLF[ -n ];
        }
        for(n = w; n < ( w + LL_DB_FILTER_HALF_SIZE); ++n)
        {
            tmpLF[ n ] = tmpLF[ 2 * w - n - 2 ];
        }
        tmpLF -= LL_DB_FILTER_HALF_SIZE;
        //row direction convolution
        if( level == 0 )
        {
            x = 0;
            j = 1;
#if defined(USE_NEON)
            {
                int32x4x2_t tmpdata00, tmpdata01;
                int32x4_t tmpdata00_32x4, tmpdata01_32x4;
                int32x4_t sum, sum_shift;

                for (x = 0, j = 1; x < sw - 3; x += 4, j += 8)
                {
                    tmpdata00 = vld2q_s32(tmpLF + j);
                    tmpdata01 = vld2q_s32(tmpLF + j + 2);

                    tmpdata00_32x4 = tmpdata00.val[0];
                    tmpdata01_32x4 = tmpdata00.val[1];

                    sum = vmulq_s32(tmpdata00_32x4, vlo_d0_32x4);
                    sum = vmlaq_s32(sum, tmpdata01_32x4, vlo_d1_32x4);

                    tmpdata00_32x4 = tmpdata01.val[0];
                    tmpdata01_32x4 = tmpdata01.val[1];

                    sum = vmlaq_s32(sum, tmpdata00_32x4, vlo_d2_32x4);
                    sum = vmlaq_s32(sum, tmpdata01_32x4, vlo_d3_32x4);

                    sum_shift = vshrq_n_s32(sum, LL_HALF_SHIFT_SIZE);
                    vst1q_s32(llBuf + x, sum_shift);
                }
            }
#endif
            for(; x < sw; ++x, j += 2)
            {
                convRes = tmpLF[ j ] * low_filter[ 0 ] + tmpLF[ j + 1 ] * low_filter[ 1 ] + tmpLF[ j + 2 ] * low_filter[ 2 ]
                          + tmpLF[ j + 3 ] * low_filter[ 3 ];
                llBuf[ x ] = convRes >> LL_HALF_SHIFT_SIZE;
            }
        }
        else
        {
            //avoid convolution data overflow
            for(x = 0; x < ( w + LL_DB_FILTER_SIZE); ++x)
            {
                rowLF64[ x ] = tmpLF[ x ];
            }
            for(x = 0, j = 1; x < sw; ++x, j += 2)
            {
                convRes64 = rowLF64[ j ] * low_filter[ 0 ] + rowLF64[ j + 1 ] * low_filter[ 1 ] + rowLF64[ j + 2 ] * low_filter[ 2 ]
                            + rowLF64[ j + 3 ] * low_filter[ 3 ];
                llBuf[ x ] = ( MInt32 ) ( convRes64 >> (LL_SHIFT_SIZE + LL_HALF_SHIFT_SIZE));
            }
        }
    }
}

#if defined(MCV_MULTI_THREAD)

MVoid thread_wl_LL_decomposition(MVoid *pParam)
{
    Wl_LL_Decom *wl_d = ( Wl_LL_Decom * ) pParam;
    MInt32 task_ID = wl_d->task_ID;
    MInt32 lret = MOK;

    dwt_LL_decomposition(wl_d->waveletBuf, wl_d->srcBuf32I, wl_d->srcBuf8U, wl_d->rowLF, wl_d->rowLF64,
                         wl_d->low_filter, wl_d->w, wl_d->h, wl_d->sw, wl_d->lineSteps, wl_d->level, wl_d->startRow, wl_d->endRow);

    wl_d->errCode = lret;
}

#endif

//just do LL frequency discrete wavelet transform
MLong dwt_LLNS(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN _src, WAVELET_LPDATA waveletBuf, MInt32 levels)
{
    MLong res = MOK;
    MInt32 m, n, i, j, x, y, l, lWidth = _src->i32Width, lHeight = _src->i32Height;
    MInt32 w = lWidth, h = lHeight, lSize;
    MFloat ratioTotal = 0, total;
    MInt32 *rowLF[DENOISE_TASK_NUM] = {MNull};
    MInt64 *rowLF64[DENOISE_TASK_NUM] = {MNull};
    MInt32 Low_D[LL_DB_FILTER_SIZE];
    MByte *_srcData = ( MByte * ) _src->ppu8Plane[ 0 ];
    MInt32 pitch = _src->pi32Pitch[ 0 ];
    MInt32 *lfBuf32I = MNull;

    //enlarge filter 1024 times
    for(i = 0; i < LL_DB_FILTER_SIZE; ++i)
    {
        Low_D[ i ] = ( MInt32 ) (( db_decom_l[ i ] * 1024 ));
        ratioTotal += db_decom_l[ i ];
    }
    total = ( ratioTotal * 1024 + 0.5f );
    Low_D[ 2 ] = total - Low_D[ 0 ] - Low_D[ 1 ] - Low_D[ 3 ];

    for(l = 0; l < levels; ++l)
    {
        MInt32 sw = waveletBuf[ l ].w, sh = waveletBuf[ l ].h, lineSteps;
        if( l == 0 )
            lineSteps = pitch;
        else
            lineSteps = w;
        MInt32 lTask_Num = DENOISE_TASK_NUM;
        rowLF[ 0 ] = ( MInt32 * ) MMemAlloc(hMemMgr, DENOISE_TASK_NUM * ( lineSteps + LL_DB_FILTER_SIZE) * sizeof(MInt32));
        if( rowLF[ 0 ] == MNull )
        {
            res = MERR_NO_MEMORY;
            goto exit;
        }
        if( l >= 1 )
        {
            rowLF64[ 0 ] = ( MInt64 * ) MMemAlloc(hMemMgr, DENOISE_TASK_NUM * ( lineSteps + LL_DB_FILTER_SIZE) * sizeof(MInt64));
            if( rowLF64 == MNull )
            {
                res = MERR_NO_MEMORY;
                goto exit;
            }
            for(j = 1; j < DENOISE_TASK_NUM; ++j)
            {
                rowLF64[ j ] = rowLF64[ 0 ] + j * ( lineSteps + LL_DB_FILTER_SIZE);
            }
        }

        for(j = 1; j < DENOISE_TASK_NUM; ++j)
        {
            rowLF[ j ] = rowLF[ 0 ] + j * ( lineSteps + LL_DB_FILTER_SIZE);
        }
#ifdef MCV_MULTI_THREAD
        {
            MInt32 taskID[DENOISE_TASK_NUM] = {0};
            Wl_LL_Decom pParams[DENOISE_TASK_NUM] = {0};

            lSize = sh / lTask_Num;
            lSize = lSize >> 1 << 1;

            pParams[ 0 ].startRow = 0;
            pParams[ 0 ].endRow = lSize;
            for(i = 1; i < lTask_Num; i++)
            {
                pParams[ i ].startRow = i * lSize;
                pParams[ i ].endRow = ( i + 1 ) * lSize;
            }
            pParams[ i - 1 ].endRow = sh;

            for(i = 0; i < lTask_Num; i++)
            {
                pParams[ i ].task_ID = i;
                pParams[ i ].waveletBuf = waveletBuf;
                pParams[ i ].srcBuf32I = lfBuf32I;
                pParams[ i ].srcBuf8U = _srcData;
                pParams[ i ].rowLF = rowLF[ i ];
                pParams[ i ].rowLF64 = rowLF64[ i ];
                pParams[ i ].low_filter = Low_D;
                pParams[ i ].w = w;
                pParams[ i ].h = h;
                pParams[ i ].sw = sw;
                pParams[ i ].lineSteps = lineSteps;
                pParams[ i ].level = l;
            }

            for(i = 0; i < lTask_Num; i++)
            {
                taskID[ i ] = mcvAddTask(mcvParallelMonitor, thread_wl_LL_decomposition, ( MVoid * ) &pParams[ i ]);
            }
            for(i = 0; i < lTask_Num; i++)
            {
                mcvWaitTask(mcvParallelMonitor, taskID[ i ]);
            }
        }
#else
        //todo:暂时注释掉 by jck
        //dwt_LL_decomposition(waveletBuf, lfBuf, rowLF[0], rowLF64[0], Low_D, w, h, sw, lineSteps, l, 0, sh);
#endif

        if( rowLF[ 0 ] )
        {
            MMemFree(hMemMgr, rowLF[ 0 ]);
            rowLF[ 0 ] = MNull;
        }
        lfBuf32I = waveletBuf[ l ].LL_data;
        w = w >> 1;
        h = h >> 1;
    }

    exit:
    if( rowLF[ 0 ] )
    {
        MMemFree(hMemMgr, rowLF[ 0 ]);
        rowLF[ 0 ] = MNull;
    }
    if( rowLF64[ 0 ] )
    {
        MMemFree(hMemMgr, rowLF64[ 0 ]);
        rowLF64[ 0 ] = MNull;
    }
    return res;
}

/**********************************************************************************************************/
//只做低频部分的小波逆变换
MVoid idwt_LL_reconstruct(WAVELET_LPDATA waveletBuf, MInt32 *currRowBuf, MInt32 *nextRowBuf, MInt32 *resData,
                          MInt32 *low_filter, MInt32 sh, MInt32 sw, MInt32 level, MInt32 startRow, MInt32 endRow, LPASVLOFFSCREEN resImg)
{
    MInt32 *rowBufs[LL_DB_FILTER_SIZE];
    MInt32 convRes, halfWin = LL_DB_FILTER_HALF_SIZE >> 1;
    MInt32 x, y, m, n, k, curr_y, next_y, w = sw << 1;
    MInt32 *waveDecomData = MNull, *rowFilter = MNull, *colFilter = MNull;
    MByte *resPtr = MNull;
    MInt32 resLinePitch = 0;
    if( resImg != MNull )
    {
        resPtr = ( MByte * ) resImg->ppu8Plane[ 0 ];
        resLinePitch = resImg->pi32Pitch[ 0 ];
    }
    rowFilter = low_filter;
    colFilter = low_filter;
    waveDecomData = waveletBuf[ level ].LL_data;

#if defined(USE_NEON)
    int32x4_t vlo_r0 = vdupq_n_s32(low_filter[0]);
    int32x4_t vlo_r1 = vdupq_n_s32(low_filter[1]);
    int32x4_t vlo_r2 = vdupq_n_s32(low_filter[2]);
    int32x4_t vlo_r3 = vdupq_n_s32(low_filter[3]);
#endif

    for(y = startRow; y < endRow; ++y)
    {
        curr_y = y << 1;
        next_y = curr_y + 1;
        MInt32 *currBuf = resData + curr_y * w;
        MInt32 *nextBuf = resData + next_y * w;
        MByte *currResImg = resPtr + curr_y * resLinePitch;
        MByte *nextResImg = resPtr + next_y * resLinePitch;

        for(m = y - halfWin, n = 0; m <= ( y + halfWin ); ++m, ++n)
        {
            MInt32 val = m;
            if( val < 0 )
                val = -val;
            else if( val >= sh )
                val = 2 * sh - val - 2;

            rowBufs[ n ] = waveDecomData + val * sw;
        }
        MInt32 *currRowRecon = currRowBuf + halfWin;
        MInt32 *nextRowRecon = nextRowBuf + halfWin;
        MInt32 *buf0 = rowBufs[ 0 ], *buf1 = rowBufs[ 1 ], *buf2 = rowBufs[ 2 ];
        //column direction convolution
        x = 0;
#if defined(USE_NEON)
        {
            int32x4_t tmpdata00_32x4, tmpdata01_32x4, tmpdata02_32x4;
            int32x4_t currSum, nextSum;
            int32x4_t currSum_shift, nextSum_shift;

            for (x = 0; x < sw - 3; x += 4)
            {
                tmpdata00_32x4 = vld1q_s32(buf0 + x);
                tmpdata01_32x4 = vld1q_s32(buf1 + x);
                tmpdata02_32x4 = vld1q_s32(buf2 + x);

                currSum = vmulq_s32(tmpdata00_32x4, vlo_r0);
                nextSum = vmulq_s32(tmpdata01_32x4, vlo_r1);
                currSum = vmlaq_s32(currSum, tmpdata01_32x4, vlo_r2);
                nextSum = vmlaq_s32(nextSum, tmpdata02_32x4, vlo_r3);

                currSum_shift = vshrq_n_s32(currSum, LL_SHIFT_SIZE);
                nextSum_shift = vshrq_n_s32(nextSum, LL_SHIFT_SIZE);

                vst1q_s32(currRowRecon + x, currSum_shift);
                vst1q_s32(nextRowRecon + x, nextSum_shift);
            }
        }
#endif
        for(; x < sw; ++x)
        {
            //even row column convolution
            currRowRecon[ x ] = ( buf0[ x ] * colFilter[ 0 ] + buf1[ x ] * colFilter[ 2 ] ) >> (LL_SHIFT_SIZE);
            //odd row column convolution
            nextRowRecon[ x ] = ( buf1[ x ] * colFilter[ 1 ] + buf2[ x ] * colFilter[ 3 ] ) >> (LL_SHIFT_SIZE);
        }
        //copy border
        for(m = -halfWin; m < 0; ++m)
        {
            currRowRecon[ m ] = currRowRecon[ -m ];
            nextRowRecon[ m ] = nextRowRecon[ -m ];
        }
        for(m = sw; m < ( sw + halfWin ); ++m)
        {
            currRowRecon[ m ] = currRowRecon[ 2 * sw - m - 2 ];
            nextRowRecon[ m ] = nextRowRecon[ 2 * sw - m - 2 ];
        }
        currRowRecon -= halfWin;
        nextRowRecon -= halfWin;
        //row direction convolution
        if( level == 0 )
        {
            if( resImg == MNull )
            {
                x = 0;
#if defined(USE_NEON)
                {
                    int32x4x2_t tmpdata_curr, tmpdata_next;
                    int32x4_t sum;
                    int32x4_t currdata00, currdata01, currdata02;
                    int32x4_t nextdata00, nextdata01, nextdata02;

                    for (x = 0; x < sw - 3; x += 4)
                    {
                        currdata00 = vld1q_s32(currRowRecon + x);
                        currdata01 = vld1q_s32(currRowRecon + x + 1);
                        currdata02 = vld1q_s32(currRowRecon + x + 2);

                        nextdata00 = vld1q_s32(nextRowRecon + x);
                        nextdata01 = vld1q_s32(nextRowRecon + x + 1);
                        nextdata02 = vld1q_s32(nextRowRecon + x + 2);

                        sum = vmulq_s32(currdata00, vlo_r0);
                        sum = vmlaq_s32(sum, currdata01, vlo_r2);
                        tmpdata_curr.val[0] = vshrq_n_s32(sum, LL_DOUBLE_SHIFT_SIZE);

                        sum = vmulq_s32(currdata01, vlo_r1);
                        sum = vmlaq_s32(sum, currdata02, vlo_r3);
                        tmpdata_curr.val[1] = vshrq_n_s32(sum, LL_DOUBLE_SHIFT_SIZE);

                        sum = vmulq_s32(nextdata00, vlo_r0);
                        sum = vmlaq_s32(sum, nextdata01, vlo_r2);
                        tmpdata_next.val[0] = vshrq_n_s32(sum, LL_DOUBLE_SHIFT_SIZE);

                        sum = vmulq_s32(nextdata01, vlo_r1);
                        sum = vmlaq_s32(sum, nextdata02, vlo_r3);
                        tmpdata_next.val[1] = vshrq_n_s32(sum, LL_DOUBLE_SHIFT_SIZE);

                        vst2q_s32(currBuf + 2 * x, tmpdata_curr);
                        vst2q_s32(nextBuf + 2 * x, tmpdata_next);
                    }

                }
#endif
                for(; x < sw; ++x)
                {
                    convRes = currRowRecon[ x ] * rowFilter[ 0 ] + currRowRecon[ x + 1 ] * rowFilter[ 2 ];
                    currBuf[ 2 * x ] = ( convRes >> (LL_DOUBLE_SHIFT_SIZE));

                    convRes = currRowRecon[ x + 1 ] * rowFilter[ 1 ] + currRowRecon[ x + 2 ] * rowFilter[ 3 ];
                    currBuf[ 2 * x + 1 ] = ( convRes >> (LL_DOUBLE_SHIFT_SIZE));

                    convRes = nextRowRecon[ x ] * rowFilter[ 0 ] + nextRowRecon[ x + 1 ] * rowFilter[ 2 ];
                    nextBuf[ 2 * x ] = ( convRes >> (LL_DOUBLE_SHIFT_SIZE));

                    convRes = nextRowRecon[ x + 1 ] * rowFilter[ 1 ] + nextRowRecon[ x + 2 ] * rowFilter[ 3 ];
                    nextBuf[ 2 * x + 1 ] = ( convRes >> (LL_DOUBLE_SHIFT_SIZE));
                }
            }
            else
            {
                MInt32 tmpVal;
                for(x = 0; x < sw; ++x)
                {
                    convRes = currRowRecon[ x ] * rowFilter[ 0 ] + currRowRecon[ x + 1 ] * rowFilter[ 2 ];
                    tmpVal = ( MInt32 ) currResImg[ 2 * x ] - ( convRes >> (LL_DOUBLE_SHIFT_SIZE));
                    currResImg[ 2 * x ] = TRIMBYTE(tmpVal);

                    convRes = currRowRecon[ x + 1 ] * rowFilter[ 1 ] + currRowRecon[ x + 2 ] * rowFilter[ 3 ];
                    tmpVal = ( MInt32 ) currResImg[ 2 * x + 1 ] - ( convRes >> (LL_DOUBLE_SHIFT_SIZE));
                    currResImg[ 2 * x + 1 ] = TRIMBYTE(tmpVal);

                    convRes = nextRowRecon[ x ] * rowFilter[ 0 ] + nextRowRecon[ x + 1 ] * rowFilter[ 2 ];
                    tmpVal = ( MInt32 ) nextResImg[ 2 * x ] - ( convRes >> (LL_DOUBLE_SHIFT_SIZE));
                    nextResImg[ 2 * x ] = TRIMBYTE(tmpVal);

                    convRes = nextRowRecon[ x + 1 ] * rowFilter[ 1 ] + nextRowRecon[ x + 2 ] * rowFilter[ 3 ];
                    tmpVal = ( MInt32 ) nextResImg[ 2 * x + 1 ] - ( convRes >> (LL_DOUBLE_SHIFT_SIZE));
                    nextResImg[ 2 * x + 1 ] = TRIMBYTE(tmpVal);
                }
            }
        }
        else
        {
            x = 0;
#if defined(USE_NEON)
            {
                int32x4x2_t tmpdata_curr, tmpdata_next;
                int32x4_t sum;
                int32x4_t currdata00, currdata01, currdata02;
                int32x4_t nextdata00, nextdata01, nextdata02;

                for (x = 0; x < sw - 3; x += 4)
                {
                    currdata00 = vld1q_s32(currRowRecon + x);
                    currdata01 = vld1q_s32(currRowRecon + x + 1);
                    currdata02 = vld1q_s32(currRowRecon + x + 2);

                    nextdata00 = vld1q_s32(nextRowRecon + x);
                    nextdata01 = vld1q_s32(nextRowRecon + x + 1);
                    nextdata02 = vld1q_s32(nextRowRecon + x + 2);

                    sum = vmulq_s32(currdata00, vlo_r0);
                    sum = vmlaq_s32(sum, currdata01, vlo_r2);
                    tmpdata_curr.val[0] = vshrq_n_s32(sum, LL_SHIFT_SIZE);

                    sum = vmulq_s32(currdata01, vlo_r1);
                    sum = vmlaq_s32(sum, currdata02, vlo_r3);
                    tmpdata_curr.val[1] = vshrq_n_s32(sum, LL_SHIFT_SIZE);

                    sum = vmulq_s32(nextdata00, vlo_r0);
                    sum = vmlaq_s32(sum, nextdata01, vlo_r2);
                    tmpdata_next.val[0] = vshrq_n_s32(sum, LL_SHIFT_SIZE);

                    sum = vmulq_s32(nextdata01, vlo_r1);
                    sum = vmlaq_s32(sum, nextdata02, vlo_r3);
                    tmpdata_next.val[1] = vshrq_n_s32(sum, LL_SHIFT_SIZE);

                    vst2q_s32(currBuf + 2 * x, tmpdata_curr);
                    vst2q_s32(nextBuf + 2 * x, tmpdata_next);
                }

            }
#endif
            for(; x < sw; ++x)
            {
                convRes = currRowRecon[ x ] * rowFilter[ 0 ] + currRowRecon[ x + 1 ] * rowFilter[ 2 ];
                currBuf[ 2 * x ] = ( convRes >> (LL_SHIFT_SIZE));

                convRes = currRowRecon[ x + 1 ] * rowFilter[ 1 ] + currRowRecon[ x + 2 ] * rowFilter[ 3 ];
                currBuf[ 2 * x + 1 ] = ( convRes >> (LL_SHIFT_SIZE));

                convRes = nextRowRecon[ x ] * rowFilter[ 0 ] + nextRowRecon[ x + 1 ] * rowFilter[ 2 ];
                nextBuf[ 2 * x ] = ( convRes >> (LL_SHIFT_SIZE));

                convRes = nextRowRecon[ x + 1 ] * rowFilter[ 1 ] + nextRowRecon[ x + 2 ] * rowFilter[ 3 ];
                nextBuf[ 2 * x + 1 ] = ( convRes >> (LL_SHIFT_SIZE));
            }
        }
    }
}

#if defined(MCV_MULTI_THREAD)

MVoid thread_wl_LL_reconstruct(MVoid *pParam)
{
    Wl_LL_Recon *wl_r = ( Wl_LL_Recon * ) pParam;
    MInt32 task_ID = wl_r->task_ID;
    MInt32 lret = MOK;

    idwt_LL_reconstruct(wl_r->waveletBuf, wl_r->currRowBuf, wl_r->nextRowBuf, wl_r->resData, wl_r->low_filter,
                        wl_r->sh, wl_r->sw, wl_r->level, wl_r->startRow, wl_r->endRow, wl_r->resImg);

    wl_r->errCode = lret;

}

#endif

//just do low-frequency inverse discrete wavelet transform
MLong idwt_LLNS(MHandle hMemMgr, MHandle mcvParallelMonitor, WAVELET_LPDATA waveletBuf, MInt32 lHeight, MInt32 lWidth, MInt32 levels, LPASVLOFFSCREEN resImg)
{
    MLong res = MOK;
    MInt32 x, y, i, j, l, k;
    MInt32 sw, sh, w, h, numPxls, lSize;
    const MInt32 lTasksNum = DENOISE_TASK_NUM;
    MInt32 *currRowBuf[lTasksNum] = {MNull}, *nextRowBuf[lTasksNum] = {MNull};
    MInt32 Low_R[LL_DB_FILTER_SIZE];
    MFloat ratioTotal = 0, total;
    MInt32 *reconLLBuf = MNull, *decomLLBuf = MNull;

    //enlarge filter 1024 times
    for(i = 0; i < LL_DB_FILTER_SIZE; ++i)
    {
        Low_R[ i ] = ( MInt32 ) (( db_recon_l[ i ] * 1024 ));
        ratioTotal += db_recon_l[ i ];
    }
    total = ( ratioTotal * 1024 + 0.5f );
    Low_R[ 1 ] = total - Low_R[ 0 ] - Low_R[ 2 ] - Low_R[ 3 ];

    for(l = levels - 1; l >= 0; --l)
    {
        sw = waveletBuf[ l ].w;
        sh = waveletBuf[ l ].h;
        w = sw << 1;
        h = sh << 1;
        numPxls = w * h;
        currRowBuf[ 0 ] = ( MInt32 * ) MMemAlloc(hMemMgr, lTasksNum * ( sw + 2 * (LL_DB_FILTER_HALF_SIZE >> 1 )) * sizeof(MInt32));
        nextRowBuf[ 0 ] = ( MInt32 * ) MMemAlloc(hMemMgr, lTasksNum * ( sw + 2 * (LL_DB_FILTER_HALF_SIZE >> 1 )) * sizeof(MInt32));
        if( currRowBuf[ 0 ] == MNull || nextRowBuf[ 0 ] == MNull )
        {
            res = MERR_NO_MEMORY;
            goto exit;
        }
        for(j = 1; j < lTasksNum; ++j)
        {
            currRowBuf[ j ] = currRowBuf[ 0 ] + j * ( sw + 2 * (LL_DB_FILTER_HALF_SIZE >> 1 ));
            nextRowBuf[ j ] = nextRowBuf[ 0 ] + j * ( sw + 2 * (LL_DB_FILTER_HALF_SIZE >> 1 ));
        }

        if( l > 0 )
        {
            decomLLBuf = waveletBuf[ l - 1 ].LL_data;
            reconLLBuf = ( MInt32 * ) MMemAlloc(hMemMgr, numPxls * sizeof(MInt32));
            if( reconLLBuf == MNull )
            {
                res = MERR_NO_MEMORY;
                goto exit;
            }
        }

#ifdef MCV_MULTI_THREAD
        {
            MInt32 taskID[lTasksNum] = {0};
            Wl_LL_Recon pParams[lTasksNum] = {0};

            lSize = sh / lTasksNum;
            lSize = lSize >> 1 << 1;

            pParams[ 0 ].startRow = 0;
            pParams[ 0 ].endRow = lSize;
            for(i = 1; i < lTasksNum; i++)
            {
                pParams[ i ].startRow = i * lSize;
                pParams[ i ].endRow = ( i + 1 ) * lSize;
            }
            pParams[ i - 1 ].endRow = sh;

            for(i = 0; i < lTasksNum; i++)
            {
                pParams[ i ].task_ID = i;
                pParams[ i ].waveletBuf = waveletBuf;
                pParams[ i ].resImg = resImg;
                pParams[ i ].currRowBuf = currRowBuf[ i ];
                pParams[ i ].nextRowBuf = nextRowBuf[ i ];
                pParams[ i ].resData = reconLLBuf;
                pParams[ i ].low_filter = Low_R;
                pParams[ i ].sw = sw;
                pParams[ i ].sh = sh;
                pParams[ i ].level = l;
            }

            for(i = 0; i < lTasksNum; i++)
            {
                taskID[ i ] = mcvAddTask(mcvParallelMonitor, thread_wl_LL_reconstruct, ( MVoid * ) &pParams[ i ]);
            }
            for(i = 0; i < lTasksNum; i++)
            {
                mcvWaitTask(mcvParallelMonitor, taskID[ i ]);
            }
        }
#else
        //todo: 已修复完成
        idwt_LL_reconstruct(waveletBuf, currRowBuf[0], nextRowBuf[0], reconLLBuf, Low_R, sh, sw, l, 0, sh, resImg);
#endif

        if( l > 0 )
        {
            MMemCpy(decomLLBuf, reconLLBuf, numPxls * sizeof(MInt32));
            if( h < waveletBuf[ l - 1 ].h )
            {
                MInt32 *LL_RowBuf = waveletBuf[ l - 1 ].LL_data;
                MInt32 LL_width = waveletBuf[ l - 1 ].w;
                for(i = h; i < waveletBuf[ l - 1 ].h; i++)
                    MMemCpy(LL_RowBuf + i * LL_width, LL_RowBuf + ( h - 1 ) * LL_width, LL_width * sizeof(MInt32));
            }
            if( w < waveletBuf[ l - 1 ].w )
            {
                MInt32 LL_width = waveletBuf[ l - 1 ].w;
                for(i = 0; i < waveletBuf[ l - 1 ].h; i++)
                {
                    MInt32 *LL_RowBuf = waveletBuf[ l - 1 ].LL_data + i * LL_width;
                    for(j = w; j < LL_width; j++)
                    {
                        LL_RowBuf[ j ] = LL_RowBuf[ w - 1 ];
                    }
                }
            }
        }

        if( reconLLBuf )
        {
            MMemFree(hMemMgr, reconLLBuf);
            reconLLBuf = MNull;
        }
        if( currRowBuf[ 0 ] )
        {
            MMemFree(hMemMgr, currRowBuf[ 0 ]);
            currRowBuf[ 0 ] = MNull;
        }
        if( nextRowBuf[ 0 ] )
        {
            MMemFree(hMemMgr, nextRowBuf[ 0 ]);
            nextRowBuf[ 0 ] = MNull;
        }
    }

    exit:
    if( reconLLBuf )
    {
        MMemFree(hMemMgr, reconLLBuf);
        reconLLBuf = MNull;
    }
    if( currRowBuf[ 0 ] )
    {
        MMemFree(hMemMgr, currRowBuf[ 0 ]);
        currRowBuf[ 0 ] = MNull;
    }
    if( nextRowBuf[ 0 ] )
    {
        MMemFree(hMemMgr, nextRowBuf[ 0 ]);
        nextRowBuf[ 0 ] = MNull;
    }
    return res;
}

/***********************************************************************************************************/
MVoid rough_dwt_LL_decomposition(WAVELET_LPDATA_8U waveletBuf, MByte *srcBuf, MByte *rowLF,
                                 MInt32 *low_filter, MInt32 w, MInt32 h, MInt32 sw, MInt32 lineSteps, MInt32 level, MInt32 startRow, MInt32 endRow)
{
    MInt32 x, y, m, n, i, j;
    MInt32 convRes;
    MByte *rowBuf[LL_DB_FILTER_SIZE];
    MByte *llData = waveletBuf[ level ].LL_data;
#if defined(USE_NEON)
    int16x4_t vlo_d0 = vdup_n_s16(low_filter[0]);
    int16x4_t vlo_d1 = vdup_n_s16(low_filter[1]);
    int16x4_t vlo_d2 = vdup_n_s16(low_filter[2]);
    int16x4_t vlo_d3 = vdup_n_s16(low_filter[3]);

    int16x4_t const_255 = vdup_n_s16(255);
    int16x4_t const_0 = vdup_n_s16(0);
#endif
    for(y = startRow, m = 2 * startRow + 1; y < endRow; ++y, m += 2)
    {
        MByte *llBuf = llData + y * sw;
        for(n = m - LL_DB_FILTER_HALF_SIZE, j = 0; n < ( m + LL_DB_FILTER_HALF_SIZE); ++n, ++j)
        {
            MInt32 val = n;
            if( val < 0 )
                val = -val;
            else if( val >= h )
                val = 2 * h - val - 2;
            rowBuf[ j ] = srcBuf + val * lineSteps;
        }
        MByte *tmpLF = rowLF + LL_DB_FILTER_HALF_SIZE;
        MByte *buf0 = rowBuf[ 0 ], *buf1 = rowBuf[ 1 ], *buf2 = rowBuf[ 2 ], *buf3 = rowBuf[ 3 ];
        //column direction convolution
        x = 0;
#if defined(USE_NEON)
        {
            uint8x8_t tmpdata_8x8;
            int16x8_t tmpdata;
            int16x4_t Sdata00, Sdata01;
            int32x4_t sum00, sum01;
            int16x4_t sum_shift00, sum_shift01, tmpsum;
            int16x8_t sum_shift;
            for (x = 0; x < w - 7; x += 8)
            {
                tmpdata_8x8 = vld1_u8(buf0 + x);
                tmpdata = vreinterpretq_s16_u16(vmovl_u8(tmpdata_8x8));
                Sdata00 = vget_low_s16(tmpdata);
                Sdata01 = vget_high_s16(tmpdata);
                sum00 = vmull_s16(Sdata00, vlo_d0);
                sum01 = vmull_s16(Sdata01, vlo_d0);

                tmpdata_8x8 = vld1_u8(buf1 + x);
                tmpdata = vreinterpretq_s16_u16(vmovl_u8(tmpdata_8x8));
                Sdata00 = vget_low_s16(tmpdata);
                Sdata01 = vget_high_s16(tmpdata);
                sum00 = vmlal_s16(sum00, Sdata00, vlo_d1);
                sum01 = vmlal_s16(sum01, Sdata01, vlo_d1);

                tmpdata_8x8 = vld1_u8(buf2 + x);
                tmpdata = vreinterpretq_s16_u16(vmovl_u8(tmpdata_8x8));
                Sdata00 = vget_low_s16(tmpdata);
                Sdata01 = vget_high_s16(tmpdata);
                sum00 = vmlal_s16(sum00, Sdata00, vlo_d2);
                sum01 = vmlal_s16(sum01, Sdata01, vlo_d2);

                tmpdata_8x8 = vld1_u8(buf3 + x);
                tmpdata = vreinterpretq_s16_u16(vmovl_u8(tmpdata_8x8));
                Sdata00 = vget_low_s16(tmpdata);
                Sdata01 = vget_high_s16(tmpdata);
                sum00 = vmlal_s16(sum00, Sdata00, vlo_d3);
                sum01 = vmlal_s16(sum01, Sdata01, vlo_d3);

                sum_shift00 = vqshrn_n_s32(sum00, LL_SHIFT_SIZE);
                sum_shift01 = vqshrn_n_s32(sum01, LL_SHIFT_SIZE);

                tmpsum = vmin_s16(sum_shift00, const_255);
                sum_shift00 = vmax_s16(tmpsum, const_0);
                tmpsum = vmin_s16(sum_shift01, const_255);
                sum_shift01 = vmax_s16(tmpsum, const_0);

                sum_shift = vcombine_s16(sum_shift00, sum_shift01);
                vst1_u8(tmpLF + x, vqmovun_s16(sum_shift));
            }
        }
#endif
        for(; x < w; ++x)
        {
            convRes = ( low_filter[ 0 ] * buf0[ x ] + low_filter[ 1 ] * buf1[ x ] + low_filter[ 2 ] * buf2[ x ]
                        + low_filter[ 3 ] * buf3[ x ] ) >> LL_SHIFT_SIZE;
            tmpLF[ x ] = TRIMBYTE(convRes);
        }
        //copy border
        for(n = -LL_DB_FILTER_HALF_SIZE; n < 0; ++n)
        {
            tmpLF[ n ] = tmpLF[ -n ];
        }
        for(n = w; n < ( w + LL_DB_FILTER_HALF_SIZE); ++n)
        {
            tmpLF[ n ] = tmpLF[ 2 * w - n - 2 ];
        }
        tmpLF -= LL_DB_FILTER_HALF_SIZE;
        //row direction convolution
        x = 0;
        j = 1;
#if defined(USE_NEON)
        {
            uint8x8x2_t tmpdata00, tmpdata01;
            int8x8_t evendata_8x8, odddata_8x8;
            int16x8_t evendata_16x8, odddata_16x8;
            int16x4_t lowdata_16x4, highdata_16x4;
            int32x4_t lowsum, highsum;
            int16x4_t lowsum_shift, highsum_shift, tmpsum_shift;
            int16x8_t sum_shift;

            for (x = 0, j = 1; x < sw - 7; x += 8, j += 16)
            {
                tmpdata00 = vld2_u8(tmpLF + j);
                tmpdata01 = vld2_u8(tmpLF + j + 2);

                evendata_8x8 = tmpdata00.val[0];
                odddata_8x8 = tmpdata00.val[1];
                evendata_16x8 = vreinterpretq_s16_u16(vmovl_u8(evendata_8x8));
                odddata_16x8 = vreinterpretq_s16_u16(vmovl_u8(odddata_8x8));

                lowdata_16x4 = vget_low_s16(evendata_16x8);
                highdata_16x4 = vget_high_s16(evendata_16x8);

                lowsum = vmull_s16(lowdata_16x4, vlo_d0);
                highsum = vmull_s16(highdata_16x4, vlo_d0);

                lowdata_16x4 = vget_low_s16(odddata_16x8);
                highdata_16x4 = vget_high_s16(odddata_16x8);

                lowsum = vmlal_s16(lowsum, lowdata_16x4, vlo_d1);
                highsum = vmlal_s16(highsum, highdata_16x4, vlo_d1);

                evendata_8x8 = tmpdata01.val[0];
                odddata_8x8 = tmpdata01.val[1];
                evendata_16x8 = vreinterpretq_s16_u16(vmovl_u8(evendata_8x8));
                odddata_16x8 = vreinterpretq_s16_u16(vmovl_u8(odddata_8x8));

                lowdata_16x4 = vget_low_s16(evendata_16x8);
                highdata_16x4 = vget_high_s16(evendata_16x8);

                lowsum = vmlal_s16(lowsum, lowdata_16x4, vlo_d2);
                highsum = vmlal_s16(highsum, highdata_16x4, vlo_d2);

                lowdata_16x4 = vget_low_s16(odddata_16x8);
                highdata_16x4 = vget_high_s16(odddata_16x8);

                lowsum = vmlal_s16(lowsum, lowdata_16x4, vlo_d3);
                highsum = vmlal_s16(highsum, highdata_16x4, vlo_d3);

                lowsum_shift = vqshrn_n_s32(lowsum, LL_SHIFT_SIZE);
                highsum_shift = vqshrn_n_s32(highsum, LL_SHIFT_SIZE);

                tmpsum_shift = vmin_s16(lowsum_shift, const_255);
                lowsum_shift = vmax_s16(tmpsum_shift, const_0);
                tmpsum_shift = vmin_s16(highsum_shift, const_255);
                highsum_shift = vmax_s16(tmpsum_shift, const_0);

                sum_shift = vcombine_s16(lowsum_shift, highsum_shift);
                vst1_u8(llBuf + x, vqmovun_s16(sum_shift));
            }
        }
#endif
        for(; x < sw; ++x, j += 2)
        {
            convRes = ( low_filter[ 0 ] * tmpLF[ j ] + low_filter[ 1 ] * tmpLF[ j + 1 ] + low_filter[ 2 ] * tmpLF[ j + 2 ]
                        + low_filter[ 3 ] * tmpLF[ j + 3 ] ) >> LL_SHIFT_SIZE;
            llBuf[ x ] = TRIMBYTE(convRes);
        }
    }
}

#if defined(MCV_MULTI_THREAD)

MVoid thread_rough_wl_LL_decomposition(MVoid *pParam)
{
    Wl_LL_Decom_8U *wl_d = ( Wl_LL_Decom_8U * ) pParam;
    MInt32 task_ID = wl_d->task_ID;
    MInt32 lret = MOK;

    rough_dwt_LL_decomposition(wl_d->waveletBuf, wl_d->srcBuf, wl_d->rowLF,
                               wl_d->low_filter, wl_d->w, wl_d->h, wl_d->sw, wl_d->lineSteps, wl_d->level, wl_d->startRow, wl_d->endRow);

    wl_d->errCode = lret;
}

#endif

//just do LL frequency discrete wavelet transform
MLong rough_dwt_LLNS(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN _src, WAVELET_LPDATA_8U waveletBuf, MInt32 levels)
{
    MLong res = MOK;
    MInt32 m, n, i, j, x, y, l, lWidth = _src->i32Width, lHeight = _src->i32Height;
    MInt32 w = lWidth, h = lHeight, lSize;
    MFloat ratioTotal = 0, scale;
    MInt32 Low_D[LL_DB_FILTER_SIZE];
    MByte *rowLF[DENOISE_TASK_NUM] = {MNull};
    MByte *_srcData = ( MByte * ) _src->ppu8Plane[ 0 ];
    MInt32 pitch = _src->pi32Pitch[ 0 ];
    MByte *lfBuf = _srcData;

    //normalize and enlarge filter to 1024
    for(i = 0; i < LL_DB_FILTER_SIZE; ++i)
        ratioTotal += db_decom_l[ i ];
    scale = 1024 / ratioTotal;

    for(i = 0; i < LL_DB_FILTER_SIZE; ++i)
    {
        Low_D[ i ] = db_decom_l[ i ] * scale + 0.5f;
    }
    Low_D[ 2 ] = 1024 - Low_D[ 0 ] - Low_D[ 1 ] - Low_D[ 3 ];

    for(l = 0; l < levels; ++l)
    {
        MInt32 lTask_Num = DENOISE_TASK_NUM;
        MInt32 sw = waveletBuf[ l ].w, sh = waveletBuf[ l ].h, lineSteps;
        if( l == 0 )
            lineSteps = pitch;
        else
            lineSteps = w;
        rowLF[ 0 ] = ( MByte * ) MMemAlloc(hMemMgr, DENOISE_TASK_NUM * ( lineSteps + LL_DB_FILTER_SIZE) * sizeof(MByte));
        if( rowLF[ 0 ] == MNull )
        {
            res = MERR_NO_MEMORY;
            goto exit;
        }

        for(j = 1; j < DENOISE_TASK_NUM; ++j)
        {
            rowLF[ j ] = rowLF[ 0 ] + j * ( lineSteps + LL_DB_FILTER_SIZE);
        }
#ifdef MCV_MULTI_THREAD
        {
            MInt32 taskID[DENOISE_TASK_NUM] = {0};
            Wl_LL_Decom_8U pParams[DENOISE_TASK_NUM] = {0};

            lSize = sh / lTask_Num;
            lSize = lSize >> 1 << 1;

            pParams[ 0 ].startRow = 0;
            pParams[ 0 ].endRow = lSize;
            for(i = 1; i < lTask_Num; i++)
            {
                pParams[ i ].startRow = i * lSize;
                pParams[ i ].endRow = ( i + 1 ) * lSize;
            }
            pParams[ i - 1 ].endRow = sh;

            for(i = 0; i < lTask_Num; i++)
            {
                pParams[ i ].task_ID = i;
                pParams[ i ].waveletBuf = waveletBuf;
                pParams[ i ].srcBuf = lfBuf;
                pParams[ i ].rowLF = rowLF[ i ];
                pParams[ i ].low_filter = Low_D;
                pParams[ i ].w = w;
                pParams[ i ].h = h;
                pParams[ i ].sw = sw;
                pParams[ i ].lineSteps = lineSteps;
                pParams[ i ].level = l;
            }

            for(i = 0; i < lTask_Num; i++)
            {
                taskID[ i ] = mcvAddTask(mcvParallelMonitor, thread_rough_wl_LL_decomposition, ( MVoid * ) &pParams[ i ]);
            }
            for(i = 0; i < lTask_Num; i++)
            {
                mcvWaitTask(mcvParallelMonitor, taskID[ i ]);
            }
        }
#else
        rough_dwt_LL_decomposition(waveletBuf, lfBuf, rowLF[0], Low_D, w, h, sw, lineSteps, l, 0, sh);
#endif
        if( rowLF[ 0 ] )
        {
            MMemFree(hMemMgr, rowLF[ 0 ]);
            rowLF[ 0 ] = MNull;
        }
        lfBuf = waveletBuf[ l ].LL_data;
        w = w >> 1;
        h = h >> 1;
    }

    exit:
    return res;
}

#endif

NS_SINFLE_IMAGE_ENHANCEMENT_END