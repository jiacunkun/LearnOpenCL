/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef INCLUDE_LIBYUV_SCALE_ARGB_H_
#define INCLUDE_LIBYUV_SCALE_ARGB_H_

#include "libyuv/basic_types.h"
#include "libyuv/scale.h"  // For FilterMode

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

LIBYUV_API
int ARGBScale(const uint8_t* src_argb,
              int src_stride_argb,
              int src_width,
              int src_height,
              uint8_t* dst_argb,
              int dst_stride_argb,
              int dst_width,
              int dst_height,
              enum FilterMode filtering);

// modify by 王钦 on 20181212
// 双线性插值在大倍数的图像缩放下存在图像信息丢失严重的情况，此函数针对这种情况
// 以2倍缩放的速率，反复调用双线性插值直至到达目的尺寸
LIBYUV_API
int MTlabARGBScale(const uint8_t* src_argb,
           int src_stride_argb,
           int src_width,
           int src_height,
           uint8_t* dst_argb,
           int dst_stride_argb,
           int dst_width,
           int dst_height,
           enum FilterMode filtering);
// end modify

// Clipped scale takes destination rectangle coordinates for clip values.
LIBYUV_API
int ARGBScaleClip(const uint8_t* src_argb,
                  int src_stride_argb,
                  int src_width,
                  int src_height,
                  uint8_t* dst_argb,
                  int dst_stride_argb,
                  int dst_width,
                  int dst_height,
                  int clip_x,
                  int clip_y,
                  int clip_width,
                  int clip_height,
                  enum FilterMode filtering);

// Scale with YUV conversion to ARGB and clipping.
LIBYUV_API
int YUVToARGBScaleClip(const uint8_t* src_y,
                       int src_stride_y,
                       const uint8_t* src_u,
                       int src_stride_u,
                       const uint8_t* src_v,
                       int src_stride_v,
                       uint32_t src_fourcc,
                       int src_width,
                       int src_height,
                       uint8_t* dst_argb,
                       int dst_stride_argb,
                       uint32_t dst_fourcc,
                       int dst_width,
                       int dst_height,
                       int clip_x,
                       int clip_y,
                       int clip_width,
                       int clip_height,
                       enum FilterMode filtering);

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // INCLUDE_LIBYUV_SCALE_ARGB_H_
