# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: NanoAINCNNThirdParty.cmake
# Brief: 聚合 NanoAI_NCNN 需要的第三方依赖配置脚本，并建立统一的 third-party 根目录约定。
#
# Notes:
# - 这里只负责组织依赖脚本顺序与总开关，不直接写具体 `find_package` 细节。
# - 新增依赖时优先保持“一个依赖一个脚本”的结构，便于跨平台排查问题。

include_guard(GLOBAL)

# ======================================================
# Third-party dependency config
# NANOAI_NCNN_THIRD_PARTY_DIR 当前与仓库级 3rdparty 保持一致，便于后续按模块拆分时仍保留扩展点。
# ======================================================
set(NANOAI_THIRD_PARTY_DIR ${NANOAI_PROJECT_ROOT}/3rdparty)
set(NANOAI_NCNN_THIRD_PARTY_DIR ${NANOAI_THIRD_PARTY_DIR})

# NCNN 与 OpenCV 是当前公开视觉 API 的硬依赖，因此这里直接强制开启，
# 避免调用方关闭后得到不完整但表面可配置的构建结果。
set(NANOAI_NCNN_WITH_NCNN ON CACHE BOOL "Enable NCNN dependency" FORCE)
set(NANOAI_NCNN_WITH_OPENCV ON CACHE BOOL "Enable OpenCV dependency" FORCE)

include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/NCNNRuntime.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/thirdparty/OpenCV.cmake)
