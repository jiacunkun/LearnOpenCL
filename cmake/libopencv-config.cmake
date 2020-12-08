# 包含以下可用变量
# libopencv_FOUND
# libopencv_VERSION
# libopencv_INCLUDE_DIRS
# libopencv_LIBS
# libopencv_LIBRARY_DIRS


set(_NAME libopencv)
set(_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}")

################################# win platform #######################################
if (MSVC)
    if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
        set(_ROOT "${_ROOT}/Win/v120/Win32")
    else ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(_ROOT "${_ROOT}/Win/v120/x64")
    endif ("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
    # debug properties

    set(OpenCV_INCLUDE_DIRS "${_ROOT}/Release/include")

    add_library(libopencv SHARED IMPORTED)
    set_property(TARGET libopencv APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_target_properties(libopencv PROPERTIES
            IMPORTED_IMPLIB_DEBUG "${_ROOT}/Debug/lib/libopencv.lib"
            )
    set_property(TARGET libopencv APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(libopencv PROPERTIES
            IMPORTED_IMPLIB_RELEASE "${_ROOT}/Release/lib/libopencv.lib"
            )
    set(OpenCV_LIBS libopencv)


elseif (APPLE)
    message("aaaaaaaaaa")
    set(OpenCV_DIR "${_ROOT}/android/opencv_4_1_0_android_sdk/sdk/native/jni")
    set(ANDROID_NDK_ABI_NAME "armeabi-v7a")
    find_package(OpenCV REQUIRED)
    if (OpenCV_FOUND)
        message(STATUS "OpenCV library found")
    else ()
        message(STATUS "OpenCV library not found")
    endif ()

    set(libopencv_FOUND ${OpenCV_FOUND})
    set(libopencv_VERSION ${OpenCV_VERSION})
    set(libopencv_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
    set(libopencv_LIBS ${OpenCV_LIBRARIES})
    set(libopencv_LIBRARY_DIRS "${OpenCV_INSTALL_PATH}/lib")

    include_directories(${OpenCV_INCLUDE_DIRS}) # Not needed for CMake >= 2.8.11
    link_directories(${libopencv_LIBRARY_DIRS})




    ################################# mac/ios platform #######################################
elseif (APPLE123)

    if (NOT IOS_DEPLOYMENT_TARGET)
        # mac platform
        # install opencv : brew install opencv
        # 设置OpenCV_DIR可以指定 opencv的路径, 这个路径下需要有OpenCVConfig.cmake等文件
        # 默认 OpenCV_DIR=/usr/local/lib/cmake/opencv4, 即我的实际路径是 /usr/local/lib/cmake/opencv4/OpenCVConfig.cmake

        # 优先使用系统安装的库
        find_package(OpenCV 4 QUIET) # QUIET 表示如果查找失败，不会在屏幕进行输出

        if (NOT OpenCV_FOUND)
            logi("use opencv 4.1")
            # 系统安装的库找不到的话, 使用自带的。(也可以切换 opencv 版本号)
            set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/macOS/lib/cmake/opencv4")

            find_package(OpenCV 4 REQUIRED  # REQUIRED表示一定要找到包，找不到的话就立即停掉整个cmake。
                    NO_MODULE       # tells CMake to use config mode
#                    NO_DEFAULT_PATH
#                    NO_CMAKE_PATH   # 关闭 CMAKE_PREFIX_PATH、CMAKE_FRAMEWORK_PATH 、CMAKE_APPBUNDLE_PATH
#                    NO_CMAKE_ENVIRONMENT_PATH # 关闭NO_CMAKE_PATH之类，还关闭 OpenCV_DIR
#                    NO_SYSTEM_ENVIRONMENT_PATH # 关闭 CMAKE_SYSTEM_PREFIX_PATH、CMAKE_SYSTEM_FRAMEWORK_PATH、CMAKE_SYSTEM_APPBUNDLE_PATH
#                    NO_CMAKE_PACKAGE_REGISTRY
#                    NO_CMAKE_BUILDS_PATH
#                    NO_CMAKE_SYSTEM_PATH
#                    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY

#                    PATHS /usr/local # look here
#                    PATHS ${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/macOS/lib
                    )

        else()
#            logi("use opencv 4.4")
        endif()


        # opencv库查找路径顺序：
        # 1、OpenCV_DIR
        # 2、${CMAKE_MODULE_PATH}、${CMAKE_PREFIX_PATH}、${CMAKE_FRAMEWORK_PATH}、${CMAKE_APPBUNDLE_PATH}
        # 3、${CMAKE_ROOT}/Modules/FindXXX.cmake  # 模块模式
        # 4、a/b/c/d
        # a => /usr/local/ 或 /usr/local 或 PATH中定义的其他路径
        # b => share 或 lib 或 lib/<arch>
        # c => cmake/XXX 或 XXX 或 XXX/cmake 或 XXX/CMAKE
        # d => XXXConfig.cmake 或 xxx-config.cmake 或 FindXXX.cmake

        if (OpenCV_FOUND)
            include_directories(${OpenCV_INCLUDE_DIRS})

            set(libopencv_FOUND ${OpenCV_FOUND})
            set(libopencv_VERSION ${OpenCV_VERSION})
            set(libopencv_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
            set(libopencv_LIBS ${OpenCV_LIBRARIES})
            set(libopencv_LIBRARY_DIRS "${OpenCV_INSTALL_PATH}/lib")

            link_directories(${libopencv_LIBRARY_DIRS})

        else ()

            # 这里有问题
            set(libopencv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/macOS")

            set(libopencv_FOUND 1)
            set(libopencv_VERSION 4.0)
            set(libopencv_INCLUDE_DIRS "${libopencv_ROOT}/include/opencv4")
            set(OpenCV_LIBS opencv_calib3d;opencv_core;opencv_dnn;opencv_features2d;opencv_flann;opencv_gapi;opencv_highgui;opencv_imgcodecs;opencv_imgproc;opencv_ml;opencv_objdetect;opencv_photo;opencv_stitching;opencv_video;opencv_videoio;opencv_aruco;opencv_bgsegm;opencv_bioinspired;opencv_ccalib;opencv_datasets;opencv_dnn_objdetect;opencv_dpm;opencv_face;opencv_freetype;opencv_fuzzy;opencv_hfs;opencv_img_hash;opencv_line_descriptor;opencv_optflow;opencv_phase_unwrapping;opencv_plot;opencv_quality;opencv_reg;opencv_rgbd;opencv_saliency;opencv_sfm;opencv_shape;opencv_stereo;opencv_structured_light;opencv_superres;opencv_surface_matching;opencv_text;opencv_tracking;opencv_videostab;opencv_xfeatures2d;opencv_ximgproc;opencv_xobjdetect;opencv_xphoto;)
#            set(OpenCV_LIBS opencv_calib3d;opencv_core;opencv_dnn;opencv_features2d;opencv_flann;opencv_gapi;opencv_highgui;opencv_imgcodecs;opencv_imgproc;opencv_ml;opencv_objdetect;opencv_photo;opencv_stitching;opencv_video;opencv_videoio;opencv_aruco;opencv_bgsegm;opencv_bioinspired;opencv_ccalib;opencv_datasets;opencv_dnn_objdetect;opencv_dpm;opencv_face;opencv_freetype;opencv_fuzzy;opencv_hfs;opencv_img_hash;opencv_line_descriptor;opencv_optflow;opencv_phase_unwrapping;opencv_plot;opencv_quality;opencv_reg;opencv_rgbd;opencv_saliency;opencv_sfm;opencv_shape;opencv_stereo;opencv_structured_light;opencv_superres;opencv_surface_matching;opencv_text;opencv_tracking;opencv_videostab;opencv_xfeatures2d;opencv_ximgproc;opencv_xobjdetect;opencv_xphoto;ade;correspondence;ippicv;ippiw;ittnotify;libjpeg-turbo;libprotobuf;libwebp;multiview;numeric;quirc;)
#            set(OpenCV_LIBS "opencv_calib3d opencv_core opencv_dnn opencv_features2d opencv_flann opencv_gapi opencv_highgui opencv_imgcodecs opencv_imgproc opencv_ml opencv_objdetect opencv_photo opencv_stitching opencv_video opencv_videoio opencv_aruco opencv_bgsegm opencv_bioinspired opencv_ccalib opencv_datasets opencv_dnn_objdetect opencv_dpm opencv_face opencv_freetype opencv_fuzzy opencv_hfs opencv_img_hash opencv_line_descriptor opencv_optflow opencv_phase_unwrapping opencv_plot opencv_quality opencv_reg opencv_rgbd opencv_saliency opencv_sfm opencv_shape opencv_stereo opencv_structured_light opencv_superres opencv_surface_matching opencv_text opencv_tracking opencv_videostab opencv_xfeatures2d opencv_ximgproc opencv_xobjdetect opencv_xphoto")
            set(libopencv_LIBRARY_DIRS ${libopencv_ROOT}/lib)

            foreach (library  ${OpenCV_LIBS})
                #                cheese_add_prebuild_library(${library} "SHARED ${libopencv_LIBRARY_DIRS}/lib${library}.4.1.dylib")
                cheese_add_prebuild_library(lib${library} "${libopencv_LIBRARY_DIRS}/lib${library}.a")
                set(libopencv_LIBS "${libopencv_LIBS};lib${library}")
            endforeach ()
        endif ()

    else ()
        # ios platform
        #        set(libopencv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/iOS/AllArch/${CMAKE_OSX_ARCHITECTURES}/")
        set(libopencv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/iOS")

        set(libopencv_FOUND 1)
        set(libopencv_VERSION 4.0)
        set(libopencv_INCLUDE_DIRS "${_ROOT}/iOS/include")
        set(libopencv_LIBS ${_NAME})
        set(libopencv_LIBRARY_DIRS ${libopencv_ROOT}/lib)

        cheese_add_prebuild_library(libopencv "${libopencv_ROOT}/lib/opencv2")
        #        cheese_add_prebuild_library(libopencv "${libopencv_ROOT}/opencv2.framework")

    endif ()


    ################################# android platform #######################################
elseif (ANDROID)
    set(OpenCV_DIR "${_ROOT}/android/opencv_4_1_0_android_sdk/sdk/native/jni")
    find_package(OpenCV REQUIRED)
    if (OpenCV_FOUND)
        message(STATUS "OpenCV library found")
    else ()
        message(STATUS "OpenCV library not found")
    endif ()

    set(libopencv_FOUND ${OpenCV_FOUND})
    set(libopencv_VERSION ${OpenCV_VERSION})
    set(libopencv_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
    set(libopencv_LIBS ${OpenCV_LIBRARIES})
    set(libopencv_LIBRARY_DIRS "${OpenCV_INSTALL_PATH}/lib")

    include_directories(${OpenCV_INCLUDE_DIRS}) # Not needed for CMake >= 2.8.11
    link_directories(${libopencv_LIBRARY_DIRS})


    ################################# UNIX platform #######################################
elseif (UNIX)
    find_package(OpenCV REQUIRED)
    if (OpenCV_FOUND)
        include_directories(${OpenCV_INCLUDE_DIRS})

        set(libopencv_FOUND ${OpenCV_FOUND})
        set(libopencv_VERSION ${OpenCV_VERSION})
        set(libopencv_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
        set(libopencv_LIBS ${OpenCV_LIBRARIES})
        set(libopencv_LIBRARY_DIRS "")

    else ()

        if ("${LINUX_DISTRIBUTION}" STREQUAL "ubuntu")
            set(libopencv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/ubuntu")
        elseif ("${LINUX_DISTRIBUTION}" STREQUAL "centos")
            set(libopencv_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/centos")
        else ()
        endif ()


        set(libopencv_FOUND 1)
        set(libopencv_VERSION 4.0)
        set(libopencv_INCLUDE_DIRS "${libopencv_ROOT}/include")
        set(libopencv_LIBS ${_NAME})
        set(libopencv_LIBRARY_DIRS ${libopencv_ROOT}/lib)

        cheese_add_prebuild_library(libopencv "${libopencv_ROOT}/lib/libopencv.a")

    endif ()

    ################################# else #######################################
else () # *nix platform

    set(libopencv_FOUND FALSE)
    set(libopencv_VERSION "")
    set(libopencv_INCLUDE_DIRS "")
    set(libopencv_LIBS "")
    set(libopencv_LIBRARY_DIRS "")

endif ()


message(STATUS "Find OpenCV : ")
message(STATUS "  libopencv_FOUND = ${libopencv_FOUND} ")
message(STATUS "  libopencv_VERSION = ${libopencv_VERSION} ")
message(STATUS "  libopencv_INCLUDE_DIRS = ${libopencv_INCLUDE_DIRS} ")
message(STATUS "  libopencv_LIBRARY_DIRS = ${libopencv_LIBRARY_DIRS} ")
message(STATUS "  libopencv_LIBS = ${libopencv_LIBS} ")


