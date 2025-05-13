#pragma once

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <QString>
#include <vector>

typedef unsigned char       BYTE;
#ifndef UNICODE

typedef std::string tstring;
typedef std::ostringstream tostringstream;
#define _tatoi atoi
#define _tfopen fopen
#define _tfgets fgets

#else

typedef std::wstring tstring;
typedef std::wostringstream tostringstream;
#define _tatoi _wtoi 
#define _tfopen _wfopen
#define _tfgets fgetws

#endif


#ifndef SAFE_FREE
#define SAFE_FREE(p) if(p != NULL) {free(p); p = NULL;}
#endif
#ifndef SAFE_DELETE
#define SAFE_DELETE(x) { if (x) delete x; x = NULL; }	//定义安全释放指针函数
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete [] (p); (p) = NULL;} }	
#endif
#ifndef SAFE_DELETE_ARRAY2
#define SAFE_DELETE_ARRAY2(p, n) { if (p != NULL) { for (int i = 0; i < n; i ++) { SAFE_DELETE_ARRAY(p[i]); } SAFE_DELETE_ARRAY(p); n = 0; } }	
#endif
#ifndef SAFE_DELETEARRAY
#define SAFE_DELETEARRAY(x) { if (x) delete[] x; x = NULL; }	// 安全释放指针数组
#endif

//#ifndef Max
//#define Max(a,b) ((a)>(b)?(a):(b))
//#endif // !Max
//#ifndef Min
//#define Min(a,b) ((a)<(b)?(a):(b))
//#endif // !Min
//#ifndef max
//#define max(a,b)    (((a) > (b)) ? (a) : (b))
//#endif // !max
//#ifndef min
//#define min(a,b)    (((a) < (b)) ? (a) : (b))
//#endif // !min

// #define MT_ALPHA	3
// #define MT_RED		2
// #define MT_GREEN	1
// #define MT_BLUE		0

#ifdef _UNICODE
#define GET_CLASSNAME(ClassName)	L#ClassName
#else
#define GET_CLASSNAME(ClassName)	#ClassName
#endif // _UNICODE

#ifdef _UNICODE
#define PrintLog wprintf
#define GET_CLASSNAME(ClassName)	L#ClassName
#else
#define PrintLog printf
#define GET_CLASSNAME(ClassName)	#ClassName
#endif // _UNICODE

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif // !M_PI
#ifndef M_2PI
#define M_2PI (2.0*M_PI)
#endif // !M_2PI

enum MT_IMAGE_FORMAT
{
	MT_UNKNOWN = -1,
	MT_JPEG = 1,
	MT_PNG = 2,
	MT_BMP = 3,
	MT_GIF = 4,
};

extern QString g_szCurPath;// 当前EXE文件的路径
extern double AngleToRadian(int nAngle); // 角度转弧度
extern int RadianToAngle(double fRadian);// 弧度转角度
extern int GetAngleFromPoints(float x1, float y1, float x2, float y2, float x3, float y3);// 求夹角
extern int Random(int nMinValue, int nMaxValue);
#endif // !__GLOBAL_H__
