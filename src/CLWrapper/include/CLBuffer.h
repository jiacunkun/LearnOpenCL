/**
 * @Author: [lq8@meitu.com] <lynch>
 * @Date:   2014-08-28T16:09:18+08:00
 * @Email:  lq8@meitu.com
 * @Filename: CLBuffer.h
 * @Last modified by:   lynch
 * @Last modified time: 2019-02-01T09:38:35+08:00
 */



#pragma once

#include "CLEvent.h"
#include "CLContext.h"

namespace mtcl
{

template<class T>
class Buffer {
public:
	typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef const ptrdiff_t difference_type;
    typedef size_t size_type;

	Buffer(const cl_mem &bufHandel, size_t col, size_t row)
		: host_ptr(0)
	{
		width = col;
		height = row;
		buffersize = width * height;
		buffer = bufHandel;
		exits = false;
	}

	Buffer(const Context &c, size_t col, size_t row, bool &res)
		: host_ptr(0), context(c)
	{
		cl_int error;
		width = col;
		height = row;
		buffersize = width * height;
		buffer = clCreateBuffer(context.getContext(), CL_MEM_READ_WRITE, buffersize*sizeof(value_type), 0, &error);
		exits = true;
		res = checkError(error);
	}

	Buffer()
		: host_ptr(0), buffersize(0)
	{
		exits = false;
		width = 0;
		height = 0;
		buffersize = width * height;
	}

	void copy(Buffer &output)
	{
		output.exits = false;
		output.event = this->event;
		output.width = this->width;
		output.height = this->height;
		output.buffer = this->buffer;
		output.context = this->context;
		output.host_ptr = this->host_ptr;
		output.buffersize = this->buffersize;
	}

	void init(const Context &c, size_t col, size_t row, bool &res)
	{
		context = c;
		width = col;
		height = row;
		buffersize = width * height;
		if (exits)
		{
			clReleaseMemObject(buffer);
		}
		cl_int error;
		buffer = clCreateBuffer(context.getContext(), CL_MEM_READ_WRITE, buffersize*sizeof(value_type), 0, &error);
		exits = true;
		res = checkError(error);
	}

	Event map(bool &res, cl_map_flags flags = CL_MAP_READ | CL_MAP_WRITE)
	{
		return map(flags, 0, 0, res);
	}

	Event map(cl_map_flags flags, const Event &event, bool &res)
	{
		return map(flags, 1, event.getEventPtr(), res);
	}

	Event map(cl_map_flags flags, cl_uint event_count, const cl_event *events, bool &res)
	{
		check_unmapped();
		cl_int error;
		cl_event e;
		host_ptr = static_cast<value_type*>(clEnqueueMapBuffer(context.getQueue(), buffer, CL_FALSE, flags, 0, buffersize*sizeof(value_type), event_count, events, &e, &error));
		res = checkError(error);
		event = Event(e);
		return event;
	}

	Event unmap(bool &res)
	{
		return unmap(0, 0, res);
	}

	Event unmap(const Event &event, bool &res)
	{
		return unmap(1, event.getEventPtr(), res);
	}

	Event unmap(cl_uint event_count, const cl_event *events, bool &res)
	{
		check_mapped();
		cl_event e;
		cl_int error = clEnqueueUnmapMemObject(context.getQueue(), buffer, host_ptr, event_count, events, &e);
		host_ptr = 0;
		res = checkError(error);
		event = Event(e);
		return event;
	}

	Event read(value_type *destination, bool &res)
	{
		return read(destination, 0, 0, res);
	}

	Event read(value_type *destination, const Event &event, bool &res)
	{
		return read(destination, 1, event.getEventPtr(), res);
	}

	Event read(value_type *destination, cl_uint event_count, const cl_event *events, bool &res)
	{
		check_unmapped();
		cl_event e;
		cl_int error = clEnqueueReadBuffer(context.getQueue(), buffer, CL_FALSE, 0, buffersize*sizeof(value_type), destination, event_count, events, &e);
		res = checkError(error);
		event = Event(e);
		return event;
	}

