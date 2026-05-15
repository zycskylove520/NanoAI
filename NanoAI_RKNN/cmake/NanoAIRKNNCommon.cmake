# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: NanoAIRKNNCommon.cmake
# Brief: 统一 NanoAI_RKNN 的全局编译标准与输出目录策略。
#
# Notes:
# - 该文件只设置通用构建基线，不负责具体第三方依赖发现。
# - 若未来不同子目标需要差异化标准，优先在目标级覆盖，而不是放宽这里的默认约束。

include_guard(GLOBAL)

function(nanoai_setup_global_options)
    # RKNN 模块依赖 NanoAIFlow 的现代模板接口，因此统一要求 C++20。
    set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
    set(CMAKE_CXX_EXTENSIONS OFF PARENT_SCOPE)

    set(CMAKE_C_STANDARD 11 PARENT_SCOPE)
    set(CMAKE_C_STANDARD_REQUIRED ON PARENT_SCOPE)
    set(CMAKE_C_EXTENSIONS OFF PARENT_SCOPE)

    # 单配置生成器默认走 Release，避免交叉编译时忘记指定构建类型而得到不可预期性能。
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
    endif()

    # 统一输出目录可让示例、库和打包脚本共享稳定产物路径。
    if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin PARENT_SCOPE)
    endif()
    if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)
    endif()
    if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)
    endif()
endfunction()
