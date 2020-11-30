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
* @brief 
*
* @author Lei Hua
* @date 2019-03-19
*/

#ifndef __ACV_OCL_CL_IMAGE_H__
#define __ACV_OCL_CL_IMAGE_H__

#include "CLConfig.h"

#ifdef HAVE_ACVCORE


#include <cmath>

#include "Image.h"
#include "acvtypes.h"
#include "CLBuffer.h"
#include "CLMEMBuffer.h"
#include "CLSVMBuffer.h"


namespace acv {
	namespace ocl {		

		

		class ACV_EXPORTS CLImage
		{
		public:
			struct CLImagePlane
			{
				CLImagePlane(): pitch(0), scanline(0){}
				int pitch;                   //< pitch of width
				int scanline;                //< actual allocated buffer height which may be greater than plane height
			};

			/** image data layout similar to ASVLSCREEN  */
			struct Shape
			{
				Shape();
				Shape(const Image& img, size_t first_plane_offset=0);
				Shape(const ASVLOFFSCREEN& img, size_t first_plane_offset=0);
				Shape(int width, int height, unsigned int pixel_format, int pitch[4], size_t ptr_offset[4]);
				int width;                   //< image width
				int height;                  //< image height
				unsigned int pixel_format;   //< image format as defined in asvloffscreen.h
				int pitch[4];                //< pitch for each plane
				size_t ptr_offset[4];        //< offset of start pointer of each plane to the start of the allocated memory.			
			};

			CLImage();
			explicit CLImage(const CLContext& context_ex);
			~CLImage();

			CLImage(const CLImage& img);
			CLImage& operator=(const CLImage& img);

			CLImage(const CLImage& img, const Rect& roi);
			CLImage operator()(const Rect& roi)const;

			/** initialize with cl context */
			void setContext(const CLContext& context_ex);
			

			/** Create CLImage with Opencl cl_mem buffer */
			bool create_with_clmem(int width, int height, unsigned int pixle_format, int alignment = 0, 
				cl_mem_flags flags = CL_MEM_READ_WRITE,
				void* host_ptr = NULL);

			/** Create CLImage with Opencl SVM buffer */
			bool create_with_svm(int width, int height, unsigned int pixle_format, int alignment = 0,
				cl_mem_flags flags = CL_MEM_READ_WRITE);

			/** Create CLImage with Opencl cl_mem buffer */
			bool create_with_clmem(const Shape& img_info,
				cl_mem_flags flags = CL_MEM_READ_WRITE,
				void* host_ptr = NULL);

			/** Create CLImage with Opencl SVM buffer */
			bool create_with_svm(const Shape& img_info,
				cl_mem_flags flags = CL_MEM_READ_WRITE,
				int alignment = 0);

			/** @brief map a cl buffer to the host as an Image
			* @param flags     map flags [CL_MAP_WRITE or CL_MAP_READ ]
			* @param blocking     if true, blocking until map finished.
			* @return             return the mapped Image.
			* @note                CLImage must be continue memory image
			*/
			Image map(cl_map_flags flags, bool blocking = true);

			/** unmap */
			bool unmap();

			/** copy (read) the data to the host (both images must have same shape and be continue memory image) */
			void copyTo(Image& mat, bool blocking = true);

			/** copy (write) the data from the host (both images must have same shape and be continue memory image ) */
			void copyFrom(const Image& mat, bool blocking = true);

			/** copy (read) the data to other CLImage (both images must have same shape and be continue memory image ) */
			void copyTo(CLImage&  cl_mat);


			const Shape& getImageInfo()const;

			void release();

			int width() const;
			int height() const;
			unsigned int pixel_format()const;
			int pitch(int i) const;
			size_t offset(int i) const;
			CLBufferBase& getCLBufferBase();
			const CLBufferBase& getCLBufferBase()const;

			CLBUFFER_TYPE buffer_type()const;
			/** check whether SVM is available */
			bool is_svm_available()const;
			int setCurrentCommandQueue(int index);
		private:
			CLBufferBase* buffer_ptr_;
			CLMEMBuffer clmem_buffer_;
			CLSVMBuffer svm_buffer_;

			//CLContext context_;
			size_t mem_size_; // total image memory size
			Shape img_info_;
		};

		

	} // namespave ocl
} // namespace acv
#endif // HAVE_ACVCORE
#endif //__ACV_OCL_CL_IMAGE_H__