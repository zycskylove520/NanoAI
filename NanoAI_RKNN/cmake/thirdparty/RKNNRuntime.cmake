# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: RKNNRuntime.cmake
# Brief: 绑定 RKNN 官方运行时头文件与库目录，供 NanoAI_RKNN 目标链接使用。
#
# Notes:
# - RKNN runtime 不随仓库自动分发，必须由使用者显式提供安装路径。
# - 这里仅注册 include/lib 路径，不主动探测库文件名以保持交叉编译环境可控。

include_guard(GLOBAL)

# rknn_runtime (external path required)
# 将路径做成 cache 变量，便于 presets、CI 或命令行工具链统一注入。
set(NANOAI_RKNN_RUNTIME_INCLUDE_DIR "" CACHE PATH "RKNN runtime include directory")
set(NANOAI_RKNN_RUNTIME_LIBRARY_DIR "" CACHE PATH "RKNN runtime library directory")

if(NOT CMAKE_CROSSCOMPILING)
	# IDE 原生探测时跳过强校验，避免开发机未安装 RKNN SDK 时无法打开工程。
	message(STATUS "Skip RKNN runtime dependency checks during native configure probe.")
	return()
endif()

if(NANOAI_RKNN_RUNTIME_INCLUDE_DIR STREQUAL "" OR NANOAI_RKNN_RUNTIME_LIBRARY_DIR STREQUAL "")
	message(FATAL_ERROR
		"RKNN runtime dependency is enabled but paths are not set. "
		"Please provide -DNANOAI_RKNN_RUNTIME_INCLUDE_DIR and -DNANOAI_RKNN_RUNTIME_LIBRARY_DIR."
	)
endif()

set(RKNN_RT_INCLUDE_DIR ${NANOAI_RKNN_RUNTIME_INCLUDE_DIR})
set(RKNN_RT_LIB_DIR ${NANOAI_RKNN_RUNTIME_LIBRARY_DIR})
list(APPEND NANOAI_RKNN_INCLUDE_DIRS ${RKNN_RT_INCLUDE_DIR})
list(APPEND NANOAI_RKNN_LINK_DIRS ${RKNN_RT_LIB_DIR})
# 统一链接 rknnrt，假设调用方提供的目录已经与目标架构匹配。
list(APPEND NANOAI_RKNN_LINK_LIBS rknnrt)
