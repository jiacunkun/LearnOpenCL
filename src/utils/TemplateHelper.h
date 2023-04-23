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
#ifndef _h_TemplateHelper_h
#define _h_TemplateHelper_h
#include <type_traits>
#include "asvloffscreen.h"
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

/**
 调用方式：
 MInt32 max_val = TMaxValue<T>::v;
 
 std::is_same<T, MDouble>::value
 */


// indicates T is signed or unsigned
template<typename T>
struct TFSigned
{
    enum
    {
        fSigned = T(-1) < 0
    };
};

// represents the bit length of T
template<typename T>
struct TBitCount
{
    enum
    {
        cBits = sizeof(T) * 8
    };
};

template<typename T, bool fSigned>
struct TMinMaxHelper
{

};

template<typename T>
struct TMinMaxHelper<T, true/*fSigned*/ >
{
    static const T min = static_cast<T>( static_cast<T>(1) << ( TBitCount<T>::cBits - 1 ));
    static const T max = static_cast<T>( ~( static_cast<T>(1) << ( TBitCount<T>::cBits - 1 )));
};

template<typename T>
struct TMinMaxHelper<T, false/*fSigned*/ >
{
    static const T min = static_cast<T>( 0 );
    static const T max = static_cast<T>(-1);
};

template<typename T>
struct TMinValue
{
    static const T v = TMinMaxHelper<T, TFSigned<T>::fSigned>::min;
};

template<typename T>
struct TMaxValue
{
    static const T v = TMinMaxHelper<T, TFSigned<T>::fSigned>::max;
};

NS_SINFLE_IMAGE_ENHANCEMENT_END

#endif /* _h_TemplateHelper_h */
