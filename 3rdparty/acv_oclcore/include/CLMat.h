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
* @brief C++ wrapper for opencl routines
*
* @author Lei Hua
* @date 2019-03-19
*/

#ifndef __ACV_OCL_CL_MAT_H__
#define __ACV_OCL_CL_MAT_H__



#include "CLBuffer.h"
#include "CLMEMBuffer.h"
#include "CLSVMBuffer.h"

#ifdef HAVE_ACVCORE
#include "Mat.h"
#endif // 
namespace acv {
	namespace ocl {
		/**
		* @brief CLMat is a wrapper of CLBuffer with data layout like acv::Mat
		*/
		class ACV_EXPORTS CLMat
		{
		public:
			/** @brief mat information compatible with acv::Mat */
			struct ACV_EXPORTS Shape
			{
				Shape();

				Shape(int _dims, int* _sizes, int _type,
					size_t* _steps // step for each dimension (in bytes)
				);
#ifdef HAVE_ACVCORE
				explicit Shape(const Mat& mat);
#endif

				Shape(const Shape& other);
				Shape& operator=(const Shape& other);

				void setShape(int _dims, const int* _sizes, int _type,
					const size_t* _steps = NULL // step for each dimension (in bytes)
				);

				/*
				For example, if the matrix type is CV_16SC3, it returns sizeof(short)*3 or 6.
				*/
				size_t elemSize() const;

				/*
				For example, if the matrix type is CV_16SC3, it returns sizeof(short) or 2.
				*/
				size_t elemSize1() const;

				int channels()const;

				//！return  total memory size in bytes
				size_t totalMemSize() const;

				//！return  check wether the memory is continue
				bool isContinueMemory()const;

				//! stride for each demension, steps[i]/elemSize1()
				int stride(int i) const;

				//! stride for offset of start pointer: offset_of_start_pointer/elemSize1()
				int offset_stride() const;

				size_t offsetToOrigin(const size_t offset, size_t* region);

				size_t originToOffset(const size_t* region);

				int type;
				//! the matrix dimensionality, >= 2
				int dims;
				//! the number of rows and columns 
				int rows, cols;
				//! Equivalent to steps[0]. 
				size_t step;
				//! size of each dimension. In case of a 2-dimensional array, such as image, rows = sizes[0], cols = sizes[1]
				int sizes[NUM_DIM];

				/**@brief steps of each dimension.

				The data layout of the array `M` is defined by the array `M.steps[]`,
				so that the address of element \f$(i_0,...,i_{M.dims-1})\f$, where \f$0\leq i_k<M.size[k]\f$, is
				computed as:
				\f[addr(M_{i_0,...,i_{M.dims-1}}) = start_pointer + offset_of_start_pointer + M.steps[0]*i_0 + M.steps[1]*i_1 + ... + M.steps[M.dims-1]*i_{M.dims-1}\f]
				In case of a 2-dimensional array, the above formula is reduced to:
				\f[addr(M_{i,j}) = M.data + M.steps[0]*i + M.steps[1]*j\f]
				Note that `M.steps[i] >= M.steps[i+1]` (in fact, `M.steps[i] >= M.steps[i+1]*M.sizes[i+1]` ).
				*/
				size_t steps[NUM_DIM];

				// the start point relative to the allocated memory buffer. it is usefull for sub matrix case.
				// size_t origin[NUM_DIM]; 

				//! offset of start pointer to the original cl memory, it is usefull for sub matrix case. 
				size_t offset_of_start_pointer;
			};


			CLMat();
			explicit CLMat(const CLContext& context);
			~CLMat();

			CLMat(const CLMat& other);
			CLMat& operator=(const CLMat& other);

#ifdef HAVE_ACVCORE
			/** @brief Construct a CLMat based on other CLMat.
			* The content of the new CLMat is set by parameter roi.
			* And the offset_of_start_pointer of the Shape of the new CLMat is set to the distance (in bytes) between (roi.x, roi.y) 
			* and the original memory start.
			*/
			CLMat(const CLMat& m, const Rect& roi);
			CLMat(const CLMat& m, const std::vector<Range>& ranges);
#endif

			/** @brief Construct a CLMat based on the CLBuffer of other CLMat. 
			* @parma offset    Offset in bytes to the original point of "other".
			*                  So it results to this->shape_.offset_of_start_pointer = other.shape_.offset_of_start_pointer + offset.
			* @note It reuses the ocl buffer of "other", and create a new CLMat on the same ocl buffer.
			*/
			CLMat(const CLMat& other, size_t offset, int rows, int cols, int mat_type, const size_t steps = 0);