	Event readRange(size_t offset, size_t length, value_type *destination, bool &res)
	{
		return readRange(offset, length, destination, 0, 0, res);
	}

	Event readRange(size_t offset, size_t length, value_type *destination, const Event &event, bool &res)
	{
		return readRange(offset, length, destination, 1, event.getEventPtr(), res);
	}

	Event readRange(size_t offset, size_t length, value_type *destination, cl_uint event_count, const cl_event *events, bool &res)
	{
		if(offset+length>buffersize)
			throw std::runtime_error("buffer too short");
		check_unmapped();
		cl_event e;
		cl_int error = clEnqueueReadBuffer(context.getQueue(), buffer, CL_FALSE, offset*sizeof(value_type), length*sizeof(value_type), destination, event_count, events, &e);
		res = checkError(error);
		event = Event(e);
		return event;
	}

	Event write(const value_type *source, bool &res)
	{
		return write(source, 0, 0, res);
	}

	Event write(const value_type *source, const Event &event, bool &res)
	{
		return write(source, 1, event.getEventPtr(), res);
	}

	Event write(const value_type *source, cl_uint event_count, const cl_event *events, bool &res)
	{
		check_unmapped();
		cl_event e;
		cl_int error = clEnqueueWriteBuffer(context.getQueue(), buffer, CL_FALSE, 0, buffersize*sizeof(value_type), source, event_count, events, &e);
		res = checkError(error);
		event = Event(e);
		return event;
	}

	Event writeRange(size_t offset, size_t length, const value_type *source, bool &res)
	{
		return writeRange(offset, length, source, 0, 0, res);
	}

	Event writeRange(size_t offset, size_t length, const value_type *source, const Event &event, bool &res)
	{
		return writeRange(offset, length, source, 1, event.getEventPtr(), res);
	}

	Event writeRange(size_t offset, size_t length, const value_type *source, cl_uint event_count, const cl_event *events, bool &res)
	{
		if(offset+length>buffersize)
			throw std::runtime_error("buffer too short");
		check_unmapped();
		cl_event e;
		cl_int error = clEnqueueWriteBuffer(context.getQueue(), buffer, CL_FALSE, offset*sizeof(value_type), length*sizeof(value_type), source, event_count, events, &e);
		res = checkError(error);
		event = Event(e);
		return event;
	}

    inline reference operator[](size_t i)
    {
        check_mapped();
        return host_ptr[i];
    }

    inline const_reference operator[](size_t i) const
    {
        check_mapped();
        return host_ptr[i];
    }

    inline value_type* data() { check_mapped(); return host_ptr; }
    inline const value_type* data() const { check_mapped(); return host_ptr; }
    inline iterator begin() { check_mapped(); return host_ptr; }
    inline const_iterator begin() const { check_mapped(); return host_ptr; }
    inline iterator end() { check_mapped(); return host_ptr+buffersize; }
    inline const_iterator end() const { check_mapped(); return host_ptr+buffersize; }

    inline size_type size() const { return buffersize; }
	inline void size(size_t& col, size_t& row) { col = width; row = height; }

	const cl_mem* getMem() const { return &buffer; }
	Event getLastEvent() { return event; }

	bool isMapped() const { return host_ptr != 0; }

	~Buffer()
	{
		if (exits)
		{
			clReleaseMemObject(buffer);
		}
	}

private:
	Buffer(const Buffer&) { }
	Buffer& operator=(const Buffer&) { return *this; }

	inline void check_mapped() const
    {
        if(!host_ptr)
            throw std::runtime_error("Buffer not mapped");
    }

    inline void check_unmapped() const
    {
        if(host_ptr)
            throw std::runtime_error("Buffer mapped");
    }

	value_type *host_ptr;
	size_t buffersize;
	cl_mem buffer;
	Event event;
	Context context;
	bool exits;
	size_t width;
	size_t height;
};

}
