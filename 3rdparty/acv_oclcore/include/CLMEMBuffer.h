/**
*
* @author Lei Hua
* @date 2019-12-10
*/
#ifndef __ACV_OCL_CLMEMBuffer_H__
#define __ACV_OCL_CLMEMBuffer_H__

#include "CLBuffer.h"

namespace acv {
	namespace ocl {

/**
* CLMEMBuffer is a class to create and manage OpenCL cl_mem buffer
*/
class ACV_EXPORTS  CLMEMBuffer :public CLBufferBase
{
public:
	typedef cl_mem cl_native_buffer_type;

	/** @brief Default construct.
	* @note It calls init(CLContext::getDefaultCLContext()) to initialize CLMEMBuffer::context_.
	*/
	CLMEMBuffer();

	/** @brief Construct with a specific CLContext */
	explicit CLMEMBuffer(const CLContext& context_ex);
	virtual ~CLMEMBuffer();

	//CLMEMBuffer(const CLMEMBuffer& other);

	/** @brief create a cl_mem with size and memory flag
	* @param size         memory size
	* @param flag         memory flag such as CL_MEM_READ_WRITE, CL_MEM_READ_ONLY, CL_MEM_WRITE_ONLY. @see OpenCL specific documents for details
	* @param host_ptr     host pointer for OPenCL cl_mem. @see OpenCL specific documents for details
	* if size is smaller than previous buffer size and flag is same as previous one,
	* the previous cl_mem will be used. If you intend to create a new one, call release before create.
	* if host_ptr != NULL, CL_MEM_USE_HOST_PTR will be appended to flag
	*/
	bool create(size_t size, cl_mem_flags flag = CL_MEM_READ_WRITE, void* host_ptr = NULL);

	bool create(size_t size, cl_mem native_clmem);

	/** @brief release the cl_mem */
	virtual void release();
	///** get native cl_mem */
	//cl_mem native_buffer()const;
	

	virtual void* map(size_t size, cl_map_flags map_flag = CL_MAP_WRITE | CL_MAP_READ, size_t offset = 0, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	virtual bool unmap(cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief copy data from the host to opencl cl_mem buffer
	* @param data    pointer on the host
	* @param size    size in bytes of data
	* @param offset  an offset relative to start point of cl_mem buffer where data will be copied to
	* @param blocking if true blocking until writting is finished
	* @see clEnqueueWriteBuffer section on OpenCL specification document for details.
	*/
	virtual bool write(const void* data, size_t size, size_t offset = 0, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);


	/** @brief copy data from opencl cl_mem buffer to the host
	* @param data    pointer on the host
	* @param size    size in bytes of data
	* @param offset  an offset relative to start point of cl_mem buffer where data will be copied from
	* @param blocking if true blocking until reading is finished
	* @see clEnqueueReadBuffer section on OpenCL specification document for details.
	*/
	virtual bool read(void* data, size_t size, size_t offset = 0, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief copy data to another CLBufferBase
	* @param src_offset    offset of source buffer
	* @param dst_buffer    destination buffer
	* @param dst_offset    offset of destination buffer
	* @param size          copy size in bytes
	* @note                If dst_buffer is CLMEMBuffer, it calls clEnqueueCopyBuffer. Otherwise
	                       it maps dst_buffer to the host and calls CLMEMBuffer::read to copy the data.
	* @see clEnqueueCopyBuffer section on OpenCL specification document for details.
	*/
	virtual bool copyTo(size_t src_offset, CLBufferBase& dst_buffer, size_t dst_offset, size_t size,
		cl_uint num_events_in_wait_list = 0, const cl_event * event_wait_list = nullptr, cl_event * event = nullptr);


	/**
	* Calls clEnqueueReadBufferRect to read the data from the buffer to the host ptr.
	* @see clEnqueueReadBufferRect section on OpenCL specification document for details
	*/
	virtual bool read(const size_t* buffer_origin, const size_t* host_origin, const size_t* region,
		size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, void* ptr,
		bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/**
	* Calls clEnqueueWriteBufferRect to write the data ofthe host ptr to the buffer. 
	*@see clEnqueueWriteBufferRect secton on OpenCL specification document for details
	*/
	virtual bool write(const size_t* buffer_origin, const size_t* host_origin, const size_t* region,
		size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const void* ptr,
		bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event * event_wait_list = nullptr, cl_event * event = nullptr);

	/**
	* Copy data between OpenCL buffer.
	* If dst_buffer is CLMEMBuffer, it just calls clEnqueueCopyBufferRect.
	* If dst_buffer is CLSVMBuffer, it maps dst_buffer to the host and calls CLMEMBuffer::read to copy the data.
	* @see clEnqueueCopyBufferRect section on OpenCL specification document for details.
	*/
	virtual bool copyTo(CLBufferBase& dst_buffer, const size_t* src_origin, const size_t* dst_origin, const size_t* region,
		size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

protected:

	cl_mem createNativeBuffer(cl_context context, cl_mem_flags flag, size_t size, void* host_ptr);

	//cl_mem cl_mem_; // 
	//void* host_ptr_;
};
} // ocl
} // acv

#endif