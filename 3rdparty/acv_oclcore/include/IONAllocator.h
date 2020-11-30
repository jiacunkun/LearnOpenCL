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

#ifndef __ACV_ION_BUFFER_ALLOCATOR_H__
#define __ACV_ION_BUFFER_ALLOCATOR_H__


#include "CL/cl.h"
#include <memory>
#include "IONBufferInfo.h"

#include "CLConfig.h"

namespace acv {
	namespace ocl {
		
		struct ACV_EXPORTS IONBufferDeletor
		{
			void operator()(IONBufferInfo* buffer);
		};

		/** @brief allocator of ION buffer */
		class ACV_EXPORTS IONAllocator
		{
		public:

			IONAllocator();
			virtual ~IONAllocator();
			
			IONBufferInfo* allocate(size_t size) const;
			void deallocate(IONBufferInfo* buffer) const;

			void setHeadID(int head_id) { head_id_ = head_id; }
			void setAlignment(int alignment) { alignment_ = alignment; }
		protected:

			mutable int device_handle_;
			int head_id_;
			int alignment_;
			ION_BUFFER_TYPE buffer_type_;
			mutable IONBufferDeletor buffer_deletor_;
		};

		/** @brief allocator of ION buffer for QUALCOMM GPU*/
		class ACV_EXPORTS QCOM_IONAllocator :public IONAllocator
		{
		public:
			QCOM_IONAllocator();
			virtual ~QCOM_IONAllocator() {}
		};

		/** @brief allocator of ION buffer for Mali GPU*/
		class ACV_EXPORTS ARM_IONAllocator :public IONAllocator
		{
		public:
			ARM_IONAllocator();
			virtual ~ARM_IONAllocator() {}
		};

		/** create an IONAllocator according to the cl_device_id
		*   it detects the GPU type [QUALCOMM GPU, Mali GPU or other] and create an IONAllocator [QCOM_IONAllocator, ARM_IONAllocator or IONAllocator]
		*/
		std::shared_ptr< IONAllocator> createIONAllocator(cl_device_id device_id);

		
	}
}



#endif //__ACV_ION_BUFFER_ALLOCATOR_H__