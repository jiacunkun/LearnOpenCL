#include <merror.h>
#include "build_pyramid.h"
#include "up_down_scale.h"
#include "imageproc.h"
#include <assert.h>
#include "DefineForDebug.h"
#include "SetLPASVLOFFSCREEN.h"
#include "up_down_scale_gaussian5x5.h"

NS_SINFLE_IMAGE_ENHANCEMENT_BEGIN

MInt32 BuildGauPyrs_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, ASVLOFFSCREEN* pyrGau, MInt32 nlevGau, MUInt16** tmpGuassBuf)
{
    START_TIME;
	MInt32 lPAF = pSrcImg->u32PixelArrayFormat;
	MInt32 lret = MOK;

	for (MInt32 i = 1; i < nlevGau; i++)
	{
#if 1
		lret = Img_Guass3x3_Down2_u8(hMemMgr, mcvParallelMonitor, &pyrGau[i - 1], &pyrGau[i], 1);
#else
	#if	1
		lret = Img_Guass5x5_Down2_u8(hMemMgr, mcvParallelMonitor, &pyrGau[i - 1], &pyrGau[i], tmpGuassBuf);
	#else
		cv::Mat large(pyrGau[i - 1].i32Height, pyrGau[i - 1].i32Width, CV_8UC1, pyrGau[i - 1].ppu8Plane[0]);
		cv::Mat small(pyrGau[i].i32Height, pyrGau[i].i32Width, CV_8UC1, pyrGau[i].ppu8Plane[0]);
		cv::resize(large, small, small.size());
	#endif
#endif
		if (lret != MOK)
		{
			return lret;
		}

	}
	END_TIME;
	return lret;
}

MVoid BuildLapPyrs_u8(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, ASVLOFFSCREEN* pyrGau, ASVLOFFSCREEN* pyrLap, MInt32 nlevLap)
{
    START_TIME;
	if (pyrGau != MNull)
	{
		CopyY(&pyrGau[0], pSrcImg);

		for (MInt32 i = 1; i < nlevLap; i++)
		{
			LPASVLOFFSCREEN pGauPre = &pyrGau[i - 1], pGauCur = &pyrGau[i];
			LPASVLOFFSCREEN pLapPre = &pyrLap[i - 1];

			//Img_Guass5x5_Down2_u8(hMemMgr, mcvParallelMonitor, pGauPre, pGauCur, MNull);
			Img_Guass3x3_Down2_u8(hMemMgr, mcvParallelMonitor, pGauPre, pGauCur, 1);
			Img_Guass5x5_Up2AndSub_u8(hMemMgr, mcvParallelMonitor, pGauCur, pGauPre, pLapPre);

		}

		CopyY(&pyrLap[nlevLap - 1], &pyrGau[nlevLap - 1]);
	}
	else
	{
		// Need 2 buffers, gauPre & gauCur
		MInt32 lWidth = pSrcImg->i32Width;
		MInt32 lHeight = pSrcImg->i32Height;

		ASVLOFFSCREEN gauSmall = { 0 }, gauLarge = { 0 };
		gauSmall.i32Width = lWidth >> 1;
		gauSmall.i32Height = lHeight >> 1;
		gauSmall.pi32Pitch[0] = lWidth >> 1;
		gauSmall.ppu8Plane[0] = (MByte*)MMemAlloc(hMemMgr, gauSmall.i32Height * gauSmall.pi32Pitch[0]);
		gauSmall.u32PixelArrayFormat = ASVL_PAF_GRAY;

		gauLarge.i32Width = lWidth >> 2;
		gauLarge.i32Height = lHeight >> 2;
		gauLarge.pi32Pitch[0] = lWidth >> 2;
		gauLarge.ppu8Plane[0] = (MByte*)MMemAlloc(hMemMgr, gauLarge.i32Height * gauLarge.pi32Pitch[0]);
		gauSmall.u32PixelArrayFormat = ASVL_PAF_GRAY;

		for (MInt32 i = 1; i < nlevLap; i++)
		{
			LPASVLOFFSCREEN pGauPre, pGauCur;
			LPASVLOFFSCREEN pLapPre = &pyrLap[i - 1];

			if (i == 1)
			{
				pGauPre = pSrcImg;
				pGauCur = &gauSmall;
			}
			else
			{
				ASVLOFFSCREEN structTmp = gauLarge;
				gauLarge = gauSmall;
				gauSmall = structTmp;
				gauSmall.i32Height = lHeight >> i;
				gauSmall.i32Width = lWidth >> i;
				gauSmall.pi32Pitch[0] = gauSmall.i32Width;

				pGauPre = &gauLarge;
				pGauCur = &gauSmall;

			}

			Img_Guass5x5_Down2_u8(hMemMgr, mcvParallelMonitor, pGauPre, pGauCur, MNull);
			Img_Guass5x5_Up2AndSub_u8(hMemMgr, mcvParallelMonitor, pGauCur, pGauPre, pLapPre);


		}

		CopyY(&pyrLap[nlevLap - 1], &gauSmall);


		MMemFree(hMemMgr, gauSmall.ppu8Plane[0]);
		gauSmall.ppu8Plane[0] = MNull;

		MMemFree(hMemMgr, gauLarge.ppu8Plane[0]);
		gauLarge.ppu8Plane[0] = MNull;
	}
    END_TIME;
}

