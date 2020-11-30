#ifndef __ACV_MAT_INL_H__
#define __ACV_MAT_INL_H__

namespace acv {
	namespace internal {
		template<typename T1, typename T2>
		class ConvertFuncAlphaBeta
		{
		public:
			ConvertFuncAlphaBeta(double alpha, double beta) :
				alpha_(alpha), beta_(beta)
			{}
			inline void operator()(unsigned char** data, size_t* steps)
			{
				T1* dst = (T1*)data[0];
				T2* src = (T2*)data[1];
				size_t num = steps[0] / sizeof(T1);

				for (size_t i = 0; i < num; i++, src++, dst++)
				{
					double value = (*src)*alpha_ + beta_;

					*dst = saturate_cast<T1>(value);
				}
			}
		private:
			const double alpha_;
			const double beta_;
		};

		template<typename T1, typename T2>
		class ConvertFuncNormal
		{
		public:
			ConvertFuncNormal()
			{}
			inline void operator()(unsigned char** data, size_t* steps)
			{
				T1* dst = (T1*)data[0];
				T2* src = (T2*)data[1];
				size_t num = steps[0] / sizeof(T1);

				for (size_t i = 0; i < num; i++, src++, dst++)
				{
					*dst = saturate_cast<T1>(*src);
				}
			}
		};


		template<typename T1>
		class MinMaxFunc
		{
		public:
			MinMaxFunc() :
				min_value_(std::numeric_limits<T1>::max()), max_value_(std::numeric_limits<T1>::min()),
				index_iter_(0), index_of_min_value_(0), index_of_max_value_(0)
			{}
			inline void operator()(unsigned char** data, size_t* step)
			{

				T1* dst = (T1*)data[0];
				size_t num = step[0] / sizeof(T1);
				for (size_t i = 0; i < num; i++, dst++, index_iter_++)
				{
					if (*dst > max_value_)
					{
						max_value_ = *dst;
						index_of_max_value_ = index_iter_;
					}
					if (*dst < min_value_)
					{
						min_value_ = *dst;
						index_of_min_value_ = index_iter_;
					}
				}
			}

			T1 min_value_;
			T1 max_value_;
			size_t index_iter_;
			size_t index_of_min_value_;
			size_t index_of_max_value_;
		};


		template<typename Functor, int num_mats>
		struct ForEachDimension
		{
			ForEachDimension(unsigned char** datas_, const size_t* steps_, const size_t *sizes_,
				int dims_, Functor& func_) :
				datas(datas_), steps(steps_), sizes(sizes_), dims(dims_), func(func_)
			{
				ForEachDim(datas, 0);
			}

			void ForEachDim(unsigned char** datas_, int dim)
			{
				if (dim == dims - 1) // the top dimension
				{
					//unsigned char* this_dim_datas[num_mats];
					size_t this_dim_steps[num_mats];
					for (size_t i = 0; i < num_mats; i++)
					{
						//this_dim_datas[i] = (datas[i] + steps[i*NUM_DIM + dim]);
						this_dim_steps[i] = (steps[i*NUM_DIM + dim] * sizes[dim]);
					}
					func(datas_, this_dim_steps);
				}
				else if (dim == dims - 2)
				{
					size_t this_dim_steps[num_mats];
					for (size_t mat_k = 0; mat_k < num_mats; mat_k++)
					{
						this_dim_steps[mat_k] = (steps[mat_k*NUM_DIM + dim + 1] * sizes[dim + 1]);
					}

					for (size_t dim_i = 0; dim_i < sizes[dim]; dim_i++)
					{
						unsigned char* this_dim_datas[num_mats];

						for (size_t mat_k = 0; mat_k < num_mats; mat_k++)
						{
							this_dim_datas[mat_k] = datas[mat_k] + steps[mat_k*NUM_DIM + dim] * dim_i;
						}

						func(this_dim_datas, this_dim_steps);
					}
				}
				else
				{
					for (size_t dim_i = 0; dim_i < sizes[dim]; dim_i++)
					{
						unsigned char* this_dim_datas[num_mats];
						for (size_t mat_k = 0; mat_k < num_mats; mat_k++)
						{
							this_dim_datas[mat_k] = datas[mat_k] + steps[mat_k*NUM_DIM + dim] * dim_i;
						}

						ForEachDim(this_dim_datas, dim + 1); // next dimension
					}
				}
			}

