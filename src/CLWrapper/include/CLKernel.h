/**
 * @Author: [lq8@meitu.com] <lynch>
 * @Date:   2014-08-28T16:09:18+08:00
 * @Email:  lq8@meitu.com
 * @Filename: CLKernel.h
 * @Last modified by:   lynch
 * @Last modified time: 2019-02-01T09:48:51+08:00
 */



#pragma once

#include "CLEvent.h"
#include "CLImage.h"
#include "CLBuffer.h"
#include "CLContext.h"

namespace mtcl
{

template<class T>
struct translate {
	typedef const T type;
};

template<class T>
struct translate<T*> {
	typedef Buffer<T> type;
};

template<class T>
class Local {
public:
	Local(size_t s) : size(s) { }
	size_t size;
};

template<class T>
struct Args {
	static void set(cl_kernel kernel, int n, T arg, bool &res)
	{
		res = checkError(clSetKernelArg(kernel, n, sizeof(T), &arg));
	}
};

template<class T>
struct Args< Buffer<T> > {
	static void set(cl_kernel kernel, int n, Buffer<T> &arg, bool &res)
	{
		res = checkError(clSetKernelArg(kernel, n, sizeof(cl_mem), arg.getMem()));
	}
};

template<class T>
struct Args< const Buffer<T> > {
	static void set(cl_kernel kernel, int n, const Buffer<T> &arg, bool &res)
	{
		res = checkError(clSetKernelArg(kernel, n, sizeof(cl_mem), arg.getMem()));
	}
};

template<class T>
struct Args< Image2D<T> > {
	static void set(cl_kernel kernel, int n, Image2D<T> &arg, bool &res)
	{
		res = checkError(clSetKernelArg(kernel, n, sizeof(cl_mem), arg.getMem()));
	}
};

template<class T>
struct Args< const Image2D<T> > {
	static void set(cl_kernel kernel, int n, const Image2D<T> &arg, bool &res)
	{
		res = checkError(clSetKernelArg(kernel, n, sizeof(cl_mem), arg.getMem()));
	}
};

template<class T>
struct Args< Local<T> > {
	static void set(cl_kernel kernel, int n, Local<T> arg, bool &res)
	{
		res = checkError(clSetKernelArg(kernel, n, sizeof(T)*arg.size, 0));
	}
};

template<class T>
void setKernelArg(cl_kernel kernel, int n, T &arg, bool &res)
{
	Args<T>::set(kernel, n, arg, res);
}

struct Worksize {
	Worksize(size_t g1, size_t l1) : dim(1)
	{
		global[0] = g1;
		local[0] = l1;
	}
	Worksize(size_t g1, size_t g2, size_t l1, size_t l2) : dim(2)
	{
		global[0] = g1; global[1] = g2;
		local[0] = l1; local[1] = l2;
	}
	Worksize(size_t g1, size_t g2, size_t g3, size_t l1, size_t l2, size_t l3) : dim(3)
	{
		global[0] = g1; global[1] = g2; global[2] = g3;
		local[0] = l1; local[1] = l2; local[2] = l3;
	}
	size_t global[3];
	size_t local[3];
	cl_uint dim;
};

template<class F>
class Kernel {
};

template<class T0>
class Kernel<void(T0)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;

