#
# libmpbase_INCLUDE_DIRS
# libmpbase_LIBS

set(_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/mpbase")
set(_NAME mpbase)

if(MSVC)

    set(libmpbase_INCLUDE_DIRS "${_ROOT}/include")

    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
        set(_ROOT "${_ROOT}/Win/lib/x86")
    else("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(_ROOT "${_ROOT}/Win/lib/x64")
    endif("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
    # debug properties
    add_library(mpbase SHARED IMPORTED)

    set_property(TARGET mpbase APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_target_properties(mpbase PROPERTIES
            IMPORTED_IMPLIB_DEBUG "${_ROOT}/mpbase.lib"
            )
    set_property(TARGET mpbase APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(mpbase PROPERTIES
            IMPORTED_IMPLIB_RELEASE "${_ROOT}/mpbase.lib"
            )
    set(libmpbase_LIBS mpbase)
else()

    if(ANDROID)
        # todo, 安卓下库的版本分的太细，需要 ANDROID_ABI与库的文件夹名称对应，这里临时设置为固定某一个版本
        # ANDROID_ABI => android_armv7a, android_armv7a_qsee, android_armv7a_qsee_tee
        set(mpbase_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/Android/${ANDROID_ABI}/")
        set(libmpbase_INCLUDE_DIRS "${_ROOT}/include")
    elseif(APPLE)

        if(IOS_DEPLOYMENT_TARGET) # IOS
            set(mpbase_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/iOS/lib/")
        else()              # macOS
            set(mpbase_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/macOS/lib/")
        endif()
    else()
        if("${LINUX_DISTRIBUTION}" STREQUAL "ubuntu")
            set(mpbase_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/ubuntu/lib")
        elseif("${LINUX_DISTRIBUTION}" STREQUAL "centos")
            set(mpbase_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/CentOS/lib")
        endif()
        set(libmpbase_INCLUDE_DIRS "${_ROOT}/include")
    endif()

    # debug properties
    add_library(mpbase STATIC IMPORTED)

    set_target_properties(mpbase PROPERTIES
            IMPORTED_LOCATION "${mpbase_ROOT}/mpbase.a"
            )
    set(libmpbase_LIBS mpbase)
    set(libmpbase_INCLUDE_DIRS "${_ROOT}/include")

endif()