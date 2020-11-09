/**
 * @Author: [lq8@meitu.com] <lynch>
 * @Date:   2014-08-28T16:09:18+08:00
 * @Email:  lq8@meitu.com
 * @Filename: CLEvent.h
 * @Last modified by:   lynch
 * @Last modified time: 2019-02-01T09:48:07+08:00
 */



#pragma once

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "CLUtility.h"

namespace mtcl
{

class Event {
public:
	Event() : assigned(false) { }
	Event(cl_event e) : assigned(true), event(e) { }
	Event(const Event &e) : assigned(e.assigned), event(e.event)
	{
		if(assigned)
			clRetainEvent(event);
	}
	Event& operator=(const Event &e)
	{
		if(assigned)
			clReleaseEvent(event);
		assigned = e.assigned;
		event = e.event;
		if(assigned)
			clRetainEvent(event);
		return *this;
	}

	cl_int getStatus()
	{
		cl_int status;
		size_t size;
		checkError(clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &status, &size));
		return status;
	}

	static void finish(const cl_command_queue &queue)
	{
		clFinish(queue);
	}

	static void flush(const cl_command_queue &queue)
	{
		clFlush(queue);
	}

	void wait()
	{
		checkError(clWaitForEvents(1,&event));
	}

	operator cl_event() const { return event; }

	cl_event const* getEventPtr() const { return &event; }

	~Event()
	{
		if(assigned)
			clReleaseEvent(event);
	}
private:
	bool assigned;
	cl_event event;
};

}
