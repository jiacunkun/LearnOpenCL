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
* @brief A Mat class is similar to cv::Mat
*
* @author Lei Hua
* @date 2017-11-08
*/


#ifndef __MINI_MAT_H__
#define __MINI_MAT_H__


#include <assert.h>
#include "acvtypes.h"
#include "acvdef.h"
#include "logger.h"
#include "saturate.h"
#include <initializer_list>
#include <vector>






//#ifndef CV_8UC1
//#define CV_8UC1 ACV_8UC1 
//#define CV_8UC2 ACV_8UC2 
//#define CV_8UC3 ACV_8UC3 
//#define CV_8UC4 ACV_8UC4 
//#define CV_8UC(n) ACV_8UC(n) 
//
//#define CV_8SC1  ACV_8SC1 
//#define CV_8SC2  ACV_8SC2 
//#define CV_8SC3  ACV_8SC3 
//#define CV_8SC4  ACV_8SC4 
//#define CV_8SC(n) ACV_8SC(n) 
//
//#define CV_16UC1   ACV_16UC1 
//#define CV_16UC2   ACV_16UC2 
//#define CV_16UC3   ACV_16UC3 
//#define CV_16UC4   ACV_16UC4 
//#define CV_16UC(n)  ACV_16UC(n)
//
//#define CV_16SC1    ACV_16SC1 
//#define CV_16SC2    ACV_16SC2 
//#define CV_16SC3    ACV_16SC3 
//#define CV_16SC4    ACV_16SC4 
//#define CV_16SC(n)   ACV_16SC(n)
//
//#define CV_32SC1    ACV_32SC1 
//#define CV_32SC2    ACV_32SC2 
//#define CV_32SC3    ACV_32SC3 
//#define CV_32SC4    ACV_32SC4 
//#define CV_32SC(n)   ACV_32SC(n)
//
//#define CV_32FC1    ACV_32FC1 
//#define CV_32FC2    ACV_32FC2 
//#define CV_32FC3    ACV_32FC3 
//#define CV_32FC4    ACV_32FC4 
//#define CV_32FC(n)   ACV_32FC(n)
//
//#define CV_64FC1    ACV_64FC1 
//#define CV_64FC2    ACV_64FC2 
//#define CV_64FC3    ACV_64FC3 
//#define CV_64FC4    ACV_64FC4 
//#define CV_64FC(n)   ACV_64FC(n)
//#endif //




namespace acv {
	namespace internal{
		class DataOwner;		
	};
	class Allocator;
	
	/**@brief Mat is similar to cv::Mat in opencv. But it allows to allocate memory by mpbase library. The maximum dimension of acv::Mat is set by NUM_DIM

	@note when changing the default allocator, it affects acv::Image since they use the same allocator.

	Here is some examples:
	\code
	void my_func()
	{	    
	    
	    void* pMem = malloc(10*1024*1024);
	    MHandle handle = MMemMgrCreate(pMem, 10*1024*1024);
	    acv::MpbaseAllocator allocator(handle);  // cpnstruct an allocator with mpbase library
		acv::setDefaultAllocator(&allocator);
		acv::Mat mat(480, 640, ACV_8UC1);  // mat will call allocator to allocte memory
		// do someting with mat


	    MMemMgrDestroy(handle);
	    free(pMem);
	    ...
	}
	\endcode
	*/
	class ACV_EXPORTS Mat
	{
	public:
		Mat();

		Mat(int rows, int cols, int type, size_t line_step = AUTO_STEP);
		Mat(int ndims, const int* sizes, int type, const size_t* steps = 0);
		
		Mat(int rows, int cols, int type, void* data, size_t line_step = AUTO_STEP);
		Mat(int ndims, const int* sizes, int type, void* data, const size_t* steps = 0);
		Mat(std::vector<int> &sizes, int type, void* data, const size_t* steps = 0);

		Mat(const Mat& m);
		Mat(const Mat& m, const Rect& roi);
		Mat(const Mat& m, const Range* ranges);
		Mat(const Mat& m, const std::vector<Range>& ranges);

		Mat operator()(const Rect& roi) const;
		Mat operator()(const Range* ranges) const;
		Mat operator()(const std::vector<Range>& ranges) const;

		template<typename _Tp, typename = typename std::enable_if<std::is_arithmetic<_Tp>::value>::type>
		explicit Mat(const std::initializer_list<_Tp> list);	
		template<typename _Tp> explicit Mat(const std::initializer_list<int> sizes, const std::initializer_list<_Tp> list);

		~Mat();

