# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: ncnn-linux-x86_64-gcc.cmake
# Brief: CMake script for NanoAI module build, dependency wiring, and install behavior.
#
# Notes:
# - Keep platform/toolchain assumptions explicit in comments when modifying this file.
# - Keep third-party dependency comments aligned with version/source changes.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# set(CMAKE_C_COMPILER /usr/bin/x86_64-linux-gnu-gcc)
# set(CMAKE_CXX_COMPILER /usr/bin/x86_64-linux-gnu-g++)