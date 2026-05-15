# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: CMakeLists.txt
# Brief: CMake script for NanoAI module build, dependency wiring, and install behavior.
#
# Notes:
# - Keep platform/toolchain assumptions explicit in comments when modifying this file.
# - Keep third-party dependency comments aligned with version/source changes.

cmake_minimum_required(VERSION 3.16)

set(NANOAI_RKNN_DEFAULT_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_LIST_DIR}/cmake/toolchains/rknn-aarch64-gcc.cmake"
    CACHE FILEPATH "Default RKNN ARM cross toolchain"
)

option(NANOAI_RKNN_USE_DEFAULT_TOOLCHAIN "Auto apply default RKNN toolchain when CMAKE_TOOLCHAIN_FILE is not set" OFF)
if(NANOAI_RKNN_USE_DEFAULT_TOOLCHAIN AND (NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL ""))
    set(CMAKE_TOOLCHAIN_FILE
        "${NANOAI_RKNN_DEFAULT_TOOLCHAIN_FILE}"
        CACHE FILEPATH "Default RKNN ARM cross toolchain" FORCE
    )
endif()

project(NanoAI_RKNN VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/NanoAIRKNNCommon.cmake)
nanoai_setup_global_options()

# ======================================================
# 构建行为
# 始终导出/安装 NanoAI_RKNN 包；示例构建由开关控制。
# 这里将安装能力与示例能力拆开，便于交叉编译产物在 CI 中只做打包、不拉起示例。
# ======================================================
option(NANOAI_RKNN_BUILD_EXAMPLES "Build examples under examples/" OFF)
set(NANOAI_RKNN_EXAMPLES "ALL" CACHE STRING "Semicolon-separated example names under examples/ (ALL or example names)")
option(NANOAI_RKNN_ENABLE_CPACK "Enable CPack package generation (optional)" OFF)

message(STATUS "NanoAI_RKNN build examples: ${NANOAI_RKNN_BUILD_EXAMPLES}")
message(STATUS "NanoAI_RKNN selected examples: ${NANOAI_RKNN_EXAMPLES}")

# NanoAI_RKNN only supports ARM cross-compilation.
# 本地编辑器中的原生 configure 常用于补全与语法探测，因此仅提示而不直接报错。
if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "NanoAI_RKNN is designed for ARM cross-compilation. Native editor probe is ignored.")
endif()

