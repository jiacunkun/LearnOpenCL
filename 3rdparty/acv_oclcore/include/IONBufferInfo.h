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

/** @file
* @brief C++ wrapper for opencl routines
*
* @author Lei Hua
* @date 2019-03-19
*/

#ifndef __ACV_OCL_ION_BUFFER_INFOMATION_H__
#define __ACV_OCL_ION_BUFFER_INFOMATION_H__

namespace acv {
	namespace ocl {
		enum ION_BUFFER_TYPE
		{
			ION_TYPE_NORMAL = 0,      //< system buffer
			ION_TYPE_QUALCOMM = 1,    //< ION buffer for QUALCOMM GPU
			ION_TYPE_ARM = 2          //< ION buffer for Mali GPU
		};
		struct IONBufferInfo
		{
			IONBufferInfo() :ptr(NULL), size(0), fd(-1), buffer_type(ION_TYPE_NORMAL) {}
			IONBufferInfo(void* _ptr, size_t _size, int _fp, ION_BUFFER_TYPE _type) :
				ptr(_ptr), size(_size), fd(_fp), buffer_type(_type) {}
			void *ptr;
			size_t size;
			int fd;
			ION_BUFFER_TYPE buffer_type;  // QUALCOMM_ION or ARM_ION or NORMAL
		};
	}
}


#endif // __ACV_OCL_ION_BUFFER_INFOMATION_H__