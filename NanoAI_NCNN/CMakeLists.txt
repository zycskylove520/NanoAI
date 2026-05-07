cmake_minimum_required(VERSION 3.16)

# ======================================================
# Linux aarch64 cross-compile preset (aligned with RKNN)
# ======================================================
option(NANOAI_NCNN_LINUX_AARCH64_PRESET "Enable Linux aarch64 GCC cross-compile preset" OFF)
set(NANOAI_NCNN_LINUX_AARCH64_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_LIST_DIR}/cmake/toolchains/ncnn-aarch64-gcc.cmake"
    CACHE FILEPATH "Default NanoAI_NCNN Linux aarch64 toolchain file"
)

# ======================================================
# Android cross-compile preset (aligned with deploy car_project)
# ======================================================
option(NANOAI_NCNN_ANDROID_PRESET "Enable Android cross-compile preset" OFF)
set(NANOAI_NCNN_ANDROID_NDK_PATH "" CACHE PATH "Android NDK root path. If empty, ANDROID_NDK_HOME or ANDROID_NDK_ROOT will be used")
set(NANOAI_NCNN_ANDROID_ABI "arm64-v8a" CACHE STRING "Android ABI")
set(NANOAI_NCNN_ANDROID_PLATFORM "android-26" CACHE STRING "Android API level")
set(NANOAI_NCNN_ANDROID_ARM_NEON ON CACHE BOOL "Enable Android ARM NEON")
set(NANOAI_NCNN_ANDROID_USE_LIBCXX ON CACHE BOOL "Use Android libc++")
set(NANOAI_NCNN_ANDROID_STL "c++_static" CACHE STRING "Android STL")

if(NANOAI_NCNN_LINUX_AARCH64_PRESET AND NANOAI_NCNN_ANDROID_PRESET)
    message(FATAL_ERROR "NANOAI_NCNN_LINUX_AARCH64_PRESET and NANOAI_NCNN_ANDROID_PRESET cannot both be ON.")
endif()

