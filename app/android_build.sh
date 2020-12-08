#!/bin/sh
set -e
echo "================================== build android !!! =================================="

current_dir=$(cd `dirname $0`; pwd)
echo $current_dir
pushd $current_dir
echo "current_dir = ${current_dir}"


###################################################################
# 检查 ANDROID_NDK 是否有设置
###################################################################
# ANDROID_NDK env
#ANDROID_NDK="/Users/yangzhu/software/android-ndk-r19c"  #手动设置"
if [ "$ANDROID_NDK" = "" ]; then
    echo "find not ANDROID_NDK"
    ANDROID_NDK=$(dirname `which ndk-build`)
    echo "set ANDROID_NDK by path of ndk-buid : $ANDROID_NDK"
else
    echo "find ANDROID_NDK from system env : $ANDROID_NDK"
fi


if [ "$ANDROID_NDK" = "" ]; then
    echo "can't find ANDROID_NDK env and ndk-buid, please set before run"
    exit -1;
fi


#try to detect NDK version
NDK_REL=$(grep -i '^Pkg.Revision'  $ANDROID_NDK/source.properties | awk -F '= ' '{print $2}')
NDK_VER=$(echo $NDK_REL | awk -F '.' '{print $1}')
if [ "$NDK_VER" -lt "16" ]; then
    echo "You need the ndk-r16 or later"
    exit 1
fi


###################################################################
# 编译开关，只需要改这里
###################################################################
# 项目名称
export PROJECT_NAME="Cheese"

export BUILD_EXE="ON"                      # 编译可执行文件，测试用
export BUILD_TEST="ON"                     # 编译测试代码，测试用
export BUILD_OPENCV="ON"                   # 带opencv库，测试用

export ALL_ARCHS="armeabi-v7a "    # armeabi x86 x86_64  armeabi-v7a  arm64-v8a
export BUILD_TYPE=Release                   # MinSizeRel, Release
export BUILD_SHARED_LIBS="ON"              # 静态库OFF, 动态库ON
export BUILD_HIDDEN_SYMBOL="OFF"            # 隐藏符号ON, 不隐藏符号OFF
export BUILD_RTTI="ON"                      # rtti switch,一般不用改
export BUILD_EXCEPTIONS="ON"                # exceptions switch,一般不用改
export BUILD_SCALE_LIB="ON"                 # 减小库大小,一般不用改
export ANDROID_PLATFORM="android-21"        # Android API 版本,一般不用改
# stltype: gnustl, stlport, c++, c++_static,一般不用改
export ANDROID_STL_32BIT="c++_static"
export ANDROID_STL_64BIT="c++_shared"
# 强制使用gcc编译器, NDK r13已经默认使用clang编译器,一般不用改
export ANDROID_TOOLCHAIN_32BIT="clang"
export ANDROID_TOOLCHAIN_64BIT="clang"
# 使用NDK r13以上版本,一般不用改
export CMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake"
#export CMAKE_TOOLCHAIN_FILE="$current_dir/android.toolchain.cmake"


# -lz -ldl -llog -landroid -ljnigraphics -lGLESv3 -lGLESv2 -lEGL

###################################################################
# 编译选项
###################################################################
# 公用FLAGS
export COMMON_FLAGS="$COMMON_FLAGS -llog"
export COMMON_FLAGS_RELEASE="-O3" # 可根据实际项目需求增加-Ofast、-O3、-O2等选项, release默认-Os


# set toolchain
# cmake 3.7以后支持Android交叉编译, 但变量是NDK R13前的方式，对于clang要特殊设置
if [ "$ANDROID_TOOLCHAIN" = "clang" ]; then
    CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION="clang"
fi


###################################################################
# 编译主函数
###################################################################
do_build()
{
    BUILD_ARCH=$1
    if [ "$BUILD_ARCH" == "armeabi-v7a" ]; then
        ANDROID_ABI="armeabi-v7a with NEON"
        ANDROID_STL=$ANDROID_STL_32BIT
        ANDROID_TOOLCHAIN=$ANDROID_TOOLCHAIN_32BIT
    else
        ANDROID_ABI="$BUILD_ARCH"
        ANDROID_STL=$ANDROID_STL_64BIT
        ANDROID_TOOLCHAIN=$ANDROID_TOOLCHAIN_64BIT
    fi

    # 编译安装目录
    echo lib${PROJECT_NAME}
    OUTPUT_DIR=$current_dir/lib${PROJECT_NAME}/android/$BUILD_ARCH
    BUILD_DIR=$current_dir/lib${PROJECT_NAME}Symbols/android/$BUILD_ARCH
    rm -rf $BUILD_DIR
    mkdir -p $BUILD_DIR
    pushd $BUILD_DIR


    cmake -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION="$CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION" \
          -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE      \
          -DANDROID_TOOLCHAIN="$ANDROID_TOOLCHAIN"          \
          -DANDROID_NDK=$ANDROID_NDK                        \
          -DANDROID_PLATFORM="$ANDROID_PLATFORM"            \
          -DANDROID_ABI="$ANDROID_ABI"                      \
          -DANDROID_STL="$ANDROID_STL"                      \
          -DCMAKE_INSTALL_PREFIX="/"                        \
          -DCMAKE_C_FLAGS="$COMMON_FLAGS"                   \
          -DCMAKE_CXX_FLAGS="$COMMON_FLAGS"                 \
          -DCMAKE_C_FLAGS_RELEASE="$COMMON_FLAGS_RELEASE"   \
          -DCMAKE_CXX_FLAGS_RELEASE="$COMMON_FLAGS_RELEASE" \
          -DCMAKE_BUILD_TYPE=$BUILD_TYPE                    \
          -DBUILD_SHARED_LIBRARY="$BUILD_SHARED_LIBS"       \
          -DBUILD_HIDDEN_SYMBOL="$BUILD_HIDDEN_SYMBOL"      \
          -DBUILD_RTTI="$BUILD_RTTI"                        \
          -DBUILD_EXCEPTIONS="$BUILD_EXCEPTIONS"            \
          -DBUILD_SCALE_LIB="$BUILD_SCALE_LIB"              \
          -DBUILD_EXE="$BUILD_EXE"                          \
          -DBUILD_TEST="$BUILD_TEST"                        \
          -DBUILD_OPENCV="$BUILD_OPENCV"                    \
          $current_dir/..


    make all -j8
    rm -rf $OUTPUT_DIR
    mkdir -p $OUTPUT_DIR
#    make install/strip DESTDIR=$OUTPUT_DIR
#    make install/strip
    popd # $BUILD_DIR
}


# 开始编译
for ARCH in $ALL_ARCHS
do
    do_build $ARCH
done

popd # $current_dir


echo " =============== build success ! =============== "


# 问题：不能打开软件提示无法打开“clang”，因为Apple无法检查其是否包含恶意软件。
# 解决：sudo xattr -rd com.apple.quarantine  /Users/yangzhu/software/android-ndk-r19c/toolchains/llvm/prebuilt/darwin-x86_64/bin/clang++
