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
#ifndef _NLM_BLOCK_H_
#define _NLM_BLOCK_H_

#include "merror.h"
#include "amcomdef.h"

#define  SHIFT_SIZE (0)
#define  BIG_BLOCK_SIZE (7)
#define  SMALL_BLOCK_SIZE (5)

#define  DOWN_SAMPLE_BIG_BLOCK_SIZE (5)
#define  DOWN_SAMPLE_SMALL_BLOCK_SIZE (3)

#define  DOUBLE_DOWN_SAMPLE_BIG_BLOCK_SIZE (10)
#define  DOUBLE_DOWN_SAMPLE_SMALL_BLOCK_SIZE (6)
#define  HALF_DOWN_SQU_BIG_BLOCK (13)
#define  HALF_DOWN_SQU_SMALL_BLOCK (5)

#define DIV_SHIFT (10)
#define INV_DIV_SHIFT_BIG (21) 
#define INV_DIV_SHIFT_SMALL (41)

#define DOWN_SAMPLE_INV_DIV_SHIFT_BIG   (41) 
#define DOWN_SAMPLE_INV_DIV_SHIFT_SMALL (114) 

#define DOWN_SAMPLE_INV_DIV_SHIFT_BIG1(a)     ( ((a)<<5) + ((a)<<3) + a)
#define DOWN_SAMPLE_INV_DIV_SHIFT_SMALL1(a)   ( ((a)<<7) - ((a)<<4) + ((a)<<1)) 


#define  DIF_VAL(val)  ((val)*(val))

#define  MAX_DIF_THRESHOLD   (1600)
#define  PIX_DIF(a, b, c)    { c = (a) - (b);\
	                           c = DIF_VAL(c);\
                               c = MIN(MAX_DIF_THRESHOLD,c);}
#define  BOX_PIX(a, b, c)    {c = (a)- (b);\
                              c = DOWN_SAMPLE_INV_DIV_SHIFT_BIG1(c);\
                              c >>= DIV_SHIFT;}

MVoid Dst_Copy_Data(MByte *pdst, MByte *psrc, MLong lwidth, MLong lheight,  MLong ldstpitch, MLong lsrcpitch);
MVoid Local_Array(MLong** dst, MLong *src, MLong step, MLong num);

#endif // _NLM_BLOCK_H_
