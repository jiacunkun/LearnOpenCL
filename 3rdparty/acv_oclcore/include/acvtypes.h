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
* @brief Definision of Point, Point3D, Size, Rect, Range which are compatible with opencv
*
* @author Lei Hua
* @date 2017-11-08
*/

#ifndef  __ACV_TYPES_H__
#define __ACV_TYPES_H__

#include <algorithm>
#include <limits.h>
#include <limits>
#include <cmath>
#include "acvdef.h"

namespace acv {
	template<typename _Tp> class _Point
	{
	public:
		typedef _Tp value_type;

		// various constructors
		_Point()
			:x(0), y(0) {}

		_Point(_Tp _x, _Tp _y)
			:x(_x), y(_y) {}

		_Point(const _Tp data[2])
			: x(data[0]), y(data[1]) {}

		_Point(const _Point& pt)
			:x(pt.x), y(pt.y) {}

		_Point& operator = (const _Point& pt)
		{
			x = pt.x; y = pt.y;
			return *this;
		}
		//! conversion to another data type
		template<typename _Tp2> operator _Point<_Tp2>() const
		{
			return _Point<_Tp2>(static_cast<_Tp2>(x), static_cast<_Tp2>(y));
		}

		//! dot product
		_Tp dot(const _Point& pt) const
		{
			return x*pt.x + y*pt.y;
		}
		//! dot product computed in double-precision arithmetics
		double ddot(const _Point& pt) const
		{
			return (double)x*pt.x + (double)y*pt.y;
		}
		//! cross-product
		double cross(const _Point& pt) const
		{
			return (double)x*pt.y - (double)y*pt.x;
		}

		_Tp x, y; //< the point coordinates
	};

	typedef _Point<int> Point2i;	
	typedef _Point<float> Point2f;
	typedef _Point<double> Point2d;
	typedef Point2i Point;

	template<typename _Tp> class _Point3
	{
	public:
		typedef _Tp value_type;

		// various constructors
		_Point3()
			: x(0), y(0), z(0) {}
		_Point3(_Tp _x, _Tp _y, _Tp _z)
			: x(_x), y(_y), z(_z) {}
		_Point3(const _Point3& pt)
			: x(pt.x), y(pt.y), z(pt.z) {}
		_Point3(const _Tp data[3])
			: x(data[0]), y(data[1]), z(data[2]) {}
		explicit _Point3(const _Point<_Tp>& pt)
			: x(pt.x), y(pt.y), z(_Tp()) {}

		_Point3& operator = (const _Point3& pt)
		{
			x = pt.x; y = pt.y; z = pt.z;
			return *this;
		}
		//! conversion to another data type
		template<typename _Tp2> operator _Point3<_Tp2>() const
		{
			return _Point3<_Tp2>(static_cast<_Tp2>(x), static_cast<_Tp2>(y), static_cast<_Tp2>(z));
		}

		//! dot product
		_Tp dot(const _Point3& pt) const
		{
			return static_cast<_Tp>(x*pt.x + y*pt.y + z*pt.z);
		}
		//! dot product computed in double-precision arithmetics
		double ddot(const _Point3& pt) const
		{
			return (double)x*pt.x + (double)y*pt.y + (double)z*pt.z;
		}
		//! cross product of the 2 3D points
		_Point3 cross(const _Point3& pt) const
		{
			return _Point3<_Tp>(y*pt.z - z*pt.y, z*pt.x - x*pt.z, x*pt.y - y*pt.x);
		}

		_Tp x, y, z; //< the point coordinates
	};

	typedef _Point3<int> Point3i;
	typedef _Point3<float> Point3f;
	typedef _Point3<double> Point3d;

	template<typename _Tp> class _Size
	{
	public:
		typedef _Tp value_type;

		//! various constructors
		_Size() : width(0), height(0) {}
		_Size(_Tp _width, _Tp _height) : width(_width), height(_height) {}
		_Size(const _Size& sz) : width(sz.width), height(sz.height) {}
		_Size(const _Point<_Tp>& pt) : width(pt.x), height(pt.y) {}

		_Size& operator = (const _Size& sz)
		{
			width = sz.width; height = sz.height;
			return *this;
		}
		//! the area (width*height)
		_Tp area() const
		{
			const _Tp result = width * height;
			//assert(!std::numeric_limits<_Tp>::is_integer
			//	|| width == 0 || result / width == height); // make sure the result fits in the return value
			return result;
		}
		//! true if empty
		bool empty() const
		{
			return width <= 0 || height <= 0;
		}

		//! conversion of another data type.
		template<typename _Tp2> operator _Size<_Tp2>() const
		{
			return _Size<_Tp2>(static_cast<_Tp2>(width), static_cast<_Tp2>(height));
		}

		_Tp width, height; // the width and the height
	};

