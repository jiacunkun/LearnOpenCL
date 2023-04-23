#include "ammem.h"
#include <math.h>
#include "merror.h"
#include "img_interpolation.h"
#include "mobilecv.h"
#include "single_image_enhancement_define.h"

#define __ARM_NEON__

#if defined(USE_NEON) || defined(__ARM_NEON__)
#if defined(__ANDROID__)
#include "arm_neon.h"
#else
#include "NEON_2_SSE.h"
#endif
#endif

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
const int DENOISE_TASK_NUM = 16; //todo: 未初始化变量值，暂且初始化，后续再看 4.9 by jck

#ifdef MCV_MULTI_THREAD

typedef struct _tag_BILINEAR_INTRPOLATION_32I
{
    MInt32			task_ID;
    MRESULT			errCode;
    MInt32* srcBuf;
    MInt32* dstBuf;
    MInt32          srows;
    MInt32          scols;
    MInt32          drows;
    MInt32          dcols;
    MInt32          downScale;
    MInt32          upScale;
    MInt32          startRow;
    MInt32          endRow;
}Bilinear_Interpolation_32I;
#endif

#ifdef MCV_MULTI_THREAD
typedef struct _tag_BILINEAR_INTRPOLATION_8U
{
    MInt32			task_ID;
    MRESULT			errCode;
    MByte* srcBuf;
    MByte* dstBuf;
    MInt32          srcH;
    MInt32          srcW;
    MInt32          dstH;
    MInt32          dstW;
    MInt32          srcPitch;
    MInt32          dstPitch;
    MInt32          upScale;
    MInt32          downScale;
    MInt32* expand_size;
    MInt32          startRow;
    MInt32          endRow;
}Bilinear_Interpolation_8U;
#endif

static const MInt32 log_2[10] = { 0, 0, 1, 1, 2, 2, 2, 2, 3, 3 };

