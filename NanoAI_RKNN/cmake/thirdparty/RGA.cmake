# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: RGA.cmake
# Brief: 引入 Rockchip RGA 头文件、库目录以及其工具源码，支撑图像硬件加速路径。
#
# Notes:
# - RGA 路径由外部 SDK 提供，目录结构默认匹配官方常见发布布局。
# - 该脚本除链接 `rga` 外，还会把 utils 源码并入主目标，因此会影响库类型选择。

include_guard(GLOBAL)

# rga (external path required)
# 通过 root 路径统一推导 include/lib/utils 目录，减少命令行参数数量。
set(NANOAI_RKNN_RGA_ROOT "" CACHE PATH "RGA root directory")
if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Skip RGA dependency checks during native configure probe.")
    return()
endif()

if(NANOAI_RKNN_RGA_ROOT STREQUAL "")
    message(FATAL_ERROR
        "RGA dependency is enabled but root path is not set. "
        "Please provide -DNANOAI_RKNN_RGA_ROOT=/path/to/librga."
    )
endif()

set(RGA_DIR ${NANOAI_RKNN_RGA_ROOT})
set(RGA_INCLUDE_DIR ${RGA_DIR}/include)
set(RGA_LIB_DIR ${RGA_DIR}/libs/Linux/gcc-aarch64)
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${RGA_INCLUDE_DIR} ${RGA_DIR}/utils/allocator/include)
list(APPEND NANOAI_RKNN_LINK_DIRS ${RGA_LIB_DIR})
list(APPEND NANOAI_RKNN_LINK_LIBS rga)

# rga utils
# 直接把官方 utils 源码编进 NanoAI_RKNN，避免额外维护单独的工具静态库目标。
file(GLOB_RECURSE RGA_UTILS_SRCS
    ${RGA_DIR}/utils/src/*.cpp
    ${RGA_DIR}/utils/allocator/src/*.cpp
)
list(APPEND NANOAI_RKNN_SRCS ${RGA_UTILS_SRCS})