			unsigned char** datas;      // start pointer for each dimension
			const size_t* steps;        // step (in byte) for each dimension
			const size_t* sizes;        // size (number of elements) for each dimension
			int dims;                   // dimensions
			Functor& func;              // function to deal with each dimension like [](unsigned char* datas[], size_t steps[])
		};
	} // internal

	

	template<int num_mats, typename Functor> inline 
		MatNForEachElement<num_mats,  Functor>::MatNForEachElement( Functor& functor, Mat& mat0) :func(functor)
	{
		//assert(num_mats == 1);
		DCHECK(num_mats == 1);
		mats[0] = &mat0;
		reduceDimensionAndRun();
	}

	template<int num_mats, typename Functor> inline
		MatNForEachElement<num_mats,  Functor>::MatNForEachElement(Functor& functor, Mat& mat0, Mat& mat1) :func(functor)
	{
		//assert(num_mats == 2);
		DCHECK(num_mats == 2);
		mats[0] = &mat0;
		mats[1] = &mat1;
		reduceDimensionAndRun();
	}

	template<int num_mats, typename Functor> inline
		MatNForEachElement<num_mats,  Functor>::MatNForEachElement(Functor& functor, Mat& mat0, Mat& mat1, Mat& mat2) :func(functor)
	{
		//assert(num_mats == 3);
		DCHECK(num_mats == 3);
		mats[0] = &mat0;
		mats[1] = &mat1;
		mats[2] = &mat2;
		reduceDimensionAndRun();
	}

	template<int num_mats, typename Functor> inline
		MatNForEachElement<num_mats,  Functor>::MatNForEachElement(Functor& functor, Mat& mat0, Mat& mat1, Mat& mat2, Mat& mat3) :func(functor)
	{
		//assert(num_mats == 4);
		DCHECK(num_mats == 4);
		mats[0] = &mat0;
		mats[1] = &mat1;
		mats[2] = &mat2;
		mats[3] = &mat3;
		reduceDimensionAndRun();
	}

	template<int num_mats, typename Functor> inline
		void MatNForEachElement<num_mats,  Functor>::reduceDimensionAndRun()
	{
		Mat& mat0 = *mats[0];
		dims = mat0.dims;

		for (int k = 0; k < num_mats; k++)
		{
			datas[k] = mats[k]->data;

			for (int m = 0; m < dims; m++)
			{
				steps[k*NUM_DIM + m] = mats[k]->steps[m];
			}
		}

		for (int m = 0; m < dims; m++)
		{
			sizes[m] = mats[0]->sizes[m];
		}

		for (int i = mat0.dims - 1; i > 0; i--)
		{
			int k;
			for (k = 0; k < num_mats; k++)
			{
				Mat& matk = *mats[k];
				if (matk.steps[i - 1] != matk.sizes[i] * matk.elemSize()) // not continous buffer
				{
					break;
				}
			}

			if (k != num_mats)
			{
				break;
			}
			else
			{
				dims--;
			}
		}

		internal::ForEachDimension<Functor, num_mats>(datas, steps, sizes, dims, func);

	}
	// T1 is the data type of this mat. T2 is the data type of other
	template<typename T1, typename T2> inline
		void Mat::convertTo(Mat& other, int _type, double alpha, double beta)const
	{
		if (&other == this)
		{
			if (_type == this->type())
			{
				return;
			}
			Mat dst;
			convertTo<T1, T2>(dst, _type, alpha, beta);
			other = dst;
			return;
		}
		other.create(dims, sizes, _type);
		DCHECK(channels() == other.channels());      // assuem same channel
		DCHECK(sizeof(T1) == elemSize1());            // check element size1 of this->type 
		DCHECK(sizeof(T2) == other.elemSize1());  // check element size1 of _type 
		if (std::abs(alpha - 1.0) < std::numeric_limits<double>::min() && std::abs(beta) < std::numeric_limits<double>::min())
		{
			internal::ConvertFuncNormal<T2, T1> converfunc;
			Mat m = *this;
			MatNForEachElement<2, internal::ConvertFuncNormal<T2, T1> >(converfunc, other, m);

		}
		else
		{
			internal::ConvertFuncAlphaBeta<T2, T1> converfunc(alpha, beta);
			Mat m = *this; // for constant issue
			MatNForEachElement<2, internal::ConvertFuncAlphaBeta<T2, T1> >(converfunc, other, m);
		}

	}