if(NANOAI_NCNN_LINUX_AARCH64_PRESET)
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
        set(CMAKE_TOOLCHAIN_FILE
            "${NANOAI_NCNN_LINUX_AARCH64_TOOLCHAIN_FILE}"
            CACHE FILEPATH "NanoAI_NCNN Linux aarch64 toolchain file" FORCE
        )
    endif()

    message(STATUS "NanoAI_NCNN Linux aarch64 preset enabled")
    message(STATUS "  CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()

if(NANOAI_NCNN_ANDROID_PRESET)
    set(ANDROID ON CACHE BOOL "Build for Android" FORCE)

    set(_NANOAI_NCNN_ANDROID_NDK "${NANOAI_NCNN_ANDROID_NDK_PATH}")
    if(_NANOAI_NCNN_ANDROID_NDK STREQUAL "")
        if(DEFINED ENV{ANDROID_NDK_HOME} AND NOT "$ENV{ANDROID_NDK_HOME}" STREQUAL "")
            set(_NANOAI_NCNN_ANDROID_NDK "$ENV{ANDROID_NDK_HOME}")
        elseif(DEFINED ENV{ANDROID_NDK_ROOT} AND NOT "$ENV{ANDROID_NDK_ROOT}" STREQUAL "")
            set(_NANOAI_NCNN_ANDROID_NDK "$ENV{ANDROID_NDK_ROOT}")
        else()
            message(FATAL_ERROR
                "NANOAI_NCNN_ANDROID_PRESET=ON requires Android NDK. "
                "Set -DNANOAI_NCNN_ANDROID_NDK_PATH=/path/to/ndk or export ANDROID_NDK_HOME/ANDROID_NDK_ROOT."
            )
        endif()
    endif()

    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
        set(CMAKE_TOOLCHAIN_FILE
            "${_NANOAI_NCNN_ANDROID_NDK}/build/cmake/android.toolchain.cmake"
            CACHE FILEPATH "Android toolchain file" FORCE
        )
    endif()

    set(ANDROID_ABI "${NANOAI_NCNN_ANDROID_ABI}" CACHE STRING "Android ABI" FORCE)
    set(ANDROID_PLATFORM "${NANOAI_NCNN_ANDROID_PLATFORM}" CACHE STRING "Android API" FORCE)
    set(ANDROID_ARM_NEON ${NANOAI_NCNN_ANDROID_ARM_NEON} CACHE BOOL "Android ARM NEON" FORCE)
    set(ANDROID_USE_LIBCXX ${NANOAI_NCNN_ANDROID_USE_LIBCXX} CACHE BOOL "Android libc++" FORCE)
    set(ANDROID_STL "${NANOAI_NCNN_ANDROID_STL}" CACHE STRING "Android STL" FORCE)

    message(STATUS "NanoAI_NCNN Android preset enabled")
    message(STATUS "  CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
    message(STATUS "  ANDROID_ABI=${ANDROID_ABI}")
    message(STATUS "  ANDROID_PLATFORM=${ANDROID_PLATFORM}")
endif()

project(NanoAI_NCNN VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/NanoAINCNNCommon.cmake)
nanoai_setup_global_options()

# ======================================================
# Build mode
# ======================================================
set(NANOAI_NCNN_BUILD_MODE "PACKAGE" CACHE STRING "Build mode: PACKAGE or PROJECTS")
set_property(CACHE NANOAI_NCNN_BUILD_MODE PROPERTY STRINGS PACKAGE PROJECTS)

string(TOUPPER "${NANOAI_NCNN_BUILD_MODE}" NANOAI_NCNN_BUILD_MODE)
if(NOT NANOAI_NCNN_BUILD_MODE STREQUAL "PACKAGE" AND NOT NANOAI_NCNN_BUILD_MODE STREQUAL "PROJECTS")
    message(FATAL_ERROR "Invalid NANOAI_NCNN_BUILD_MODE='${NANOAI_NCNN_BUILD_MODE}'. Expected PACKAGE or PROJECTS.")
endif()

message(STATUS "NanoAI_NCNN build mode: ${NANOAI_NCNN_BUILD_MODE}")

# ======================================================
# Paths
# ======================================================
get_filename_component(NANOAI_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)

set(NANOAIFLOW_ROOT "" CACHE PATH "NanoAIFlow install prefix (platform-agnostic, optional)")

# Backward compatibility for old variable names.
set(NANOAIFLOW_ROOT_HOST "" CACHE PATH "[Deprecated] NanoAIFlow host install prefix")
set(NANOAIFLOW_ROOT_ANDROID "" CACHE PATH "[Deprecated] NanoAIFlow Android install prefix")

if(NOT NANOAIFLOW_ROOT)
    if(NANOAIFLOW_ROOT_HOST)
        set(NANOAIFLOW_ROOT "${NANOAIFLOW_ROOT_HOST}")
    elseif(NANOAIFLOW_ROOT_ANDROID)
        set(NANOAIFLOW_ROOT "${NANOAIFLOW_ROOT_ANDROID}")
    endif()
endif()

if(NANOAIFLOW_ROOT)
    list(APPEND CMAKE_PREFIX_PATH "${NANOAIFLOW_ROOT}")
endif()

if(NANOAIFLOW_ROOT AND (NOT DEFINED NanoAIFlow_DIR OR NanoAIFlow_DIR STREQUAL ""))
    set(NanoAIFlow_DIR "${NANOAIFLOW_ROOT}/lib/cmake/NanoAIFlow" CACHE PATH "NanoAIFlow package config directory" FORCE)
endif()

set(NANOAI_NCNN_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/NanoAI_NCNN" CACHE STRING "Install path for NanoAI_NCNN CMake package files")

# ======================================================
# Content config
# ======================================================
set(NANOAI_NCNN_PUBLIC_INCLUDE_DIR ${NANOAI_PROJECT_ROOT}/include)
set(NANOAI_NCNN_INCLUDE_DIRS ${NANOAI_NCNN_PUBLIC_INCLUDE_DIR})
set(NANOAI_NCNN_LINK_DIRS)
set(NANOAI_NCNN_LINK_LIBS)
set(NANOAI_NCNN_SRCS)

# ======================================================
# Dependency discovery
# ======================================================
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(_NANOAI_NCNN_NEED_FLOW ON)
if(NANOAI_NCNN_BUILD_MODE STREQUAL "PROJECTS" AND DEFINED NANOAI_NCNN_PROJECTS AND NANOAI_NCNN_PROJECTS STREQUAL "NONE")
    set(_NANOAI_NCNN_NEED_FLOW OFF)
endif()

if(_NANOAI_NCNN_NEED_FLOW)
    find_package(NanoAIFlow CONFIG REQUIRED)
    if(NOT TARGET NanoAI::Flow)
        message(FATAL_ERROR
            "NanoAIFlow found but target NanoAI::Flow is missing. "
            "Please check your NanoAIFlow package installation."
        )
    endif()
else()
    message(STATUS "NanoAIFlow discovery skipped because NANOAI_NCNN_PROJECTS=NONE")
endif()

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
# Base link libraries
# ======================================================
set(NANOAI_NCNN_BASE_LIBS
    NanoAI::Flow
    Threads::Threads
    dl
    m
    stdc++
)

# ======================================================
# Target definition
# ======================================================
if(NANOAI_NCNN_SRCS)
    add_library(NanoAI_NCNN SHARED
        ${NANOAI_NCNN_SRCS}
    )

    target_compile_definitions(NanoAI_NCNN
        PUBLIC
        NANOAI_NCNN_WITH_NCNN_RUNTIME=1
    )

    target_include_directories(NanoAI_NCNN
        PUBLIC
        ${NANOAI_NCNN_BUILD_INTERFACE_INCLUDE_DIRS}
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    target_link_directories(NanoAI_NCNN
        PUBLIC
        ${NANOAI_NCNN_BUILD_INTERFACE_LINK_DIRS}
    )

    target_link_libraries(NanoAI_NCNN
        PUBLIC
        ${NANOAI_NCNN_BASE_LIBS}
        ${NANOAI_NCNN_LINK_LIBS}
    )
else()
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
endif()

add_library(NanoAI_NCNN::NanoAI_NCNN ALIAS NanoAI_NCNN)

# ======================================================
# Package export and install
# ======================================================
if(NANOAI_NCNN_BUILD_MODE STREQUAL "PACKAGE")
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
endif()

# ======================================================
# Projects mode
# ======================================================
if(NANOAI_NCNN_BUILD_MODE STREQUAL "PROJECTS")
    add_subdirectory(projects)
endif()
