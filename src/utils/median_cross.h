#include "asvloffscreen.h"

MRESULT Cross_Median_Filter_5x5(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
	MInt32 width, MInt32 height, MInt32 cn);

MRESULT Cross_Median_Filter_5x5_P010(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* src, MInt32 src_step, MByte* dst, MInt32 dst_step,
	MInt32 width, MInt32 height, MInt32 cn);

MInt32 Cross_Median_Filter_3x3(MHandle hMemMgr, MHandle mcvParallelMonitor, MByte* srcImg, MInt32 lPitchSrc, MByte* dstImg, MInt32 lPitchDst,
	MInt32 lWidth, MInt32 lHeight, MInt32 cn);

MInt32 Cross_Median_Filter_3x3_P010(MHandle mcvParallelMonitor, MByte* srcImg, MInt32 lPitchSrc, MByte* dstImg, MInt32 lPitchDst, MInt32 lWidth, MInt32 lHeight, MInt32 cn);

