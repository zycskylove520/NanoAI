# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: NanoAINCNNCommon.cmake
# Brief: 收敛 NanoAI_NCNN 的全局编译标准、默认构建类型与输出目录策略。
#
# Notes:
# - 该文件只设置跨平台通用选项，不处理具体第三方依赖或目标平台差异。
# - 修改默认值时要同步检查 README 与打包文档中的构建示例是否仍然准确。

include_guard(GLOBAL)

function(nanoai_setup_global_options)
    # 统一使用 C++20/C11，保证头文件中的现代类型与并发设施可在所有子目标中一致可用。
    set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
    set(CMAKE_CXX_EXTENSIONS OFF PARENT_SCOPE)

    set(CMAKE_C_STANDARD 11 PARENT_SCOPE)
    set(CMAKE_C_STANDARD_REQUIRED ON PARENT_SCOPE)
    set(CMAKE_C_EXTENSIONS OFF PARENT_SCOPE)

    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        # 单配置生成器默认落到 Release，避免端侧推理场景误用 Debug 带来明显性能偏差。
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
    endif()

    if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        # 统一输出目录可以减少 examples 与主库在多平台构建时查找产物的心智负担。
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin PARENT_SCOPE)
    endif()
    if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)
    endif()
    if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)
    endif()
endfunction()
