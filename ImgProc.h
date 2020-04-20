//
// Created by ZJ-DB1117 on 2019/6/5.
//

#ifndef OCLDEMO_IMGPROC_H
#define OCLDEMO_IMGPROC_H


#include "CLProgram.h"

bool CalcuHistogram(mtcl::Program &program, const unsigned char* pImage, int height, int width, int nBins, int* pHistogram);


#endif //OCLDEMO_IMGPROC_H
