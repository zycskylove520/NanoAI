# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: JPEGTurbo.cmake
# Brief: 配置 jpeg_turbo 的头文件与库目录，用于高性能 JPEG 解码场景。
#
# Notes:
# - 该脚本假设 aarch64 产物位于 `Linux/aarch64` 目录，若 SDK 布局变化需同步更新。
# - 只追加 `turbojpeg` 链接项，具体上层是否使用由源码路径决定。

include_guard(GLOBAL)

# jpeg_turbo
# 使用 root 路径推导 include/lib 目录，保持与其他 thirdparty 脚本风格一致。
set(NANOAI_RKNN_JPEG_TURBO_ROOT "" CACHE PATH "jpeg_turbo root directory")
if(NOT CMAKE_CROSSCOMPILING)
	message(STATUS "Skip jpeg_turbo dependency checks during native configure probe.")
	return()
endif()

if(NANOAI_RKNN_JPEG_TURBO_ROOT STREQUAL "")
	message(FATAL_ERROR
		"jpeg_turbo dependency is enabled but root path is not set. "
		"Please provide -DNANOAI_RKNN_JPEG_TURBO_ROOT=/path/to/jpeg_turbo."
	)
endif()

set(JPEG_TURBO_DIR ${NANOAI_RKNN_JPEG_TURBO_ROOT})
set(JPEG_TURBO_INCLUDE_DIR ${JPEG_TURBO_DIR}/include)
set(JPEG_TURBO_LIB_DIR ${JPEG_TURBO_DIR}/Linux/aarch64)
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${JPEG_TURBO_INCLUDE_DIR})
list(APPEND NANOAI_RKNN_LINK_DIRS ${JPEG_TURBO_LIB_DIR})
list(APPEND NANOAI_RKNN_LINK_LIBS turbojpeg)