	typedef _Size<int> Size2i;
	//typedef _Size<int64> Size2l;
	typedef _Size<float> Size2f;
	typedef _Size<double> Size2d;
	typedef Size2i Size;

	template<typename _Tp> class _Rect
	{
	public:
		typedef _Tp value_type;

		//! various constructors
		_Rect()
			: x(0), y(0), width(0), height(0) {}
		_Rect(_Tp _x, _Tp _y, _Tp _width, _Tp _height)
			: x(_x), y(_y), width(_width), height(_height) {}
		_Rect(const _Rect& r)
			: x(r.x), y(r.y), width(r.width), height(r.height) {}
		_Rect(const _Point<_Tp>& org, const _Size<_Tp>& sz)
			: x(org.x), y(org.y), width(sz.width), height(sz.height) {}

		_Rect(const _Point<_Tp>& pt1, const _Point<_Tp>& pt2)
		{
			x = std::min(pt1.x, pt2.x);
			y = std::min(pt1.y, pt2.y);
			width = std::max(pt1.x, pt2.x) - x;
			height = std::max(pt1.y, pt2.y) - y;
		}

		_Rect& operator = (const _Rect& r)
		{
			x = r.x;
			y = r.y;
			width = r.width;
			height = r.height;
			return *this;
		}

		//! the top-left corner
		_Point<_Tp> tl() const
		{
			return _Point<_Tp>(x, y);
		}
		//! the bottom-right corner
		_Point<_Tp> br() const
		{
			return _Point<_Tp>(x + width, y + height);
		}

		//! size (width, height) of the rectangle
		_Size<_Tp> size() const
		{
			return _Size<_Tp>(width, height);
		}

		//! area (width*height) of the rectangle
		_Tp area() const
		{
			const _Tp result = width * height;
			assert(!std::numeric_limits<_Tp>::is_integer
				|| width == 0 || result / width == height); // make sure the result fits in the return value
			return result;
		}
		//! true if empty
		bool empty() const
		{
			return width <= 0 || height <= 0;
		}

		//! conversion to another data type
		template<typename _Tp2> operator _Rect<_Tp2>() const
		{
			return _Rect<_Tp2>(static_cast<_Tp2>(x), static_cast<_Tp2>(y), static_cast<_Tp2>(width), static_cast<_Tp2>(height));
		}

		//! checks whether the rectangle contains the point
		bool contains(const _Point<_Tp>& pt) const
		{
			return x <= pt.x && pt.x < x + width && y <= pt.y && pt.y < y + height;
		}

		_Tp x, y, width, height; //< the top-left corner, as well as width and height of the rectangle
	};

	typedef _Rect<int> Rect2i;
	typedef _Rect<float> Rect2f;
	typedef _Rect<double> Rect2d;
	typedef Rect2i Rect;


	class ACV_EXPORTS Range
	{
	public:
		Range()
			:start(0), end(0)
		{}

		Range(int _start, int _end)
			:start(_start), end(_end)
		{}

		int size() const 
		{
			return end - start;
		}

		bool empty() const
		{
			return end == start;
		}
		static Range all()
		{
			return Range(INT_MIN, INT_MAX);
		}

		int start, end;
	};


	///////////////////// operations /////////////////////////////

	////////////////////// Point /////////////////////////
	template<typename _Tp> static inline
		_Point<_Tp>& operator += (_Point<_Tp>& lhs, const _Point<_Tp>& rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		return lhs;
	}

	template<typename _Tp> static inline
		_Point<_Tp>& operator -= (_Point<_Tp>& lhs, const _Point<_Tp>& rhs)
	{
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		return lhs;
	}

