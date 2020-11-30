%module libyuv

%{
#define SWIG_FILE_WITH_INIT
#include "./include/libyuv/scale.h"
#include "./include/libyuv/row.h"
#include "./include/libyuv/scale_argb.h"
#include "./include/libyuv/basic_types.h"
#include "./include/libyuv/scale_row.h"
%}

%include "numpy.i"
/*  need this for correct module initialization */
%init %{
    import_array();
%}

%apply ( unsigned char* IN_ARRAY1, int DIM1) {( unsigned char* a, int size_a)}
%apply (unsigned char* INPLACE_ARRAY1, int DIM1) {(unsigned char *b, int size_b)}
namespace libyuv{
typedef enum FilterMode {
  kFilterNone = 0,      // Point sample; Fastest.
  kFilterLinear = 1,    // Filter horizontally only.
  kFilterBilinear = 2,  // Faster than box, but lower quality scaling down.
  kFilterBox = 3,        // Highest quality.
  kFilterMTlabBilinear = 4
} FilterModeEnum;
}

%inline %{
    int ARGBScale_func(unsigned char * a, int size_a,int src_stride,int src_width, int src_height,  unsigned char* b, int size_b,int dst_stride, int dst_width, int dst_height,  libyuv::FilterMode filter) {
        return libyuv::ARGBScale(a, src_stride, src_width, src_height, b,dst_stride, dst_width, dst_height, filter);
    }
    int GRAYScale_func(unsigned char* a, int size_a, int src_stride, int src_width, int src_height, unsigned char* b, int size_b, int dst_stride, int dst_width, int dst_height, libyuv::FilterMode filter) {
        libyuv::ScalePlane(a, src_stride, src_width, src_height, b, dst_stride, dst_width, dst_height, filter);
        return 0;
    }
    int MTlabARGBScale_func(unsigned char* a, int size_a, int src_stride, int src_width, int src_height, unsigned char* b, int size_b, int dst_stride, int dst_width, int dst_height, libyuv::FilterMode filter){
        libyuv::MTlabARGBScale(a, src_stride, src_width, src_height, b, dst_stride, dst_width, dst_height, filter);
    }
%}
