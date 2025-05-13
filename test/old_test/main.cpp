#include "SingleImageDeniseHandle.h"
#include <cstdio>
#include <string>
#include <vector>

using namespace std;

#undef QT_UI

#ifdef QT_UI
#include "mainwindow.h"
#include <QApplication>

int main02(int argc, char* argv[])
{
	QApplication a(argc, argv);
	CMainWindow w;
	w.show();
	return a.exec();
}

#else

int main()
{
	string folder_path = "E:\\svn\\SingleImageEnhancement\\images\\thread\\2.BMP"; //path of folder, you can replace "*.*" by "*.jpg" or "*.png"
	vector<cv::String> file_names;
	cv::glob(folder_path, file_names);   //get file names

#if 1
	int i = 0;
#else
	for (int i = 0; i < file_names.size(); i++)
#endif
	{
		char srcPath[255] = "";
		sprintf(srcPath, file_names[i].c_str());
		printf("Input image is %s\n", srcPath);

		char guidedPath[255] = "";
		sprintf(guidedPath, "E:\\svn\\V2.2_Samsung_Note20_S21_WithRedraw\\data\\src.png");
		//        printf("Input image is %s\n", guidedPath);

		char maskPath[255] = "";
		sprintf(maskPath, "E:\\PicZoom_3.0Trunk\\MFNS_Debug\\jck\\src.BMP");
		//        printf("Input image is %s\n", guidedPath);

		BASE_SINGLE_IMAGE_PARAM pdbParam;

		{
			pdbParam.lUVReprocIntensity = 80;
			pdbParam.lUVReprocMethod = UV_Guide_API;
			pdbParam.lYReprocIntensity = 5;
			pdbParam.lYReprocMethod = Y_Anis_API;
			pdbParam.lYDetailLuma = 50;
			pdbParam.lSharpenIntensity = 0;
		}

		SingleImageDenoiseHandle().single_image_denoise_handle(srcPath, guidedPath, maskPath, pdbParam);

	}
	return 0;
}

#endif
