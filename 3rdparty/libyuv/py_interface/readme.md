#### 注意事项
* 选择与当前平台相符的脚本进行build安装。
* 需要事先安装好swig。
* 编译后会有以libyuv开头的一个.so动态库（不同的python版本动态库的命名略有不同）和一个libyuv.py文件，将这两个文件拷贝至python脚本目录下，就可import libyuv。