#
# acv_oclcore_INCLUDE_DIRS
# acv_oclcore_LIBS

set(_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/acv_oclcore")
set(_NAME acv_oclcore)

if(MSVC)

    set(acv_oclcore_INCLUDE_DIRS "${_ROOT}/include")

    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
        set(_ROOT "${_ROOT}/lib/vs2019_win32/")
    else("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(_ROOT "${_ROOT}/lib/vs2019_win64/")
    endif("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
    # debug properties
    add_library(acv_oclcore SHARED IMPORTED)

    set_property(TARGET acv_oclcore APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_target_properties(acv_oclcore PROPERTIES
            IMPORTED_IMPLIB_DEBUG "${_ROOT}/arcsoft_acv_oclcored.lib"
            )
    set_property(TARGET acv_oclcore APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(acv_oclcore PROPERTIES
            IMPORTED_IMPLIB_RELEASE "${_ROOT}/arcsoft_acv_oclcore.lib"
            )
    set(acv_oclcore_LIBS acv_oclcore)
else()
    if(ANDROID)
        set(acv_oclcore_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/${_NAME}/lib/Android/${ANDROID_ABI}/")
        set(acv_oclcore_INCLUDE_DIRS "${_ROOT}/include")
    endif()

    # debug properties
    add_library(acv_oclcore STATIC IMPORTED)
    message(${acv_oclcore_ROOT})
    set_target_properties(acv_oclcore PROPERTIES
            IMPORTED_LOCATION "${acv_oclcore_ROOT}/libarcsoft_acv_oclcore.a"
            )
    set(acv_oclcore_LIBS acv_oclcore)
    set(acv_oclcore_INCLUDE_DIRS "${_ROOT}/include")

endif()