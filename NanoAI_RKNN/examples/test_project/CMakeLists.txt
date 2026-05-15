# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: CMakeLists.txt
# Brief: 定义 `test_project` 示例目标，并将其链接到当前构建出的 `NanoAI_RKNN` 目标。
#
# Notes:
# - 该示例假设所有交叉编译参数已经在根工程中配置完成。
# - 这里不重复声明第三方依赖，统一复用根目标的链接结果以降低示例维护成本。

cmake_minimum_required(VERSION 3.16)

# 该子项目不支持独立构建，必须通过 NanoAI_RKNN 根 CMake 选择构建。
# 这样可确保工具链、NanoAIFlow 与 RKNN 相关依赖全部沿用根配置结果。
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR
        "examples/test_project does not support standalone configure. "
        "Configure from NanoAI_RKNN root with -DNANOAI_RKNN_BUILD_EXAMPLES=ON and select this example. "
        "Cross-compiler and tool paths should be provided from the root configure/build invocation."
    )
endif()

# ======================================================
# NanoAI RKNN 模块来源
# 同时兼容别名目标与真实目标名，便于后续安装态/源码态保持统一链接写法。
# ======================================================
set(NANOAI_RKNN_TARGET NanoAI_RKNN)

if(TARGET NanoAI_RKNN::NanoAI_RKNN)
    set(NANOAI_RKNN_TARGET NanoAI_RKNN::NanoAI_RKNN)
elseif(TARGET NanoAI_RKNN)
    set(NANOAI_RKNN_TARGET NanoAI_RKNN)
else()
    message(FATAL_ERROR
        "NanoAI_RKNN target not found. This subproject must be built from NanoAI_RKNN root."
    )
endif()

set(NANOAI_RKNN_DEMO_TARGET NanoAI_rknn_demo)

# 示例只保留一个最小可执行目标，用于验证加载、预处理、推理和后处理链路是否可运行。
add_executable(${NANOAI_RKNN_DEMO_TARGET}
    ${CMAKE_CURRENT_LIST_DIR}/demo_main.cpp
)

target_link_libraries(${NANOAI_RKNN_DEMO_TARGET}
    PRIVATE
    ${NANOAI_RKNN_TARGET}
)