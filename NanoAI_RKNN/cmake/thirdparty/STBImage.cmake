# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: STBImage.cmake
# Brief: 注册 `stb_image` 头文件搜索路径，供示例或轻量图像解码代码直接包含。
#
# Notes:
# - `stb_image` 为头文件库，因此这里只追加 include 目录，不涉及链接项。
# - 显式路径配置可避免与系统或其他子模块自带的 stb 版本混用。

include_guard(GLOBAL)

# stb_image
# cache 变量让调用方可在不同 SDK 安装位置间快速切换。
set(NANOAI_RKNN_STB_IMAGE_INCLUDE_DIR "" CACHE PATH "stb_image include directory")
if(NOT CMAKE_CROSSCOMPILING)
	message(STATUS "Skip stb_image dependency checks during native configure probe.")
	return()
endif()

if(NANOAI_RKNN_STB_IMAGE_INCLUDE_DIR STREQUAL "")
	message(FATAL_ERROR
		"stb_image dependency is enabled but include path is not set. "
		"Please provide -DNANOAI_RKNN_STB_IMAGE_INCLUDE_DIR=/path/to/stb_image."
	)
endif()

set(STB_IMAGE_INCLUDE_DIR ${NANOAI_RKNN_STB_IMAGE_INCLUDE_DIR})
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${STB_IMAGE_INCLUDE_DIR})
