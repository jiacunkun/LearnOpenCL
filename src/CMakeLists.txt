# ./src/CMakeList.txt
# debug宏定义
configure_file( common/DefineForDebug.h.in  ${CMAKE_CURRENT_LIST_DIR}/common/DefineForDebug.h )
option(DEBUG_OUTPUT "Build test apps or not" OFF)
if(DEBUG_OUTPUT)
    message(STATUS "Open debug output")
    find_package(OpenCV REQUIRED)
    add_definitions(-DDEBUG_OUTPUT)
    add_definitions(-DBUILD_OPENCV)
endif()

option(OPEN_LOG "Open log apps or not" OFF)
if(OPEN_LOG)
    # 添加宏定义
    message(STATUS "OPEN_LOG")
    add_definitions(-DCALCULATE_TIME=1)  #统计时间
    add_definitions(-DLOG_TAG="${LIBRARY_NAME}") #定义当前算法库的log
else()
    message(STATUS "CLOSE_LOG")
    add_definitions(-DCALCULATE_TIME=0)  #统计时间
    add_definitions(-DDISABLE_LOG) #关闭log
endif()

#option(DYNAMIC_DENOISE "Set to ON to build use DYNAMIC_DENOISE" OFF)    
if (DYNAMIC_DENOISE)
    add_definitions(-DDYNAMIC_DENOISE)
    message(STATUS "Use DYNAMIC_DENOISE")
endif()

option(USE_SSE "Set to ON to build use SSE"  OFF)    #打开SSE加速开关
if (USE_SSE)
    add_definitions(-D_SSE_OPT_)
    message(STATUS "USE_SSE")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /msse4.2")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
endif()

#OpenMP
option(USE_OPENMP "Open openmp apps or not" OFF)
if (USE_OPENMP)
    message(STATUS "USE_OPENMP")
    find_package(OpenMP)
    set(OpenMP_LIBS OpenMP::OpenMP_CXX)
endif ()

# 添加第三方库的配置文件
#include("${CMAKE_SOURCE_DIR}/cmake/libyuv-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libmpbase-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libmobilecv-config.cmake")

# 搜集算法库所需的头文件和源码文件, 可以用一些正则来匹配你的源码,
# 但是一般的教程不建议这么做, 一个一个加避免出错
# 对于过多的文件，正则化来匹配
# 搜集算法库所需的头文件和源码文件
if(ANDROID)
    file(GLOB COMMON_INCLUDE common/*.cpp common/*.h)
else()
    file(GLOB COMMON_INCLUDE common/*.cpp common/*.h common/win/*.h common/win/*.cpp)
endif()
file(GLOB UTILS_SRC utils/*.cpp utils/*.h utils/imgFilter/*.h utils/imgFilter/*.cpp)
file(GLOB IMGRESIZE_SRC utils/imgResize/*.cpp utils/imgResize/*.h)
file(GLOB SRC ./*.h ./*.cpp)

file(GLOB GUIDER_FILTER guideFilter/*.cpp guideFilter/*.h)
file(GLOB WAVELET_FILTER wavelet/*.cpp wavelet/*.h)
file(GLOB ANIS_GUIDE_PYR anisGuidePyrFilter/*.cpp anisGuidePyrFilter/*.h)
file(GLOB ANIS_PRY anisotropicPyrFilter/*.cpp anisotropicPyrFilter/*.h)
file(GLOB NLM_FILTER pryDenoiseNLM/*.cpp pryDenoiseNLM/*.h)
file(GLOB SHARPEN_SRC sharpen/*.cpp sharpen/*.h)
file(GLOB DECOLORNOISE_SRC deColorNoise/*.cpp deColorNoise/*.h)
file(GLOB PYRAMID_SRC pyramidBuild/*.cpp pyramidBuild/*.h)
file(GLOB NLM_SSE_SRC sse_nlm/*.cpp sse_nlm/*.h)
file(GLOB EDGE_FILTER_SRC edgeFilter/*.cpp edgeFilter/*.h)

set(SOURCE_FILES
        ${SRC}
        ${GUIDER_FILTER}
        ${COMMON_INCLUDE}
        ${WAVELET_FILTER}
        ${ANIS_GUIDE_PYR}
        ${ANIS_PRY}
        ${NLM_FILTER}
        ${UTILS_SRC}
        ${IMGRESIZE_SRC}
        ${SHARPEN_SRC}
        ${DECOLORNOISE_SRC}
        ${PYRAMID_SRC}
        ${NLM_SSE_SRC}
        ${EDGE_FILTER_SRC}
        )

# 在编译一个target的时候并不需要头文件列表, 但是在开发中我们希望它们出现
# 在IDE中, 因此在这里我们一并把他们列出来
file(GLOB HEADER_FILES
        ${LIBRARY_INCLUDE_DIRS}/*.h
        )

# 加入头文件搜索路径
include_directories(
        ${libyuv_INCLUDE_DIRS}
        ${libmobilecv_INCLUDE_DIRS}
        ${libmpbase_INCLUDE_DIRS}
        ${LIBRARY_INCLUDE_DIRS}
        ./
        common/
        common/win/
        guideFilter/
        wavelet/
        anisGuidePyrFilter/
        anisotropicPyrFilter/
        pryDenoiseNLM/
        wavelet/
        utils/
        utils/imgResize/
        utils/imgFilter/
        deColorNoise/
        sharpen/
        pyramidBuild/
        sse_nlm/
        edgeFilter/
        ${OpenCV_INCLUDE_DIRS}
)

# 创建我们的算法库, 用上之前我们定义的 `LIBRARY_NAME` 防止硬编码
if(MSVC)
    if(BUILD_SHARED_LIBS)
        message(STATUS "BUILD_SHARED_LIBS")
        add_definitions("-DENABLE_DLL")
    endif()
endif()
add_library(${LIBRARY_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# asan
if(BUILD_ASAN_LIBS)
    message("Build asan libs....")
    set_target_properties(${LIBRARY_NAME} PROPERTIES LINK_FLAGS -fsanitize=address)
    target_compile_options(${LIBRARY_NAME} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
endif()

if(BUILD_HWASAN_LIBS)
    set_target_properties(${LIBRARY_NAME} PROPERTIES LINK_FLAGS -fsanitize=hwaddress)
    target_compile_options(${LIBRARY_NAME} PUBLIC -fsanitize=hwaddress -fno-omit-frame-pointer)
endif()


# 一些友好的设置: debug库加入 "d" 后缀
set_target_properties(${LIBRARY_NAME} PROPERTIES DEBUG_POSTFIX "d")
if(ANDROID)
find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log )
endif()

 target_link_libraries(${LIBRARY_NAME}
            ${libmobilecv_LIBS}
            ${libmpbase_LIBS}
            ${libyuv_LIBS}
            ${log-lib}
            ${OpenCV_LIBS}
            ${OpenMP_LIBS}
            )

# 设置库的安装目录, 我们将 .dll 和 .lib 放在同一个文件夹
# 一般市面上的库会将 *.dll 放在 bin文件夹, 把 *.lib 放在 lib文件夹
# 我们为了方便放在一起, 也省去不同平台的差异
install(TARGETS ${LIBRARY_NAME} LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib"
        RUNTIME DESTINATION "lib"
        COMPONENT library)

# 安装接口头文件, 因为我们将接口头文件分开放置, 此处我们可以直接安装
# 整个目录. 我提倡将头文件分开, 这是其中一个原因
install(DIRECTORY "${LIBRARY_INCLUDE_DIRS}/" DESTINATION include/)