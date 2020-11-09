/**
 * @Author: [lq8@meitu.com] <lynch>
 * @Date:   2014-08-28T16:09:18+08:00
 * @Email:  lq8@meitu.com
 * @Filename: CLUtility.h
 * @Last modified by:   lynch
 * @Last modified time: 2019-02-01T09:49:40+08:00
 */



#pragma once

#include "CLLog.h"

#include <stdexcept>
#include <string>
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace mtcl
{

#define OPENCL_ERROR_CASE(ERROR) case ERROR: return #ERROR;

inline std::string getStringFromError(cl_int error)
{
	switch(error)
	{
		OPENCL_ERROR_CASE(CL_SUCCESS)
		OPENCL_ERROR_CASE(CL_DEVICE_NOT_FOUND)
		OPENCL_ERROR_CASE(CL_DEVICE_NOT_AVAILABLE)
		OPENCL_ERROR_CASE(CL_COMPILER_NOT_AVAILABLE)
		OPENCL_ERROR_CASE(CL_MEM_OBJECT_ALLOCATION_FAILURE)
		OPENCL_ERROR_CASE(CL_OUT_OF_RESOURCES)
		OPENCL_ERROR_CASE(CL_OUT_OF_HOST_MEMORY)
		OPENCL_ERROR_CASE(CL_PROFILING_INFO_NOT_AVAILABLE)
		OPENCL_ERROR_CASE(CL_MEM_COPY_OVERLAP)
		OPENCL_ERROR_CASE(CL_IMAGE_FORMAT_MISMATCH)
		OPENCL_ERROR_CASE(CL_IMAGE_FORMAT_NOT_SUPPORTED)
		OPENCL_ERROR_CASE(CL_BUILD_PROGRAM_FAILURE)
		OPENCL_ERROR_CASE(CL_MAP_FAILURE)
		OPENCL_ERROR_CASE(CL_MISALIGNED_SUB_BUFFER_OFFSET)
		OPENCL_ERROR_CASE(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST)
		OPENCL_ERROR_CASE(CL_INVALID_VALUE)
		OPENCL_ERROR_CASE(CL_INVALID_DEVICE_TYPE)
		OPENCL_ERROR_CASE(CL_INVALID_PLATFORM)
		OPENCL_ERROR_CASE(CL_INVALID_DEVICE)
		OPENCL_ERROR_CASE(CL_INVALID_CONTEXT)
		OPENCL_ERROR_CASE(CL_INVALID_QUEUE_PROPERTIES)
		OPENCL_ERROR_CASE(CL_INVALID_COMMAND_QUEUE)
		OPENCL_ERROR_CASE(CL_INVALID_HOST_PTR)
		OPENCL_ERROR_CASE(CL_INVALID_MEM_OBJECT)
		OPENCL_ERROR_CASE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
		OPENCL_ERROR_CASE(CL_INVALID_IMAGE_SIZE)
		OPENCL_ERROR_CASE(CL_INVALID_SAMPLER)
		OPENCL_ERROR_CASE(CL_INVALID_BINARY)
		OPENCL_ERROR_CASE(CL_INVALID_BUILD_OPTIONS)
		OPENCL_ERROR_CASE(CL_INVALID_PROGRAM)
		OPENCL_ERROR_CASE(CL_INVALID_PROGRAM_EXECUTABLE)
		OPENCL_ERROR_CASE(CL_INVALID_KERNEL_NAME)
		OPENCL_ERROR_CASE(CL_INVALID_KERNEL_DEFINITION)
		OPENCL_ERROR_CASE(CL_INVALID_KERNEL)
		OPENCL_ERROR_CASE(CL_INVALID_ARG_INDEX)
		OPENCL_ERROR_CASE(CL_INVALID_ARG_VALUE)
		OPENCL_ERROR_CASE(CL_INVALID_ARG_SIZE)
		OPENCL_ERROR_CASE(CL_INVALID_KERNEL_ARGS)
		OPENCL_ERROR_CASE(CL_INVALID_WORK_DIMENSION)
		OPENCL_ERROR_CASE(CL_INVALID_WORK_GROUP_SIZE)
		OPENCL_ERROR_CASE(CL_INVALID_WORK_ITEM_SIZE)
		OPENCL_ERROR_CASE(CL_INVALID_GLOBAL_OFFSET)
		OPENCL_ERROR_CASE(CL_INVALID_EVENT_WAIT_LIST)
		OPENCL_ERROR_CASE(CL_INVALID_EVENT)
		OPENCL_ERROR_CASE(CL_INVALID_OPERATION)
		OPENCL_ERROR_CASE(CL_INVALID_GL_OBJECT)
		OPENCL_ERROR_CASE(CL_INVALID_BUFFER_SIZE)
		OPENCL_ERROR_CASE(CL_INVALID_MIP_LEVEL)
		OPENCL_ERROR_CASE(CL_INVALID_GLOBAL_WORK_SIZE)
		OPENCL_ERROR_CASE(CL_INVALID_PROPERTY)
		default: return "OPENCL Unknown Error Code";
	}
}

#undef OPENCL_ERROR_CASE

//***************************************************************************
//opencl错误检测宏定义
//***************************************************************************
// #define EXIT(err) throw std::runtime_error(getStringFromError(err))
#define EXIT(err)
//检查返回值

#define CheckError(string, err) { \
    if(CL_SUCCESS != err) { \
        LOGE("[err][%s][%d], %s failed : err = %d(%s)\n", __FILE__, __LINE__, string, err, getStringFromError(err).c_str()); \
        EXIT(err); \
    } \
	else { \
        LOGI("[succeed] %s succeed\n", string); \
    } \
}

