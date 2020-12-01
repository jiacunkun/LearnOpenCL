#include "arcsoft_example.h"
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include "mobilecv.h"
#include "imgpyramid.h"

using namespace std;

int main(int argc, char* argv[])
{
	int width = 1280;
	int height = 720;

	

	MHandle EX_handle = nullptr;



	MRESULT rval = EX_Init(&EX_handle);
	if (rval!=MOK)
	{
		return 0;
	}

	std::vector<unsigned char> buffer(width * height);
    string filename = "../../data/011_1280x720.gray";
	FILE* fp = fopen(filename.c_str(), "rb");
	if (fp)
	{
		fread(&buffer[0], 1, width * height, fp);
		fclose(fp);
	}
	else
	{
		std::cout << "can't open image" << std::endl;
	}

	ASVLOFFSCREEN img = {0};
	img.i32Width = width;
	img.i32Height = height;
	img.u32PixelArrayFormat = ASVL_PAF_GRAY;
	img.pi32Pitch[0] = width;
	img.ppu8Plane[0] = &buffer[0];

#if 0
    MHandle hMemMgr = MNull;
    MHandle mcvParallelMonitor = MNull;
    mcvParallelMonitor = mcvParallelInit(hMemMgr, 16);
    if (mcvParallelMonitor == 0)
    {
        printf("Failed to start parallel engine!!\n");
        return -1;
    }

    ImgPyramidDenoise_Block_NLM(hMemMgr, mcvParallelMonitor, &img, &img,
                                            10, 1);

    if (mcvParallelMonitor)
    {
        if (mcvParallelUninit(mcvParallelMonitor) < 0)
        {
            return -1;
        }
    }
#else
	EX_Process(EX_handle, &img);
#endif

	EX_Uninit(&EX_handle);

    if (1)
    {
        FILE * fpOutput = nullptr;
        string srcName, dstName, extName;
        size_t pAt = 0;

        srcName = filename;;
        pAt = srcName.find_last_of('.');
        extName = srcName.substr(pAt);
        dstName = srcName.substr(0, pAt-9) + "_640x360_res" + extName;

        fpOutput = fopen(dstName.c_str(), "wb");
        if (!fpOutput)
        {
            cout << "Can't open dst file" << dstName << endl;
        }

        cout << "Result saved to " << dstName << endl;

        fwrite(img.ppu8Plane[0], 1, img.i32Height* img.i32Width * 3 / 2, fpOutput);

        fclose(fpOutput);
        fpOutput = nullptr;
    }
}

