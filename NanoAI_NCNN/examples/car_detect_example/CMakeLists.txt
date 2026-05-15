# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: CMakeLists.txt
# Brief: 定义 `car_detect_example` 示例目标，并复用顶层导出的 NanoAI_NCNN 依赖集合。
#
# Notes:
# - 该示例不支持 standalone configure，必须从 `NanoAI_NCNN` 根目录统一继承依赖与工具链配置。
# - 示例目标只补充自身 `include/` 与 `src/`，不重复声明 NCNN/OpenCV 等第三方库细节。

cmake_minimum_required(VERSION 3.16)

# 保证示例与主库在同一次 configure 中解析依赖，避免示例单独配置时找不到根目标或平台预设。
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR
        "projects/car_project does not support standalone configure. "
        "Configure from NanoAI_NCNN root with -DNANOAI_NCNN_BUILD_MODE=PROJECTS and select this project."
    )
endif()

set(NANOAI_NCNN_TARGET NanoAI_NCNN)

if(TARGET NanoAI_NCNN::NanoAI_NCNN)
    set(NANOAI_NCNN_TARGET NanoAI_NCNN::NanoAI_NCNN)
elseif(TARGET NanoAI_NCNN)
    set(NANOAI_NCNN_TARGET NanoAI_NCNN)
else()
    message(FATAL_ERROR
        "NanoAI_NCNN target not found. This subproject must be built from NanoAI_NCNN root."
    )
endif()

set(NANOAI_NCNN_CAR_TARGET NanoAI_ncnn_car_detect_example)

add_executable(${NANOAI_NCNN_CAR_TARGET}
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
)

target_include_directories(${NANOAI_NCNN_CAR_TARGET}
    PRIVATE
    # 示例私有头文件仅服务于该 demo，不应泄漏到公共接口中。
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${NANOAI_NCNN_CAR_TARGET}
    PRIVATE
    # 所有第三方依赖均由 NanoAI_NCNN 统一透传，示例只需链接聚合目标即可。
    ${NANOAI_NCNN_TARGET}
)

