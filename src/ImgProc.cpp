//
// Created by ZJ-DB1117 on 2019/6/5.
//

#include "ImgProc.h"

#include "CLEvent.h"
#include "CLBuffer.h"
#include "CLUtility.h"
#include "CLContext.h"
#include "CLProgram.h"
#include "CLKernel.h"
#include "CLLog.h"

#include <iostream>
#include <algorithm>

typedef unsigned char uchar;

#ifndef GET_ND_BLOCK_SIZE
#define GET_ND_BLOCK_SIZE(bolckSize, width, step) \
    ((bolckSize) = (width) % (step) == 0 ? (width) / (step) : (width) / (step) + 1)
#endif
using namespace mtcl;

bool CalcuHistogram(Program &program, Context& context, const unsigned char* pImage, int height, int width, int nBins,
                    unsigned int* pHistogram)
{
    bool ret = false;
    if (pImage == NULL)
    {
        LOGE("%s[%d]: pImage == NULL.\n", __FUNCTION__, __LINE__);
        return ret;
    }

    Buffer<unsigned char> inputImage(context, width, height, ret);
    inputImage.write(pImage, ret);

    for (int i = 0; i < nBins; i ++)
    {
        pHistogram[i] = 0;
    }

    Buffer<unsigned int> histBuffer(context, nBins, 1, ret);
    histBuffer.write(pHistogram, ret);

    size_t blockW = 0;
    GET_ND_BLOCK_SIZE(blockW, width, 8);
    mtcl::Kernel<void(unsigned char*, unsigned int*)>  hist1 = program.getKernel<void(unsigned char*, unsigned int*)>("Histogram");
    hist1(Worksize(blockW, height, 8, 8), inputImage, histBuffer, ret);
    return true;
}
