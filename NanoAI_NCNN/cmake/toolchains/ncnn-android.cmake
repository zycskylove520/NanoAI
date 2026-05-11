# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: ncnn-android.cmake
# Brief: CMake script for NanoAI module build, dependency wiring, and install behavior.
#
# Notes:
# - Keep platform/toolchain assumptions explicit in comments when modifying this file.
# - Keep third-party dependency comments aligned with version/source changes.

# Toolchain wrapper for NanoAI_NCNN Android cross compile.
set(ANDROID_ABI "arm64-v8a" CACHE STRING "Android ABI")
set(ANDROID_PLATFORM "android-26" CACHE STRING "Android API")
set(ANDROID_ARM_NEON ON CACHE BOOL "Android ARM NEON")
set(ANDROID_USE_LIBCXX ON CACHE BOOL "Android libc++")
set(ANDROID_STL "c++_static" CACHE STRING "Android STL")