MVoid fast_bilinear_8U(MByte* srcBuf, MInt32 srows, MInt32 scols, MInt32 srcPitch, MByte* dstBuf, MInt32 drows, MInt32 dcols,
    MInt32 dstPitch, MInt32 upScale, MInt32 downScale, MInt32 startRow, MInt32 endRow)
{
    MInt32 row, col, i, j, index;
    MByte  currVal, nextVal;
    MShort tmpVal = 0;
    MInt32 subRow = 0, scale = 0;
    MByte* currSrcBuf = MNull, * nextSrcBuf = MNull;
    MByte* row1Buf = MNull, * row2Buf = MNull, * row3Buf = MNull, * row4Buf = MNull, * row5Buf = MNull;
    if (downScale != 0)
    {
        //下采样
        for (row = startRow; row < endRow; ++row)
        {
            i = row << downScale;
            if (i >= srows)
                i = srows - 1;
            for (col = 0; col < dcols; ++col)
            {
                j = col << downScale;
                if (j >= scols)
                    j = scols - 1;
                dstBuf[row * dstPitch + col] = srcBuf[i * srcPitch + j];
            }
        }
    }
    else if (upScale == 1)
    {
        //上采样放大一倍
        scale = 1 << upScale;
        for (row = startRow; row < endRow; row += scale)
        {
            subRow = row >> upScale;
            currSrcBuf = srcBuf + subRow * srcPitch;
            nextSrcBuf = (subRow + 1) >= srows ? currSrcBuf - srcPitch : currSrcBuf + srcPitch;

            row1Buf = dstBuf + row * dstPitch;
            row2Buf = (row + 1) >= drows ? dstBuf + (drows - 1) * dstPitch : row1Buf + dstPitch;
            row3Buf = (row + 2) >= drows ? dstBuf + (drows - 1) * dstPitch : row2Buf + dstPitch;

            if (row == startRow)
            {
                for (col = 0; col < scols; ++col)
                {
                    currVal = currSrcBuf[col];
                    if (col + 1 >= scols)
                        nextVal = currSrcBuf[scols - 2];
                    else
                        nextVal = currSrcBuf[col + 1];

                    index = col << upScale;
                    row1Buf[index] = currVal;
                    row1Buf[(index + 1) >= dcols ? dcols - 1 : index + 1] = (currVal + nextVal) >> 1;
                }
                index = (scols - 1) << upScale;
                if ((index + 1) < dcols)
                {
                    for (i = index + 2; i < dcols; ++i)
                        row1Buf[i] = row1Buf[index + 1];
                }
            }

            for (col = 0; col < scols; ++col)
            {
                currVal = nextSrcBuf[col];
                if (col + 1 >= scols)
                    nextVal = nextSrcBuf[scols - 2];
                else
                    nextVal = nextSrcBuf[col + 1];

                index = col << upScale;
                row3Buf[index] = currVal;
                row3Buf[(index + 1) >= dcols ? dcols - 1 : index + 1] = (currVal + nextVal + 1) >> 1;
            }
            index = (scols - 1) << upScale;
            if ((index + 1) < dcols)
            {
                for (i = index + 2; i < dcols; ++i)
                    row3Buf[i] = row3Buf[index + 1];
            }

            for (col = 0; col < dcols; ++col)
            {
                row2Buf[col] = (row1Buf[col] + row3Buf[col] + 1) >> 1;
            }
        }
    }
    else if (upScale == 2)
    {
        //上采样放大两倍
        scale = 1 << upScale;
#if CV_NEON
        uint8x8_t currdata_8x8, nextdata_8x8, rowdata00_8x8, rowdata01_8x8, rowdata02_8x8;
        uint8x8x4_t resdata_8x8x4;
        uint16x8_t tmpdata_16x8, currdata00_16x8, nextdata00_16x8;
        uint16x8_t const_16x8;
        const_16x8 = vdupq_n_u16(3);
#endif
        for (row = startRow; row < endRow; row += scale)
        {
            subRow = row >> upScale;
            currSrcBuf = srcBuf + subRow * srcPitch;
            nextSrcBuf = (subRow + 1) >= srows ? currSrcBuf - srcPitch : currSrcBuf + srcPitch;

            row1Buf = dstBuf + row * dstPitch;
            row2Buf = (row + 1) >= drows ? dstBuf + (drows - 1) * dstPitch : row1Buf + dstPitch;
            row3Buf = (row + 2) >= drows ? dstBuf + (drows - 1) * dstPitch : row2Buf + dstPitch;
            row4Buf = (row + 3) >= drows ? dstBuf + (drows - 1) * dstPitch : row3Buf + dstPitch;
            row5Buf = (row + 4) >= drows ? dstBuf + (drows - 1) * dstPitch : row4Buf + dstPitch;

            if (row == startRow)
            {
                col = 0;
#if CV_NEON
                for (col = 0; col < scols - 8; col += 8)
                {
                    resdata_8x8x4.val[0] = vld1_u8(currSrcBuf + col);
                    nextdata_8x8 = vld1_u8(currSrcBuf + col + 1);

                    currdata00_16x8 = vmovl_u8(resdata_8x8x4.val[0]);
                    nextdata00_16x8 = vmovl_u8(nextdata_8x8);
                    tmpdata_16x8 = vmlaq_u16(nextdata00_16x8, currdata00_16x8, const_16x8);
                    resdata_8x8x4.val[1] = vrshrn_n_u16(tmpdata_16x8, 2);
                    resdata_8x8x4.val[2] = vrhadd_u8(resdata_8x8x4.val[0], nextdata_8x8);
                    tmpdata_16x8 = vmlaq_u16(currdata00_16x8, nextdata00_16x8, const_16x8);
                    resdata_8x8x4.val[3] = vrshrn_n_u16(tmpdata_16x8, 2);
                    vst4_u8(row1Buf + (col << upScale), resdata_8x8x4);
                }
#endif
                for (; col < scols; ++col)
                {
                    currVal = currSrcBuf[col];
                    if (col + 1 >= scols)
                        nextVal = currSrcBuf[scols - 2];
                    else
                        nextVal = currSrcBuf[col + 1];

                    index = col << upScale;
                    row1Buf[index] = currVal;
                    row1Buf[(index + 1) >= dcols ? dcols - 1 : index + 1] = (3 * currVal + nextVal + 2) >> 2;
                    row1Buf[(index + 2) >= dcols ? dcols - 1 : index + 2] = (currVal + nextVal + 1) >> 1;
                    row1Buf[(index + 3) >= dcols ? dcols - 1 : index + 3] = (currVal + 3 * nextVal + 2) >> 2;
                }
                index = (scols - 1) << upScale;
                if ((index + 3) < dcols)
                {
                    for (i = index + 4; i < dcols; ++i)
                        row1Buf[i] = row1Buf[index + 3];
                }
            }

            col = 0;
#if CV_NEON
            for (col = 0; col < scols - 8; col += 8)
            {
                resdata_8x8x4.val[0] = vld1_u8(nextSrcBuf + col);
                nextdata_8x8 = vld1_u8(nextSrcBuf + col + 1);

                currdata00_16x8 = vmovl_u8(resdata_8x8x4.val[0]);
                nextdata00_16x8 = vmovl_u8(nextdata_8x8);
                tmpdata_16x8 = vmlaq_u16(nextdata00_16x8, currdata00_16x8, const_16x8);
                resdata_8x8x4.val[1] = vrshrn_n_u16(tmpdata_16x8, 2);
                resdata_8x8x4.val[2] = vrhadd_u8(resdata_8x8x4.val[0], nextdata_8x8);
                tmpdata_16x8 = vmlaq_u16(currdata00_16x8, nextdata00_16x8, const_16x8);
                resdata_8x8x4.val[3] = vrshrn_n_u16(tmpdata_16x8, 2);
                vst4_u8(row5Buf + (col << upScale), resdata_8x8x4);
            }
#endif
            for (; col < scols; ++col)
            {
                currVal = nextSrcBuf[col];
                if (col + 1 >= scols)
                    nextVal = nextSrcBuf[scols - 2];
                else
                    nextVal = nextSrcBuf[col + 1];

                index = col << upScale;
                row5Buf[index] = currVal;
                row5Buf[(index + 1) >= dcols ? dcols - 1 : index + 1] = (3 * currVal + nextVal + 2) >> 2;
                row5Buf[(index + 2) >= dcols ? dcols - 1 : index + 2] = (currVal + nextVal + 1) >> 1;
                row5Buf[(index + 3) >= dcols ? dcols - 1 : index + 3] = (currVal + 3 * nextVal + 2) >> 2;
            }
            index = (scols - 1) << upScale;
            if ((index + 3) < dcols)
            {
                for (i = index + 4; i < dcols; ++i)
                    row5Buf[i] = row5Buf[index + 3];
            }

            col = 0;
#if CV_NEON
            for (col = 0; col < dcols - 7; col += 8)
            {
                currdata_8x8 = vld1_u8(row1Buf + col);
                nextdata_8x8 = vld1_u8(row5Buf + col);

                currdata00_16x8 = vmovl_u8(currdata_8x8);
                nextdata00_16x8 = vmovl_u8(nextdata_8x8);
                tmpdata_16x8 = vmlaq_u16(nextdata00_16x8, currdata00_16x8, const_16x8);
                rowdata00_8x8 = vrshrn_n_u16(tmpdata_16x8, 2);
                rowdata01_8x8 = vrhadd_u8(currdata_8x8, nextdata_8x8);
                tmpdata_16x8 = vmlaq_u16(currdata00_16x8, nextdata00_16x8, const_16x8);
                rowdata02_8x8 = vrshrn_n_u16(tmpdata_16x8, 2);
                vst1_u8(row2Buf + col, rowdata00_8x8);
                vst1_u8(row3Buf + col, rowdata01_8x8);
                vst1_u8(row4Buf + col, rowdata02_8x8);
            }
#endif
            for (; col < dcols; ++col)
            {
                currVal = row1Buf[col];
                nextVal = row5Buf[col];

                row2Buf[col] = (3 * currVal + nextVal + 2) >> 2;
                row3Buf[col] = (currVal + nextVal + 1) >> 1;
                row4Buf[col] = (currVal + 3 * nextVal + 2) >> 2;
            }
        }
    }
}

#ifdef MCV_MULTI_THREAD
MVoid thread_fast_bilinear_8U(MVoid* pParam)
{
    Bilinear_Interpolation_8U* bi = (Bilinear_Interpolation_8U*)pParam;
    MInt32 task_ID = bi->task_ID;
    MInt32 lret = MOK;

    fast_bilinear_8U(bi->srcBuf, bi->srcH, bi->srcW, bi->srcPitch, bi->dstBuf, bi->dstH, bi->dstW,
        bi->dstPitch, bi->upScale, bi->downScale, bi->startRow, bi->endRow);

    bi->errCode = lret;
}
#endif

