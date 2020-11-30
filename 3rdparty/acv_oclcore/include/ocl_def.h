#ifndef __ACV_OCL_BASE_DEFINITION_H__
#define __ACV_OCL_BASE_DEFINITION_H__

#ifdef HAVE_ACVCORE

#include "acvdef.h"

#else
	
#if (defined _WIN32 || defined WINCE || defined __CYGWIN__) && defined ACVCore_EXPORT
#  define ACV_EXPORTS __declspec(dllexport)
#elif defined __GNUC__ && __GNUC__ >= 4
#  define ACV_EXPORTS __attribute__ ((visibility ("default")))
#else
#  define ACV_EXPORTS
#endif

#ifndef ACV_INLINE
#  if defined __cplusplus
#    define ACV_INLINE static inline
#  elif defined _MSC_VER
#    define ACV_INLINE __inline
#  else
#    define ACV_INLINE static
#  endif
#endif

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#include <stdbool.h>
#endif

#ifndef ACV_PRIMITIVE_TYPES_DEFINED
#define ACV_PRIMITIVE_TYPES_DEFINED

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define uint32_type unsigned int
#define int64_type __int64
#define uint64_type unsigned __int64
#elif defined(__cplusplus) && __cplusplus >= 201103L && !defined(__APPLE__)
#include <cstdint>
#define uint32_type std::uint32_t
#define int64_type int64_t
#define uint64_type uint64_t
#else
#include <stdint.h>
#define uint32_type uint32_t
#define int64_type int64_t
#define uint64_type uint64_t
#endif //

typedef signed char schar;     // signed 1 byte integer
typedef unsigned char uchar;   // unsigned 1 byte integer
typedef unsigned short ushort; // unsigned 2 byte integer
typedef uint32_type   uint;	   // unsigned 4 byte integer
typedef int64_type    int64;   // signed 8 byte integer
typedef uint64_type   uint64;  // unsigned 8 byte integer

#endif //ACV_PRIMITIVE_TYPES_DEFINED

#ifdef HAVE_MPBASE
#include "ammem.h"
#define ACV_MMemAlloc(h, size) MMemAlloc(h, size)
#define ACV_MMemAlloc(h, pointer) MMemFree(h, pointer)
#else
#include <stdlib.h>
typedef void* MHandle;
#define ACV_MMemAlloc(h, size) malloc(size)
#define ACV_MMemFree(h, pointer) free(pointer)
#endif

#define ACV_8U   0
#define ACV_8S   1
#define ACV_16U  2
#define ACV_16S  3
#define ACV_32S  4
#define ACV_32F  5
#define ACV_64F  6
#define ACV_USRTYPE1 7

#define ACV_CN_MAX     512
#define ACV_CN_SHIFT   3
#define ACV_DEPTH_MAX  (1 << ACV_CN_SHIFT)

#define ACV_MAT_DEPTH_MASK       (ACV_DEPTH_MAX - 1)
#define ACV_MAT_DEPTH(flags)     ((flags) & ACV_MAT_DEPTH_MASK)

#define ACV_MAKETYPE(depth,cn) (ACV_MAT_DEPTH(depth) + (((cn)-1) << ACV_CN_SHIFT))
#define ACV_MAKE_TYPE ACV_MAKETYPE

#define ACV_8UC1 ACV_MAKETYPE(ACV_8U,1)
#define ACV_8UC2 ACV_MAKETYPE(ACV_8U,2)
#define ACV_8UC3 ACV_MAKETYPE(ACV_8U,3)
#define ACV_8UC4 ACV_MAKETYPE(ACV_8U,4)
#define ACV_8UC(n) ACV_MAKETYPE(ACV_8U,(n))

#define ACV_8SC1 ACV_MAKETYPE(ACV_8S,1)
#define ACV_8SC2 ACV_MAKETYPE(ACV_8S,2)
#define ACV_8SC3 ACV_MAKETYPE(ACV_8S,3)
#define ACV_8SC4 ACV_MAKETYPE(ACV_8S,4)
#define ACV_8SC(n) ACV_MAKETYPE(ACV_8S,(n))

