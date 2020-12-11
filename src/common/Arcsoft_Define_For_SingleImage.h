/*
 * brief: 
 */
#ifndef _DEFINE_H
#define _DEFINE_H

//#define CALCULATE_TIME 1

//define namespace
#define NS_SINFLE_IMAGE_ENHANCEMENT_OCL_BEGIN   namespace acv { namespace ocl {
#define NS_SINFLE_IMAGE_ENHANCEMENT_OCL_END     } }
#define USING_NS_SINFLE_IMAGE_ENHANCEMENT   using namespace acv::ocl;

//define multi thread
#define MULTI_THREAD
#define MCV_MULTI_THREAD

//malloc memory
#undef SAFE_MALLOC
#define SAFE_MALLOC(context, dataType, length)  (dataType*)MMemAlloc(context, (length)*sizeof(dataType))

//safe release memory
#undef SAFE_DELETE
#define SAFE_DELETE(x) if((x)!=nullptr){ delete (x); (x)=nullptr; }

#undef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) if((x)!=nullptr){ delete[] (x); (x)=nullptr; }

#undef SAFE_FREE_ARRAY
#define SAFE_FREE_ARRAY(context, x) if((x)!=nullptr){ MMemFree(context, (x)); (x)=nullptr; }

// check func state
#ifndef CHECK_ERROR
#define CHECK_ERROR(code)	if(MOK != (code)) { goto exit; }
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