	template<typename T1>
	void Mat::transpose(Mat& other)const
	{
		DCHECK(dims == 2);
		if (&other == this)
		{
			DCHECK(cols == rows);
			unsigned char* dst = other.data;
			unsigned char* dst_t = dst;
			for (int y = 0; y < rows; y++)
			{
				dst_t = other.data + y * sizeof(T1) + (y + 1)*other.step;
				T1* dst_ptr = (T1*)dst;
				for (int x = y + 1; x < cols; x++)
				{
					T1 value = dst_ptr[x];
					T1* dst_t_ptr = (T1*)dst_t;
					dst_ptr[x] = dst_t_ptr[0];
					dst_t_ptr[0] = value;
					dst_t += other.step;
				}
				dst += other.step;
			}
		}
		else
		{
			other.create(cols, rows, type());
			const unsigned char* src_line = this->data;

			T1* dst_ptr0 = (T1*)other.data;
			for (int y = 0; y < rows; y++)
			{
				const T1* src_ptr = (T1*)src_line;

				unsigned char* dst_ptr = (unsigned char*)dst_ptr0;
				for (int x = 0; x < cols; x++)
				{
					T1* dst = (T1*)dst_ptr;
					dst[0] = src_ptr[x];
					dst_ptr += other.step;
				}
				src_line += this->step;
				dst_ptr0++;
			}
		}
	}

	template<typename T1>
	void Mat::minMaxValue(double* min_value, double* max_value, size_t* position_for_min_value, size_t* position_for_max_value)const
	{
		DCHECK(channels() == 1);      // 1 channel
		DCHECK(sizeof(T1) == elemSize1());            // check element size1 of this->type
		internal::MinMaxFunc<T1> min_max_func;
		Mat m = *this; // for constant issue
		MatNForEachElement<1, internal::MinMaxFunc<T1> >(min_max_func, m);
		if (min_value)
			*min_value = min_max_func.min_value_;
		if (max_value)
			*max_value = min_max_func.max_value_;

		size_t accumulate_sizes[NUM_DIM];
		accumulate_sizes[dims - 1] = m.sizes[dims - 1];
		for (int i = m.dims - 1; i > 0; i--)
		{
			accumulate_sizes[i - 1] = m.sizes[i - 1] + accumulate_sizes[i];
		}

		if (position_for_min_value)
		{
			size_t index_of_min_value = min_max_func.index_of_min_value_;
			for (int i = 0; i < m.dims - 1; i++)
			{
				position_for_min_value[i] = index_of_min_value / accumulate_sizes[i + 1];
				index_of_min_value = index_of_min_value % accumulate_sizes[i + 1];
			}
			position_for_min_value[m.dims - 1] = index_of_min_value;
		}

		if (position_for_max_value)
		{
			size_t index_of_max_value = min_max_func.index_of_max_value_;
			for (int i = 0; i < m.dims - 1; i++)
			{
				position_for_max_value[i] = index_of_max_value / accumulate_sizes[i + 1];
				index_of_max_value = index_of_max_value % accumulate_sizes[i + 1];
			}
			position_for_max_value[m.dims - 1] = index_of_max_value;
		}
	}

	template<typename _Tp, typename> inline
		Mat::Mat(const std::initializer_list<_Tp> init_list)
		: Mat()
	{
		DCHECK(init_list.size() != 0);
		Mat((int)init_list.size(), 1, traits::Type<_Tp>::value, (uchar*)init_list.begin()).copyTo(*this);
	}

	template<typename _Tp> inline
		Mat::Mat(const std::initializer_list<int> sizes, const std::initializer_list<_Tp> init_list)
		: Mat()
	{
		size_t size_total = 1;
		for (auto size : sizes)
			size_total *= size;
		DCHECK(init_list.size() != 0);
		DCHECK(size_total == init_list.size());
		Mat((int)sizes.size(), (int*)sizes.begin(), traits::Type<_Tp>::value, (uchar*)init_list.begin()).copyTo(*this);
	}

