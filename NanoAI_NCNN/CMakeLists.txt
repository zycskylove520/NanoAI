cmake_minimum_required(VERSION 3.16)

## =====================================================
# Linux x86_64 交叉编译预设
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
# 框架始终 install，examples 可选构建，所有依赖 INTERFACE 链接。
## =====================================================
option(NANOAI_NCNN_BUILD_EXAMPLES "Build NanoAI_NCNN examples (optional)" OFF)
set(NANOAI_NCNN_EXAMPLES "ALL" CACHE STRING "Semicolon-separated example names under examples/ (ALL or example names)")

message(STATUS "NanoAI_NCNN build examples: ${NANOAI_NCNN_BUILD_EXAMPLES}")
message(STATUS "NanoAI_NCNN selected examples: ${NANOAI_NCNN_EXAMPLES}")

# ======================================================
# 路径配置
# ======================================================
get_filename_component(NANOAI_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)

set(NANOAIFLOW_ROOT "" CACHE PATH "NanoAIFlow install prefix (platform-agnostic, optional)")

if(NANOAIFLOW_ROOT)
    # 让 find_package() 可以优先从指定安装目录查找 NanoAIFlow。
    list(APPEND CMAKE_PREFIX_PATH "${NANOAIFLOW_ROOT}")
endif()

if(NANOAIFLOW_ROOT AND (NOT DEFINED NanoAIFlow_DIR OR NanoAIFlow_DIR STREQUAL ""))
    set(NanoAIFlow_DIR "${NANOAIFLOW_ROOT}/lib/cmake/NanoAIFlow" CACHE PATH "NanoAIFlow package config directory" FORCE)
endif()

set(NANOAI_NCNN_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/NanoAI_NCNN" CACHE STRING "Install path for NanoAI_NCNN CMake package files")

# ======================================================
# 内容配置
# ======================================================
set(NANOAI_NCNN_PUBLIC_INCLUDE_DIR ${NANOAI_PROJECT_ROOT}/include)  # NanoAI_NCNN 公开头文件目录
set(NANOAI_NCNN_INCLUDE_DIRS ${NANOAI_NCNN_PUBLIC_INCLUDE_DIR})  # NanoAI_NCNN 头文件搜索目录列表，可能会在第三方库依赖中追加
set(NANOAI_NCNN_LINK_DIRS)  # NanoAI_NCNN 链接库搜索目录列表，可能会在第三方库依赖中追加
set(NANOAI_NCNN_LINK_LIBS)  # NanoAI_NCNN 链接库列表，可能会在第三方库依赖中追加

# ======================================================
# 依赖发现
# ======================================================

# threads 依赖
set(_NANOAI_NCNN_NEED_THREADS ON)
find_package(Threads REQUIRED)

# NanoAIFlow 依赖
set(_NANOAI_NCNN_NEED_FLOW ON)
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
# ======================================================
set(NANOAI_NCNN_BASE_LIBS
    dl
    m
    stdc++
)

if(_NANOAI_NCNN_NEED_THREADS)
    list(APPEND NANOAI_NCNN_BASE_LIBS Threads::Threads)
endif()

if(_NANOAI_NCNN_NEED_FLOW)
    list(APPEND NANOAI_NCNN_BASE_LIBS NanoAI::Flow)
endif()

# ======================================================
# 目标定义
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
# 包导出与安装（始终可用）
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

# ======================================================
# 示例构建（可选）
# ======================================================
if(NANOAI_NCNN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
