include_guard(GLOBAL)

set(NANOAI_NCNN_OPENCV_DIR "" CACHE PATH "OpenCV_DIR for find_package(OpenCV)")

# linux x86_64 预设启用 OpenCV 依赖，且默认自动搜索，用户需要根据实际情况提供 OpenCV_DIR
if(NANOAI_NCNN_LINUX_x86_64_PRESET)
    if(NANOAI_NCNN_OPENCV_DIR STREQUAL "")
        find_package(OpenCV REQUIRED)
    else()
        set(OpenCV_DIR ${NANOAI_NCNN_OPENCV_DIR})
        find_package(OpenCV REQUIRED)
    endif()
endif()

# Android 预设启用 OpenCV 依赖，且默认 OpenCV_DIR 为空，用户需要手动提供 OpenCV_DIR
if(NANOAI_NCNN_ANDROID_PRESET)
    if(NANOAI_NCNN_OPENCV_DIR STREQUAL "")
        message(FATAL_ERROR
            "OpenCV dependency is enabled but OpenCV_DIR is not set. "
            "Please provide -DNANOAI_NCNN_OPENCV_DIR."
        )
    endif()

    # # OpenCV Android SDK's OpenCVConfig.cmake uses ANDROID_NDK_ABI_NAME to
    # # choose abi-<name>/OpenCVConfig.cmake. Newer NDK toolchains may only set
    # # CMAKE_ANDROID_ARCH_ABI / ANDROID_ABI, so bridge it here.
    # if((NOT DEFINED ANDROID_NDK_ABI_NAME OR ANDROID_NDK_ABI_NAME STREQUAL ""))
    #     if(DEFINED CMAKE_ANDROID_ARCH_ABI AND NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "")
    #         set(ANDROID_NDK_ABI_NAME "${CMAKE_ANDROID_ARCH_ABI}" CACHE STRING "Android ABI name for OpenCV Android SDK" FORCE)
    #     elseif(DEFINED ANDROID_ABI AND NOT ANDROID_ABI STREQUAL "")
    #         set(ANDROID_NDK_ABI_NAME "${ANDROID_ABI}" CACHE STRING "Android ABI name for OpenCV Android SDK" FORCE)
    #     endif()
    # endif()

    set(OpenCV_DIR ${NANOAI_NCNN_OPENCV_DIR})
    find_package(OpenCV REQUIRED)
endif()

list(APPEND NANOAI_NCNN_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
list(APPEND NANOAI_NCNN_LINK_LIBS ${OpenCV_LIBS})