			/** @brief Construct a CLMat based on the CLBuffer of other CLMat.
			* @parma offset    Offset in bytes to the original point of "other".
			*                  So it results to this->shape_.offset_of_start_pointer = other.shape_.offset_of_start_pointer + offset.
			* @note It reuses the ocl buffer of "other", and create a new CLMat on the same ocl buffer.
			*/
			CLMat(const CLMat& other, size_t offset, int ndims, const int* sizes, int mat_type, const size_t* steps = nullptr);
#ifdef HAVE_ACVCORE
			CLMat operator()(const Rect& roi) const;
			CLMat operator()(const std::vector<Range>& ranges)const;
#endif


			/** initialize with cl context */
			void setContext(const CLContext& context_ex);

			/** check whether Opencl SVM is available */
			bool is_svm_available()const;

			
			/** Create CLMat with an OpenCL cl_mem buffer
			* @param rows        number of rows of the CLMat
			* @param cols        number of cols of the CLMat
			* @param mat_type    type of the data such as ACV_8UC1.
			* @param step        memory step (in bytes) for each row. @see Mat for details.
			* @param flags       memory flag for cl_mem buffer. @see CLMemBuffer for details.
			* @param host_ptr    host ptr for cl_mem buffer. @see CLMemBuffer for details.
			*/
			bool create_with_clmem(int rows, int cols, int mat_type, const size_t steps = 0,
				cl_mem_flags flags = CL_MEM_READ_WRITE,
				void* host_ptr = NULL);	


			bool create_with_clmem(cl_mem native_clmem, int rows, int cols, int mat_type, const size_t steps = 0);
			
			/** Create CLMat with an OpenCL cl_mem buffer
			* @param ndims        number of dimension of the CLMat. 
			* @param sizes        sizes for each dimension. For 3D mat, the order of sizes is "{batch, height, width}" and sizes[0] = batch. For 2D mat, the order is "{height, width}" and sizes[0] = height.
			* @param mat_type     type of the data such as ACV_8UC1. 
			* @param steps        memory step (in bytes) for each dimensions. For 3D mat, the order of steps is "{slice_pitch, row_pitch, sizes_of_element}" and steps[0]=slice_pitch.
			*                     For 2D mat, the order is "{row_pitch, sizes_of_element}" and steps[0] = row_pitch. For type of ACV_16SC1 and ACV_16SC2, sizes_of_element is 2 and 4 repectively.
			* @param flags        memory flag for cl_mem buffer. @see CLMemBuffer for details.
			* @param host_ptr     host ptr for cl_mem buffer. @see CLMemBuffer for details.
			*/
			bool create_with_clmem(int ndims, const int* sizes, int mat_type, const size_t* steps = nullptr,
				cl_mem_flags flags = CL_MEM_READ_WRITE,				
				void* host_ptr = NULL);


			bool create_with_clmem(cl_mem native_clmem, int ndims, const int* sizes, int mat_type, const size_t* steps = nullptr);

			/** Create CLMat with an OpenCL SVM buffer
			* @param rows        number of rows of the CLMat
			* @param cols        number of cols of the CLMat
			* @param mat_type    type of the data such as ACV_8UC1.
			* @param step        memory step (in bytes) for each row. 
			* @param flags       memory flag for SVM buffer. @see CLSVMBuffer for details.
			* @param alignment   alignmentr for SVM buffer. @see CLSVMBuffer for details.
			*/
			bool create_with_svm(int rows, int cols, int mat_type, const size_t steps = 0,
				cl_mem_flags flags = CL_MEM_READ_WRITE,
				int alignment = 0);


			bool create_with_svm(void* native_svm, int rows, int cols, int mat_type, const size_t steps = 0);

			/** Create CLMat with an OpenCL SVM buffer
			* @param ndims        number of dimension of the CLMat. 
			* @param sizes        sizes for each dimension. For 3D mat, the order of sizes is "{batch, height, width}" and sizes[0] = batch. For 2D mat, the order is "{height, width}" and sizes[0] = height.
			* @param mat_type     type of the data such as ACV_8UC1. 
			* @param steps        memory step (in bytes) for each dimensions. For 3D mat, the order of steps is "{slice_pitch, row_pitch, sizes_of_element}" and steps[0]=slice_pitch.
			*                     For 2D mat, the order is "{row_pitch, sizes_of_element}" and steps[0] = row_pitch. For type of ACV_16SC1 and ACV_16SC2, sizes_of_element is 2 and 4 repectively.
			* @param flags        memory flag for SVM buffer.. @see CLSVMBuffer for details.
			* @param alignment    alignmentr for SVM buffer. @see CLSVMBuffer for details.
			*/
			bool create_with_svm(int ndims, const int* sizes, int mat_type, const size_t* steps = nullptr,
				cl_mem_flags flags = CL_MEM_READ_WRITE,
				int alignment=0);