	Event operator()(const Worksize &ws, A0 &arg0, bool &res)
	{
		return operator()(ws, arg0, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, const Event &event, bool &res)
	{
		return operator()(ws, arg0, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}
		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};

template<class T0, class T1>
class Kernel<void(T0, T1)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, bool &res)
	{
		return operator()(ws, arg0, arg1, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};

template<class T0, class T1, class T2>
class Kernel<void(T0, T1, T2)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


template<class T0, class T1, class T2, class T3>
class Kernel<void(T0, T1, T2, T3)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


template<class T0, class T1, class T2, class T3, class T4>
class Kernel<void(T0, T1, T2, T3, T4)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


template<class T0, class T1, class T2, class T3, class T4, class T5>
class Kernel<void(T0, T1, T2, T3, T4, T5)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


template<class T0, class T1, class T2, class T3, class T4, class T5, class T6>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};

template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;
	typedef typename translate<T10>::type A10;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, 0, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		setKernelArg(kernel, 10, arg10, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};

template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
 		class T11>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;
	typedef typename translate<T10>::type A10;
	typedef typename translate<T11>::type A11;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		setKernelArg(kernel, 10, arg10, res);
		setKernelArg(kernel, 11, arg11, res);
		if (res)
		{
			// cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, 0, NULL, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};

template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
 		class T11, class T12>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;
	typedef typename translate<T10>::type A10;
	typedef typename translate<T11>::type A11;
	typedef typename translate<T12>::type A12;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		setKernelArg(kernel, 10, arg10, res);
		setKernelArg(kernel, 11, arg11, res);
		setKernelArg(kernel, 12, arg12, res);
		if (res)
		{
			// cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, 0, NULL, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};

template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
 		class T11, class T12, class T13>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;
	typedef typename translate<T10>::type A10;
	typedef typename translate<T11>::type A11;
	typedef typename translate<T12>::type A12;
	typedef typename translate<T13>::type A13;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		setKernelArg(kernel, 10, arg10, res);
		setKernelArg(kernel, 11, arg11, res);
		setKernelArg(kernel, 12, arg12, res);
		setKernelArg(kernel, 13, arg13, res);
		if (res)
		{
			// cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, 0, NULL, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};

template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
 		class T11, class T12, class T13, class T14>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;
	typedef typename translate<T10>::type A10;
	typedef typename translate<T11>::type A11;
	typedef typename translate<T12>::type A12;
	typedef typename translate<T13>::type A13;
	typedef typename translate<T14>::type A14;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		setKernelArg(kernel, 10, arg10, res);
		setKernelArg(kernel, 11, arg11, res);
		setKernelArg(kernel, 12, arg12, res);
		setKernelArg(kernel, 13, arg13, res);
		setKernelArg(kernel, 14, arg14, res);
		if (res)
		{
			// cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, 0, NULL, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};

template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
 		class T11, class T12, class T13, class T14, class T15>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;
	typedef typename translate<T10>::type A10;
	typedef typename translate<T11>::type A11;
	typedef typename translate<T12>::type A12;
	typedef typename translate<T13>::type A13;
	typedef typename translate<T14>::type A14;
	typedef typename translate<T15>::type A15;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, arg15, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, arg15, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		setKernelArg(kernel, 10, arg10, res);
		setKernelArg(kernel, 11, arg11, res);
		setKernelArg(kernel, 12, arg12, res);
		setKernelArg(kernel, 13, arg13, res);
		setKernelArg(kernel, 14, arg14, res);
		setKernelArg(kernel, 15, arg15, res);
		if (res)
		{
			// cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, 0, NULL, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};

template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
 		class T11, class T12, class T13, class T14, class T15, class T16>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;
	typedef typename translate<T10>::type A10;
	typedef typename translate<T11>::type A11;
	typedef typename translate<T12>::type A12;
	typedef typename translate<T13>::type A13;
	typedef typename translate<T14>::type A14;
	typedef typename translate<T15>::type A15;
	typedef typename translate<T16>::type A16;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, arg15, arg16, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, arg15, arg16, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		setKernelArg(kernel, 10, arg10, res);
		setKernelArg(kernel, 11, arg11, res);
		setKernelArg(kernel, 12, arg12, res);
		setKernelArg(kernel, 13, arg13, res);
		setKernelArg(kernel, 14, arg14, res);
		setKernelArg(kernel, 15, arg15, res);
		setKernelArg(kernel, 16, arg16, res);
		if (res)
		{
			// cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, 0, NULL, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
 		class T11, class T12, class T13, class T14, class T15, class T16, class T17>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;
	typedef typename translate<T10>::type A10;
	typedef typename translate<T11>::type A11;
	typedef typename translate<T12>::type A12;
	typedef typename translate<T13>::type A13;
	typedef typename translate<T14>::type A14;
	typedef typename translate<T15>::type A15;
	typedef typename translate<T16>::type A16;
	typedef typename translate<T17>::type A17;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, A17 &arg17, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, arg15, arg16, arg17, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, A17 &arg17, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, arg15, arg16, arg17, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, A17 &arg17, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		setKernelArg(kernel, 10, arg10, res);
		setKernelArg(kernel, 11, arg11, res);
		setKernelArg(kernel, 12, arg12, res);
		setKernelArg(kernel, 13, arg13, res);
		setKernelArg(kernel, 14, arg14, res);
		setKernelArg(kernel, 15, arg15, res);
		setKernelArg(kernel, 16, arg16, res);
		setKernelArg(kernel, 17, arg17, res);
		if (res)
		{
			// cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, 0, NULL, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
 		class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;
	typedef typename translate<T10>::type A10;
	typedef typename translate<T11>::type A11;
	typedef typename translate<T12>::type A12;
	typedef typename translate<T13>::type A13;
	typedef typename translate<T14>::type A14;
	typedef typename translate<T15>::type A15;
	typedef typename translate<T16>::type A16;
	typedef typename translate<T17>::type A17;
	typedef typename translate<T18>::type A18;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, A17 &arg17, A18 &arg18, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, A17 &arg17, A18 &arg18, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, A17 &arg17, A18 &arg18, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		setKernelArg(kernel, 10, arg10, res);
		setKernelArg(kernel, 11, arg11, res);
		setKernelArg(kernel, 12, arg12, res);
		setKernelArg(kernel, 13, arg13, res);
		setKernelArg(kernel, 14, arg14, res);
		setKernelArg(kernel, 15, arg15, res);
		setKernelArg(kernel, 16, arg16, res);
		setKernelArg(kernel, 17, arg17, res);
		setKernelArg(kernel, 18, arg18, res);
		if (res)
		{
			// cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, 0, NULL, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
 		class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
class Kernel<void(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19)> {
public:
	Kernel(const Context &c, cl_kernel k) : kernel(k), context(c) {}
	Kernel(const Kernel &k, bool &res) : kernel(k.kernel), context(k.context)
	{
		res = checkError(clRetainKernel(kernel));
	}

	typedef typename translate<T0>::type A0;
	typedef typename translate<T1>::type A1;
	typedef typename translate<T2>::type A2;
	typedef typename translate<T3>::type A3;
	typedef typename translate<T4>::type A4;
	typedef typename translate<T5>::type A5;
	typedef typename translate<T6>::type A6;
	typedef typename translate<T7>::type A7;
	typedef typename translate<T8>::type A8;
	typedef typename translate<T9>::type A9;
	typedef typename translate<T10>::type A10;
	typedef typename translate<T11>::type A11;
	typedef typename translate<T12>::type A12;
	typedef typename translate<T13>::type A13;
	typedef typename translate<T14>::type A14;
	typedef typename translate<T15>::type A15;
	typedef typename translate<T16>::type A16;
	typedef typename translate<T17>::type A17;
	typedef typename translate<T18>::type A18;
	typedef typename translate<T19>::type A19;

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, A17 &arg17, A18 &arg18, A19 &arg19, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19, 0, res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, A17 &arg17, A18 &arg18, A19 &arg19, const Event &event, bool &res)
	{
		return operator()(ws, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			 			  arg11, arg12, arg13, arg14, arg15, arg16, arg17, arg18, arg19, 1, event.getEventPtr(), res);
	}

	Event operator()(const Worksize &ws, A0 &arg0, A1 &arg1, A2 &arg2, A3 &arg3, A4 &arg4, A5 &arg5, A6 &arg6,
		 A7 &arg7, A8 &arg8, A9 &arg9, A10 &arg10, A11 &arg11, A12 &arg12, A13 &arg13, A14 &arg14, A15 &arg15,
		 A16 &arg16, A17 &arg17, A18 &arg18, A19 &arg19, cl_uint event_count, const cl_event *events, bool &res)
	{
		cl_event event;
		setKernelArg(kernel, 0, arg0, res);
		setKernelArg(kernel, 1, arg1, res);
		setKernelArg(kernel, 2, arg2, res);
		setKernelArg(kernel, 3, arg3, res);
		setKernelArg(kernel, 4, arg4, res);
		setKernelArg(kernel, 5, arg5, res);
		setKernelArg(kernel, 6, arg6, res);
		setKernelArg(kernel, 7, arg7, res);
		setKernelArg(kernel, 8, arg8, res);
		setKernelArg(kernel, 9, arg9, res);
		setKernelArg(kernel, 10, arg10, res);
		setKernelArg(kernel, 11, arg11, res);
		setKernelArg(kernel, 12, arg12, res);
		setKernelArg(kernel, 13, arg13, res);
		setKernelArg(kernel, 14, arg14, res);
		setKernelArg(kernel, 15, arg15, res);
		setKernelArg(kernel, 16, arg16, res);
		setKernelArg(kernel, 17, arg17, res);
		setKernelArg(kernel, 18, arg18, res);
		setKernelArg(kernel, 19, arg19, res);
		if (res)
		{
			cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, event_count, events, &event);
			// cl_int error = clEnqueueNDRangeKernel(context.getQueue(), kernel, ws.dim, 0, ws.global, ws.local, 0, NULL, &event);
			res = checkError(error);
		}

		return Event(event);
	}

	~Kernel()
	{
		checkError(clReleaseKernel(kernel));
	}
private:
	cl_kernel kernel;
	Context context;
};


}
