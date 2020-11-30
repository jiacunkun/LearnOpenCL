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
* @brief Allocators for acv::Mat and acv::Image
*
* @author Lei Hua
* @date 2017-11-08
*/

#ifndef __ACV_ALLOCATOR_H__
#define __ACV_ALLOCATOR_H__

#include <assert.h>
#include "acvdef.h"

#define ACV_MALLOC_ALIGN 64

namespace acv {
	/**@brief allocator interface for allocating memory

	Allocator is designed to allocate/deallocate memory for acv::Mat and acv::Image. Call setDefaultAllocator to
	set the default Allocator for both acv::Mat and acv::Image.

	@see StdAllocator, AlignedAllocator and MpbaseAllocator.
	*/
	class ACV_EXPORTS Allocator
	{
	public:
		Allocator();
		virtual ~Allocator();
		virtual void* allocate(size_t size) const = 0;
		virtual void deallocate(void* buffer) const = 0;
	private:
	};

	/**@brief stdandard allocator

	Allocate/deallocate memory by malloc/free
	*/
	class ACV_EXPORTS StdAllocator : public Allocator
	{
	public:
		StdAllocator();
		virtual ~StdAllocator();
		virtual void* allocate(size_t size) const;
		virtual void deallocate(void* buffer) const;
	private:

	};

	/**@brief aligned allocator

	Allocate/deallocate memory by malloc/free. But the returned memory pointer is aligned by ACV_MALLOC_ALIGN
	*/
	class ACV_EXPORTS AlignedAllocator : public Allocator
	{
	public:
		AlignedAllocator(int alignmnet = ACV_MALLOC_ALIGN);
		void setPtrAlignment(int alignment);
		virtual ~AlignedAllocator();
		virtual void* allocate(size_t size) const;
		virtual void deallocate(void* buffer) const;
	private:
		int ptr_alignment_;
	};

	/**@brief allocator based on mpbase library
	* Allocate/deallocate memory by MMemAlloc/MMemFree.
	* @note If one want to use mpbase remember to define HAVE_MPBASE
	*/
	class ACV_EXPORTS MpbaseAllocator : public Allocator
	{
	public:
		explicit MpbaseAllocator(const MHandle handle) :handle_(handle) {}
		virtual ~MpbaseAllocator() {}
		virtual void* allocate(size_t size) const  // defined as inline function to avoid the denpendency on mpbase library
		{
			(void)handle_; // avoid no used warning
			return ACV_MMemAlloc(handle_, size);
		}
		virtual void deallocate(void* buffer) const // defined as inline function to avoid the denpendency on mpbase library
		{
			ACV_MMemFree(handle_, buffer);
		}
	private:
		MHandle handle_;
	};



	//* allocate memory by malloc but the returned memory pointer is aligned by ACV_MALLOC_ALIGN
	ACV_EXPORTS void* fastMalloc(size_t size, int n = ACV_MALLOC_ALIGN);
	//! free memory pointer which is from fastMalloc
	ACV_EXPORTS void fastFree(void* ptr);

	/** @brief Aligns a pointer to the specified number of bytes.

	The function returns the aligned pointer of the same type as the input pointer:
	\f[\texttt{(_Tp*)(((size_t)ptr + n-1) & -n)}\f]
	@param ptr Aligned pointer.
	@param n Alignment size that must be a power of two.
	*/
	template<typename _Tp> static inline _Tp* alignPtr(_Tp* ptr, int n = (int)sizeof(_Tp))
	{
		assert((n & (n - 1)) == 0); // n is a power of 2
		return (_Tp*)(((size_t)ptr + n - 1) & -n);
	}

	ACV_EXPORTS size_t alignMemorySize(size_t sz, int n);


	//ACV_EXPORTS Allocator* getDefaultAlignedAllocator();

	class ACV_EXPORTS DefaultAlignedAllocator
	{
	public:
		static AlignedAllocator* allocator();		
	private:
		DefaultAlignedAllocator() {}
	
		AlignedAllocator aligned_allocator_;
		static DefaultAlignedAllocator* instance_;
	};


	
};


#endif //__ACV_ALLOCATOR_H__