			bool create_with_svm(void* native_svm, int ndims, const int* sizes, int mat_type, const size_t* steps = nullptr);

			/** unmap */
			bool unmap();

#ifdef HAVE_ACVCORE			

			/** @brief map a cl buffer to the host as a Mat
			* @param flags     map flags [CL_MAP_WRITE or CL_MAP_READ ]
			* @return             return the mapped Mat.
			*					  if it is svm buffer, the data pointer of returned Mat is set to the memory pointer allocated by clSVMAlloc
			*					  if it is cl_mem buffer, the data pointer of returned Mat is set to the memory pointer from clEnqueueMapBuffer
			* @note            it is blocked util map is finished.
			*/
			Mat map(cl_map_flags flags = CL_MAP_READ | CL_MAP_WRITE, bool is_blocking = true);

			/** @brief create a Mat in the host and copy the data in the davice into the Mat
			* sample as:
			*     Mat tmp(rows(), cols(), getShape().type);
			*     copyTo(tmp);
			*     return tmp;
			*/
			Mat assignMat();
			

			/** copy (read) the data to the host for a region
			* @note Only valide when dimenion is less than or equal to 3.
			*/
			bool copyTo(Mat& mat, const size_t* buffer_origion, const size_t* host_origin, const size_t* region, bool blocking =true);

			/** copy (read) the whole data to the host. The shape of both Mats must be same.*/
			bool copyTo(Mat& mat, bool blocking = true);
			

			/** copy (write) the data from the host
			* @note Only valide when dimenion is less than or equal to 3. 
			*/
			bool copyFrom(const Mat& mat, const size_t* buffer_origion, const size_t* host_origin, const size_t* region, bool blocking = true);

			/** copy (write) the whole data form the host to the buffer. The shape of both Mats must be same.*/
			bool copyFrom(const Mat& mat, bool blocking = true);
			
#endif
			/** map the buffer to the hospt space
			* @param flags            mapping flags such as  CL_MAP_READ, CL_MAP_WRITE
			* @param is_blocking      blocking or not.
			* @param origin           origin of the data (in pixels). If data type is ACV_16SC2, the origin of the memory would be origin*sizeof(short)*2.
			* @param region           region of the data (in pixels). If data type is ACV_16SC2, the origin of the memory would be region*sizeof(short)*2.
			* @note                   For 3D mat, the order of origin and region is [index_of_batch, index_of_rows, index_of_colunms] and [region_of_batch, region_of_rows, region_of_colunms] respectively. 
			                          For 2D mat, the order of origin and region is [index_of_rows, index_of_colunms] and [region_of_rows, region_of_colunms] respectively. 
			*/
			void* mapToPtr(cl_map_flags flags = CL_MAP_READ | CL_MAP_WRITE, bool is_blocking = true, size_t* origin = nullptr, size_t* region = nullptr);

			/** copy the whole data from the buffer to a host pointer*/
			bool copyTo(void* host_ptr, bool is_blocking = false);
			/** copy the whole data from a host pointer to the buffer*/
			bool copyFrom(const void* host_ptr, bool is_blocking = false);

			/** copy data from the buffer to a host pointer
			* @param buffer_origion   origin of the data in the buffer (in pixels). If data type is ACV_16SC2, the origin of the memory would be origin*sizeof(short)*2.
			* @param host_origin      origin of the data in the host (in pixels). If data type is ACV_16SC2, the origin of the memory would be origin*sizeof(short)*2.
			* @param region           region of the data (in pixels). If data type is ACV_16SC2, the origin of the memory in bytes would be region*sizeof(short)*2.
			* @param host_row_pitch   row pitch of the host data (in bytes).
			* @param hos_slice_pitch  slice pitch of the host data (in bytes).
			* @param is_blocking      blocking or not.
			* @note                   For 3D mat, the order of origin and region is [index_of_batch, index_of_rows, index_of_colunms] and [region_of_batch, region_of_rows, region_of_colunms] respectively. 
			                          For 2D mat, the order of origin and region is [index_of_rows, index_of_colunms] and [region_of_rows, region_of_colunms] respectively. 
			*/
			bool copyTo(const size_t* buffer_origion, const size_t* host_origin, const size_t* region, size_t host_row_pitch, 
				size_t hos_slice_pitch, void* host_ptr, bool is_blocking = false);

