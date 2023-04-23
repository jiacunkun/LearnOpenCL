/*
 * brief: 各平台的公共宏定义
 */
#ifndef _DEFINE_H
#define _DEFINE_H

#include <ammem.h>
#include <merror.h>
#include "BasicTimer.h"
#include "ArcsoftLog.h"

 //#define INITOCL_FROM_SOURCE // ocl是否采用源码编译

 /******************************定义OpenMP*************************************/
#define NH_OMP_THREAD_NUM (8)
// #define ACV_ENABLE_OCL
#define ASYNC_WARP_AND_FUS
#define ENABLE_DEGHOST

#ifdef _OPENMP
#include "omp.h"
#define NH_ENABLE_OPENMP
#endif

/******************************统计时间*************************************/
//#define CALCULATE_TIME

#if CALCULATE_TIME
#define START_TIME \
            BasicTimer time;\
            LOGD("%s++", __FUNCTION__);

#define END_TIME \
            LOGD("%s[%d]: is finished timer count = %fms!", __FUNCTION__, __LINE__, time.UpdateAndGetDelta());\
            LOGD("%s--\n", __FUNCTION__);

#else
#define START_TIME
#define END_TIME
#endif

/******************************neon头文件*************************************/
#if defined(__ANDROID__)||defined(ANDROID)
//打开neon
#define USE_NEON
#define __ARM_NEON__
#define _ARM_NEON_
#include "arm_neon.h"
#else
#define USE_NEON
//#define _ARM_NEON_
#include "win/NEON_2_SSE.h"
#endif




/******************************命名空间宏定义*************************************/
#define NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN   namespace Single_Image_Enhancement_FOR_SR {
#define NS_SINFLE_IMAGE_ENHANCEMENT_END     }
#define USING_NS_SINFLE_IMAGE_ENHANCEMENT   using namespace Single_Image_Enhancement_FOR_SR;

#define NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN   namespace acv { namespace ocl {
#define NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END     } }
#define USING_NS_SINFLE_IMAGE_ENHANCEMENT_OCL   using namespace acv::ocl;


/******************************定义多线程*************************************/
#define MULTI_THREAD
#define MCV_MULTI_THREAD

#include <omp.h>

/******************************定义数据结构*************************************/
struct Rect
{
    MInt32 x;
    MInt32 y;
    MInt32 lWidth;
    MInt32 lHeight;
};

/******************************内存管理*************************************/
//创建
#undef SAFE_MALLOC
#define SAFE_MALLOC(context, dataType, length)  (dataType*)MMemAlloc(context, (length)*sizeof(dataType))

//安全释放内存
#undef SAFE_DELETE
#define SAFE_DELETE(x) if((x)!=nullptr){ delete (x); (x)=nullptr; }

#undef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) if((x)!=nullptr){ delete[] (x); (x)=nullptr; }

#undef SAFE_FREE_ARRAY
#define SAFE_FREE_ARRAY(context, x) if((x)!=nullptr){ MMemFree(context, (x)); (x)=nullptr; }


/******************************检查函数状态*************************************/
#ifndef CHECK_ERROR
#define CHECK_ERROR(code)	if(0 != (code)) { LOGD("%s is fail!", __FUNCTION__); return code; }
#endif

#ifndef CHECK_MEMORY
#define CHECK_MEMORY(ptr)	if(MNull == (ptr)) { LOGD("new memory is fail!"); return MERR_NO_MEMORY; }
#endif

#ifndef CheckFuncStatus
#define CheckFuncStatus(string, err)                                               \
    {                                                                              \
        if (0 != err)                                                          \
        {                                                                          \
            LOGE("[Error=%d][%s][%d],run %s failed\n", err, __FILE__, __LINE__, string); \
            return err;                                                            \
        }                                                                          \
    }
#endif

/******************************数据处理*************************************/
// 绝对值
#ifndef	ABS
#define ABS(x)		((x) > 0 ? (x) : -(x))
#endif

#ifndef	MAX
#define MAX(min, x)		((x) > min ? (x) : min)
#endif

#ifndef	MIN
#define MIN(max, x)		((x) < max ? (x) : max)
#endif

#ifndef	ROUND
#define ROUND(x)		((x) > 0 ? (x)+0.5 : (x)-0.5)
#endif

// 限制数据范围
#ifndef CLAMP
#define CLAMP(x, min, max)          \
    {                               \
        ((x) = (x) > min ? (x) : min);       \
        ((x) = (x) < max ? (x) : max);       \
    }
#endif

#ifndef TRIMBYTE_255
#define TRIMBYTE_255(x)	((x) < 0)?(0) : ((x) > 255 ? 255 :(x))
#endif

#ifndef TRIMBYTE_1020
#define TRIMBYTE_1020(x)	((x) < 0)?(0) : ((x) > 1020 ? 1020 :(x))
#endif

#ifndef TRIMBYTE
#define TRIMBYTE(x)	(MByte)( (x) < 0 ? 0  : ( (x) > 255 ? 255 : (x) ) )
#endif
/****************************************************************************************\
                                plaform detection
\****************************************************************************************/
//#pragma mark - plaform detection
// WIN
#if defined(_WIN32) || defined(_WIN32_) || defined(WIN32) || defined(_WIN64_) || defined(WIN64) || defined(_WIN64)
#   define PLATFORM_WINDOWS 1


#   if _MSC_VER < 1600 // before Visual Studio 2010 have not <stdint.h>
#       define STDINT_MISSING   1
#   endif

// ANDROIDELF
#elif defined(ANDROIDELF)
#   define PLATFORM_ANDROIDELF 1
#   define PLATFORM_ANDROID 1

// ANDROID
#elif defined(ANDROID) || defined(_ANDROID_)
#   define PLATFORM_ANDROID 1
#   if !defined(__clang__) && (GCC_VERSION < 40500) // test if GCC > 4.5.0 https://gcc.gnu.org/c99status.html
#       define STDINT_MISSING   1
#   endif

// ios && MAC
#elif defined(__APPLE__)
// macro define of TARGET_OS_IPHONE, TARGET_OS_SIMULATOR, TARGET_CPU_ARM, TARGET_CPU_ARM64 etc
#   include <TargetConditionals.h>

#   undef PLATFORM_IOS
#   if TARGET_IPHONE_SIMULATOR
#       define PLATFORM_IOS     1              // iOS Simulator
#       define PLATFORM_IOS_SIMULATOR     1    // iOS Simulator
#   elif TARGET_OS_IPHONE
#       define PLATFORM_IOS     1              // iOS device
#       define PLATFORM_IOS_DEVICE     1       // iOS device
#   elif TARGET_OS_MAC
#       define PLATFORM_MAC     1              // Other kinds of Mac OS
#       define PLATFORM_OSX     1
#   else
#       error "Unknown Apple platform"
#   endif

// UNIX
#elif defined(__unix__) || defined(__unix) || defined(unix)
#   define PLATFORM_UNIX   1

// LINUX
#elif defined(__linux__)  || defined(linux) || defined(__linux)
#   define PLATFORM_LINUX   1

// FreeBSD
#elif defined(__FreeBSD__)
#   define PLATFORM_FreeBSD   1

// NetBSD
#elif defined(__NetBSD__)
#   define PLATFORM_NetBSD   1

// unknown
#else
#error  "unknown platfom"
#   define PLATFORM_UNKNOWN 1
#   define STDINT_MISSING   1
#endif


#endif //_DEFINE_H
