# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: Utils.cmake
# Brief: 将 RKNN 示例/工具源码目录中的通用 C/C++ 源文件并入 NanoAI_RKNN 构建。
#
# Notes:
# - 该脚本会把外部 utils 目录源码直接纳入主目标，适合快速复用，但也会扩大 ABI 面。
# - 若未来需要更强边界控制，可改为单独构建静态库再链接。

include_guard(GLOBAL)

# utils
# 使用 root 路径统一推导 include/src，可与外部 SDK 发布包目录保持一致。
set(NANOAI_RKNN_UTILS_ROOT "" CACHE PATH "RKNN utils root directory")
if(NOT CMAKE_CROSSCOMPILING)
	message(STATUS "Skip utils dependency checks during native configure probe.")
	return()
endif()

if(NANOAI_RKNN_UTILS_ROOT STREQUAL "")
	message(FATAL_ERROR
		"utils dependency is enabled but root path is not set. "
		"Please provide -DNANOAI_RKNN_UTILS_ROOT=/path/to/utils."
	)
endif()

set(UTILS_DIR ${NANOAI_RKNN_UTILS_ROOT})
set(UTILS_INCLUDE_DIR ${UTILS_DIR}/include)
set(UTILS_SOURCE_DIR ${UTILS_DIR}/src)
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${UTILS_INCLUDE_DIR})
# 直接收集全部 utils 源文件，优先降低接入成本；若后续编译时间过长可再细分白名单。
file(GLOB_RECURSE UTILS_SOURCES ${UTILS_SOURCE_DIR}/*.c ${UTILS_SOURCE_DIR}/*.cpp)
list(APPEND NANOAI_RKNN_SRCS ${UTILS_SOURCES})