//双线性插值的快速实现
// todo:为编译通过，暂时注释掉 by jck
MLong fastBilinearInter_8U(MHandle mcvParallelMonitor, MByte* srcBuf, MInt32 srcH, MInt32 srcW, MInt32 srcPitch, MByte* dstBuf,
    MInt32 dstH, MInt32 dstW, MInt32 dstPitch, MInt32 upScale, MInt32 downScale)
{
    MLong res = MOK;
    MInt32 i, scale, padsize;

    if (upScale != 0 && downScale == 0)
    {
        if (upScale > 2)
        {
            res = MERR_UNSUPPORTED;
            goto exit;
        }
        if ((dstH / srcH) != (1 << upScale) || (dstW / srcW) != (1 << upScale))
        {
            res = MERR_INVALID_PARAM;
            goto exit;
        }
        padsize = (1 << upScale) - 1;
    }
    else if (upScale == 0 && downScale != 0)
    {
        if (downScale > 2)
        {
            res = MERR_UNSUPPORTED;
            goto exit;
        }
        if ((srcH / dstH) != (1 << downScale) || (srcW / dstW) != (1 << downScale))
        {
            res = MERR_INVALID_PARAM;
            goto exit;
        }
        padsize = (1 << downScale) - 1;
    }
    else
    {
        res = MERR_UNSUPPORTED;
        goto exit;
    }

#ifdef MCV_MULTI_THREAD
    {
        const MInt32 lTask_Num = DENOISE_TASK_NUM;
        MInt32 lSize, taskID[lTask_Num] = { 0 };
        Bilinear_Interpolation_8U pParams[lTask_Num] = { 0 };

        if (downScale != 0)
        {
            lSize = dstH / lTask_Num;
            lSize = lSize >> 1 << 1;

            pParams[0].startRow = 0;
            pParams[0].endRow = lSize;

            for (i = 1; i < lTask_Num; ++i)
            {
                pParams[i].startRow = i * lSize;
                pParams[i].endRow = (i + 1) * lSize;
            }
            pParams[lTask_Num - 1].endRow = dstH;
        }
        if (upScale != 0)
        {
            scale = 1 << upScale;
            lSize = dstH / (scale * lTask_Num);
            lSize = lSize >> 1 << 1;

            pParams[0].startRow = 0;
            pParams[0].endRow = scale * lSize;

            for (i = 1; i < lTask_Num; ++i)
            {
                pParams[i].startRow = i * scale * lSize;
                pParams[i].endRow = (i + 1) * scale * lSize;
            }
            pParams[lTask_Num - 1].endRow = dstH;
        }

        for (i = 0; i < lTask_Num; i++)
        {
            pParams[i].task_ID = i;
            pParams[i].srcBuf = srcBuf;
            pParams[i].dstBuf = dstBuf;
            pParams[i].srcH = srcH;
            pParams[i].srcW = srcW;
            pParams[i].dstH = dstH;
            pParams[i].dstW = dstW;
            pParams[i].srcPitch = srcPitch;
            pParams[i].dstPitch = dstPitch;
            pParams[i].downScale = downScale;
            pParams[i].upScale = upScale;
        }

        for (i = 0; i < lTask_Num; i++)
        {
            taskID[i] = mcvAddTask(mcvParallelMonitor, thread_fast_bilinear_8U, (MVoid*)&pParams[i]);
        }
        for (i = 0; i < lTask_Num; i++)
        {
            mcvWaitTask(mcvParallelMonitor, taskID[i]);
        }

    }
#else
    //todo:此处参数传入有问题，待后面排查。2020.4.9 by jck
    //fast_bilinear_8U(srcBuf, srows, scols, dstBuf, drows, dcols, upScale, downScale, 0, drows);
#endif

exit:
    return res;
}
/********************************************************************************************/

