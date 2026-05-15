# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: NanoAIRKNNThirdParty.cmake
# Brief: 汇总 RKNN 交叉编译所需第三方依赖开关，并按顺序加载各依赖配置脚本。
#
# Notes:
# - 该文件负责决定“需要哪些依赖”，具体路径校验由各 thirdparty 子脚本完成。
# - include 顺序应保持稳定，避免后续脚本依赖前面已追加的 include/source 列表时出现差异。

include_guard(GLOBAL)

# ======================================================
# 3rdparty 依赖库配置
# 当前默认强制开启全部 RKNN 运行时相关依赖，目标是优先保证开箱即用，
# 若未来支持更细粒度裁剪，可在这里把 FORCE 改为普通默认值。
# ======================================================
set(NANOAI_THIRD_PARTY_DIR ${NANOAI_PROJECT_ROOT}/3rdparty)
set(NANOAI_RKNN_THIRD_PARTY_DIR ${NANOAI_THIRD_PARTY_DIR}/rknn)

set(NANOAI_RKNN_WITH_RKNN_RUNTIME ON CACHE BOOL "Enable RKNN runtime dependency" FORCE)
set(NANOAI_RKNN_WITH_RGA ON CACHE BOOL "Enable RGA dependency" FORCE)
set(NANOAI_RKNN_WITH_OPENCV ON CACHE BOOL "Enable OpenCV dependency" FORCE)
set(NANOAI_RKNN_WITH_STB_IMAGE ON CACHE BOOL "Enable stb_image dependency" FORCE)
set(NANOAI_RKNN_WITH_JPEG_TURBO ON CACHE BOOL "Enable jpeg_turbo dependency" FORCE)
set(NANOAI_RKNN_WITH_UTILS ON CACHE BOOL "Enable third-party utils sources" FORCE)

if(NANOAI_RKNN_WITH_UTILS AND NOT NANOAI_RKNN_WITH_RGA)
	# utils 源码依赖 RGA allocator 头文件与实现，关闭 RGA 后继续编译会产生缺失符号。
	message(WARNING "NANOAI_RKNN_WITH_UTILS requires NANOAI_RKNN_WITH_RGA=ON; disabling utils sources.")
	set(NANOAI_RKNN_WITH_UTILS OFF)
endif()

# 依赖脚本按“核心运行时 -> 图像加速/解码 -> 辅助源码”顺序加载，便于阅读与问题定位。
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/RKNNRuntime.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/RGA.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/OpenCV.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/STBImage.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/JPEGTurbo.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/Utils.cmake)