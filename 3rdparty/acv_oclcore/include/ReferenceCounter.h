/**
*
* @author Lei Hua
* @date 2019-12-10
*/
#ifndef __ACV_OCL_REFERENCE_COUNTER_H__
#define __ACV_OCL_REFERENCE_COUNTER_H__

#include "ocl_def.h"

#ifdef ACV_XADD
// allow to use user-defined macro
#elif defined __GNUC__
#  if defined __clang__ && __clang_major__ >= 3 && !defined __ANDROID__ && !defined __EMSCRIPTEN__ && !defined(__CUDACC__)
#    ifdef __ATOMIC_ACQ_REL
#      define ACV_XADD(addr, delta) __c11_atomic_fetch_add((_Atomic(int)*)(addr), delta, __ATOMIC_ACQ_REL)
#    else
#      define ACV_XADD(addr, delta) __atomic_fetch_add((_Atomic(int)*)(addr), delta, 4)
#    endif
#  else
#    if defined __ATOMIC_ACQ_REL && !defined __clang__
// version for gcc >= 4.7
#      define ACV_XADD(addr, delta) (int)__atomic_fetch_add((unsigned*)(addr), (unsigned)(delta), __ATOMIC_ACQ_REL)
#    else
#      define ACV_XADD(addr, delta) (int)__sync_fetch_and_add((unsigned*)(addr), (unsigned)(delta))
#    endif
#  endif
#elif defined _MSC_VER && !defined RC_INVOKED
#  include <intrin.h>
#  define ACV_XADD(addr, delta) (int)_InterlockedExchangeAdd((long volatile*)addr, delta)
#else
ACV_INLINE ACV_XADD(int* addr, int delta) { int tmp = *addr; *addr += delta; return tmp; }
#error "ACV_XADD is defined without atomic operation"
#endif

template<typename Derived>
class ReferenceCounter
{
public:
	ReferenceCounter(Derived* ptr)
	{
		derived_ptr_ = ptr;
		refcount = 1;
	}
	void addRef()
	{
		ACV_XADD(&refcount, 1);
	}
	//void decRef()
	void decRef()
	{
		if (ACV_XADD(&refcount, -1) == 1) delete derived_ptr_;
	}
	int refcount;
	Derived* derived_ptr_;
};


#endif // __ACV_OCL_REFERENCE_COUNTER_H__