#define ACV_16UC1 ACV_MAKETYPE(ACV_16U,1)
#define ACV_16UC2 ACV_MAKETYPE(ACV_16U,2)
#define ACV_16UC3 ACV_MAKETYPE(ACV_16U,3)
#define ACV_16UC4 ACV_MAKETYPE(ACV_16U,4)
#define ACV_16UC(n) ACV_MAKETYPE(ACV_16U,(n))

#define ACV_16SC1 ACV_MAKETYPE(ACV_16S,1)
#define ACV_16SC2 ACV_MAKETYPE(ACV_16S,2)
#define ACV_16SC3 ACV_MAKETYPE(ACV_16S,3)
#define ACV_16SC4 ACV_MAKETYPE(ACV_16S,4)
#define ACV_16SC(n) ACV_MAKETYPE(ACV_16S,(n))

#define ACV_32SC1 ACV_MAKETYPE(ACV_32S,1)
#define ACV_32SC2 ACV_MAKETYPE(ACV_32S,2)
#define ACV_32SC3 ACV_MAKETYPE(ACV_32S,3)
#define ACV_32SC4 ACV_MAKETYPE(ACV_32S,4)
#define ACV_32SC(n) ACV_MAKETYPE(ACV_32S,(n))

#define ACV_32FC1 ACV_MAKETYPE(ACV_32F,1)
#define ACV_32FC2 ACV_MAKETYPE(ACV_32F,2)
#define ACV_32FC3 ACV_MAKETYPE(ACV_32F,3)
#define ACV_32FC4 ACV_MAKETYPE(ACV_32F,4)
#define ACV_32FC(n) ACV_MAKETYPE(ACV_32F,(n))

#define ACV_64FC1 ACV_MAKETYPE(ACV_64F,1)
#define ACV_64FC2 ACV_MAKETYPE(ACV_64F,2)
#define ACV_64FC3 ACV_MAKETYPE(ACV_64F,3)
#define ACV_64FC4 ACV_MAKETYPE(ACV_64F,4)
#define ACV_64FC(n) ACV_MAKETYPE(ACV_64F,(n))


#define ACV_MAT_CONT_FLAG_SHIFT  14
#define ACV_MAT_CONT_FLAG        (1 << ACV_MAT_CONT_FLAG_SHIFT)
#define ACV_SUBMAT_FLAG_SHIFT    15
#define ACV_SUBMAT_FLAG          (1 << ACV_SUBMAT_FLAG_SHIFT)

#define ACV_MAT_CN_MASK          ((ACV_CN_MAX - 1) << ACV_CN_SHIFT)
#define ACV_MAT_CN(flags)        ((((flags) & ACV_MAT_CN_MASK) >> ACV_CN_SHIFT) + 1)
#define ACV_MAT_TYPE_MASK        (ACV_DEPTH_MAX*ACV_CN_MAX - 1)
#define ACV_MAT_TYPE(flags)      ((flags) & ACV_MAT_TYPE_MASK)

#define ACV_IS_MAT_CONT(flags)   ((flags) & ACV_MAT_CONT_FLAG)
#define ACV_IS_CONT_MAT          ACV_IS_MAT_CONT

#define ACV_IS_SUBMAT(flags)     ((flags) & ACV_MAT_SUBMAT_FLAG)

/** Size of each channel item,
0x124489 = 1000 0100 0100 0010 0010 0001 0001 ~ array of sizeof(arr_type_elem) */
#define ACV_ELEM_SIZE1(type) \
    ((((sizeof(size_t)<<28)|0x8442211) >> ACV_MAT_DEPTH(type)*4) & 15)

/** 0x3a50 = 11 10 10 01 01 00 00 ~ array of log2(sizeof(arr_type_elem)) */
#define ACV_ELEM_SIZE(type) \
    (ACV_MAT_CN(type) << ((((sizeof(size_t)/4+1)*16384|0x3a50) >> ACV_MAT_DEPTH(type)*2) & 3))

#define NUM_DIM 4

#endif // HAVE_ACVCORE



#endif //__ACV_OCL_BASE_DEFINITION_H__