MVoid fast_bilinear_32I(MInt32* srcBuf, MInt32 srows, MInt32 scols, MInt32* dstBuf, MInt32 drows, MInt32 dcols,
    MInt32 upScale, MInt32 downScale, MInt32 startRow, MInt32 endRow)
{
    MInt32 row, col, i, j, index;
    MInt32 currVal, nextVal;
    MInt32 subRow = 0, scale = 0;
    MInt32* currSrcBuf = MNull, * nextSrcBuf = MNull;
    MInt32* row1Buf = MNull, * row2Buf = MNull, * row3Buf = MNull, * row4Buf = MNull, * row5Buf = MNull;
    if (downScale != 0)
    {
        //下采样
        for (row = startRow; row < endRow; ++row)
        {
            i = row << downScale;
            if (i >= srows)
                i = srows - 1;
            for (col = 0; col < dcols; ++col)
            {
                j = col << downScale;
                if (j >= scols)
                    j = scols - 1;
                dstBuf[row * dcols + col] = srcBuf[i * scols + j];
            }
        }
    }
    else if (upScale == 1)
    {
        //上采样放大一倍
        scale = 1 << upScale;
#if CV_NEON
        int32x4_t tmpdata_32x4, rowdata00_32x4, rowdata01_32x4;
        int32x4x2_t resdata_32x4x2;
#endif
        for (row = startRow; row < endRow; row += scale)
        {
            subRow = row >> upScale;
            currSrcBuf = srcBuf + subRow * scols;
            nextSrcBuf = (subRow + 1) >= srows ? currSrcBuf - scols : currSrcBuf + scols;

            row1Buf = dstBuf + row * dcols;
            row2Buf = (row + 1) >= drows ? dstBuf + (drows - 1) * dcols : row1Buf + dcols;
            row3Buf = (row + 2) >= drows ? dstBuf + (drows - 1) * dcols : row2Buf + dcols;

            if (row == startRow)
            {
                col = 0;
#if CV_NEON
                for (col = 0; col < scols - 4; col += 4)
                {
                    resdata_32x4x2.val[0] = vld1q_s32(currSrcBuf + col);
                    tmpdata_32x4 = vld1q_s32(currSrcBuf + col + 1);

                    resdata_32x4x2.val[1] = vrhaddq_s32(resdata_32x4x2.val[0], tmpdata_32x4);
                    vst2q_s32(row1Buf + (col << upScale), resdata_32x4x2);
                }
#endif
                for (; col < scols; ++col)
                {
                    currVal = currSrcBuf[col];
                    if (col + 1 >= scols)
                        nextVal = currSrcBuf[scols - 2];
                    else
                        nextVal = currSrcBuf[col + 1];

                    index = col << upScale;
                    row1Buf[index] = currVal;
                    row1Buf[(index + 1) >= dcols ? dcols - 1 : index + 1] = (currVal + nextVal + 1) >> 1;
                }
                index = (scols - 1) << upScale;
                if ((index + 1) < dcols)
                {
                    for (i = index + 2; i < dcols; ++i)
                        row1Buf[i] = row1Buf[index + 1];
                }
            }

            col = 0;
#if CV_NEON
            for (col = 0; col < scols - 4; col += 4)
            {
                resdata_32x4x2.val[0] = vld1q_s32(nextSrcBuf + col);
                tmpdata_32x4 = vld1q_s32(nextSrcBuf + col + 1);

                resdata_32x4x2.val[1] = vrhaddq_s32(resdata_32x4x2.val[0], tmpdata_32x4);
                vst2q_s32(row3Buf + (col << upScale), resdata_32x4x2);
            }
#endif
            for (; col < scols; ++col)
            {
                currVal = nextSrcBuf[col];
                if (col + 1 >= scols)
                    nextVal = nextSrcBuf[scols - 2];
                else
                    nextVal = nextSrcBuf[col + 1];

                index = col << upScale;
                row3Buf[index] = currVal;
                row3Buf[(index + 1) >= dcols ? dcols - 1 : index + 1] = (currVal + nextVal + 1) >> 1;
            }
            index = (scols - 1) << upScale;
            if ((index + 1) < dcols)
            {
                for (i = index + 2; i < dcols; ++i)
                    row3Buf[i] = row3Buf[index + 1];
            }

            col = 0;
#if CV_NEON
            for (col = 0; col < dcols - 4; col += 4)
            {
                rowdata00_32x4 = vld1q_s32(row1Buf + col);
                rowdata01_32x4 = vld1q_s32(row3Buf + col);

                tmpdata_32x4 = vrhaddq_s32(rowdata00_32x4, rowdata01_32x4);
                vst1q_s32(row2Buf + col, tmpdata_32x4);
            }
#endif
            for (; col < dcols; ++col)
            {
                row2Buf[col] = (row1Buf[col] + row3Buf[col] + 1) >> 1;
            }
        }
    }
    else if (upScale == 2)
    {
        //上采样放大两倍
        scale = 1 << upScale;
        for (row = startRow; row < endRow; row += scale)
        {
            subRow = row >> upScale;
            currSrcBuf = srcBuf + subRow * scols;
            nextSrcBuf = (subRow + 1) >= srows ? currSrcBuf - scols : currSrcBuf + scols;

            row1Buf = dstBuf + row * dcols;
            row2Buf = (row + 1) >= drows ? dstBuf + (drows - 1) * dcols : row1Buf + dcols;
            row3Buf = (row + 2) >= drows ? dstBuf + (drows - 1) * dcols : row2Buf + dcols;
            row4Buf = (row + 3) >= drows ? dstBuf + (drows - 1) * dcols : row3Buf + dcols;
            row5Buf = (row + 4) >= drows ? dstBuf + (drows - 1) * dcols : row4Buf + dcols;

            if (row == startRow)
            {
                for (col = 0; col < scols; ++col)
                {
                    currVal = currSrcBuf[col];
                    if (col + 1 >= scols)
                        nextVal = currSrcBuf[scols - 2];
                    else
                        nextVal = currSrcBuf[col + 1];

                    index = col << upScale;
                    row1Buf[index] = currVal;
                    row1Buf[(index + 1) >= dcols ? dcols - 1 : index + 1] = (3 * currVal + nextVal + 2) >> 2;
                    row1Buf[(index + 2) >= dcols ? dcols - 1 : index + 2] = (currVal + nextVal + 1) >> 1;
                    row1Buf[(index + 3) >= dcols ? dcols - 1 : index + 3] = (currVal + 3 * nextVal + 2) >> 2;
                }
                index = (scols - 1) << upScale;
                if ((index + 3) < dcols)
                {
                    for (i = index + 4; i < dcols; ++i)
                        row1Buf[i] = row1Buf[index + 3];
                }
            }

            for (col = 0; col < scols; ++col)
            {
                currVal = nextSrcBuf[col];
                if (col + 1 >= scols)
                    nextVal = nextSrcBuf[scols - 2];
                else
                    nextVal = nextSrcBuf[col + 1];

                index = col << upScale;
                row5Buf[index] = currVal;
                row5Buf[(index + 1) >= dcols ? dcols - 1 : index + 1] = (3 * currVal + nextVal + 2) >> 2;
                row5Buf[(index + 2) >= dcols ? dcols - 1 : index + 2] = (currVal + nextVal + 1) >> 1;
                row5Buf[(index + 3) >= dcols ? dcols - 1 : index + 3] = (currVal + 3 * nextVal + 2) >> 2;
            }
            index = (scols - 1) << upScale;
            if ((index + 3) < dcols)
            {
                for (i = index + 4; i < dcols; ++i)
                    row5Buf[i] = row5Buf[index + 3];
            }

            for (col = 0; col < dcols; ++col)
            {
                currVal = row1Buf[col];
                nextVal = row5Buf[col];

                row2Buf[col] = (3 * currVal + nextVal + 2) >> 2;
                row3Buf[col] = (currVal + nextVal + 1) >> 1;
                row4Buf[col] = (currVal + 3 * nextVal + 2) >> 2;
            }
        }
    }
}


#ifdef MCV_MULTI_THREAD
MVoid thread_fast_bilinear_32I(MVoid* pParam)
{
    Bilinear_Interpolation_32I* bi = (Bilinear_Interpolation_32I*)pParam;
    MInt32 task_ID = bi->task_ID;
    MInt32 lret = MOK;

    fast_bilinear_32I(bi->srcBuf, bi->srows, bi->scols, bi->dstBuf, bi->drows, bi->dcols, bi->upScale, bi->downScale, bi->startRow, bi->endRow);

    bi->errCode = lret;
}
#endif

//双线性插值的快速实现
MLong fastBilinearInter_32I(MHandle mcvParallelMonitor, MInt32* srcBuf, MInt32* dstBuf, MInt32 lImgHeight, MInt32 lImgWidth, MInt32 upScale, MInt32 downScale)
{
    MLong res = MOK;
    MInt32 i, scale, padsize, srows, scols, drows, dcols;

    if (upScale != 0 && downScale == 0)
    {
        if (upScale > 2)
        {
            res = MERR_UNSUPPORTED;
            goto exit;
        }
        padsize = (1 << upScale) - 1;
        drows = lImgHeight;
        dcols = lImgWidth;
        srows = lImgHeight >> upScale;
        scols = lImgWidth >> upScale;
    }
    else if (upScale == 0 && downScale != 0)
    {
        if (downScale > 2)
        {
            res = MERR_UNSUPPORTED;
            goto exit;
        }
        padsize = (1 << downScale) - 1;
        srows = lImgHeight;
        scols = lImgWidth;
        drows = lImgHeight >> downScale;
        dcols = lImgWidth >> downScale;
    }
    else
    {
        res = MERR_UNSUPPORTED;
        goto exit;
    }

#ifdef MCV_MULTI_THREAD
    {
        const MInt32 lTask_Num = DENOISE_TASK_NUM;
        MInt32 lSize, taskID[lTask_Num] = { 0 };
        Bilinear_Interpolation_32I pParams[lTask_Num] = { 0 };

        if (downScale != 0)
        {
            lSize = drows / lTask_Num;
            lSize = lSize >> 1 << 1;

            pParams[0].startRow = 0;
            pParams[0].endRow = lSize;

            for (i = 1; i < lTask_Num; ++i)
            {
                pParams[i].startRow = i * lSize;
                pParams[i].endRow = (i + 1) * lSize;
            }
            pParams[lTask_Num - 1].endRow = drows;
        }
        if (upScale != 0)
        {
            scale = 1 << upScale;
            lSize = drows / (scale * lTask_Num);
            lSize = lSize >> 1 << 1;

            pParams[0].startRow = 0;
            pParams[0].endRow = scale * lSize;

            for (i = 1; i < lTask_Num; ++i)
            {
                pParams[i].startRow = i * scale * lSize;
                pParams[i].endRow = (i + 1) * scale * lSize;
            }
            pParams[lTask_Num - 1].endRow = drows;
        }

        for (i = 0; i < lTask_Num; i++)
        {
            pParams[i].task_ID = i;
            pParams[i].srcBuf = srcBuf;
            pParams[i].dstBuf = dstBuf;
            pParams[i].srows = srows;
            pParams[i].scols = scols;
            pParams[i].drows = drows;
            pParams[i].dcols = dcols;
            pParams[i].upScale = upScale;
            pParams[i].downScale = downScale;
        }

        for (i = 0; i < lTask_Num; i++)
        {
            taskID[i] = mcvAddTask(mcvParallelMonitor, thread_fast_bilinear_32I, (MVoid*)&pParams[i]);
        }
        for (i = 0; i < lTask_Num; i++)
        {
            mcvWaitTask(mcvParallelMonitor, taskID[i]);
        }

    }
#else
    fast_bilinear_32I(srcBuf, srows, scols, dstBuf, drows, dcols, upScale, downScale, 0, drows);
#endif

exit:
    return res;
}
/********************************************************************************************/
//任意分辨率图片之间的双线性插值
//#ifndef ALPHA_TABLE_BL
//#define ALPHA_TABLE_BL
//typedef struct _tag_ALPHATABLE_BL {
//	MInt32 pre_id;
//	MInt32 next_id;
//	MInt32 pre_alpha;
//	MInt32 next_alpha;
//} AlphaTable_BL;
//#endif