	inline
		unsigned char* Mat::ptr(int y)
	{
		DCHECK(y == 0 || (data && dims >= 1 && (unsigned)y < (unsigned)sizes[0]));
		return data + steps[0] * y;
	}

	inline
		const unsigned char* Mat::ptr(int y) const
	{
		DCHECK(y == 0 || (data && dims >= 1 && (unsigned)y < (unsigned)sizes[0]));
		return data + steps[0] * y;
	}

	template<typename _Tp> inline
		_Tp* Mat::ptr(int y)
	{
		DCHECK(y == 0 || (data && dims >= 1 && (unsigned)y < (unsigned)sizes[0]));
		return (_Tp*)(data + steps[0] * y);
	}

	template<typename _Tp> inline
		const _Tp* Mat::ptr(int y) const
	{
		DCHECK(y == 0 || (data && dims >= 1 && data && (unsigned)y < (unsigned)sizes[0]));
		return (const _Tp*)(data + steps[0] * y);
	}

	inline
		unsigned char* Mat::ptr(int i0, int i1)
	{
		DCHECK(dims >= 2);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)sizes[0]);
		DCHECK((unsigned)i1 < (unsigned)sizes[1]);
		return data + i0 * steps[0] + i1 * steps[1];
	}

	inline
		const unsigned char* Mat::ptr(int i0, int i1) const
	{
		DCHECK(dims >= 2);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)sizes[0]);
		DCHECK((unsigned)i1 < (unsigned)sizes[1]);
		return data + i0 * steps[0] + i1 * steps[1];
	}

	template<typename _Tp> inline
		_Tp* Mat::ptr(int i0, int i1)
	{
		DCHECK(dims >= 2);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)sizes[0]);
		DCHECK((unsigned)i1 < (unsigned)sizes[1]);
		return (_Tp*)(data + i0 * steps[0] + i1 * steps[1]);
	}

	template<typename _Tp> inline
		const _Tp* Mat::ptr(int i0, int i1) const
	{
		DCHECK(dims >= 2);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)sizes[0]);
		DCHECK((unsigned)i1 < (unsigned)sizes[1]);
		return (const _Tp*)(data + i0 * steps[0] + i1 * steps[1]);
	}

	inline
		unsigned char* Mat::ptr(int i0, int i1, int i2)
	{
		DCHECK(dims >= 3);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)sizes[0]);
		DCHECK((unsigned)i1 < (unsigned)sizes[1]);
		DCHECK((unsigned)i2 < (unsigned)sizes[2]);
		return data + i0 * steps[0] + i1 * steps[1] + i2 * steps[2];
	}

	inline
		const unsigned char* Mat::ptr(int i0, int i1, int i2) const
	{
		DCHECK(dims >= 3);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)sizes[0]);
		DCHECK((unsigned)i1 < (unsigned)sizes[1]);
		DCHECK((unsigned)i2 < (unsigned)sizes[2]);
		return data + i0 * steps[0] + i1 * steps[1] + i2 * steps[2];
	}

	template<typename _Tp> inline
		_Tp* Mat::ptr(int i0, int i1, int i2)
	{
		DCHECK(dims >= 3);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)sizes[0]);
		DCHECK((unsigned)i1 < (unsigned)sizes[1]);
		DCHECK((unsigned)i2 < (unsigned)sizes[2]);
		return (_Tp*)(data + i0 * steps[0] + i1 * steps[1] + i2 * steps[2]);
	}

	template<typename _Tp> inline
		const _Tp* Mat::ptr(int i0, int i1, int i2) const
	{
		DCHECK(dims >= 3);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)sizes[0]);
		DCHECK((unsigned)i1 < (unsigned)sizes[1]);
		DCHECK((unsigned)i2 < (unsigned)sizes[2]);
		return (const _Tp*)(data + i0 * steps[0] + i1 * steps[1] + i2 * steps[2]);
	}

	inline
		unsigned char* Mat::ptr(const int* idx)
	{
		int i, d = dims;
		unsigned char* p = data;
		DCHECK(d >= 1 && p);
		for (i = 0; i < d; i++)
		{
			DCHECK((unsigned)idx[i] < (unsigned)sizes[i]);
			p += idx[i] * steps[i];
		}
		return p;
	}

	inline
		const unsigned char* Mat::ptr(const int* idx) const
	{
		int i, d = dims;
		unsigned char* p = data;
		DCHECK(d >= 1 && p);
		for (i = 0; i < d; i++)
		{
			DCHECK((unsigned)idx[i] < (unsigned)sizes[i]);
			p += idx[i] * steps[i];
		}
		return p;
	}

	template<typename _Tp> inline
		_Tp& Mat::at(int i0, int i1)
	{
		DCHECK(dims <= 2);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)sizes[0]);
		//DCHECK((unsigned)(i1 * DataType<_Tp>::channels) < (unsigned)(sizes[1] * channels()));
		DCHECK(sizeof(_Tp) == elemSize());
		return ((_Tp*)(data + steps[0] * i0))[i1];
	}

	template<typename _Tp> inline
		const _Tp& Mat::at(int i0, int i1) const
	{
		DCHECK(dims <= 2);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)sizes[0]);
		//DCHECK((unsigned)(i1 * DataType<_Tp>::channels) < (unsigned)(sizes[1] * channels()));
		DCHECK(sizeof(_Tp) == elemSize());
		return ((const _Tp*)(data + steps[0] * i0))[i1];
	}

	template<typename _Tp> inline
		_Tp& Mat::at(int i0)
	{
		DCHECK(dims <= 2);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)(sizes[0] * sizes[1]));
		DCHECK(elemSize() == sizeof(_Tp));
		if (/*isContinuous() ||*/ sizes[0] == 1)
			return ((_Tp*)data)[i0];
		if (sizes[1] == 1)
			return *(_Tp*)(data + steps[0] * i0);
		int i = i0 / cols, j = i0 - i * cols;
		return ((_Tp*)(data + steps[0] * i))[j];
	}

	template<typename _Tp> inline
		const _Tp& Mat::at(int i0) const
	{
		DCHECK(dims <= 2);
		DCHECK(data);
		DCHECK((unsigned)i0 < (unsigned)(sizes[0] * sizes[1]));
		DCHECK(elemSize() == sizeof(_Tp));
		if (/*isContinuous() ||*/ sizes[0] == 1)
			return ((const _Tp*)data)[i0];
		if (sizes[1] == 1)
			return *(const _Tp*)(data + steps[0] * i0);
		int i = i0 / cols, j = i0 - i * cols;
		return ((const _Tp*)(data + steps[0] * i))[j];
	}

	template<typename _Tp> inline
		_Tp& Mat::at(int i0, int i1, int i2)
	{
		DCHECK(elemSize() == sizeof(_Tp));
		return *(_Tp*)ptr(i0, i1, i2);
	}

	template<typename _Tp> inline
		const _Tp& Mat::at(int i0, int i1, int i2) const
	{
		DCHECK(elemSize() == sizeof(_Tp));
		return *(const _Tp*)ptr(i0, i1, i2);
	}

	template<typename _Tp> inline
		_Tp& Mat::at(const int* idx)
	{
		DCHECK(elemSize() == sizeof(_Tp));
		return *(_Tp*)ptr(idx);
	}

	template<typename _Tp> inline
		const _Tp& Mat::at(const int* idx) const
	{
		DCHECK(elemSize() == sizeof(_Tp));
		return *(const _Tp*)ptr(idx);
	}



	////////////////////////////// MatT<_Tp> ////////////////////////////

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT()
	//	: Mat()
	//{
	//	flags = (flags & ~ACV_MAT_TYPE_MASK) + traits::Type<_Tp>::value;
	//}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(int _rows, int _cols)
	//	: Mat(_rows, _cols, traits::Type<_Tp>::value)
	//{
	//}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(int _dims, const int* _sz)
	//	: Mat(_dims, _sz, traits::Type<_Tp>::value)
	//{}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(std::vector<int>& _sizes):
	//	Mat(_sizes, traits::Type<_Tp>::value)
	//{}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(int _dims, const int* _sz, _Tp* _data, const size_t* _steps)
	//	: Mat(_dims, _sz, traits::Type<_Tp>::value, _data, _steps)
	//{}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(std::vector<int>& _sizes, _Tp* _data, const size_t* _steps)
	//	: Mat(_sizes, traits::Type<_Tp>::value, _data, _steps)
	//{}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(const MatT<_Tp>& m, const std::vector<Range>& ranges)
	//	: Mat(m, ranges)
	//{}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(const Mat& m)
	//	: Mat()
	//{
	//	flags = (flags & ~ACV_MAT_TYPE_MASK) + traits::Type<_Tp>::value;
	//	*this = m;
	//}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(const MatT& m)
	//	: Mat(m)
	//{}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(int _rows, int _cols, _Tp* _data, size_t steps)
	//	: Mat(_rows, _cols, traits::Type<_Tp>::value, _data, steps)
	//{}


	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(const MatT& m, const Rect& roi)
	//	: Mat(m, roi)
	//{}


	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(std::initializer_list<_Tp> list)
	//	: Mat(list)
	//{}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(const std::initializer_list<int> sizes, std::initializer_list<_Tp> list)
	//	: Mat(sizes, list)
	//{}


	//template<typename _Tp> inline
	//	MatT<_Tp>& MatT<_Tp>::operator = (const Mat& m)
	//{
	//	if (traits::Type<_Tp>::value == m.type())
	//	{
	//		Mat::operator = (m);
	//		return *this;
	//	}
	//	if (traits::Depth<_Tp>::value == m.depth())
	//	{
	//		return (*this = m.reshape(DataType<_Tp>::channels, m.dims, 0));
	//	}
	//	//CV_Assert(DataType<_Tp>::channels == m.channels() || m.empty());
	//	m.convertTo(*this, type());
	//	return *this;
	//}

	//template<typename _Tp> inline
	//	MatT<_Tp>& MatT<_Tp>::operator = (const MatT& m)
	//{
	//	Mat::operator=(m);
	//	return *this;
	//}


	//template<typename _Tp> inline
	//	void MatT<_Tp>::create(int _rows, int _cols)
	//{
	//	Mat::create(_rows, _cols, traits::Type<_Tp>::value);
	//}

	//
	//template<typename _Tp> inline
	//	void MatT<_Tp>::create(const Size& _size)
	//{
	//	Mat::create(size.height, size.width, traits::Type<_Tp>::value);
	//}

	//template<typename _Tp> inline
	//	void MatT<_Tp>::create(int _dims, const int* _sz)
	//{
	//	Mat::create(_dims, _sz, traits::Type<_Tp>::value);
	//}

	//template<typename _Tp> inline
	//	void MatT<_Tp>::create(const std::vector<int>& _sizes)
	//{
	//	Mat::create(_sizes, traits::Type<_Tp>::value);
	//}

	//template<typename _Tp> inline
	//	void MatT<_Tp>::release()
	//{
	//	Mat::release();
	//}



	//template<typename _Tp> inline
	//	MatT<_Tp> MatT<_Tp>::clone() const
	//{
	//	return MatT(Mat::clone());
	//}

	//template<typename _Tp> inline
	//	size_t MatT<_Tp>::elemSize() const
	//{
	//	DCHECK(Mat::elemSize() == sizeof(_Tp));
	//	return sizeof(_Tp);
	//}

	//template<typename _Tp> inline
	//	size_t MatT<_Tp>::elemSize1() const
	//{
	//	return Mat::elemSize1();
	//}

	//template<typename _Tp> inline
	//	int MatT<_Tp>::type() const
	//{
	//	DCHECK(Mat::type() == traits::Type<_Tp>::value);
	//	return traits::Type<_Tp>::value;
	//}

	//template<typename _Tp> inline
	//	int MatT<_Tp>::depth() const
	//{
	//	DCHECK(Mat::depth() == traits::Depth<_Tp>::value);
	//	return traits::Depth<_Tp>::value;
	//}

	//template<typename _Tp> inline
	//	int MatT<_Tp>::channels() const
	//{		
	//	return Mat::channels();
	//}

	//template<typename _Tp> inline
	//	size_t MatT<_Tp>::stepT(int i) const
	//{
	//	return steps[i] / elemSize();
	//}

	//template<typename _Tp> inline
	//	size_t MatT<_Tp>::step1(int i) const
	//{
	//	return steps[i] / elemSize1();
	//}


	//template<typename _Tp> inline
	//	MatT<_Tp> MatT<_Tp>::operator()(const Rect& roi) const
	//{
	//	return MatT<_Tp>(*this, roi);
	//}

	//template<typename _Tp> inline
	//	MatT<_Tp> MatT<_Tp>::operator()(const Range* ranges) const
	//{
	//	return MatT<_Tp>(*this, ranges);
	//}

	//template<typename _Tp> inline
	//	MatT<_Tp> MatT<_Tp>::operator()(const std::vector<Range>& ranges) const
	//{
	//	return MatT<_Tp>(*this, ranges);
	//}

	//template<typename _Tp> inline
	//	_Tp* MatT<_Tp>::operator [](int y)
	//{
	//	DCHECK(0 <= y && y < size.p[0]);
	//	return (_Tp*)(data + y * step.p[0]);
	//}

	//template<typename _Tp> inline
	//	const _Tp* MatT<_Tp>::operator [](int y) const
	//{
	//	DCHECK(0 <= y && y < size.p[0]);
	//	return (const _Tp*)(data + y * step.p[0]);
	//}

	//template<typename _Tp> inline
	//	_Tp& MatT<_Tp>::operator ()(int i0, int i1)
	//{
	//	DCHECK(dims <= 2);
	//	DCHECK(data);
	//	DCHECK((unsigned)i0 < (unsigned)sizes[0]);
	//	DCHECK((unsigned)i1 < (unsigned)sizes[1]);
	//	DCHECK(type() == traits::Type<_Tp>::value);
	//	return ((_Tp*)(data + steps[0] * i0))[i1];
	//}

	//template<typename _Tp> inline
	//	const _Tp& MatT<_Tp>::operator ()(int i0, int i1) const
	//{
	//	DCHECK(dims <= 2);
	//	DCHECK(data);
	//	DCHECK((unsigned)i0 < (unsigned)size.p[0]);
	//	DCHECK((unsigned)i1 < (unsigned)size.p[1]);
	//	DCHECK(type() == traits::Type<_Tp>::value);
	//	return ((const _Tp*)(data + steps[0] * i0))[i1];
	//}

	//

	//template<typename _Tp> inline
	//	_Tp& MatT<_Tp>::operator ()(const int* idx)
	//{
	//	return Mat::at<_Tp>(idx);
	//}

	//template<typename _Tp> inline
	//	const _Tp& MatT<_Tp>::operator ()(const int* idx) const
	//{
	//	return Mat::at<_Tp>(idx);
	//}
	//   
	//template<typename _Tp> inline
	//	_Tp& MatT<_Tp>::operator ()(int i0)
	//{
	//	return this->at<_Tp>(i0);
	//}

	//template<typename _Tp> inline
	//	const _Tp& MatT<_Tp>::operator ()(int i0) const
	//{
	//	return this->at<_Tp>(i0);
	//}

	//template<typename _Tp> inline
	//	_Tp& MatT<_Tp>::operator ()(int i0, int i1, int i2)
	//{
	//	return this->at<_Tp>(i0, i1, i2);
	//}

	//template<typename _Tp> inline
	//	const _Tp& MatT<_Tp>::operator ()(int i0, int i1, int i2) const
	//{
	//	return this->at<_Tp>(i0, i1, i2);

	//}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(MatT&& m)
	//	: Mat(m)
	//{
	//}

	//template<typename _Tp> inline
	//	MatT<_Tp>& MatT<_Tp>::operator = (MatT&& m)
	//{
	//	Mat::operator = (std::move(m));
	//	return *this;
	//}

	//template<typename _Tp> inline
	//	MatT<_Tp>::MatT(Mat&& m)
	//	: Mat()
	//{
	//	flags = (flags & ~ACV_MAT_TYPE_MASK) + traits::Type<_Tp>::value;
	//	*this = m;
	//}

	//template<typename _Tp> inline
	//	MatT<_Tp>& MatT<_Tp>::operator = (Mat&& m)
	//{
	//	if (traits::Type<_Tp>::value == m.type())
	//	{
	//		Mat::operator = ((Mat&&)m);
	//		return *this;
	//	}
	//	if (traits::Depth<_Tp>::value == m.depth())
	//	{
	//		Mat::operator = ((Mat&&)m.reshape(DataType<_Tp>::channels, m.dims, 0));
	//		return *this;
	//	}
	//	CV_DbgAssert(DataType<_Tp>::channels == m.channels());
	//	m.convertTo(*this, type());
	//	return *this;
	//}
}

#endif //