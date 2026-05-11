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

# This subproject must be built from NanoAI_NCNN root.
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
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${NANOAI_NCNN_CAR_TARGET}
    PRIVATE
    ${NANOAI_NCNN_TARGET}
)