			/** copy data from a host pointer to the buffer.
			* @param buffer_origion   origin of the data in the buffer (in pixels). If data type is ACV_16SC2, the origin of the memory would be origin*sizeof(short)*2.
			* @param host_origin      origin of the data in the host (in pixels). If data type is ACV_16SC2, the origin of the memory would be origin*sizeof(short)*2.
			* @param region           region of the data (in pixels). If data type is ACV_16SC2, the origin of the memory in bytes would be region*sizeof(short)*2.
			* @param host_row_pitch   row pitch of the host data (in bytes).
			* @param hos_slice_pitch  slice pitch of the host data (in bytes).
			* @param is_blocking      blocking or not.
			* @note                   For 3D mat, the order of origin and region is [index_of_batch, index_of_rows, index_of_colunms] and [region_of_batch, region_of_rows, region_of_colunms] respectively.
									  For 2D mat, the order of origin and region is [index_of_rows, index_of_colunms] and [region_of_rows, region_of_colunms] respectively.
			*/
			bool copyFrom(const size_t* buffer_origion, const size_t* host_origin, const size_t* region, size_t host_row_pitch, size_t hos_slice_pitch, const void* host_ptr, bool is_blocking = false);

			/** copy data from a host pointer to the buffer.
			* @param dst_mat          the destination.       
			* @param src_origin       origin of the data in source buffer (in pixels). If data type is ACV_16SC2, the origin of the memory would be origin*sizeof(short)*2.
			* @param dst_origin       origin of the data in destination buffer (in pixels). If data type is ACV_16SC2, the origin of the memory would be origin*sizeof(short)*2.
			* @param region           region of the data (in pixels). If data type is ACV_16SC2, the origin of the memory in bytes would be region*sizeof(short)*2.
			* @param is_blocking      blocking or not.
			* @note                   For 3D mat, the order of origin and region is [index_of_batch, index_of_rows, index_of_colunms] and [region_of_batch, region_of_rows, region_of_colunms] respectively.
									  For 2D mat, the order of origin and region is [index_of_rows, index_of_colunms] and [region_of_rows, region_of_colunms] respectively.
			*/
			bool copyTo(CLMat& dst_mat, const size_t* src_origin, const size_t* dst_origin, const size_t* region, bool is_blocking = false);
			/** copy (read) the data to other CLMat (must have same shape ) */
			void copyTo(CLMat&  cl_mat);
	

			const Shape& getShape()const;
			

			void release();

			int cols() const;    //!< number of columns for 2D Mat. otherwise cols() == size(dimensions()-1);
			int width() const;   //!< same as cols()
			int rows() const;    //!< number of rows for 2D Mat, otherwise rows() == size(dimensions()-2);
			int height() const;  //!< same as rows()
			int stride(int i) const;   //!< stepi(i)/sizeof(array_element_type). For example, the array_element_type of ACV_16SC1 and ACV_16SC2 both are short
			int size(int i) const;     //!< size of each dimenstion. For 2D Mat: size(1) == cols() and size(0) == rows()
			
			/* The data layout of the array `M` is defined by the array `M.steps[]`,
				so that the address of element \f$(i_0, ..., i_{ M.dims - 1 })\f$, where \f$0\leq i_k < M.size(k)\f$, is
				computed as :
			    \f[addr(M_{ i_0,...,i_{M.dims - 1} }) = start_pointer + offset() + M.steps(0) * i_0 + M.steps(1) * i_1 + ... + M.steps(M.dimensions() - 1) * i_{ M.dimensions() - 1 }\f]
				In case of a 2 - dimensional array, the above formula is reduced to :
			    \f[addr(M_{ i,j }) = M.data + M.steps(0) * i + M.steps(1) * j\f]
				Note that `M.steps(i) >= M.steps(i + 1)` (in fact, `M.steps(i) >= M.steps(i + 1) * M.sizes(i + 1)`).
			*/
			size_t steps(int i) const; 
			                           
			int dimensions() const;    //!< number of dimensions
			size_t offset() const;     //!< offset to the original buffer pointer. Some sub-mat may have offset
			size_t offset_stride() const; //!< offset()/sizeof(array_element_type)
			CLBUFFER_TYPE buffer_type() const;
			CLBufferBase& getCLBufferBase();
			const CLBufferBase& getCLBufferBase()const;

			int setCurrentCommandQueue(int i); //!< set the command queue to the ith device in the CLContext

		private:
			CLBufferBase* buffer_ptr_;

			CLMEMBuffer clmem_buffer_;
			CLSVMBuffer svm_buffer_;			
			Shape shape_;
		};

		
	} // ocl
} // acv


#endif // !__ACV_OCL_CL_MAT_H__