	template<typename _Tp> static inline
		_Point<_Tp>& operator *= (_Point<_Tp>& lhs, int rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x * rhs);
		lhs.y = static_cast<_Tp>(lhs.y * rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		_Point<_Tp>& operator *= (_Point<_Tp>& lhs, float rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x * rhs);
		lhs.y = static_cast<_Tp>(lhs.y * rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		_Point<_Tp>& operator *= (_Point<_Tp>& lhs, double rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x * rhs);
		lhs.y = static_cast<_Tp>(lhs.y * rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		_Point<_Tp>& operator /= (_Point<_Tp>& lhs, int rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x / rhs);
		lhs.y = static_cast<_Tp>(lhs.y / rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		_Point<_Tp>& operator /= (_Point<_Tp>& lhs, float rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x / rhs);
		lhs.y = static_cast<_Tp>(lhs.y / rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		_Point<_Tp>& operator /= (_Point<_Tp>& lhs, double rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x / rhs);
		lhs.y = static_cast<_Tp>(lhs.y / rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		double norm(const _Point<_Tp>& pt)
	{
		return std::sqrt((double)pt.x*pt.x + (double)pt.y*pt.y);
	}

	template<typename _Tp> static inline
		bool operator == (const _Point<_Tp>& lhs, const _Point<_Tp>& rhs)
	{
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	template<typename _Tp> static inline
		bool operator != (const _Point<_Tp>& lhs, const _Point<_Tp>& rhs)
	{
		return lhs.x != rhs.x || lhs.y != rhs.y;
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator + (const _Point<_Tp>& lhs, const _Point<_Tp>& rhs)
	{
		return _Point<_Tp>(static_cast<_Tp>(lhs.x + rhs.x), static_cast<_Tp>(lhs.y + rhs.y));
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator - (const _Point<_Tp>& lhs, const _Point<_Tp>& rhs)
	{
		return _Point<_Tp>(static_cast<_Tp>(lhs.x - rhs.x), static_cast<_Tp>(lhs.y - rhs.y));
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator - (const _Point<_Tp>& lhs)
	{
		return _Point<_Tp>(static_cast<_Tp>(-lhs.x), static_cast<_Tp>(-lhs.y));
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator * (const _Point<_Tp>& lhs, int rhs)
	{
		return _Point<_Tp>(static_cast<_Tp>(lhs.x*rhs), static_cast<_Tp>(lhs.y*rhs));
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator * (int lhs, const _Point<_Tp>& rhs)
	{
		return _Point<_Tp>(static_cast<_Tp>(rhs.x*lhs), static_cast<_Tp>(rhs.y*lhs));
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator * (const _Point<_Tp>& lhs, float rhs)
	{
		return _Point<_Tp>(static_cast<_Tp>(lhs.x*rhs), static_cast<_Tp>(lhs.y*rhs));
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator * (float lhs, const _Point<_Tp>& rhs)
	{
		return _Point<_Tp>(static_cast<_Tp>(rhs.x*lhs), static_cast<_Tp>(rhs.y*lhs));
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator * (const _Point<_Tp>& lhs, double rhs)
	{
		return _Point<_Tp>(static_cast<_Tp>(lhs.x*rhs), static_cast<_Tp>(lhs.y*rhs));
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator * (double lhs, const _Point<_Tp>& rhs)
	{
		return _Point<_Tp>(static_cast<_Tp>(rhs.x*lhs), static_cast<_Tp>(rhs.y*lhs));
	}


	template<typename _Tp> static inline
		_Point<_Tp> operator / (const _Point<_Tp>& lhs, int rhs)
	{
		_Point<_Tp> tmp(lhs);
		tmp /= rhs;
		return tmp;
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator / (const _Point<_Tp>& lhs, float rhs)
	{
		_Point<_Tp> tmp(lhs);
		tmp /= rhs;
		return tmp;
	}

	template<typename _Tp> static inline
		_Point<_Tp> operator / (const _Point<_Tp>& lhs, double rhs)
	{
		_Point<_Tp> tmp(lhs);
		tmp /= rhs;
		return tmp;
	}


	/////////////////////// Point3 //////////////////////////////////////

	template<typename _Tp> static inline
		_Point3<_Tp>& operator += (_Point3<_Tp>& lhs, const _Point3<_Tp>& rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		lhs.z += rhs.z;
		return lhs;
	}

	template<typename _Tp> static inline
		_Point3<_Tp>& operator -= (_Point3<_Tp>& lhs, const _Point3<_Tp>& rhs)
	{
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		lhs.z -= rhs.z;
		return lhs;
	}

	template<typename _Tp> static inline
		_Point3<_Tp>& operator *= (_Point3<_Tp>& lhs, int rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x * rhs);
		lhs.y = static_cast<_Tp>(lhs.y * rhs);
		lhs.z = static_cast<_Tp>(lhs.z * rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		_Point3<_Tp>& operator *= (_Point3<_Tp>& lhs, float rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x * rhs);
		lhs.y = static_cast<_Tp>(lhs.y * rhs);
		lhs.z = static_cast<_Tp>(lhs.z * rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		_Point3<_Tp>& operator *= (_Point3<_Tp>& lhs, double rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x * rhs);
		lhs.y = static_cast<_Tp>(lhs.y * rhs);
		lhs.z = static_cast<_Tp>(lhs.z * rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		_Point3<_Tp>& operator /= (_Point3<_Tp>& lhs, int rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x / rhs);
		lhs.y = static_cast<_Tp>(lhs.y / rhs);
		lhs.z = static_cast<_Tp>(lhs.z / rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		_Point3<_Tp>& operator /= (_Point3<_Tp>& lhs, float rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x / rhs);
		lhs.y = static_cast<_Tp>(lhs.y / rhs);
		lhs.z = static_cast<_Tp>(lhs.z / rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		_Point3<_Tp>& operator /= (_Point3<_Tp>& lhs, double rhs)
	{
		lhs.x = static_cast<_Tp>(lhs.x / rhs);
		lhs.y = static_cast<_Tp>(lhs.y / rhs);
		lhs.z = static_cast<_Tp>(lhs.z / rhs);
		return lhs;
	}

	template<typename _Tp> static inline
		double norm(const _Point3<_Tp>& pt)
	{
		return std::sqrt((double)pt.x*pt.x + (double)pt.y*pt.y + (double)pt.z*pt.z);
	}

	template<typename _Tp> static inline
		bool operator == (const _Point3<_Tp>& lhs, const _Point3<_Tp>& rhs)
	{
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
	}

	template<typename _Tp> static inline
		bool operator != (const _Point3<_Tp>& lhs, const _Point3<_Tp>& rhs)
	{
		return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
	}

	template<typename _Tp> static inline
		_Point3<_Tp> operator + (const _Point3<_Tp>& lhs, const _Point3<_Tp>& rhs)
	{
		return _Point3<_Tp>(static_cast<_Tp>(lhs.x + rhs.x), static_cast<_Tp>(lhs.y + rhs.y), static_cast<_Tp>(lhs.z + rhs.z));
	}

	template<typename _Tp> static inline
		_Point3<_Tp> operator - (const _Point3<_Tp>& lhs, const _Point3<_Tp>& rhs)
	{
		return _Point3<_Tp>(static_cast<_Tp>(lhs.x - rhs.x), static_cast<_Tp>(lhs.y - rhs.y), static_cast<_Tp>(lhs.z - rhs.z));
	}

	template<typename _Tp> static inline
		_Point3<_Tp> operator - (const _Point3<_Tp>& lhs)
	{
		return _Point3<_Tp>(static_cast<_Tp>(-lhs.x), static_cast<_Tp>(-lhs.y), static_cast<_Tp>(-lhs.z));
	}

	template<typename _Tp> static inline
		_Point3<_Tp> operator * (const _Point3<_Tp>& lhs, int rhs)
	{
		return _Point3<_Tp>(static_cast<_Tp>(lhs.x*rhs), static_cast<_Tp>(lhs.y*rhs), static_cast<_Tp>(lhs.z*rhs));
	}

	template<typename _Tp> static inline
		_Point3<_Tp> operator * (int lhs, const _Point3<_Tp>& rhs)
	{
		return _Point3<_Tp>(static_cast<_Tp>(rhs.x * lhs), static_cast<_Tp>(rhs.y * lhs), static_cast<_Tp>(rhs.z * lhs));
	}

	template<typename _Tp> static inline
		_Point3<_Tp> operator * (const _Point3<_Tp>& lhs, float rhs)
	{
		return _Point3<_Tp>(static_cast<_Tp>(lhs.x * rhs), static_cast<_Tp>(lhs.y * rhs), static_cast<_Tp>(lhs.z * rhs));
	}

	template<typename _Tp> static inline
		_Point3<_Tp> operator * (float lhs, const _Point3<_Tp>& rhs)
	{
		return _Point3<_Tp>(static_cast<_Tp>(rhs.x * lhs), static_cast<_Tp>(rhs.y * lhs), static_cast<_Tp>(rhs.z * lhs));
	}

	template<typename _Tp> static inline
		_Point3<_Tp> operator * (const _Point3<_Tp>& lhs, double rhs)
	{
		return _Point3<_Tp>(static_cast<_Tp>(lhs.x * rhs), static_cast<_Tp>(lhs.y * rhs), static_cast<_Tp>(lhs.z * rhs));
	}

	template<typename _Tp> static inline
		_Point3<_Tp> operator * (double lhs, const _Point3<_Tp>& rhs)
	{
		return _Point3<_Tp>(static_cast<_Tp>(rhs.x * lhs), static_cast<_Tp>(rhs.y * lhs), static_cast<_Tp>(rhs.z * lhs));
	}

	////////////////////  Size  /////////////////////////////////
	template<typename _Tp> static inline
		_Size<_Tp>& operator *= (_Size<_Tp>& lhs, _Tp rhs)
	{
		lhs.width *= rhs;
		lhs.height *= rhs;
		return lhs;
	}

	template<typename _Tp> static inline
		_Size<_Tp> operator * (const _Size<_Tp>& lhs, _Tp rhs)
	{
		_Size<_Tp> tmp(lhs);
		tmp *= rhs;
		return tmp;
	}

	template<typename _Tp> static inline
		_Size<_Tp>& operator /= (_Size<_Tp>& lhs, _Tp rhs)
	{
		lhs.width /= rhs;
		lhs.height /= rhs;
		return lhs;
	}

	template<typename _Tp> static inline
		_Size<_Tp> operator / (const _Size<_Tp>& lhs, _Tp rhs)
	{
		_Size<_Tp> tmp(lhs);
		tmp /= rhs;
		return tmp;
	}

	template<typename _Tp> static inline
		_Size<_Tp>& operator += (_Size<_Tp>& lhs, const _Size<_Tp>& rhs)
	{
		lhs.width += rhs.width;
		lhs.height += rhs.height;
		return lhs;
	}

	template<typename _Tp> static inline
		_Size<_Tp> operator + (const _Size<_Tp>& lhs, const _Size<_Tp>& rhs)
	{
		_Size<_Tp> tmp(lhs);
		tmp += rhs;
		return tmp;
	}

	template<typename _Tp> static inline
		_Size<_Tp>& operator -= (_Size<_Tp>& lhs, const _Size<_Tp>& rhs)
	{
		lhs.width -= rhs.width;
		lhs.height -= rhs.height;
		return lhs;
	}

	template<typename _Tp> static inline
		_Size<_Tp> operator - (const _Size<_Tp>& lhs, const _Size<_Tp>& rhs)
	{
		_Size<_Tp> tmp(lhs);
		tmp -= rhs;
		return tmp;
	}

	template<typename _Tp> static inline
		bool operator == (const _Size<_Tp>& lhs, const _Size<_Tp>& rhs)
	{
		return lhs.width == rhs.width && lhs.height == rhs.height;
	}

	template<typename _Tp> static inline
		bool operator != (const _Size<_Tp>& lhs, const _Size<_Tp>& rhs)
	{
		return !(lhs == rhs);
	}


	/////////////////////// Rect ///////////////////////////////////

	template<typename _Tp> static inline
		_Rect<_Tp>& operator += (_Rect<_Tp>& lhs, const _Point<_Tp>& rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		return lhs;
	}

	template<typename _Tp> static inline
		_Rect<_Tp>& operator -= (_Rect<_Tp>& lhs, const _Point<_Tp>& rhs)
	{
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		return lhs;
	}

	template<typename _Tp> static inline
		_Rect<_Tp>& operator += (_Rect<_Tp>& lhs, const _Size<_Tp>& rhs)
	{
		lhs.width += rhs.width;
		lhs.height += rhs.height;
		return lhs;
	}

	template<typename _Tp> static inline
		_Rect<_Tp>& operator -= (_Rect<_Tp>& lhs, const _Size<_Tp>& rhs)
	{
		lhs.width -= rhs.width;
		lhs.height -= rhs.height;
		return lhs;
	}

	template<typename _Tp> static inline
		_Rect<_Tp>& operator &= (_Rect<_Tp>& lhs, const _Rect<_Tp>& rhs)
	{
		_Tp x1 = std::max(lhs.x, rhs.x);
		_Tp y1 = std::max(lhs.y, rhs.y);
		lhs.width = std::min(lhs.x + lhs.width, rhs.x + rhs.width) - x1;
		lhs.height = std::min(lhs.y + lhs.height, rhs.y + rhs.height) - y1;
		lhs.x = x1;
		lhs.y = y1;
		if (lhs.width <= 0 || lhs.height <= 0)
			lhs = Rect();
		return lhs;
	}

	template<typename _Tp> static inline
		_Rect<_Tp>& operator |= (_Rect<_Tp>& lhs, const _Rect<_Tp>& rhs)
	{
		if (lhs.empty()) {
			lhs = rhs;
		}
		else if (!rhs.empty()) {
			_Tp x1 = std::min(lhs.x, rhs.x);
			_Tp y1 = std::min(lhs.y, rhs.y);
			lhs.width = std::max(lhs.x + lhs.width, rhs.x + rhs.width) - x1;
			lhs.height = std::max(lhs.y + lhs.height, rhs.y + rhs.height) - y1;
			lhs.x = x1;
			lhs.y = y1;
		}
		return lhs;
	}

	template<typename _Tp> static inline
		bool operator == (const _Rect<_Tp>& lhs, const _Rect<_Tp>& rhs)
	{
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width && lhs.height == rhs.height;
	}

	template<typename _Tp> static inline
		bool operator != (const _Rect<_Tp>& lhs, const _Rect<_Tp>& rhs)
	{
		return lhs.x != rhs.x || lhs.y != rhs.y || lhs.width != rhs.width || lhs.height != rhs.height;
	}

	template<typename _Tp> static inline
		_Rect<_Tp> operator + (const _Rect<_Tp>& lhs, const _Point<_Tp>& rhs)
	{
		return _Rect<_Tp>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.width, lhs.height);
	}

	template<typename _Tp> static inline
		_Rect<_Tp> operator - (const _Rect<_Tp>& lhs, const _Point<_Tp>& rhs)
	{
		return _Rect<_Tp>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.width, lhs.height);
	}

	template<typename _Tp> static inline
		_Rect<_Tp> operator + (const _Rect<_Tp>& lhs, const _Size<_Tp>& rhs)
	{
		return _Rect<_Tp>(lhs.x, lhs.y, lhs.width + rhs.width, lhs.height + rhs.height);
	}

	template<typename _Tp> static inline
		_Rect<_Tp> operator & (const _Rect<_Tp>& lhs, const _Rect<_Tp>& rhs)
	{
		_Rect<_Tp> c = lhs;
		return c &= rhs;
	}

	template<typename _Tp> static inline
		_Rect<_Tp> operator | (const _Rect<_Tp>& lhs, const _Rect<_Tp>& rhs)
	{
		_Rect<_Tp> c = lhs;
		return c |= rhs;
	}


	///////////////// Range /////////////////////////

	static inline
		bool operator == (const Range& lhs, const Range& rhs)
	{
		return lhs.start == rhs.start && lhs.end == rhs.end;
	}

	static inline
		bool operator != (const Range& lhs, const Range& rhs)
	{
		return !(lhs == rhs);
	}

	static inline
		bool operator !(const Range& r)
	{
		return r.start == r.end;
	}

	static inline
		Range operator & (const Range& lhs, const Range& rhs)
	{
		Range r(std::max(lhs.start, rhs.start), std::min(lhs.end, rhs.end));
		r.end = std::max(r.end, r.start);
		return r;
	}

	static inline
		Range& operator &= (Range& lhs, const Range& rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	static inline
		Range operator + (const Range& lhs, int delta)
	{
		return Range(lhs.start + delta, lhs.end + delta);
	}

	static inline
		Range operator + (int delta, const Range& lhs)
	{
		return Range(lhs.start + delta, lhs.end + delta);
	}

	static inline
		Range operator - (const Range& lhs, int delta)
	{
		return lhs + (-delta);
	}

	////////////////// Vec //////////////////////////////////
	template<typename _Tp, int cn> class Vec
	{
	public:
		//! default constructor
		Vec() {};

		Vec(_Tp v0)
		{
			val[0] = v0;
		}
		Vec(_Tp v0, _Tp v1)
		{
			val[0] = v0;
			val[1] = v1;
		}
		Vec(_Tp v0, _Tp v1, _Tp v2)
		{
			val[0] = v0;
			val[1] = v1;
			val[2] = v2;
		}
		Vec(_Tp v0, _Tp v1, _Tp v2, _Tp v3)
		{
			val[0] = v0;
			val[1] = v1;
			val[2] = v2;
			val[3] = v3;
		}

		explicit Vec(const _Tp* values)
		{
			for (int i = 0; i < cn; i++)
			{
				val[i] = values[i];
			}
		}

		Vec(const Vec<_Tp, cn>& v)
		{
			for (int i = 0; i < cn; i++)
			{
				val[i] = v[i];
			}
		}

		static Vec all(_Tp alpha)
		{
			Vec vec;
			for (int i = 0; i < cn; i++)
			{
				vec.val[i] = alpha;
			}
			return vec;
		}

		/*! element access */
		const _Tp& operator [](int i) const
		{
			return val[i];
		}
		_Tp& operator[](int i)
		{
			return val[i];
		}
		const _Tp& operator ()(int i) const
		{
			return val[i];
		}
		_Tp& operator ()(int i)
		{
			return val[i];
		}

		_Tp val[cn];
	};

	/** @name Shorter aliases for the most popular specializations of Vec<T,n>
	@{
	*/
	typedef Vec<unsigned char, 2> Vec2b;
	typedef Vec<unsigned char, 3> Vec3b;
	typedef Vec<unsigned char, 4> Vec4b;

	typedef Vec<float, 2> Vec2f;
	typedef Vec<float, 3> Vec3f;
	typedef Vec<float, 4> Vec4f;


	namespace traits
	{
		template<typename T>
		struct Depth
		{
			enum { value = -1 };
		};

		template<typename T>
		struct Type
		{
			enum { value = ACV_MAKETYPE(Depth<T>::value, 1)};
		};


#define DEPTH_TYPE_DEFINE_NORMAL(name, type) \
        template<> struct Depth<name>	{enum { value = type }; }; \
		template<> struct Type<name> { enum { value = ACV_MAKETYPE(type, 1) }; }

		DEPTH_TYPE_DEFINE_NORMAL(bool, ACV_8U);
		DEPTH_TYPE_DEFINE_NORMAL(uchar, ACV_8U);
		DEPTH_TYPE_DEFINE_NORMAL(schar, ACV_8S);
		DEPTH_TYPE_DEFINE_NORMAL(ushort, ACV_16U);
		DEPTH_TYPE_DEFINE_NORMAL(short, ACV_16S);
		DEPTH_TYPE_DEFINE_NORMAL(int, ACV_32S);
		DEPTH_TYPE_DEFINE_NORMAL(float, ACV_32F);
		DEPTH_TYPE_DEFINE_NORMAL(double, ACV_64F);

#undef DEPTH_TYPE_DEFINE_NORMAL
		


		
#define DEPTH_TYPE_DEFINE_CLASS(name, channels) \
        template<typename _Tp> \
		struct Depth< name<_Tp> > { enum { value = Depth<_Tp>::value }; }; \
		template<typename _Tp> \
		struct Type< name<_Tp> > { enum { value = ACV_MAKETYPE(Depth<_Tp>::value, channels) }; } 

		DEPTH_TYPE_DEFINE_CLASS(_Point, 2);
		DEPTH_TYPE_DEFINE_CLASS(_Point3, 3);
		DEPTH_TYPE_DEFINE_CLASS(_Size, 2);
		DEPTH_TYPE_DEFINE_CLASS(_Rect, 4);
#undef  DEPTH_TYPE_DEFINE_CLASS


		template<typename _Tp, int cn>
		struct Depth< Vec<_Tp, cn> > { enum { value = Depth<_Tp>::value }; };
		template<typename _Tp, int cn>
		struct Type< Vec<_Tp, cn> > { enum { value = ACV_MAKETYPE(Depth<_Tp>::value, cn) }; };


		template<>
		struct Depth< Range > { enum { value = Depth<int>::value }; };
		template<>
		struct Type< Range > { enum { value = ACV_MAKETYPE(Depth<int>::value, 2) }; };
	}
	
};



#endif // ! __ACV_TYPES_H__