		Mat& operator = (const Mat& m);
		void copyTo(Mat& m) const;

		void create(int rows, int cols, int type, size_t line_step = AUTO_STEP);
		void create(int ndims, const int* sizes, int type, const size_t* steps = 0);
		void create(std::vector<int>& sizes, int type, const size_t* steps = 0);
		void create_like(const Mat& m);
		void release();

		/** @brief Returns the matrix element size in bytes.

		For example, if the matrix type is ACV_16SC3, it returns 3\*sizeof(short) or 6.
		*/
		size_t elemSize() const;

		/** @brief Returns the size of each matrix element channel in bytes.

		For example, if the matrix type is ACV_16SC3 , it returns sizeof(short) or 2.
		*/
		size_t elemSize1() const;

		/** @brief Returns a normalized step for given dimension.

		Equivalent to steps[i] / elemSize1().
		*/
		size_t step1(int i) const;

		int type() const;
		int depth() const;   //!< returns ACV_8U, ACV_8S, ..., or ACV_64F
		int channels() const;
		bool empty() const;
		size_t total() const;  //!< total elements
		size_t totalMemSize() const; //!< total memory size
		bool isContinueMemory()const;
		

		Mat row(int i) const;
		Mat col(int j) const;
		Mat clone() const;

		// for debug
		void trace() const;
		bool save(const char* file_name)const;
		static Mat load(const char* file_name, int rows, int cols, int type);

		//! Change the default allocator. the original default allocator is AlignedAllocator. Return the original default allocator.
		//! not thread-safe
		Allocator* setAllocator(Allocator* allocator);

		/**@brief Convert to another type with the same size, dimensions and channel
		* T1 is the data type of this Mat, T2 is the data type of other. 
		* It is equivalent to 'other.at<T2><y, x> = this->at<T1>(y, x)*alpha + beta' for 2 dimensional Mat
		* 
		*@param other  The destination Mat. If other has different size or dimension, it will be released and a new Mat is created for other.
		*@param type   Mat type of other
		*@param alpha  Scalar 
		*@param beta   An offset
		*/
		template<typename T1, typename T2>
		void convertTo(Mat& other, int type, double alpha, double beta)const;

		/**@brief Tranpose the Mat
		* T1 is the data type of this Mat. It is only applied to Mats with 2 dimension
		*
		*@param other  The destination Mat. If other's size is not correct, it will be released and a new Mat is created for other.
		               If transpose to itself (&other==this), it must be a square matrix. 
		*/
		template<typename T1>
		void transpose(Mat& other)const;
		
		/**@brief Find minimum and maximum value
		* T1 is the data type of this Mat. It is only applied to 1 channel Mats
		*		
		*@param min_value   Pointer of minimum value. Can be NULL
		*@param max_value   Pointer of maximum value. Can be NULL
		*@param position_of_min_value  return index of each dimension where the min_value locates. Can be NULL.
		*@param position_of_max_value  return index of each dimension where the max_value locates. Can be NULL.
		*/
		template<typename T1>
		void minMaxValue(double* min_value, double* max_value, size_t* position_of_min_value = NULL, size_t*position_of_max_value=NULL)const;

		void setZeros();
		static Mat zeros(int rows, int cols, int type);

#define DECLEAR_ELEMENTS_ASSESS(prefix, type, op_name ) \
		prefix type op_name(int i0 = 0);	  \
		prefix const type op_name(int i0 = 0) const; \
		prefix type op_name(int row, int col); \
		prefix const type op_name(int row, int col) const; \
		prefix type op_name(int i0, int i1, int i2);  \
		prefix const type op_name(int i0, int i1, int i2) const; \
		prefix	type op_name(const int* idx); \
		prefix const type op_name(const int* idx) const;

		DECLEAR_ELEMENTS_ASSESS( , uchar*, ptr)
		DECLEAR_ELEMENTS_ASSESS(template<typename _Tp>, _Tp*, ptr)
		DECLEAR_ELEMENTS_ASSESS(template<typename _Tp>, _Tp&, at)

#undef 	DECLEAR_ELEMENTS_ASSESS	


		Mat(Mat&& m);
		Mat& operator = (Mat&& m);


		enum { MAGIC_VAL = 0x42FF0000, AUTO_STEP = 0, CONTINUOUS_FLAG = ACV_MAT_CONT_FLAG, SUBMATRIX_FLAG = ACV_SUBMAT_FLAG };
		enum { MAGIC_MASK = 0xFFFF0000, TYPE_MASK = 0x00000FFF, DEPTH_MASK = 7 };