#ifdef MCV_MULTI_THREAD
typedef struct _tag_Org_Bilinear_Inter
{
    MInt32			task_ID;
    MByte* srcBuf;
    MByte* dstBuf;
    MInt32          srows;
    MInt32          scols;
    MInt32          drows;
    MInt32          dcols;
    MFloat          scale_x;
    MFloat          scale_y;
    AlphaTable_BL* xofs;
    AlphaTable_BL* yofs;
    MInt32          startRow;
    MInt32          endRow;
}Org_Bilinear_Inter;
#endif

#define BL_SHIFT_NUM   (128)

MVoid calBiliInterTable(MInt32 srows, MInt32 scols, MInt32 drows, MInt32 dcols, AlphaTable_BL* xofs, AlphaTable_BL* yofs)
{
    MInt32 x, y;
    MFloat xscale = (MFloat)scols / dcols;
    MFloat yscale = (MFloat)srows / drows;
    MFloat fy, fx;
    MInt32 sy, sx;

    for (y = 0; y < drows; ++y)
    {
        fy = yscale * y;
        sy = floor(fy);
        fy -= sy;
        if (fy < 0)
        {
            fy = 0;
            sy = 0;
        }
        yofs[y].pre_id = sy;
        yofs[y].next_id = (sy + 1) >= srows ? (srows - 1) : (sy + 1);
        yofs[y].pre_alpha = (MInt32)((1.0f - fy) * BL_SHIFT_NUM + 0.5f);
        yofs[y].next_alpha = BL_SHIFT_NUM - yofs[y].pre_alpha;
    }

    for (x = 0; x < dcols; ++x)
    {
        fx = xscale * x;
        sx = floor(fx);
        fx -= sx;
        if (fx < 0)
        {
            fx = 0;
            sx = 0;
        }
        xofs[x].pre_id = sx;
        xofs[x].next_id = (sx + 1) >= scols ? (scols - 1) : (sx + 1);
        xofs[x].pre_alpha = (MInt32)((1.0f - fx) * BL_SHIFT_NUM + 0.5f);
        xofs[x].next_alpha = BL_SHIFT_NUM - xofs[x].pre_alpha;
    }
}

MVoid OrgLocalBilinearInter_8U(MByte* srcBuf, MInt32 srows, MInt32 scols, MByte* dstBuf, MInt32 drows, MInt32 dcols,
    MFloat scale_x, MFloat scale_y, AlphaTable_BL* xofs, AlphaTable_BL* yofs, MInt32 startRow, MInt32 endRow)
{
    MInt32 pre_i, pre_j, next_i, next_j, row, col;
    MInt32 pre_xcoff, next_xcoff, pre_ycoff, next_ycoff;
    MByte* prePtr = MNull, * nextPtr = MNull, * dstPtr = MNull;

    MUInt32* tmpRowBuf = (MUInt32*)MMemAlloc(MNull, scols * sizeof(MUInt32));
    for (row = startRow; row < endRow; ++row)
    {
        pre_i = yofs[row].pre_id;
        next_i = yofs[row].next_id;
        pre_ycoff = yofs[row].pre_alpha;
        next_ycoff = yofs[row].next_alpha;

        prePtr = srcBuf + pre_i * scols;
        nextPtr = srcBuf + next_i * scols;
        dstPtr = dstBuf + row * dcols;

        col = 0;
#if CV_NEON
        {
            uint8x8_t predata, nextdata;
            uint8x8_t precoff, nextcoff;
            uint16x8_t tmpdata00, tmpdata01;
            uint32x4_t resdata;
            precoff = vdup_n_u8(pre_ycoff);
            nextcoff = vdup_n_u8(next_ycoff);
            for (col = 0; col < scols - 7; col += 8)
            {
                predata = vld1_u8(prePtr + col);
                nextdata = vld1_u8(nextPtr + col);

                tmpdata00 = vmull_u8(precoff, predata);
                tmpdata01 = vmull_u8(nextcoff, nextdata);

                resdata = vaddl_u16(vget_low_u16(tmpdata00), vget_low_u16(tmpdata01));
                vst1q_u32(tmpRowBuf + col, resdata);
                resdata = vaddl_u16(vget_high_u16(tmpdata00), vget_high_u16(tmpdata01));
                vst1q_u32(tmpRowBuf + col + 4, resdata);
            }
        }
#endif
        for (; col < scols; ++col)
        {
            tmpRowBuf[col] = (MUInt32)(pre_ycoff * prePtr[col]) + (MUInt32)(next_ycoff * nextPtr[col]);
        }

        for (col = 0; col < dcols; ++col)
        {
            pre_j = xofs[col].pre_id;
            next_j = xofs[col].next_id;
            pre_xcoff = xofs[col].pre_alpha;
            next_xcoff = xofs[col].next_alpha;

            dstPtr[col] = (MByte)((tmpRowBuf[pre_j] * pre_xcoff + tmpRowBuf[next_j] * next_xcoff) >> 14);
        }
    }
    if (tmpRowBuf)
    {
        MMemFree(MNull, tmpRowBuf);
        tmpRowBuf = MNull;
    }
}

