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
* @brief AutoBuffer
*
* @author Lei Hua
* @date 2017-11-08
*/

#ifndef __ACV_AUTO_BUFER_H__
#define __ACV_AUTO_BUFER_H__

#include "acvdef.h"
#include <string.h>

namespace acv {

	/** @brief  Automatically Allocated Buffer

	The class create a small fixed-size array on stack and use it if it's large enough. If the required buffer size
	is larger than the fixed size, another buffer of sufficient size is allocated dynamically
	and released after the processing by new/delete operators. The size of array on stack is set by the template parameter fixed_size.
		
	Here is some examples:
	\code
	void my_func(int m)
	{
	   acv::AutoBuffer<float, 10> buffer1; // which is euqal to float buffer1[10];
	   buffer1.allocate(100);              // 100 > 10, so another buffer containing 100 floats is allocated dynamicly
	   
	   acv::AutoBuffer<float, 10> buffer2(100); // it is same as buffer1
	   
	    ...
	}
	\endcode
	*/
	template<typename _Tp, size_t fixed_size = 1024 / sizeof(_Tp) + 8> 
	class AutoBuffer
	{
	public:
		typedef _Tp value_type;
		//! the default constructor
		AutoBuffer()
		{		
			ptr_ = buffer_;
			size_ = fixed_size;
		}

		//! constructor taking the real buffer size
		AutoBuffer(size_t _size)
		{			
			ptr_ = buffer_;
			size_ = fixed_size;
			allocate(_size);
		}

		//! destructor. calls deallocate()
		~AutoBuffer()
		{
			deallocate();			
		}

		//! allocates the new buffer of size _size. if the _size is small enough, stack-allocated buffer is used
		void allocate(size_t size)
		{
			if (size <= size_)
			{
				size_ = size;
				return;
			}
			deallocate();
			size_ = size;
			if (size > fixed_size)
			{
				ptr_ = new _Tp[size];				
			}
		}

		//! deallocates the buffer if it was dynamically allocated
		void deallocate()
		{
			if (ptr_ != buffer_)
			{
				delete[] ptr_;				
				ptr_ = buffer_;
				size_ = fixed_size;
			}
		}

		//! resizes the buffer and preserves the content
		void resize(size_t size)
		{
			if (size <= size_)
			{
				size_ = size;
				return;
			}
			size_t i, prevsize = size_, minsize = prevsize > size ? size : prevsize;
			_Tp* prevptr_ = ptr_;
			
            size_ = size;
			ptr_ = size > fixed_size ? (_Tp*)new _Tp[size_] : buffer_;
			

			if (ptr_ != prevptr_)
				for (i = 0; i < minsize; i++)
					ptr_[i] = prevptr_[i];
			for (i = prevsize; i < size; i++)
				ptr_[i] = _Tp();

			if (prevptr_ != buffer_)
			{
				delete[] prevptr_;				
			}

		}

		void setZeros()
		{
			if (ptr_)
			{
				memset(ptr_, 0, sizeof(ptr_[0])*size());
			}
		}

		//! returns the current buffer size
		size_t size() const { return size_; }

		//! returns pointer to the real buffer, stack-allocated or head-allocated
		operator _Tp* () { return ptr_; }

		//! returns read-only pointer to the real buffer, stack-allocated or head-allocated
		operator const _Tp* () const { return ptr_;	}
		
		
	protected:
	
		//! pointer to the real buffer, can point to buffer_ if the buffer is small enough
		_Tp* ptr_;
		//! size of the real buffer
		size_t size_;
		//! pre-allocated buffer. At least 1 element to confirm C++ standard reqirements
		_Tp buffer_[(fixed_size > 0) ? fixed_size : 1];

		
		//! disallow copy and assign
		AutoBuffer(const AutoBuffer<_Tp, fixed_size>& buffer_);	
		AutoBuffer<_Tp, fixed_size>& operator = (const AutoBuffer<_Tp, fixed_size>& buffer_);
	};

