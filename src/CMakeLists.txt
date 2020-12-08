# ./src/CMakeList.txt
# 定义工程路径
add_definitions(-DPROJECT_PATH="${CMAKE_SOURCE_DIR}") #定义当前算法库的log

# debug宏定义
option(DEBUG_OUTPUT "Build test apps or not" OFF)
message(${DEBUG_OUTPUT})
if(DEBUG_OUTPUT)
    find_package(OpenCV REQUIRED)
    add_definitions(-DDEBUG_OUTPUT)
endif()

option(OPEN_LOG "Open log apps or not" OFF)
if(1)
    # 添加宏定义
    message(log)
    add_definitions(-DCALCULATE_TIME=1)  #统计时间
    add_definitions(-DLOG_TAG="${LIBRARY_NAME}") #定义当前算法库的log
else()
    message(no log)
    add_definitions(-DCALCULATE_TIME=0)  #统计时间
    add_definitions(-DDISABLE_LOG) #关闭log
endif()

# 添加第三方库的配置文件
include("${CMAKE_SOURCE_DIR}/cmake/libyuv-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libmpbase-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libmobilecv-config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/libacv_oclcore-config.cmake")

# 搜集算法库所需的头文件和源码文件, 可以用一些正则来匹配你的源码,
# 但是一般的教程不建议这么做, 一个一个加避免出错
# 对于过多的文件，正则化来匹配
# 搜集算法库所需的头文件和源码文件
file(GLOB SRC *.cpp *.h)
file(GLOB SRC_NLM_OCL nlmOCL/*.cpp nlmOCL/*.h)
file(GLOB SRC_COMMON common/*.cpp common/*.h)


set(SOURCE_FILES
        ${SRC}
        ${SRC_NLM_OCL}
        ${SRC_COMMON}
        )

# 在编译一个target的时候并不需要头文件列表, 但是在开发中我们希望它们出现
# 在IDE中, 因此在这里我们一并把他们列出来
file(GLOB HEADER_FILES
        ${LIBRARY_INCLUDE_DIRS}/*.h
        )

# 加入头文件搜索路径
include_directories(
        ${LIBRARY_INCLUDE_DIRS}
        ${libyuv_INCLUDE_DIRS}
        ${libmobilecv_INCLUDE_DIRS}
        ${libmpbase_INCLUDE_DIRS}
        ${acv_oclcore_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
        ./
        CL/
        nlmOCL/
        common/
)

# 创建我们的算法库, 用上之前我们定义的 `LIBRARY_NAME` 防止硬编码
add_library(${LIBRARY_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# 一些友好的设置: debug库加入 "d" 后缀
set_target_properties(${LIBRARY_NAME} PROPERTIES DEBUG_POSTFIX "d")
if(ANDROID)
    find_library( # Sets the name of the path variable.
            log-lib

            # Specifies the name of the NDK library that
            # you want CMake to locate.
            log )
    target_link_libraries(${LIBRARY_NAME}
            ${libmobilecv_LIBS}
            ${libmpbase_LIBS}
            ${libyuv_LIBS}
            ${log-lib}
            ${acv_oclcore_LIBS}
            )
else()
    target_link_libraries(${LIBRARY_NAME}
            ${libmobilecv_LIBS}
            ${libmpbase_LIBS}
            ${libyuv_LIBS}
            ${log-lib}
            ${OpenCV_LIBS}
            ${acv_oclcore_LIBS}
            )
endif()

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


