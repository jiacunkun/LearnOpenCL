#pragma once

#include <asvloffscreen.h>
#include "single_image_enhancement_define.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

#define MAX_PYRAMID_LAYER 4

class arcsoft_edge_filter
{
public:
	arcsoft_edge_filter();
	~arcsoft_edge_filter();

	MInt32 init(MHandle hMemMgr, MHandle mcvParallelMonitor, MInt32 lWidth, MInt32 lHeight, MInt32 lStride, MInt32 lPryLayer);
	MVoid release();
	MInt32 run(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pGuided, LPASVLOFFSCREEN pDstImg, MFloat* pValue, MInt32 lScale);
	MInt32 run(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MFloat* pValue, MInt32 lScale);
	MInt32 run2(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pGuided, LPASVLOFFSCREEN pDstImg, MFloat* pValue, MInt32 lScale);
	MInt32 runWithNLM(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pGuided, LPASVLOFFSCREEN pDstImg, MFloat* pValue, MInt32 lScale);
	MInt32 runB(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MFloat* pValue, MInt32 lScale);
    MInt32 runValue(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDstImg, MFloat* pValue, MInt32 lScale);

private:
	MInt32 CalcDetailMask();
	MVoid  FillExpandPixels(LPASVLOFFSCREEN pSrcDst, MInt32 lExpandSize);
	MVoid ImgAddDiff_sub128_u8(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pDiffImg, LPASVLOFFSCREEN pDstImg);
	MVoid ImgSubImg_add128_u8(LPASVLOFFSCREEN pSrcImg, LPASVLOFFSCREEN pSubImg, LPASVLOFFSCREEN pDstImg);

private:
	MHandle m_hMemMgr;
	MHandle m_mcvParallelMonitor;
	MInt32 m_lPryLayer;
	MInt32 m_lExpandSize;
	MInt32 m_lWidth;
	MInt32 m_lHeight;

	ASVLOFFSCREEN m_pSrcPyrGau[MAX_PYRAMID_LAYER];
	ASVLOFFSCREEN m_pSrcPyrGauInner[MAX_PYRAMID_LAYER];
	ASVLOFFSCREEN m_pGuidePyrGau[MAX_PYRAMID_LAYER];
	ASVLOFFSCREEN m_pGuidePyrGauInner[MAX_PYRAMID_LAYER];
	ASVLOFFSCREEN m_pDstPyrGau[MAX_PYRAMID_LAYER];
	ASVLOFFSCREEN m_pDstPyrGauInner[MAX_PYRAMID_LAYER];
	ASVLOFFSCREEN m_pMeanA[MAX_PYRAMID_LAYER];
	ASVLOFFSCREEN m_pMeanB[MAX_PYRAMID_LAYER];
	ASVLOFFSCREEN m_pMeanAInner[MAX_PYRAMID_LAYER];
	ASVLOFFSCREEN m_pMeanBInner[MAX_PYRAMID_LAYER];
};

NS_SINFLE_IMAGE_ENHANCEMENT_END