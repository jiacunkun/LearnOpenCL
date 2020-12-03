#include "arcsoft_example.h"
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include "mobilecv.h"

#include "PyramidNLM_OCL_Handle.h"

using namespace std;

static int ParseWidthHeight(std::string srcName, int* height, int* width)
{
    size_t pAt = srcName.find_last_of('x');
    if (!pAt)
    {
        pAt = srcName.find_last_of('X');
        if (!pAt)
        {
            std::cout << "x or X not found!" << std::endl;

            return -1;
        }
    }

    std::string tmpStr = srcName.substr(pAt + 1);
    *height = stoi(tmpStr);

    tmpStr = srcName.substr(0, pAt);
    pAt = tmpStr.find_last_of("_");
    if (!pAt)
    {
        return -1;
    }

    tmpStr = tmpStr.substr(pAt + 1);
    *width = stoi(tmpStr);

    return 0;
}


int main(int argc, char* argv[])
{
    int lret = 0;


    char filename[255] = { 0 };
    MInt32 height = 3472;
    MInt32 width = 4624;
    MInt32 lLayer = 0;
    MFloat pEps[4] = { 0 };
    MInt32 lScale = 1;
    MInt32 pSharpenIntensity[4] = { 0 };

    sprintf(filename, "E:\\svn\\NLM_OCL\\data\\ISO01025_007_1_4624x3472.NV21");

    lret = ParseWidthHeight(filename, &height, &width);
    if (lret != 0)
    {
        return lret;
    }

    ////////////////////////////////////////////////
    // read image
    ////////////////////////////////////////////////
    ASVLOFFSCREEN guided;
    {
        guided.ppu8Plane[0] = MNull;
        guided.pi32Pitch[0] = 0;
        guided.i32Width = width;
        guided.i32Height = height;
    }

    if (1)
    {
        FILE* fpInput = nullptr;

        int nFileLen = 0;
        fpInput = fopen(filename, "rb");

        cout << "Open file " << filename << endl;

        fseek(fpInput, 0, SEEK_END);
        nFileLen = ftell(fpInput);
        fseek(fpInput, 0, SEEK_SET);

        if (nFileLen != width * height * 3 / 2)
        {
            cout << "File size mismatch!" << endl;
            return -1;
        }

        guided.u32PixelArrayFormat = ASVL_PAF_NV21;
        guided.pi32Pitch[0] = width;
        guided.pi32Pitch[1] = width;
        guided.ppu8Plane[0] = (MByte*)malloc(height * width * 3 / 2);
        guided.ppu8Plane[1] = guided.ppu8Plane[0] + height * width;

        if (guided.ppu8Plane[0] == MNull)
        {
            cout << "Out of memory!" << endl;
            return -1;
        }

        cout << "jck 1" << endl;

        fread(guided.ppu8Plane[0], 1, nFileLen, fpInput);

        fclose(fpInput);
        fpInput = nullptr;

    }

    // run demo
    {
        MFloat fNoiseVarY = 10;
        MFloat fNoiseVarUV = 10;
        lret = PyramidNLM_OCL_Handle(&guided, &guided, fNoiseVarY, fNoiseVarUV);
    }

    if (1)
    {
        FILE* fpOutput = nullptr;
        string srcName, dstName, extName;
        size_t pAt = 0;

        srcName = filename;;
        pAt = srcName.find_last_of('.');
        extName = srcName.substr(pAt);
        dstName = srcName.substr(0, pAt) + "_res" + extName;

        fpOutput = fopen(dstName.c_str(), "wb");
        if (!fpOutput)
        {
            cout << "Can't open dst file" << dstName << endl;
        }

        cout << "Result saved to " << dstName << endl;

        fwrite(guided.ppu8Plane[0], 1, guided.i32Height * guided.i32Width * 3 / 2, fpOutput);

        fclose(fpOutput);
        fpOutput = nullptr;
    }

exit:

    if (guided.ppu8Plane[0])
    {
        free(guided.ppu8Plane[0]);
        guided.ppu8Plane[0] = nullptr;
    }

    return lret;
}

