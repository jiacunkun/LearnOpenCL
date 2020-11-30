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
* @brief Image object
*
* @author Lei Hua
* @date 2017-7-22
*/

#ifndef __ACV_IMAGE_H__
#define __ACV_IMAGE_H__

#include "asvloffscreen.h"
#include "acvtypes.h"
#include "acvdef.h"

namespace acv 
{
	namespace internal{
		class DataOwner;
	};

/**@bref A wrapping class of ASVLOFFSCREEN struct. 

Right now (2017/11/08) the supported color format includes:
-# ASVL_PAF_RGB24_B8G8R8
-# ASVL_PAF_RGB24_R8G8B8

-#	ASVL_PAF_RGB32_B8G8R8
-#	ASVL_PAF_RGB32_B8G8R8A8
-#	ASVL_PAF_RGB32_R8G8B8
-#	ASVL_PAF_RGB32_A8R8G8B8
-#	ASVL_PAF_RGB32_R8G8B8A8

-#	ASVL_PAF_GRAY
	
-#	ASVL_PAF_NV12
-#	ASVL_PAF_NV21
-#	ASVL_PAF_LPI422H
-#	ASVL_PAF_LPI422H2

It is easy to support other color formats by modifying GetPlaneSizes and the construct Image(const Image& other, const Rect& roi).

The copying or assignment behavior of Image mostly like the cv::Mat. Image use a reference counter to record the ownership of the data memory.
The data memory will be released automaticly when the reference counter reaches to zero. So that construct copying or assigning an Image object is cheap,
there is no data copied. The default allocator of Image is AlignedAllocator. It can be changed by acv::setDefaultAllocator. Image also support mpbase by setting
the default allocator to acv::MpbaseAllocator.

@note when changing the default allocator, it affects acv::Mat since they use the same allocator.

Here is some examples:
\code
void my_func()
{

void* pMem = malloc(10*1024*1024);
MHandle handle = MMemMgrCreate(pMem, 10*1024*1024);
acv::MpbaseAllocator allocator(handle);  // cpnstruct an allocator with mpbase library
acv::setDefaultAllocator(&allocator);
acv::Image img(480, 640, ASVL_PAF_NV12);  // mat will call allocator to allocte memory
// do someting with img


MMemMgrDestroy(handle);
free(pMem);
...
}
\endcode
*/

	class Allocator;

class ACV_EXPORTS Image
{
public:
	/** default constructor.
	*/
	Image();

	/** @overload
	* @param width         image width
	* @param height        image height
	* @param pixle_format  image format. See definition of ASVLOFFSCREEN
	* @param aligment      align data memory for each line width aligment (in byte). 0 is no aligment
	*/
	Image(int width, int height, unsigned int pixle_format, int alignment = 0);
	
	/** @overload
	* @param width         image width
	* @param height        image height
	* @param pixle_format  image format. See definition of ASVLOFFSCREEN
	* @param data          Pointer to the user data. Image constructors that take data and size information and do not
                           allocate matrix data. Instead, they just initialize the image header that points to the specified
                           data, which means that no data is copied.
	* @param aligment      align data memory for each line width aligment (in byte). 0 is no aligment
	*/
	Image(int width, int height, unsigned int pixle_format, unsigned char* data, int alignment = 0);
	
	/** @overload
	* @param asvl         ASVLOFFSCREEN object. Image constructors that take data and size information in asvl and do not
                          allocate matrix data. Instead, they just initialize the image header that points to the specified
                          data, which means that no data is copied.
	*/
	Image(const ASVLOFFSCREEN& asvl);
	
	/** @overload
	* @param other         Construct Image from other object. The reference counter would incease if other have reference counter and no data is copied.
	*/
	Image(const Image& other);

	/** @overload
	* @param other         Image that (as a whole or partly) is assigned to the constructed matrix. No data is copied
                           by these constructors. Instead, the header pointing to m data or its sub-image is constructed and
                           associated with it. The reference counter, if any, is incremented.
	* @param roi           Region of interest.
	*/
	Image(const Image& other, const Rect& roi);
	
	//! destructor - calls release()
	~Image();


	/** Create a image with data memery allocated. It will release the previous image object before creating. A reference counter is assigned to it and initilized to 1.
	* @param width         image width
	* @param height        image height
	* @param pixle_format  image format. See definition of ASVLOFFSCREEN
	* @param aligment      Align data memory for each line width aligment (in byte). 0 is no aligment
	*/
	void create(int width, int height, unsigned int pixle_format, int alignment = 0);
	
	
	/** @brief Decrease the reference counter. 
	When reference counter reaches to 0, the image data will be deallocated and the pointer of the reference counter will be set to NULL's. 
	*/
	void release();

	//! Convert to ASVLOFFSCREEN object
	ASVLOFFSCREEN ToASVLOFFSCREEN();
	const ASVLOFFSCREEN ToASVLOFFSCREEN() const;

	/** @brief  Copy to other image including image header and the image data. 
	* @param other         The destination. If the size and the format is not equal to source, a now image will be created and assigned to other.
	*/
	void copyTo(Image& other)const;

	/** @brief save to a file */
	bool save(const char* file_name) const;
	/** @brief load from a file */
	static Image load(const char* file_name, int width, int height, unsigned int pixle_format);


	/** @brief assignment operators

	These are available assignment operators. Since they all are very different, make sure to read the
	operator parameters description.
	@param other            Assigned, right-hand-side image. Image assignment is an O(1) operation. This means that
	no data is copied but the data is shared and the reference counter, if any, is incremented. Before
	assigning new data, the old data is de-referenced via Image::release .
	*/
	Image& operator = (const Image& other);

	//! returns true if image data is NULL
	bool empty()const;

	//! total buffer size in bytes
	size_t total()const;
	size_t totalMemSize()const;

	//! Change the default allocator. the original default allocator is AlignedAllocator. Return the original default allocator.
	//! not thread-safe
	static Allocator* setAllocator(Allocator* allocator);
	
	//! color format. See asvloffsreen.h
	unsigned int     pixel_format;
	//! image width
	union { int width, cols; };
	//! image height
	union { int height, rows; };
	//! data pointers of each image plane
	unsigned char*	 plane_ptrs[4];
	//! line steps of each image plane
	size_t	         plane_line_steps[4];  
	//! heights of each image plane
	size_t           plane_heights[4];
private:


	void incReference();

	void decReference();

	internal::DataOwner* owner;

	//! get default allocator which is AlignedAllocator when it is not set by the user.
	//static Allocator* getDefaultAllocator();
	static Allocator* allocator;
};



}; //namespace acv


#endif //__ACV_IMAGE_H__