#ifdef MCV_MULTI_THREAD
MVoid thread_OrgBilinearInter_8U(MVoid* pParam)
{
    Org_Bilinear_Inter* bi = (Org_Bilinear_Inter*)pParam;

    OrgLocalBilinearInter_8U(bi->srcBuf, bi->srows, bi->scols, bi->dstBuf, bi->drows, bi->dcols,
        bi->scale_x, bi->scale_y, bi->xofs, bi->yofs, bi->startRow, bi->endRow);
}
#endif

MInt32 OrgBilinearInter_8U(MHandle mcvParallelMonitor, MByte* pSrcBuf, MInt32 srows, MInt32 scols, MByte* pDstBuf, MInt32 drows, MInt32 dcols)
{
    MInt32 res = MOK, i;
    MFloat scale_x = (MFloat)scols / (MFloat)dcols;
    MFloat scale_y = (MFloat)srows / (MFloat)drows;

    AlphaTable_BL* xofs = (AlphaTable_BL*)MMemAlloc(MNull, dcols * sizeof(AlphaTable_BL));
    AlphaTable_BL* yofs = (AlphaTable_BL*)MMemAlloc(MNull, drows * sizeof(AlphaTable_BL));
    if (xofs == MNull || yofs == MNull)
    {
        res = MERR_NO_MEMORY;
        goto exit;
    }
    calBiliInterTable(srows, scols, drows, dcols, xofs, yofs);

#ifdef MCV_MULTI_THREAD
    {
        const MInt32 lTask_Num = DENOISE_TASK_NUM;
        MInt32 lSize, taskID[lTask_Num] = { 0 };
        Org_Bilinear_Inter pParams[lTask_Num] = { 0 };

        lSize = drows / lTask_Num;
        lSize = lSize >> 1 << 1;

        pParams[0].startRow = 0;
        pParams[0].endRow = lSize;
        for (i = 1; i < lTask_Num; i++)
        {
            pParams[i].startRow = pParams[i - 1].endRow;
            pParams[i].endRow = pParams[i].startRow + lSize;
        }
        pParams[lTask_Num - 1].endRow = drows;

        for (i = 0; i < lTask_Num; i++)
        {
            pParams[i].task_ID = i;
            pParams[i].srcBuf = pSrcBuf;
            pParams[i].dstBuf = pDstBuf;
            pParams[i].srows = srows;
            pParams[i].scols = scols;
            pParams[i].drows = drows;
            pParams[i].dcols = dcols;
            pParams[i].scale_x = scale_x;
            pParams[i].scale_y = scale_y;
            pParams[i].xofs = xofs;
            pParams[i].yofs = yofs;
        }

        for (i = 0; i < lTask_Num; i++)
        {
            taskID[i] = mcvAddTask(mcvParallelMonitor, thread_OrgBilinearInter_8U, (MVoid*)&pParams[i]);
        }
        for (i = 0; i < lTask_Num; i++)
        {
            mcvWaitTask(mcvParallelMonitor, taskID[i]);
        }

    }
#else
    OrgLocalBilinearInter_8U(pSrcBuf, srows, scols, pDstBuf, drows, dcols, scale_x, scale_y, xofs, yofs, 0, drows);
#endif
exit:
    if (xofs)
    {
        MMemFree(MNull, xofs);
        xofs = MNull;
    }
    if (yofs)
    {
        MMemFree(MNull, yofs);
        yofs = MNull;
    }
    return res;
}

/***********************************************************************************************************/
MVoid LocalFastBilinear_2X_8UC2(MByte* srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch, MByte* dstBuf, MInt32 dstW, MInt32 dstH,
    MInt32 dstPitch, MInt32 startRow, MInt32 endRow, MInt32* expand_size)
{
    MInt32 i, j, k1, k2, k3, m1, m2;
    MByte* srcRow0 = MNull, * srcRow1 = MNull;
    MByte* dstRow0 = MNull, * dstRow1 = MNull;
#ifdef __ARM_NEON__
    uint8x8x2_t currdata00, currdata01;
    uint8x8x2_t nextdata00, nextdata01;
    uint8x8x4_t resdata;
    uint16x8_t tmpdata00, tmpdata01;
#endif
    startRow = startRow + expand_size[0];
    endRow = endRow - expand_size[1];
    expand_size[2] = expand_size[2] << 1;
    expand_size[3] = expand_size[3] << 1;

    for (i = startRow; i < endRow; i += 2)
    {
        k1 = ((i - expand_size[0]) >> 1) >= srcH ? srcH - 1 : ((i - expand_size[0]) >> 1);
        k2 = (k1 + 1) >= srcH ? srcH - 1 : k1 + 1;
        k3 = (i + 1) >= dstH ? dstH - 1 : i + 1;

        srcRow0 = srcBuf + k1 * srcPitch;
        srcRow1 = srcBuf + k2 * srcPitch;
        dstRow0 = dstBuf + i * dstPitch;
        dstRow1 = dstBuf + k3 * dstPitch;

        j = expand_size[2];
#ifdef __ARM_NEON__
        for (; j < dstW - expand_size[3] - 36; j += 32)
        {
            m1 = (j - expand_size[2]) >> 1;
            m2 = m1 + 2;

            currdata00 = vld2_u8(srcRow0 + m1);
            currdata01 = vld2_u8(srcRow0 + m2);
            nextdata00 = vld2_u8(srcRow1 + m1);
            nextdata01 = vld2_u8(srcRow1 + m2);

            resdata.val[0] = currdata00.val[0];
            resdata.val[1] = currdata00.val[1];
            resdata.val[2] = vrhadd_u8(currdata00.val[0], currdata01.val[0]);
            resdata.val[3] = vrhadd_u8(currdata00.val[1], currdata01.val[1]);
            vst4_u8(dstRow0 + j, resdata);

            resdata.val[0] = vrhadd_u8(currdata00.val[0], nextdata00.val[0]);
            resdata.val[1] = vrhadd_u8(currdata00.val[1], nextdata00.val[1]);
            tmpdata00 = vaddl_u8(currdata00.val[0], currdata01.val[0]);
            tmpdata01 = vaddl_u8(nextdata00.val[0], nextdata01.val[0]);
            tmpdata00 = vqaddq_u16(tmpdata00, tmpdata01);
            resdata.val[2] = vrshrn_n_u16(tmpdata00, 2);
            tmpdata00 = vaddl_u8(currdata00.val[1], currdata01.val[1]);
            tmpdata01 = vaddl_u8(nextdata00.val[1], nextdata01.val[1]);
            tmpdata00 = vqaddq_u16(tmpdata00, tmpdata01);
            resdata.val[3] = vrshrn_n_u16(tmpdata00, 2);
            vst4_u8(dstRow1 + j, resdata);
        }
#endif
        for (; j < dstW - expand_size[3]; j += 4)
        {
            m1 = (j - expand_size[2]) >> 1;
            m2 = (m1 + 2) >= (srcW - 1) ? srcW - 2 : m1 + 2;

            dstRow0[j] = srcRow0[m1];
            dstRow0[j + 1] = srcRow0[m1 + 1];
            dstRow0[j + 2] = (srcRow0[m1] + srcRow0[m2] + 1) >> 1;
            dstRow0[j + 3] = (srcRow0[m1 + 1] + srcRow0[m2 + 1] + 1) >> 1;

            dstRow1[j] = (srcRow0[m1] + srcRow1[m1] + 1) >> 1;
            dstRow1[j + 1] = (srcRow0[m1 + 1] + srcRow1[m1 + 1] + 1) >> 1;
            dstRow1[j + 2] = (srcRow0[m1] + srcRow0[m2] + srcRow1[m1] + srcRow1[m2] + 2) >> 2;
            dstRow1[j + 3] = (srcRow0[m1 + 1] + srcRow0[m2 + 1] + srcRow1[m1 + 1] + srcRow1[m2 + 1] + 2) >> 2;
        }
    }

    if ((endRow % 2) != 0)
    {
        MMemCpy(dstBuf + (dstH - 1) * dstPitch, dstBuf + (dstH - 2) * dstPitch, dstPitch);
    }
}


