/**
*
* @author Lei Hua
* @date 2019-12-10
*/

#ifndef __ACV_CLBUFFER_BASE_H__
#define __ACV_CLBUFFER_BASE_H__

#include "CLContext.h"
#ifdef HAVE_ACV_CORE
#include "Mat.h"
#endif // HAVE_ACV_CORE

namespace acv {
	namespace ocl {


enum struct CLBUFFER_TYPE
{
	CLMEM,          ///< buffer type is cl_mem
	CLMEM_IMAGE,    ///< buffer type is clImage2D or clImage3D
	SVM             ///< buffer type is SVM
};

/**@brief Basic class for CLMEMBuffer, CLSVMBuffer and CLImageBuffer*/
class ACV_EXPORTS CLBufferBase
{
public:

	CLBufferBase();

	explicit CLBufferBase(const CLContext& context);

	virtual ~CLBufferBase();

	CLBufferBase(const CLBufferBase& other);
	CLBufferBase& operator=(const CLBufferBase& other);

	bool setContext(const CLContext& context);

	bool isEmpty()const;

	/** Set the command queue of a device in the CLContext by the device's index.
	* If the command queue is not set,
	* The default one is the command queue of the device with index==0.
	* Same commands are based on command queue such as buffer reading, buffer writting.
	*/
	virtual int setCurrentCommandQueue(int index);

	cl_command_queue getCurrentCommandQueue();

	const cl_command_queue getCurrentCommandQueue()const;

	bool is_svm_available()const;

	bool is_fine_grain_svm_available()const;

	const CLContext& getCLContext()const;
	CLContext& getCLContext();

	CLBUFFER_TYPE buffer_type();

	const CLBUFFER_TYPE buffer_type()const;

	size_t size()const;

	cl_mem native_cl_mem()const;
	
	void* native_svm()const;

	/** Only release the opencl buffer but context_ is not released
	*   If one wants to detach from the CLContext setting before, you should call releaseAll 
	*/
	virtual void release();

	/** Release the opencl buffer and detach from CLContext setting before*/
	void releaseAll();

	virtual void* map(size_t size, cl_map_flags map_flag, size_t offset, bool blocking,
		cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event) = 0;

	virtual bool unmap(cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event) = 0;

	/** @brief copy data from the host to opencl cl_mem buffer
	* @param data    pointer on the host
	* @param size    size in bytes of data
	* @param offset  an offset relative to start point of cl_mem buffer where data will be copied to
	* @param blocking if true blocking until writting is finished
	*/
	virtual bool write(const void* data, size_t size, size_t offset, bool blocking, 
		cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event * event) = 0;

	/** @brief copy data from opencl cl_mem buffer to the host
	* @param data    pointer on the host
	* @param size    size in bytes of data
	* @param offset  an offset relative to start point of cl_mem buffer where data will be copied from
	* @param blocking if true blocking until reading is finished
	*/
	virtual bool read(void* data, size_t size, size_t offset, bool blocking ,
		cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event * event) = 0;

	/** @brief copy data to another CLBufferBase
	* @param src_offset    offset of source buffer
	* @param dst_buffer    destination buffer
	* @param dst_offset    offset of destination buffer
	* @param size          copy size in bytes
	*/
	virtual bool copyTo(size_t src_offset, CLBufferBase& dst_buffer, size_t dst_offset, size_t size,
		cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event * event) = 0;

	   
	/**
	* For CLMEMBuffer, it just calls clEnqueueReadBufferRect.
	* For CLSVMBuffer, it maps the buffer to the host and copy the mapped data to the host ptr.
	* @see clEnqueueReadBufferRect section on OpenCL specification document for details
	*/
	virtual bool read(const size_t* buffer_origin, const size_t* host_origin, const size_t* region,
		size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, void* ptr,
		bool blocking ,
		cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event) = 0;

	/**
	* For CLMEMBuffer, it just calls clEnqueueWriteBufferRect.
	* For CLSVMBuffer, it maps the buffer to the host and copy the data on the host to the mapped buffer.
	* @see clEnqueueWriteBufferRect section on OpenCL specification document for details.
	*/
	virtual bool write(const size_t* buffer_origin, const size_t* host_origin, const size_t* region,
		size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const void* ptr,
		bool blocking ,
		cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)=0;

	/**
	* Copy data between OpenCL buffer.
	* If the source buffer and the destination buffer are CLMEMBuffer, it just calls clEnqueueCopyBufferRect.
	* If one of source buffer or the destination buffer is CLSVMBuffer, it maps the CLSVMBuffer to the host and calls read or write to copy data.
	* @see clEnqueueCopyBufferRect section on OpenCL specification document for details.
	*/
	virtual bool copyTo(CLBufferBase& dst_buffer, const size_t* src_origin, const size_t* dst_origin, const size_t* region,
		size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch,
		cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)=0;

protected:


	//bool checkContext();


	class Impl;
	Impl* ptr_;
	void* mapped_ptr_;
	int   current_command_queue_index_;

	CLContext context_;
};


	} // ocl
} //acv

#endif