	/** @brief  Automatically Allocated Buffer by Mpbase library

	The class create a small fixed-size array on stack and use it if it's large enough. If the required buffer size
	is larger than the fixed size, another buffer of sufficient size is allocated dynamically
	and released after the processing by MMemAlloc/MMemFree operators. The size of array on stack is set by the template parameter fixed_size.
	
	Note: when _Tp is a class, it's constructor and destructor are not called during allocating and deallocating operation.

	Here is some examples:
	\code
	void my_func(int m)
	{
	    void* pMem = malloc(10*1024*1024);
	    MHandle handle = MMemMgrCreate(pMem, 10*1024*1024);
	    acv::AutoMpbaseBuffer<float, 10> buffer3(handle);    // with 10 floats stack buffer.
	    buffer3.allocate(100);                         // allocate 100 floats in handle by MMemAlloc. It will be deallocated by MMemFree when buffer3 is destroyed.
	    
	    MMemMgrDestroy(handle);
	    free(pMem);
	    ...
	}
	\endcode
	*/
	template<typename _Tp, size_t fixed_size = 1024 / sizeof(_Tp) + 8>
	class AutoMpbaseBuffer
	{
	public:
		typedef _Tp value_type;

		// static test whether _Tp is a class
		//typedef struct { char a[2]; } Two;
		//template<typename C> static char test(int C::*);
		//template<typename C> static Two test(...);		

		//template <int x> struct IT_IS_NOT_A_CLASS;
		//template <> struct IT_IS_NOT_A_CLASS<2> { enum { value = 1 }; };
		//template<int x> struct is_class_test{};
		//
		//// if there is a compiling error, _Tp would be a class. AutoMpbaseBuffer can't accept a class
		//typedef is_class_test<sizeof(IT_IS_NOT_A_CLASS<sizeof(AutoMpbaseBuffer<_Tp, fixed_size>::test<_Tp>(0))>)> _Tp_is_not_class;

		//! the default constructor
		explicit AutoMpbaseBuffer(MHandle handle):handle_(handle)
		{
			ptr_ = buffer_;
			size_ = fixed_size;
		}

		//! constructor taking the real buffer size
		AutoMpbaseBuffer(size_t _size, MHandle handle):handle_(handle)
		{
			ptr_ = buffer_;
			size_ = fixed_size;
			allocate(_size);
		}

		//! destructor. calls deallocate()
		~AutoMpbaseBuffer()
		{
			deallocate();
		}

		//! allocates the new buffer of size _size. if the _size is small enough, stack-allocated buffer is used
		void allocate(size_t size)
		{
			if (size <= size_)
			{
				size_ = size;
				return;
			}
			deallocate();
			size_ = size;
			if (size > fixed_size)
			{
				ptr_ = (_Tp*) ACV_MMemAlloc(handle_, size*sizeof(_Tp));
			}
		}

		//! deallocates the buffer if it was dynamically allocated
		void deallocate()
		{
			if (ptr_ != buffer_)
			{
				ACV_MMemFree(handle_, ptr_);
				ptr_ = buffer_;
				size_ = fixed_size;
			}
		}

		//! resizes the buffer and preserves the content
		void resize(size_t size)
		{
			if (size <= size_)
			{
				size_ = size;
				return;
			}
			size_t i, prevsize = size_, minsize = prevsize > size ? size : prevsize;
			_Tp* prevptr_ = ptr_;

			ptr_ = size > fixed_size ? (_Tp*)ACV_MMemAlloc(handle_, size * sizeof(_Tp)) : buffer_;
			size_ = size;

			if (ptr_ != prevptr_)
				for (i = 0; i < minsize; i++)
					ptr_[i] = prevptr_[i];
			for (i = prevsize; i < size; i++)
				ptr_[i] = _Tp();

			if (prevptr_ != buffer_)
			{
				MMemFree(handle_, prevptr_);			
			}

		}

		//! returns the current buffer size
		size_t size() const { return size_; }

		//! returns pointer to the real buffer, stack-allocated or head-allocated
		operator _Tp* () { return ptr_; }

		//! returns read-only pointer to the real buffer, stack-allocated or head-allocated
		operator const _Tp* () const { return ptr_; }


	protected:

		//! pointer to the real buffer, can point to buffer_ if the buffer is small enough
		_Tp* ptr_;
		//! size of the real buffer
		size_t size_;
		//! pre-allocated buffer. At least 1 element to confirm C++ standard reqirements
		_Tp buffer_[(fixed_size > 0) ? fixed_size : 1];

		//! memory handle from Mpbase library
		MHandle handle_;


		//! disallow copy and assign
		AutoMpbaseBuffer(const AutoMpbaseBuffer<_Tp, fixed_size>& buffer_);
		AutoMpbaseBuffer<_Tp, fixed_size>& operator = (const AutoMpbaseBuffer<_Tp, fixed_size>& buffer_);
	};	



	

	
};


#endif //