#ifdef MCV_MULTI_THREAD
MVoid threadLocalFastBilinear_2X_8UC2(MVoid* pParam)
{
    Bilinear_Interpolation_8U* bi = (Bilinear_Interpolation_8U*)pParam;
    LocalFastBilinear_2X_8UC2(bi->srcBuf, bi->srcW, bi->srcH, bi->srcPitch, bi->dstBuf, bi->dstW, bi->dstH,
        bi->dstPitch, bi->startRow, bi->endRow, bi->expand_size);
}
#endif

MInt32 FastBilinear_2X_8UC2(MHandle mcvParallelMonitor, MByte* srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch, MByte* dstBuf, MInt32 dstW,
    MInt32 dstH, MInt32 dstPitch)
{
    MInt32 res = MOK, lSize, i;
    const MInt32 lTask_Num = 8;
    MInt32 expand_size[4] = { 0, 0, 0, 0 };

#ifdef MCV_MULTI_THREAD
    {
        MFloat  scale = 1.0f / lTask_Num;
        MInt32 lSize, taskID[lTask_Num] = { 0 };
        Bilinear_Interpolation_8U pParams[lTask_Num] = { 0 };

        lSize = dstH / lTask_Num;
        lSize = lSize >> 1 << 1;

        pParams[0].startRow = 0;
        pParams[0].endRow = lSize;
        for (i = 1; i < lTask_Num; i++)
        {
            pParams[i].startRow = pParams[i - 1].endRow;
            pParams[i].endRow = pParams[i].startRow + lSize;
        }
        pParams[lTask_Num - 1].endRow = dstH;

        for (i = 0; i < lTask_Num; i++)
        {
            pParams[i].task_ID = i;
            pParams[i].srcBuf = srcBuf;
            pParams[i].dstBuf = dstBuf;
            pParams[i].srcH = srcH;
            pParams[i].srcW = srcW;
            pParams[i].dstH = dstH;
            pParams[i].dstW = dstW;
            pParams[i].srcPitch = srcPitch;
            pParams[i].dstPitch = dstPitch;
            pParams[i].expand_size = expand_size;
        }

        for (i = 0; i < lTask_Num; i++)
        {
            taskID[i] = mcvAddTask(mcvParallelMonitor, threadLocalFastBilinear_2X_8UC2, (MVoid*)&pParams[i]);
        }
        for (i = 0; i < lTask_Num; i++)
        {
            res = mcvWaitTask(mcvParallelMonitor, taskID[i]);
            if (MOK != res)
            {
                goto exit;
            }
        }
    }
#endif

exit:
    return res;
}

