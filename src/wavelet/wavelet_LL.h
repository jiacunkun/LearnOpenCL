//
// Created by jck7075 on 2020/4/10.
//

#pragma once

#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"


typedef struct tagWAVELET_DATA
{
    MInt32 h;
    MInt32 w;
    MInt32 *LL_data;
    MInt32 *HL_data;
    MInt32 *LH_data;
    MInt32 *HH_data;

    MInt32 recon_h;
    MInt32 recon_w;
    MInt32 *LL_recon;            //低频的重构数据
    MInt32 *LL_detail;           //重构前和重构后丢失的细节数据
}WAVELET_DATA, *WAVELET_LPDATA;

typedef struct tagWAVELET_DATA_8U
{
    MInt32 h;
    MInt32 w;
    MByte *LL_data;

    MInt32 recon_h;
    MInt32 recon_w;
    MByte *LL_recon;            //低频的重构数据
    MByte *LL_detail;           //重构前和重构后丢失的细节数据
}WAVELET_DATA_8U, *WAVELET_LPDATA_8U;

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN
/**
 * @brief 针对低频的小波分解，低频数据为8bit
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param _src
 * @param waveletBuf
 * @param levels
 * @return
 */
MLong rough_dwt_LLNS(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN _src, WAVELET_LPDATA_8U waveletBuf, MInt32 levels);

/**
 * @brief 针对低频LL的小波分解，低频数据为32bit
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param _src
 * @param waveletBuf
 * @param levels
 * @return
 */
MLong dwt_LLNS(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN _src, WAVELET_LPDATA waveletBuf, MInt32 levels);

/**
 * @brief 针对低频LL的小波重构，低频数据为32bit
 * @param hMemMgr
 * @param mcvParallelMonitor
 * @param waveletBuf
 * @param lHeight
 * @param lWidth
 * @param levels
 * @param resImg
 * @return
 */
MLong idwt_LLNS(MHandle hMemMgr, MHandle mcvParallelMonitor, WAVELET_LPDATA waveletBuf, MInt32 lHeight, MInt32 lWidth, MInt32 levels, LPASVLOFFSCREEN resImg);

NS_SINFLE_IMAGE_ENHANCEMENT_END