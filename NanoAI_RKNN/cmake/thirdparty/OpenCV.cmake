# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: OpenCV.cmake
# Brief: 通过 `find_package(OpenCV)` 引入视觉预处理所需的 OpenCV 头文件与链接库。
#
# Notes:
# - 这里依赖调用方提供可用于目标架构的 OpenCV CMake 配置目录。
# - OpenCV 结果直接透传到 NanoAI_RKNN，避免在本模块中重新拆分组件列表。

include_guard(GLOBAL)

# opencv
# 显式要求 OpenCV_DIR，避免交叉编译时误拾取主机系统的 x86 OpenCV 安装。
set(NANOAI_RKNN_OPENCV_DIR "" CACHE PATH "OpenCV_DIR for find_package(OpenCV)")
if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Skip OpenCV dependency checks during native configure probe.")
    return()
endif()

if(NANOAI_RKNN_OPENCV_DIR STREQUAL "")
    message(FATAL_ERROR
        "OpenCV dependency is enabled but OpenCV_DIR is not set. "
        "Please provide -DNANOAI_RKNN_OPENCV_DIR=/path/to/opencv4/cmake directory."
    )
endif()

set(OpenCV_DIR ${NANOAI_RKNN_OPENCV_DIR})

find_package(OpenCV REQUIRED)
# 直接复用 OpenCV 自带的 include/lib 列表，保持与其官方导出配置一致。
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
list(APPEND NANOAI_RKNN_LINK_LIBS ${OpenCV_LIBS})