MInt32 BoxDownSample_C2_rows(MByte* srcdata, MInt32 srcW, MInt32 srcH, MInt32 lSrcPitch, MByte* dstdata, MInt32 dstW, MInt32 dstH, MInt32 lDstPitch,
    MInt32 startRow, MInt32 endRow)
{
    const MInt32 shiftBit = 15;
    MInt32 lret = MOK, i, j, boxKernel = 3, halfKernel = boxKernel >> 1;
    MInt32 row1, row2, row3, col1, col2, col3;
    MByte* rowBuf1 = MNull, * rowBuf2 = MNull, * rowBuf3 = MNull, * dstptr = MNull;
    MInt32 tmpVal = 0, ratio = 3640, halfShiftVal = 1 << (shiftBit - 1);
#ifdef __ARM_NEON__
    uint8x8x4_t predata01, predata02;
    uint8x8x4_t currdata01, currdata02;
    uint8x8x4_t nextdata01, nextdata02;
    uint16x8_t tmpdata, combdata;
    uint16x4_t lowdata, highdata;
    uint32x4_t sumdata;
    uint8x8x2_t resdata;
#endif

    for (i = startRow; i < endRow; i++) // endRow <= dstH
    {
        row1 = 2 * i - 1 > 0 ? 2 * i - 1 : 2 * i + 1;
        row2 = 2 * i;
        row3 = 2 * i + 1 > srcH - 1 ? srcH - 1 : 2 * i + 1;
        dstptr = dstdata + i * lDstPitch;
        rowBuf1 = srcdata + row1 * lSrcPitch;
        rowBuf2 = srcdata + row2 * lSrcPitch;
        rowBuf3 = srcdata + row3 * lSrcPitch;

        //j = 0
        {
            tmpVal = rowBuf1[2] + rowBuf1[0] + rowBuf1[2] + rowBuf2[2] + rowBuf2[0] +
                rowBuf2[2] + rowBuf3[2] + rowBuf3[0] + rowBuf3[2];
            dstptr[0] = (ratio * tmpVal + halfShiftVal) >> shiftBit;

            tmpVal = rowBuf1[3] + rowBuf1[1] + rowBuf1[3] + rowBuf2[3] + rowBuf2[1] +
                rowBuf2[3] + rowBuf3[3] + rowBuf3[1] + rowBuf3[3];
            dstptr[1] = (ratio * tmpVal + halfShiftVal) >> shiftBit;
        }

        j = 2;
#ifdef __ARM_NEON__
        for (; j < dstW - 20; j += 16)
        {
            predata01 = vld4_u8(rowBuf1 + 2 * j - 2);
            predata02 = vld4_u8(rowBuf1 + 2 * j + 2);
            currdata01 = vld4_u8(rowBuf2 + 2 * j - 2);
            currdata02 = vld4_u8(rowBuf2 + 2 * j + 2);
            nextdata01 = vld4_u8(rowBuf3 + 2 * j - 2);
            nextdata02 = vld4_u8(rowBuf3 + 2 * j + 2);

            tmpdata = vaddl_u8(predata01.val[0], predata01.val[2]);
            tmpdata = vaddw_u8(tmpdata, predata02.val[0]);
            tmpdata = vaddw_u8(tmpdata, currdata01.val[0]);
            tmpdata = vaddw_u8(tmpdata, currdata01.val[2]);
            tmpdata = vaddw_u8(tmpdata, currdata02.val[0]);
            tmpdata = vaddw_u8(tmpdata, nextdata01.val[0]);
            tmpdata = vaddw_u8(tmpdata, nextdata01.val[2]);
            tmpdata = vaddw_u8(tmpdata, nextdata02.val[0]);
            sumdata = vmull_u16(vget_low_u16(tmpdata), vdup_n_u16(ratio));
            sumdata = vaddq_u32(sumdata, vdupq_n_u32(halfShiftVal));
            lowdata = vshrn_n_u32(sumdata, shiftBit);
            sumdata = vmull_u16(vget_high_u16(tmpdata), vdup_n_u16(ratio));
            sumdata = vaddq_u32(sumdata, vdupq_n_u32(halfShiftVal));
            highdata = vshrn_n_u32(sumdata, shiftBit);
            combdata = vcombine_u16(lowdata, highdata);
            resdata.val[0] = vqmovn_u16(combdata);

            tmpdata = vaddl_u8(predata01.val[1], predata01.val[3]);
            tmpdata = vaddw_u8(tmpdata, predata02.val[1]);
            tmpdata = vaddw_u8(tmpdata, currdata01.val[1]);
            tmpdata = vaddw_u8(tmpdata, currdata01.val[3]);
            tmpdata = vaddw_u8(tmpdata, currdata02.val[1]);
            tmpdata = vaddw_u8(tmpdata, nextdata01.val[1]);
            tmpdata = vaddw_u8(tmpdata, nextdata01.val[3]);
            tmpdata = vaddw_u8(tmpdata, nextdata02.val[1]);
            sumdata = vmull_u16(vget_low_u16(tmpdata), vdup_n_u16(ratio));
            sumdata = vaddq_u32(sumdata, vdupq_n_u32(halfShiftVal));
            lowdata = vshrn_n_u32(sumdata, shiftBit);
            sumdata = vmull_u16(vget_high_u16(tmpdata), vdup_n_u16(ratio));
            sumdata = vaddq_u32(sumdata, vdupq_n_u32(halfShiftVal));
            highdata = vshrn_n_u32(sumdata, shiftBit);
            combdata = vcombine_u16(lowdata, highdata);
            resdata.val[1] = vqmovn_u16(combdata);

            vst2_u8(dstptr + j, resdata);
        }
#endif
        for (; j < dstW; j += 2)
        {
            col1 = 2 * j - 2 > 0 ? 2 * j - 2 : 2 * j + 2;
            col2 = 2 * j;
            col3 = 2 * j + 2 > srcW - 2 ? srcW - 2 : 2 * j + 2;

            tmpVal = rowBuf1[col1] + rowBuf1[col2] + rowBuf1[col3] + rowBuf2[col1] + rowBuf2[col2] +
                rowBuf2[col3] + rowBuf3[col1] + rowBuf3[col2] + rowBuf3[col3];
            dstptr[j] = (ratio * tmpVal + halfShiftVal) >> shiftBit;

            tmpVal = rowBuf1[col1 + 1] + rowBuf1[col2 + 1] + rowBuf1[col3 + 1] + rowBuf2[col1 + 1] + rowBuf2[col2 + 1] +
                rowBuf2[col3 + 1] + rowBuf3[col1 + 1] + rowBuf3[col2 + 1] + rowBuf3[col3 + 1];
            dstptr[j + 1] = (ratio * tmpVal + halfShiftVal) >> shiftBit;
        }
    }
    return lret;
}

#ifdef MCV_MULTI_THREAD
MVoid thread_BoxDownSample_C2(MVoid* pParam)
{
    Bilinear_Interpolation_8U* bi = (Bilinear_Interpolation_8U*)pParam;
    BoxDownSample_C2_rows(bi->srcBuf, bi->srcW, bi->srcH, bi->srcPitch, bi->dstBuf, bi->dstW, bi->dstH,
        bi->dstPitch, bi->startRow, bi->endRow);
}
#endif

MInt32 BoxDownSample_C2(MHandle mcvParallelMonitor, MByte* srcBuf, MInt32 srcW, MInt32 srcH, MInt32 srcPitch,
    MByte* dstBuf, MInt32 dstW, MInt32 dstH, MInt32 dstPitch)
{
    MInt32 res = MOK, lSize, i;
    const MInt32 lTask_Num = 16;

#ifdef MCV_MULTI_THREAD
    if (mcvParallelMonitor)
    {
        MFloat  scale = 1.0f / lTask_Num;
        MInt32 lSize, taskID[lTask_Num] = { 0 };
        Bilinear_Interpolation_8U pParams[lTask_Num] = { 0 };

        lSize = dstH / lTask_Num;
        lSize = lSize >> 1 << 1;

        pParams[0].startRow = 0;
        pParams[0].endRow = lSize;
        for (i = 1; i < lTask_Num; i++)
        {
            pParams[i].startRow = pParams[i - 1].endRow;
            pParams[i].endRow = pParams[i].startRow + lSize;
        }
        pParams[lTask_Num - 1].endRow = dstH;

        for (i = 0; i < lTask_Num; i++)
        {
            pParams[i].task_ID = i;
            pParams[i].srcBuf = srcBuf;
            pParams[i].dstBuf = dstBuf;
            pParams[i].srcH = srcH;
            pParams[i].srcW = srcW;
            pParams[i].dstH = dstH;
            pParams[i].dstW = dstW;
            pParams[i].srcPitch = srcPitch;
            pParams[i].dstPitch = dstPitch;
        }

        for (i = 0; i < lTask_Num; i++)
        {
            taskID[i] = mcvAddTask(mcvParallelMonitor, thread_BoxDownSample_C2, (MVoid*)&pParams[i]);
        }
        for (i = 0; i < lTask_Num; i++)
        {
            res = mcvWaitTask(mcvParallelMonitor, taskID[i]);
            if (MOK != res)
            {
                goto exit;
            }
        }
    }
    else
    {
        BoxDownSample_C2_rows(srcBuf, srcW, srcH, srcPitch, dstBuf, dstW, dstH, dstPitch, 0, dstH);
    }
#else
    BoxDownSample_C2_rows(srcBuf, srcW, srcH, srcPitch, dstBuf, dstW, dstH, dstPitch, 0, dstH);
#endif

exit:
    return res;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END