MInt32 BuildGauPyrs_s16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, ASVLOFFSCREEN* pyrGau, MInt32 nlevGau)
{
	MInt32 lPAF = pSrcImg->u32PixelArrayFormat;
	MInt32 lret = MOK;

	assert(pyrGau[0].u32PixelArrayFormat == ASVL_PAF_RAW10_GRAY_16B);

	if (pSrcImg->ppu8Plane[0] != pyrGau[0].ppu8Plane[0])
	{
		lret = CopyY(&pyrGau[0], pSrcImg);
	}
	if (lret != MOK)
	{
		return lret;
	}

	for (MInt32 i = 1; i < nlevGau; i++)
	{
		lret = Img_Guass5x5_Down2_u16(hMemMgr, mcvParallelMonitor, &pyrGau[i - 1], &pyrGau[i]);
		if (lret != MOK)
		{
			return lret;
		}
	}

	return lret;
}

static MVoid Img_Sub_Add512_s16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg0, LPASVLOFFSCREEN pSrcImg1, LPASVLOFFSCREEN pDstImg)
{
	MInt32 lWidth = pSrcImg0->i32Width;
	MInt32 lHeight = pSrcImg0->i32Height;

#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif	
	for (MInt32 y = 0; y < lHeight; y++)
	{
		MUInt16* pDataSrc0 = (MUInt16 *)(pSrcImg0->ppu8Plane[0] + y * pSrcImg0->pi32Pitch[0]);
		MUInt16* pDataSrc1 = (MUInt16 *)(pSrcImg1->ppu8Plane[0] + y * pSrcImg1->pi32Pitch[0]);
		MUInt16* pDataDst = (MUInt16*)(pDstImg->ppu8Plane[0] + y * pDstImg->pi32Pitch[0]);
		for (MInt32 x = 0; x < lWidth; x++)
		{
			pDataDst[x] = TRIM_10B(pDataSrc0[x] - pDataSrc1[x] + 512);
		}
	}
}

static MVoid Img_Add_Sub512_s16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg0, LPASVLOFFSCREEN pSrcImg1, LPASVLOFFSCREEN pDstImg)
{
	MInt32 lWidth = pSrcImg0->i32Width;
	MInt32 lHeight = pSrcImg0->i32Height;

#if	defined NH_ENABLE_OPENMP
#pragma omp parallel for num_threads(NH_OMP_THREAD_NUM) schedule(dynamic)
#endif
	for (MInt32 y = 0; y < lHeight; y++)
	{
		MUInt16* pDataSrc0 = (MUInt16*)(pSrcImg0->ppu8Plane[0] + y * pSrcImg0->pi32Pitch[0]);
		MUInt16* pDataSrc1 = (MUInt16*)(pSrcImg1->ppu8Plane[0] + y * pSrcImg1->pi32Pitch[0]);
		MUInt16* pDataDst = (MUInt16*)(pDstImg->ppu8Plane[0] + y * pDstImg->pi32Pitch[0]);

		if (y == lHeight - 1)
		{
			int a = 1;
		}

		for (MInt32 x = 0; x < lWidth; x++)
		{
			pDataDst[x] = TRIM_10B(pDataSrc0[x] + pDataSrc1[x] - 512);
		}
	}
}

