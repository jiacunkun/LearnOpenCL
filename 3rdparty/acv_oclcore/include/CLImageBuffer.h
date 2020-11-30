/**
*
* @author Lei Hua
* @date 2019-12-10
*/

#ifndef __ACV_OCL_CLIMAGEBUFFER_H__
#define __ACV_OCL_CLIMAGEBUFFER_H__

#include "CLBuffer.h"

#ifdef HAVE_ACVCORE
#include "Mat.h"
#endif

namespace acv {
	namespace ocl {

class CLMEMBuffer;

/**
* CLImageBuffer is a class to create and manage OpenCL clImage2D or/and clImage3D object
*/
class ACV_EXPORTS CLImageBuffer
{
public:
	typedef cl_mem cl_native_buffer_type;

	/** @brief Default construct.
	* @note It calls init(CLContext::getDefaultCLContext()) to initialize CLImageBuffer::context_.
	*/
	CLImageBuffer();

	/** @brief Construct with a specific CLContext */
	explicit CLImageBuffer(const CLContext& context_ex);

	virtual ~CLImageBuffer();

	CLImageBuffer(const CLImageBuffer& other);
	CLImageBuffer& operator=(const CLImageBuffer& other);

	bool setContext(const CLContext& context);

	/** Set the command queue of a device in the CLContext by the device's index.
	* If the command queue is not set,
	* The default one is the command queue of the device with index==0.
	* Same commands are based on command queue such as buffer reading, buffer writting.
	*/
	int setCurrentCommandQueue(int index);

	cl_command_queue getCurrentCommandQueue();

	const cl_command_queue getCurrentCommandQueue()const;


	const CLContext& getCLContext()const;
	CLContext& getCLContext();

	CLBUFFER_TYPE buffer_type();

	const CLBUFFER_TYPE buffer_type()const;


	cl_mem native_cl_mem()const;

	static cl_int getSupportedImageFormats(const CLContext& context_ex, cl_mem_flags flags, cl_mem_object_type image_type, cl_uint num_entries,
		cl_image_format* image_formats, cl_uint* num_image_formats);

	/** @brief create a 2D or 3D image object
	* @param channel_order                    channel order. @see definition of cl_image_format
	* @param channel_data_type                channel data type. @see definition of cl_image_format
	* @param image_width                      image width
	* @param image_height                     image height
	* @param image_depth                      image depth (unused for image2D)
	* @param image_row_pitch                  row pitch in bytes. (unused when host_ptr == NULL)
	* @param image_slice_pitch                slice pitch in bytes for image3D. (unused when host_ptr == NULL)
	* @param num_dim                          number of dimensions: 2 or 3
	* @param flag                             memory flag: CL_MEM_WRITE_ONLY, CL_MEM_READ_ONLY,CL_MEM_READ_WRITE.
											  CL_MEM_USE_HOST_PTR will be appended to flag automatically when host_ptr is not NULL
	* @param host_ptr                         if host_ptr is not NULL, host_ptr is used to create a image as a host pointer.
	*/
	bool create(cl_channel_order channel_order, cl_channel_type channel_data_type,
		size_t image_width, size_t image_height, size_t image_depth,
		size_t image_row_pitch, size_t image_slice_pitch, int num_dim,
		cl_mem_flags flag = CL_MEM_READ_WRITE, void* host_ptr = NULL);

	/** @brief create a 2D or 3D image object
	* @param format                           image formatr. @see definition of cl_image_format
	* @param sizes                            image size [width, height, depth], depth is unused for image2D
	* @param pitchs                           image pitch in bytes. [row_pitch, slice_pitch]. (unused when host_ptr == NULL)
	* @param num_dim                          number of dimensions: 2 or 3
	* @param flag                             memory flag: CL_MEM_WRITE_ONLY, CL_MEM_READ_ONLY,CL_MEM_READ_WRITE.
											  CL_MEM_USE_HOST_PTR will be appended to flag automatically when host_ptr is not NULL
	* @param host_ptr                         if host_ptr is not NULL, host_ptr is used to create a image as a host pointer.
	*/
	bool create(const cl_image_format* format, const size_t* sizes, const size_t* pitchs, int num_dim,
		cl_mem_flags flag = CL_MEM_READ_WRITE, void* host_ptr = NULL);
	//#ifdef HAVE_OPENCL_1_2
				//bool create(const cl_image_format* format, const cl_image_desc *image_desc,
				//	cl_mem_flags flag = CL_MEM_READ_WRITE, void* host_ptr = NULL); // since opencl 1.2
	//#endif

	size_t width() const;
	size_t cols() const; // same as width()
	size_t height() const;
	size_t rows() const;  // same as height()
	size_t depth() const; // the size of the third dimension
	size_t pitch(int i) const;  // i == 0: row_pitch; i==1 : slice_pitch
	size_t dimensions() const;