		/*! includes several bit-fields:
		- the magic signature
		- continuity flag
		- depth
		- number of channels
		*/
		int flags;
		//! the matrix dimensionality, >= 2
		int dims;
		//! the number of rows and columns or (sizes[dims-2], sizes[dims-1]) when the matrix has more than 2 dimensions
		int rows, cols;
		//! Equivalent to steps[0]. 
		size_t step;
		//! size of each dimension. In case of a 2-dimensional array, such as image, rows = sizes[0], cols = sizes[1]
		int sizes[NUM_DIM];

		/**@brief steps of each dimension.

		The data layout of the array `M` is defined by the array `M.steps[]`, 
		so that the address of element \f$(i_0,...,i_{M.dims-1})\f$, where \f$0\leq i_k<M.size[k]\f$, is
		computed as:
		\f[addr(M_{i_0,...,i_{M.dims-1}}) = M.data + M.steps[0]*i_0 + M.steps[1]*i_1 + ... + M.steps[M.dims-1]*i_{M.dims-1}\f]
		In case of a 2-dimensional array, the above formula is reduced to:
		\f[addr(M_{i,j}) = M.data + M.steps[0]*i + M.steps[1]*j\f]
		Note that `M.steps[i] >= M.steps[i+1]` (in fact, `M.steps[i] >= M.steps[i+1]*M.sizes[i+1]` ).
		*/		
		size_t steps[NUM_DIM];

		unsigned char* data;

	private:
		void setSize(int _dims, const int* _sizes,
			const size_t* _steps);

		//! increase reference count.
		void incReference();
		//! decrease reference count.
		void decReference();		

		internal::DataOwner* owner;

		//! get default allocator which is AlignedAllocator when it is not set by the user.
		//static Allocator* getDefaultAllocator();
		Allocator* allocator;
	};

	

	/* @brief a helper struct to operate on each element for several mats whoes element' size is same
	* If all the mats have continual memory MatNForEachElement will reduce them to one dimentsion mat.
	* example:
	* @code
	*template<typename _T0, typename _T1, typename _T2, typename _T3>
	*struct MultAddOp {
	*	 operation to one dimentsion. 
	*	* @param data    start pointer of data in each mat for the deepest dimension whose memory is continual
	*	* @param steps   steps (in byte) of each mat for the deepest dimension whose memory is continual
	*	
	*void operator()(unsigned char** data, size_t* steps)
	*{
	*	_T0* dst = (_T0*)data[0];
	*	_T1* src1 = (_T1*)data[1];
	*	_T2* src2 = (_T2*)data[2];
	*	_T3* src3 = (_T3*)data[3];
	*
	*	size_t size = steps[0] / sizeof(_T0);  // it is safe to get the size of this dimension since the memory is continual
	*	//size_t size = steps[1] / sizeof(_T1); // same as steps[0] / sizeof(_T0)
	*	//size_t size = steps[2] / sizeof(_T2); // same as steps[0] / sizeof(_T0)
	*	//size_t size = steps[3] / sizeof(_T3); // same as steps[0] / sizeof(_T0)
	*	for (size_t i = 0; i < size; i++)
	*	{
	*		dst[i] = static_cast<_T0>(src1[i] * src2[i] + src3[i]);
	*	}
	*}
	*};
	*
	* 
	*void multAdd(Mat& dst, Mat& src1, Mat& src2, Mat& src3) // dst = src1*src2 + src3
	*{
	*	MatNForEachElement<4, MultAddOp<float, uchar, short, int> >(dst, src1, src2, src3);
	*}
	* @endcode
	*/
	template<int num_mats, typename Functor>
	struct MatNForEachElement
	{
		MatNForEachElement(Functor& functor, Mat& mat0) ;

		MatNForEachElement(Functor& functor, Mat& mat0, Mat& mat1);

		MatNForEachElement(Functor& functor, Mat& mat0, Mat& mat1, Mat& mat2);

		MatNForEachElement(Functor& functor, Mat& mat0, Mat& mat1, Mat& mat2, Mat& mat3);

		void reduceDimensionAndRun();

		Mat* mats[num_mats];
		unsigned char* datas[num_mats];  // start pointer for each dimension
		size_t steps[NUM_DIM*num_mats];	 // step (in byte) for each dimension
		size_t sizes[NUM_DIM];			 // size (number of elements) for each dimension
		int dims;						 // dimensions
		Functor& func;					 // function to deal with each dimension like [](unsigned char* datas[], size_t steps[])
	};


	

	
} // acv

#include "Mat.inl.h"

#endif // __MINI_MAT_H__