/**
*
* @author Lei Hua
* @date 2019-12-10
*/
#ifndef __ACV_OCL_CLSVMBUFFER_H__
#define __ACV_OCL_CLSVMBUFFER_H__

#include "CLBuffer.h"

namespace acv {
	namespace ocl {

/**
* CLSVMBuffer is a class to create and manage OpenCL SVM buffer
*/
class ACV_EXPORTS CLSVMBuffer: public CLBufferBase
{
public:
	typedef void* cl_native_buffer_type;
	/** @brief Default construct.
	* @note It calls setCLContext(CLContext::getDefaultCLContext()) to initialize context_.
	*/
	CLSVMBuffer();

	/** @brief Construct with a specific contect*/
	explicit CLSVMBuffer(const CLContext& context_ex);
	virtual ~CLSVMBuffer();

	//CLSVMBuffer(const CLSVMBuffer& other);


	virtual int setCurrentCommandQueue(int index);

	bool checkSVMCapbility();


	/** @brief create a SVM with size and memory flag
	* @param size             memory size
	* @param flag             memory flag such as CL_MEM_READ_WRITE, CL_MEM_WRITE_ONLY, CL_MEM_READ_ONLY
	* @param alignment        alignment to allocate SVM. @see clSVMAlloc section on opencl specific documents for more details
	* @note if size is smaller than previous buffer size and flag is same as previous one,
	* the previous one will be used. If you intend to create a new one, call release before create.
	*/
	bool create(size_t size, cl_mem_flags flag = CL_MEM_READ_WRITE, cl_uint alignment = 0);


	bool create(size_t size, void* native_svm_buffer);

	/** @brief release the cl_mem */
	virtual void release();


	/** @brief Map data by clEnqueueSVMMap. For fine grain svm buffer, it does nothing.
	* @param size      size in bytes of data for mapping
	* @param map_flag  map flags
	* @param offset    an offset relative to start point of svm buffer
	* @param blocking  if true blocking until mapping is finished
	* @see clEnqueueSVMMap section on the OpenCL specification document for details. 
	*/
	virtual void* map(size_t size, cl_map_flags map_flag = CL_MAP_WRITE | CL_MAP_READ, size_t offset = 0, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief unmap data by clEnqueueSVMMap. For fine grain svm buffer, it does nothing.*/
	virtual bool unmap(cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief copy data from the host to svm buffer
	* @param data    pointer of data on the host
	* @param size    size of data in bytes
	* @param offset  offset ot the start of svm buffer
	* @note it is equivalent to { map, copy, unmap} operations.
	*/
	virtual bool write(const void* data, size_t size, size_t offset = 0, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief copy data from svm buffer to the host
	* @param data    pointer of data on the host
	* @param size    size of data in bytes
	* @param offset  offset ot the start of svm buffer
	* @note it is equivalent to { map, copy, unmap} operations.
	*/
	virtual bool read(void* data, size_t size, size_t offset = 0, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	virtual bool copyTo(size_t src_offset, CLBufferBase& dst_buffer, size_t dst_offset, size_t size,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** Copy data from svm buffer to the host. 
	* It is limilar to clEnqueueReadBufferRect but it works on SVM buffer.
	* It achieves this by { map, copy, unmap} operations.
	*/
	bool read(const size_t* buffer_origin, const size_t* host_origin, const size_t* region,
		size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, void* ptr,
		bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event * event_wait_list = nullptr, cl_event * event = nullptr);

	/** Copy data from the host to svm buffer. 
	* It is limilar to clEnqueueReadBufferRect but it works on SVM buffer.
	* It achieves this by { map, copy, unmap} operations.
	*/
	bool write(const size_t* buffer_origin, const size_t* host_origin, const size_t* region,
		size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const void* ptr,
		bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event * event_wait_list = nullptr, cl_event * event = nullptr);

	/** Copy data to another opencl buffer.
	bool copyTo(CLBufferBase& dst_buffer, const size_t* src_origin, const size_t* dst_origin, const size_t* region,
	* @note It maps the source to the host and calls dst_buffer.write to achieve the copying.
	*/
	bool copyTo(CLBufferBase& dst_buffer, const size_t* src_origin, const size_t* dst_origin, const size_t* region,
		size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch,
		cl_uint num_events_in_wait_list = 0, const cl_event * event_wait_list = nullptr, cl_event * event = nullptr);

private:

	void* createNativeBuffer(cl_context context, size_t size, cl_mem_flags flag, cl_uint alignment = 0);


	//void* data_;
};
}
}

#endif // __ACV_OCL_CLSVMBUFFER_H__