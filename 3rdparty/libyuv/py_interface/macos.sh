#!/bin/sh

current_dir=`pwd`/`dirname $0`
echo $current_dir

# 拷贝头文件
cp -r $current_dir/../include $current_dir/

# 处理头文件
cd $current_dir/include/libyuv/ 
for file in `ls $current_dir/include/libyuv/`
do
    echo $file
    sed -i "" 's/#include \"libyuv/#include \"\./' $file
done 


# 编译libyuv_python库
cd $current_dir
swig -c++ -python libyuv.i
python ubuntu.py build_ext --inplace

# 删除头文件与源文件
rm -rf $current_dir/include
