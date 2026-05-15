# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: CMakeLists.txt
# Brief: 定义 NanoAI_NCNN 的构建入口、预设工具链、依赖查找与包导出行为。
#
# Notes:
# - 该模块以 INTERFACE 包装方式暴露 NCNN 与 OpenCV 依赖，重点在复用现有运行时而非生成自有核心源码库。
# - 构建脚本同时兼顾 Linux x86_64 与 Android 预设，要求调用方显式确认目标平台路径与工具链策略。

cmake_minimum_required(VERSION 3.16)

## =====================================================
# Linux x86_64 交叉编译预设
# 该预设更偏向“快速固定工具链参数”的便捷入口，而非完整替代用户自定义 toolchain 文件。
## =====================================================
option(NANOAI_NCNN_LINUX_x86_64_PRESET "Enable Linux x86_64 GCC cross-compile preset" OFF)
if(NANOAI_NCNN_LINUX_x86_64_PRESET)
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
        message(INFO "CMAKE_TOOLCHAIN_FILE is not set. The Linux x86_64 preset will set it to a default value.")
    endif()
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/toolchains/ncnn-linux-x86_64-gcc.cmake")

    message(STATUS "NanoAI_NCNN Linux x86_64 preset enabled.")
    message(STATUS "CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()

## =====================================================
# Android 交叉编译预设
# Android 场景要求调用方先显式提供 NDK toolchain，再由本脚本补充 ABI/STL 等常用参数。
## =====================================================
option(NANOAI_NCNN_ANDROID_PRESET "Enable Android cross-compile preset" OFF)
if(NANOAI_NCNN_ANDROID_PRESET)
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
        message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE must be set when NANOAI_NCNN_ANDROID_PRESET is enabled.")
    endif()
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/toolchains/ncnn-android.cmake")

    message(STATUS "NanoAI_NCNN Android preset enabled.")
    message(STATUS "CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()

## 只允许使用其中一种交叉编译预设
# 两个预设会共同影响系统平台变量，若同时开启会导致目标平台语义冲突。
if(NANOAI_NCNN_LINUX_x86_64_PRESET AND NANOAI_NCNN_ANDROID_PRESET)
    message(FATAL_ERROR "NANOAI_NCNN_LINUX_x86_64_PRESET and NANOAI_NCNN_ANDROID_PRESET cannot both be ON.")
endif()

project(NanoAI_NCNN VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/NanoAINCNNCommon.cmake)
nanoai_setup_global_options()


## =====================================================
# 构建行为
# 示例构建按需开启；主库本体始终以 INTERFACE 方式导出，便于下游直接复用外部依赖配置。
## =====================================================
option(NANOAI_NCNN_BUILD_EXAMPLES "Build NanoAI_NCNN examples (optional)" OFF)
set(NANOAI_NCNN_EXAMPLES "ALL" CACHE STRING "Semicolon-separated example names under examples/ (ALL or example names)")
option(NANOAI_NCNN_ENABLE_CPACK "Enable CPack package generation (optional)" OFF)

message(STATUS "NanoAI_NCNN build examples: ${NANOAI_NCNN_BUILD_EXAMPLES}")
message(STATUS "NanoAI_NCNN selected examples: ${NANOAI_NCNN_EXAMPLES}")

# ======================================================
# 路径配置
# 允许显式注入 NanoAIFlow 安装前缀，避免在多版本或多平台环境中依赖系统默认搜索顺序。
# ======================================================
get_filename_component(NANOAI_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)

set(NANOAI_NCNN_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/NanoAI_NCNN" CACHE STRING "Install path for NanoAI_NCNN CMake package files")

set(NANOAIFLOW_ROOT "" CACHE PATH "NanoAIFlow install prefix (platform-agnostic, optional)")

if(NANOAIFLOW_ROOT)
    # 让 find_package() 可以优先从指定安装目录查找 NanoAIFlow。
    list(APPEND CMAKE_PREFIX_PATH "${NANOAIFLOW_ROOT}")
endif()

# ======================================================
# 内容配置
# thirdparty 脚本通过 list(APPEND) 追加 include/link 信息，因此这里先建立统一的可扩展列表。
# ======================================================
set(NANOAI_NCNN_PUBLIC_INCLUDE_DIR ${NANOAI_PROJECT_ROOT}/include)  # NanoAI_NCNN 公开头文件目录
set(NANOAI_NCNN_INCLUDE_DIRS ${NANOAI_NCNN_PUBLIC_INCLUDE_DIR})  # NanoAI_NCNN 头文件搜索目录列表，可能会在第三方库依赖中追加
set(NANOAI_NCNN_LINK_DIRS)  # NanoAI_NCNN 链接库搜索目录列表，可能会在第三方库依赖中追加
set(NANOAI_NCNN_LINK_LIBS)  # NanoAI_NCNN 链接库列表，可能会在第三方库依赖中追加

# ======================================================
# 依赖发现
# NanoAIFlow 为强制依赖；NCNN/OpenCV 等外部库在 thirdparty 脚本中完成细节处理。
# ======================================================

# threads 依赖
# set(_NANOAI_NCNN_NEED_THREADS ON)
# find_package(Threads REQUIRED)

# NanoAIFlow 依赖
find_package(NanoAIFlow CONFIG REQUIRED)
if(NOT TARGET NanoAI::Flow)
    message(FATAL_ERROR
        "NanoAIFlow found but target NanoAI::Flow is missing. "
        "Please check your NanoAIFlow package installation."
    )
endif()

# 第三方库依赖（如果有）会在 NanoAINCNNThirdParty.cmake 中处理
include(${CMAKE_CURRENT_LIST_DIR}/cmake/NanoAINCNNThirdParty.cmake)

set(NANOAI_NCNN_BUILD_INTERFACE_INCLUDE_DIRS)
foreach(_nanoai_ncnn_inc IN LISTS NANOAI_NCNN_INCLUDE_DIRS)
    list(APPEND NANOAI_NCNN_BUILD_INTERFACE_INCLUDE_DIRS "$<BUILD_INTERFACE:${_nanoai_ncnn_inc}>")
endforeach()

set(NANOAI_NCNN_BUILD_INTERFACE_LINK_DIRS)
foreach(_nanoai_ncnn_link_dir IN LISTS NANOAI_NCNN_LINK_DIRS)
    list(APPEND NANOAI_NCNN_BUILD_INTERFACE_LINK_DIRS "$<BUILD_INTERFACE:${_nanoai_ncnn_link_dir}>")
endforeach()

# ======================================================
# 基础链接库
# 当前公开接口默认把 `dl/m/stdc++` 一并透传给下游，减少集成时遗漏系统库的概率。
# ======================================================
set(NANOAI_NCNN_BASE_LIBS
    dl
    m
    stdc++
    NanoAI::Flow
)

# ======================================================
# 目标定义
# 使用 INTERFACE 库意味着 NanoAI_NCNN 自身不产出独立对象文件，
# 其价值在于统一公开头文件、依赖路径和编译定义。
# ======================================================
add_library(NanoAI_NCNN INTERFACE)

target_compile_definitions(NanoAI_NCNN
    INTERFACE
    NANOAI_NCNN_WITH_NCNN_RUNTIME=1
)

target_include_directories(NanoAI_NCNN
    INTERFACE
    ${NANOAI_NCNN_BUILD_INTERFACE_INCLUDE_DIRS}
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_directories(NanoAI_NCNN
    INTERFACE
    ${NANOAI_NCNN_BUILD_INTERFACE_LINK_DIRS}
)

target_link_libraries(NanoAI_NCNN
    INTERFACE
    ${NANOAI_NCNN_BASE_LIBS}
    ${NANOAI_NCNN_LINK_LIBS}
)

add_library(NanoAI_NCNN::NanoAI_NCNN ALIAS NanoAI_NCNN)

# ======================================================
# 包导出与安装
# 即使 examples 关闭，也始终导出 Config/Targets，保证 `find_package` 使用路径稳定。
# ======================================================
install(TARGETS NanoAI_NCNN
    EXPORT NanoAI_NCNNTargets
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAI_NCNNConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NanoAI_NCNNConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAI_NCNNConfig.cmake"
    INSTALL_DESTINATION ${NANOAI_NCNN_INSTALL_CMAKEDIR}
)

install(EXPORT NanoAI_NCNNTargets
    FILE NanoAI_NCNNTargets.cmake
    NAMESPACE NanoAI_NCNN::
    DESTINATION ${NANOAI_NCNN_INSTALL_CMAKEDIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAI_NCNNConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAI_NCNNConfigVersion.cmake"
    DESTINATION ${NANOAI_NCNN_INSTALL_CMAKEDIR}
)

if(NANOAI_NCNN_ENABLE_CPACK)
    # 仅在显式请求时生成打包目标，避免常规开发配置产生多余副作用。
    set(CPACK_PACKAGE_NAME "NanoAI_NCNN")
    set(CPACK_PACKAGE_VENDOR "NanoAI")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_GENERATOR "TGZ;ZIP")
    include(CPack)
endif()

# ======================================================
# 示例构建（可选）
# 示例用于验证完整视觉链路，默认关闭以减少首次接入时的依赖面。
# ======================================================
if(NANOAI_NCNN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