	//size_t image_array_size() const;

	cl_image_format image_format()const;

	/** Release the OpenCL clImage2D or/and clImage3D object only*/
	void release();

	/** Release the OpenCL clImage2D or/and clImage3D object and the CLContext*/
	void releaseAll();

	/** @brief map data to the host. It is an one dimension version
	* @param size              size of the image data to be mapped.
	* @param map_flag          map flag [CL_MAP_WRITE, CL_MAP_READ]
	* @param offset            offset to the origin. 
	* @param blocking          if it is set to true, blocking until mapping is finished
	* @return                  mapped pointer which can be accessed in the host.
	*/
	void* map(size_t size, cl_map_flags map_flag, size_t offset, bool blocking,
		cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

	/** @brief map data to the host
	* @param origin            origin of the image data to be mapped.
	* @param region            region of the image data to be mapped.
	* @param image_row_pitch   Returns the scan-line pitch in bytes for the mapped region. This must be a non-NULL value.
	* @param image_row_pitch   Returns the size in bytes of each 2D slice for the mapped region.
							   For a 2D image, zero is returned if this argument is not NULL.
	* @param map_flag          map flag [CL_MAP_WRITE, CL_MAP_READ]
	* @param blocking          if it is set to true, blocking until mapping is finished
	* @return                  mapped pointer which can be accessed in the host.
	*/
	void* map(const size_t origin[3], const size_t region[3],
		size_t *image_row_pitch,
		size_t *image_slice_pitch,
		cl_map_flags map_flag = CL_MAP_WRITE | CL_MAP_READ, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);
#ifdef HAVE_ACVCORE
	Mat mapToMat(const size_t origin[3], const size_t region[3],
		int mat_type,
		cl_map_flags map_flag = CL_MAP_WRITE | CL_MAP_READ,
		bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	Mat mapToMat(
		int mat_type,
		cl_map_flags map_flag = CL_MAP_WRITE | CL_MAP_READ,
		bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);
#endif // HAVE_ACV_CORE
	 
	/*
	* unmap
	*/
	bool unmap(cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief copy data from the host to opencl cl_mem buffer
			* @param data    pointer on the host
			* @param size    size in bytes of data
			* @param offset  an offset relative to start point of cl_mem buffer where data will be copied to
			* @param blocking if true blocking until writting is finished
			*/
	bool write(const void* data, size_t size, size_t offset = 0, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief copy data from opencl cl_mem buffer to the host
	* @param data    pointer on the host
	* @param size    size in bytes of data
	* @param offset  an offset relative to start point of cl_mem buffer where data will be copied from
	* @param blocking if true blocking until reading is finished
	*/
	bool read(void* data, size_t size, size_t offset = 0, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief null function. since this function is defined in CLBufferBase as a pure virtual function, It has to define one.
	*/
	bool copyTo(size_t src_offset, CLBufferBase& dst_buffer, size_t dst_offset, size_t size,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief copy data from the host to opencl image buffer
	* @param origin        the (x, y, z) offset in pixels in the image from where to write or write.
	* @param region        the (width, height, depth) in pixels of the 2D or 3D rectangle being write or written.
	* @param row_pitch     the length of each row in bytes.
						   If row_pitch is set to 0, the appropriate row pitch is calculated based on the size of each element in bytes multiplied by width.
	* @parma slice_pitch   size in bytes of the 2D slice of the 3D region of a 3D image being written. it must be 0 for image2D.
						   If slice_pitch is set to 0, the appropriate slice pitch is calculated based on the row_pitch * height.
	* @param data          the pointer to a buffer in host memory where image data is to be read from.
	* @param blocking      If blocking is set to true, blocking until writting is finished.
	* @note                row_pitch and slice_pitch descript the memory layout of inputting "data" instead of CLImageBuffer memory.
	*/
	bool write(const size_t origin[3], const size_t region[3],
		size_t row_pitch, size_t slice_pitch, const void* data, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief copy the whole image data from the host to opencl image buffer
	* @param row_pitch     the length of each row in bytes.
						   If row_pitch is set to 0, the appropriate row pitch is calculated based on the size of each element in bytes multiplied by width.
	* @parma slice_pitch   size in bytes of the 2D slice of the 3D region of a 3D image being written. .
						   If slice_pitch is set to 0, the appropriate slice pitch is calculated based on the row_pitch * height.
	* @param data          the pointer to a buffer in host memory where image data is to be read from.
	* @param blocking      If blocking is set to true, blocking until writting is finished.
	* @note                row_pitch and slice_pitch descript the memory layout of inputting "data" instead of CLImageBuffer memory.
	*/
	bool write(size_t row_pitch, size_t slice_pitch, const void* data, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

#ifdef HAVE_ACVCORE
	bool write(const Mat& src, const size_t origin[3] = nullptr, const size_t region[3] = nullptr,
		bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);
#endif //HAVE_ACV_CORE

	/** @brief copy data from an opencl image buffer to the host
	* @param origin       the (x, y, z) offset in pixels in the image from where to read.
	* @param region       the (width, height, depth) in pixels of the 2D or 3D rectangle being read.
	* @param row_pitch    The length of each row in bytes.
						  If row_pitch is set to 0, the appropriate row pitch is calculated based on the size of each element in bytes multiplied by width.
	* @parma slice_pitch  Size in bytes of the 2D slice of the 3D region of a 3D image being read.
						  If slice_pitch is set to 0, the appropriate slice pitch is calculated based on the row_pitch * height.
	* @param data         The pointer to a buffer in host memory where image data is to be written to.
	* @param blocking     If blocking is set to true, blocking until writting is finished.
	* @note               row_pitch and slice_pitch descript the memory layout of "data" instead of CLImageBuffer memory.
	*/
	bool read(const size_t origin[3], const size_t region[3],
		size_t row_pitch, size_t slice_pitch, void* data, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief copy the whole image data from an opencl image buffer to the host
	* @param row_pitch    The length of each row in bytes.
						  If row_pitch is set to 0, the appropriate row pitch is calculated based on the size of each element in bytes multiplied by width.
	* @parma slice_pitch  Size in bytes of the 2D slice of the 3D region of a 3D image being read.
						  If slice_pitch is set to 0, the appropriate slice pitch is calculated based on the row_pitch * height.
	* @param data         The pointer to a buffer in host memory where image data is to be written to.
	* @param blocking     If blocking is set to true, blocking until writting is finished.
	* @note               row_pitch and slice_pitch descript the memory layout of "data" instead of CLImageBuffer memory.
	*/
	bool read(size_t row_pitch, size_t slice_pitch, void* data, bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

#ifdef HAVE_ACVCORE
	bool read(Mat& src, const size_t origin[3] = nullptr, const size_t region[3] = nullptr,
		bool blocking = true,
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);
#endif //HAVE_ACV_CORE
	/** @brief copy image data to another opencl image buffer
	* @param src_origin   origin of source image buffer
	* @param dst_image    destination image
	* @param dst_origin   origin of destination image buffer
	* @param region       region to be copied
	*/
	bool copyTo(const size_t src_origin[3], CLImageBuffer& dst_image, const size_t dst_origin[3], const size_t region[3],
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr)const;

	/** @brief copy image data to CLMEMBuffer
	* @param dst_image    destination buffer
	* @param dst_offset   offset of the destination buffer
	* @param src_origin   origin of source image buffer
	* @param region       region to be copied
	*/
	bool copyTo(CLMEMBuffer& dst_image, size_t dst_offset, const size_t src_origin[3], const size_t region[3],
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr)const;

	/** @brief copy image data from another opencl image buffer
	* @param dst_origin   origin of destination image buffer
	* @param src_image    source image
	* @param src_origin   origin of source image buffer
	* @param region       region to be copied
	*/
	bool copyFrom(const size_t dst_origin[3], const CLImageBuffer& src_image, const size_t src_origin[3], const size_t region[3],
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

	/** @brief copy image data from CLMEMBuffer
	* @param src_image    source buffer
	* @param src_offset   offset of the source buffer
	* @param dst_origin   origin of destination image buffer
	* @param region       region to be copied
	*/
	bool copyFrom(const CLMEMBuffer& src_image, size_t src_offset, const size_t dst_origin[3], const size_t region[3],
		cl_uint num_events_in_wait_list = 0, const cl_event* event_wait_list = nullptr, cl_event * event = nullptr);

protected:

	//struct NativeBufferHolder
	//{
	//	NativeBufferHolder(CLBUFFER_TYPE type, void* buffer_ptr, cl_context context);
	//	~NativeBufferHolder();

	//	CLBUFFER_TYPE type_;
	//	void* buffer_ptr_;
	//	cl_context  cl_context_;

	//	NativeBufferHolder(const NativeBufferHolder&) = delete;
	//	NativeBufferHolder& operator=(const NativeBufferHolder&) = delete;
	//};


	cl_mem createNativeBuffer(cl_context context, cl_mem_flags flag, cl_channel_order channel_order, cl_channel_type channle_type,
		size_t image_width, size_t image_height, size_t image_depth,
		size_t image_row_pitch, size_t image_slice_pitch, int num_dim,
		void* host_ptr);
	
	//bool checkContext();

	//std::shared_ptr<NativeBufferHolder> buffer_holder_;
	

	CLContext context_;
	int   current_command_queue_index_;
	void* mapped_ptr_;	

	class Impl;
	Impl* ptr_;
};
    }
}

#endif // __ACV_OCL_CLIMAGEBUFFER_H__