MVoid BuildLapPyrs_s16(MHandle hMemMgr, MHandle mcvParallelMonitor, LPASVLOFFSCREEN pSrcImg, ASVLOFFSCREEN* pyrGau, ASVLOFFSCREEN* pyrLap, MInt32 nlevLap)
{

	if (pyrGau != MNull)
	{
		CopyY(&pyrGau[0], pSrcImg);

		for (MInt32 i = 1; i < nlevLap; i++)
		{
			LPASVLOFFSCREEN pGauPre = &pyrGau[i - 1], pGauCur = &pyrGau[i];
			LPASVLOFFSCREEN pLapPre = &pyrLap[i - 1];

			Img_Guass5x5_Down2_u16(hMemMgr, mcvParallelMonitor, pGauPre, pGauCur);
			Img_Guass5x5_Up2_sub_u16(hMemMgr, mcvParallelMonitor, pGauCur, pGauPre, pLapPre);

		}

		CopyY(&pyrLap[nlevLap - 1], &pyrGau[nlevLap - 1]);
	}
	else
	{
		// Need 2 buffers, gauPre & gauCur
		MInt32 lWidth = pSrcImg->i32Width;
		MInt32 lHeight = pSrcImg->i32Height;

		ASVLOFFSCREEN gauSmall = { 0 }, gauLarge = { 0 };
		AllocOffscreenMemory(hMemMgr, lWidth >> 1, lHeight >> 1, ASVL_PAF_RAW10_GRAY_16B, &gauSmall);
		AllocOffscreenMemory(hMemMgr, lWidth >> 2, lHeight >> 2, ASVL_PAF_RAW10_GRAY_16B, &gauLarge);

		for (MInt32 i = 1; i < nlevLap; i++)
		{
			LPASVLOFFSCREEN pGauPre, pGauCur;
			LPASVLOFFSCREEN pLapPre = &pyrLap[i - 1];

			if (i == 1)
			{
				pGauPre = pSrcImg;
				pGauCur = &gauSmall;
			}
			else
			{
				ASVLOFFSCREEN structTmp = gauLarge;
				gauLarge = gauSmall;
				gauSmall = structTmp;
				gauSmall.i32Height = lHeight >> i;
				gauSmall.i32Width = lWidth >> i;
				gauSmall.pi32Pitch[0] = gauSmall.i32Width * sizeof(MUInt16);

				pGauPre = &gauLarge;
				pGauCur = &gauSmall;

			}


			Img_Guass5x5_Down2_u16(hMemMgr, mcvParallelMonitor, pGauPre, pGauCur);
			Img_Guass5x5_Up2_sub_u16(hMemMgr, mcvParallelMonitor, pGauCur, pGauPre, pLapPre);

		}

		CopyY(&pyrLap[nlevLap - 1], &gauSmall);


		MMemFree(hMemMgr, gauSmall.ppu8Plane[0]);
		gauSmall.ppu8Plane[0] = MNull;

		MMemFree(hMemMgr, gauLarge.ppu8Plane[0]);
		gauLarge.ppu8Plane[0] = MNull;
	}

}

MRESULT ReconstructLapLacianPyramid_S16(MHandle hMemMgr, MHandle mcvParallelMonitor, ASVLOFFSCREEN* pLapPyr, LPASVLOFFSCREEN pDstImg, MInt32 nLevs)
{
	MRESULT lret = MOK;
	MInt32 lWidth = 0, lHeight = 0;
	MInt32 nChannels = 0;
	ASVLOFFSCREEN tmpImg = { MNull };
	MBool isLastPyr = MFalse;

	if (!pLapPyr || nLevs < 1)
		return MERR_INVALID_PARAM;

	lWidth = pDstImg->i32Width;
	lHeight = pDstImg->i32Height;

	lret = AllocOffscreenMemory(hMemMgr, lWidth, lHeight, ASVL_PAF_RAW10_GRAY_16B, &tmpImg);
	if (MOK != lret)
		goto exit;

	for (MInt32 i = nLevs - 1; i > 0; i--)
	{
		LPASVLOFFSCREEN lapPre = &pLapPyr[i - 1];
		LPASVLOFFSCREEN lapCur = &pLapPyr[i];

		tmpImg.i32Width = lapPre->i32Width;
		tmpImg.i32Height = lapPre->i32Height;
		tmpImg.pi32Pitch[0] = tmpImg.i32Width * sizeof(MInt16);

		if (i == 1)
		{
			Img_Guass5x5_Up2_add_u16(hMemMgr, mcvParallelMonitor, lapCur, lapPre, pDstImg);
		} 
		else
		{
			Img_Guass5x5_Up2_add_u16(hMemMgr, mcvParallelMonitor, lapCur, lapPre, lapPre);
		}

	}

	//MMemCpy(pDstImg->pData, pLapPyr->pyr[0].pData, pDstImg->lHeight * pDstImg->lStep * sizeof(MInt16));

exit:
	FreeOffscreenMemory(hMemMgr, &tmpImg);
	return lret;
}

NS_SINFLE_IMAGE_ENHANCEMENT_END