# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: OpenCV.cmake
# Brief: 查找 OpenCV 并把头文件与链接信息追加到 NanoAI_NCNN 的公共接口列表中。
#
# Notes:
# - OpenCV 组件集合通过缓存变量公开，便于不同部署场景裁剪到最小依赖面。
# - Android 或交叉编译场景通常需要显式指定 `OPENCV_CMAKE_DIR`，避免误命中主机版本。

include_guard(GLOBAL)

set(OPENCV_CMAKE_DIR "" CACHE PATH "OpenCV_DIR for find_package(OpenCV)")
set(NANOAI_NCNN_OPENCV_COMPONENTS "core;imgproc;imgcodecs" CACHE STRING "OpenCV components for NanoAI_NCNN public API")

if(NANOAI_NCNN_WITH_OPENCV)
    set(_NANOAI_NCNN_OPENCV_FIND_ARGS REQUIRED)
    if(NOT NANOAI_NCNN_OPENCV_COMPONENTS STREQUAL "")
        list(APPEND _NANOAI_NCNN_OPENCV_FIND_ARGS COMPONENTS ${NANOAI_NCNN_OPENCV_COMPONENTS})
    endif()

    if(OPENCV_CMAKE_DIR STREQUAL "")
        find_package(OpenCV ${_NANOAI_NCNN_OPENCV_FIND_ARGS})
    else()
        # 通过显式覆盖 OpenCV_DIR，确保 find_package 不会优先拾取系统默认安装。
        set(OpenCV_DIR ${OPENCV_CMAKE_DIR})
        find_package(OpenCV ${_NANOAI_NCNN_OPENCV_FIND_ARGS})
    endif()

    if(OpenCV_INCLUDE_DIRS)
        list(APPEND NANOAI_NCNN_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
    endif()

    # 优先使用 OpenCV 导出的 target，只有旧式变量包时才退回原始库名列表。
    set(_NANOAI_NCNN_OPENCV_TARGET_LIBS)
    if(OpenCV_LIBS)
        foreach(_ncnn_opencv_lib IN LISTS OpenCV_LIBS)
            if(TARGET ${_ncnn_opencv_lib})
                list(APPEND _NANOAI_NCNN_OPENCV_TARGET_LIBS ${_ncnn_opencv_lib})
            endif()
        endforeach()
    endif()

    if(_NANOAI_NCNN_OPENCV_TARGET_LIBS)
        list(APPEND NANOAI_NCNN_LINK_LIBS ${_NANOAI_NCNN_OPENCV_TARGET_LIBS})
    else()
        list(APPEND NANOAI_NCNN_LINK_LIBS ${OpenCV_LIBS})
    endif()
endif()