if(CMAKE_CROSSCOMPILING AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    message(FATAL_ERROR "NanoAI_RKNN supports ARM platforms only. Current CMAKE_SYSTEM_PROCESSOR='${CMAKE_SYSTEM_PROCESSOR}'.")
endif()

# ======================================================
# 路径与依赖前缀
# 允许通过显式前缀把已安装的 NanoAIFlow 包注入 CMAKE_PREFIX_PATH，
# 避免把依赖路径硬编码到仓库内。
# ======================================================
get_filename_component(NANOAI_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)

set(NANOAIFLOW_ROOT "" CACHE PATH "NanoAIFlow install prefix (optional). If set, it will be appended to CMAKE_PREFIX_PATH")
if(NANOAIFLOW_ROOT)
    list(APPEND CMAKE_PREFIX_PATH "${NANOAIFLOW_ROOT}")
endif()

set(NANOAI_RKNN_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/NanoAI_RKNN" CACHE STRING "Install path for NanoAI_RKNN CMake package files")

# ======================================================
# 工程内容配置（先初始化，供 thirdparty 配置追加）
# thirdparty 子脚本通过 list(APPEND) 填充 include、link 与 source 列表，
# 因此这里必须先完成空列表初始化，避免缓存变量污染本次配置。
# ======================================================
set(NANOAI_RKNN_PUBLIC_INCLUDE_DIR ${NANOAI_PROJECT_ROOT}/include)
set(NANOAI_RKNN_INCLUDE_DIRS ${NANOAI_RKNN_PUBLIC_INCLUDE_DIR})
set(NANOAI_RKNN_LINK_DIRS)
set(NANOAI_RKNN_LINK_LIBS)
set(NANOAI_RKNN_SRCS)

# ======================================================
# 依赖发现
# 仅在交叉编译时严格查找 RKNN 相关依赖；原生探测路径保持最小依赖，方便 IDE 打开工程。
# ======================================================
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(CMAKE_CROSSCOMPILING)
    find_package(NanoAIFlow CONFIG REQUIRED)

    if(NOT TARGET NanoAI::Flow)
        message(FATAL_ERROR
            "NanoAIFlow found but target NanoAI::Flow is missing. "
            "Please check your NanoAIFlow package installation."
        )
    endif()

    include(${CMAKE_CURRENT_LIST_DIR}/cmake/NanoAIRKNNThirdParty.cmake)
else()
    message(STATUS "Native configure probe detected. Skip NanoAIFlow and third-party dependency discovery.")
endif()

set(NANOAI_RKNN_BUILD_INTERFACE_INCLUDE_DIRS)
foreach(_nanoai_rknn_inc IN LISTS NANOAI_RKNN_INCLUDE_DIRS)
    list(APPEND NANOAI_RKNN_BUILD_INTERFACE_INCLUDE_DIRS "$<BUILD_INTERFACE:${_nanoai_rknn_inc}>")
endforeach()

set(NANOAI_RKNN_BUILD_INTERFACE_LINK_DIRS)
foreach(_nanoai_rknn_link_dir IN LISTS NANOAI_RKNN_LINK_DIRS)
    list(APPEND NANOAI_RKNN_BUILD_INTERFACE_LINK_DIRS "$<BUILD_INTERFACE:${_nanoai_rknn_link_dir}>")
endforeach()

# ======================================================
# 基础链接库
# 将平台无关的线程库与交叉编译特有系统库拆开，便于后续扩展 host-only 测试目标。
# ======================================================
set(NANOAI_RKNN_BASE_LIBS
    Threads::Threads
)

if(CMAKE_CROSSCOMPILING)
    list(APPEND NANOAI_RKNN_BASE_LIBS
        NanoAI::Flow
        dl
        m
        stdc++
    )
endif()

# ======================================================
# 目标定义（examples 目录外，仅头文件发布）
# 当 thirdparty 没有追加源文件时退化为 INTERFACE 库，
# 这样纯头文件/预编译依赖场景仍能正常导出包配置。
# ======================================================
if(NANOAI_RKNN_SRCS)
    add_library(NanoAI_RKNN SHARED
        ${NANOAI_RKNN_SRCS}
    )

    target_compile_definitions(NanoAI_RKNN
        PUBLIC
        NANOAI_RKNN_WITH_RKNN_RUNTIME=1
    )

    target_include_directories(NanoAI_RKNN
        PUBLIC
        ${NANOAI_RKNN_BUILD_INTERFACE_INCLUDE_DIRS}
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    target_link_directories(NanoAI_RKNN
        PUBLIC
        ${NANOAI_RKNN_BUILD_INTERFACE_LINK_DIRS}
    )

    target_link_libraries(NanoAI_RKNN
        PUBLIC
        ${NANOAI_RKNN_BASE_LIBS}
        ${NANOAI_RKNN_LINK_LIBS}
    )
else()
    add_library(NanoAI_RKNN INTERFACE)

    target_compile_definitions(NanoAI_RKNN
        INTERFACE
        NANOAI_RKNN_WITH_RKNN_RUNTIME=1
    )

    target_include_directories(NanoAI_RKNN
        INTERFACE
        ${NANOAI_RKNN_BUILD_INTERFACE_INCLUDE_DIRS}
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    target_link_directories(NanoAI_RKNN
        INTERFACE
        ${NANOAI_RKNN_BUILD_INTERFACE_LINK_DIRS}
    )

    target_link_libraries(NanoAI_RKNN
        INTERFACE
        ${NANOAI_RKNN_BASE_LIBS}
        ${NANOAI_RKNN_LINK_LIBS}
    )
endif()

add_library(NanoAI_RKNN::NanoAI_RKNN ALIAS NanoAI_RKNN)

# ======================================================
# 包导出与安装（始终可用）
# 无论是否构建示例，都导出统一的 Config/Targets，保证下游 find_package 行为稳定。
# ======================================================
install(TARGETS NanoAI_RKNN
    EXPORT NanoAI_RKNNTargets
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAI_RKNNConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NanoAI_RKNNConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAI_RKNNConfig.cmake"
    INSTALL_DESTINATION ${NANOAI_RKNN_INSTALL_CMAKEDIR}
)

install(EXPORT NanoAI_RKNNTargets
    FILE NanoAI_RKNNTargets.cmake
    NAMESPACE NanoAI_RKNN::
    DESTINATION ${NANOAI_RKNN_INSTALL_CMAKEDIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAI_RKNNConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/NanoAI_RKNNConfigVersion.cmake"
    DESTINATION ${NANOAI_RKNN_INSTALL_CMAKEDIR}
)

if(NANOAI_RKNN_ENABLE_CPACK)
    # 仅在显式开启时引入 CPack，避免常规开发配置阶段生成多余打包目标。
    set(CPACK_PACKAGE_NAME "NanoAI_RKNN")
    set(CPACK_PACKAGE_VENDOR "NanoAI")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_GENERATOR "TGZ;ZIP")
    include(CPack)
endif()

# ======================================================
# 示例构建（可选）
# 示例依赖完整交叉编译运行时环境，默认关闭以减少首次接入门槛。
# ======================================================
if(NANOAI_RKNN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()