//检查返回值并打印字符串
#undef CheckErrorWithString
#define CheckErrorWithString(info, string, err) { \
    if(CL_SUCCESS != err) { \
        LOGE("[err][%s][%d], %s failed : err = %d(%s)\n", __FILE__, __LINE__, info, err, getStringFromError(err).c_str()); \
        EXIT(err); \
    } \
	else { \
        LOGI("[succeed] %s = %s\n", info, string); \
    } \
}

//检查返回值并打印变量
#undef CheckErrorWithVariable
#define CheckErrorWithVariable(info, variable, err) { \
    if(CL_SUCCESS != err) { \
        LOGE("[err][%s][%d] %s failed : err = %d(%s)\n", __FUNCTION__, __LINE__, info, err, getStringFromError(err).c_str()); \
        EXIT(err); \
    } \
	else { \
        LOGI("[succeed] %s = %lu\n", info, (unsigned long)(variable)); \
    } \
}

//检查返回值并打印变量
#undef CheckErrorWithBool
#define CheckErrorWithBool(info, bool_val, err) { \
    if(CL_SUCCESS != err) { \
        LOGE("[err][%s][%d] %s failed : err = %d(%s)\n", __FUNCTION__, __LINE__, info, err, getStringFromError(err).c_str()); \
        EXIT(err); \
    } \
	else { \
        LOGI("[succeed] %s = %s\n", info, bool_val == true ? "true" : "false"); \
    } \
}

//检查返回值并打印对象ID
#undef CheckErrorWithID
#define CheckErrorWithID(info, idname, ID, err) { \
    if(CL_SUCCESS != err || ID == NULL) { \
        LOGE("[err][%s][%d] %s failed : err = %d(%s)\n", __FUNCTION__, __LINE__, info, err, getStringFromError(err).c_str()); \
        EXIT(err); \
    } \
	else { \
        LOGI("[succeed] %s succeed : %s is %d\n", info, idname, *(unsigned char*)(&ID)); \
    } \
}

inline bool checkError(cl_int err)
{
	if(CL_SUCCESS != err)
	{
		LOGE("[%s][%d] CL Run Time Error %s\n", __FUNCTION__, __LINE__, getStringFromError(err).c_str());
		EXIT(err);
		return false;
	}
	else
	{
		return true;
	}
}

template<class T>
struct type2format {
};

#define OPENCL_TYPE2DEFINE(T, VALUE)                                \
template<>                                                          \
struct type2format<T> {                                             \
    static const cl_channel_type type = VALUE;                      \
    static const cl_channel_order order = CL_R;                     \
};                                                                  \
template<>                                                          \
struct type2format<T##2> {                                          \
    static const cl_channel_type type = VALUE;                      \
    static const cl_channel_order order = CL_RG;                    \
};                                                                  \
template<>                                                          \
struct type2format<T##4> {                                          \
    static const cl_channel_type type = VALUE;                      \
    static const cl_channel_order order = CL_RGBA;                  \
};                                                                  \


OPENCL_TYPE2DEFINE(cl_char, CL_SIGNED_INT8)
OPENCL_TYPE2DEFINE(cl_short, CL_SIGNED_INT16)
OPENCL_TYPE2DEFINE(cl_int, CL_SIGNED_INT32)
OPENCL_TYPE2DEFINE(cl_uchar, CL_UNSIGNED_INT8)
OPENCL_TYPE2DEFINE(cl_ushort, CL_UNSIGNED_INT16)
OPENCL_TYPE2DEFINE(cl_uint, CL_UNSIGNED_INT32)
OPENCL_TYPE2DEFINE(cl_float, CL_FLOAT)

#undef OPENCL_TYPE2DEFINE

}
