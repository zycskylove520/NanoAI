# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: ncnn-android.cmake
# Brief: 为 NanoAI_NCNN 的 Android 构建补充 ABI、API 与 STL 等常用缓存参数。
#
# Notes:
# - 该文件不是完整 Android toolchain；它假定上层已经提供 Android NDK 的 `CMAKE_TOOLCHAIN_FILE`。
# - 这里设置的缓存值偏向 arm64-v8a + libc++ 静态链接的常见端侧部署默认组合。

# 仅补齐 Android 侧常见默认参数，真正的 NDK toolchain 入口仍由顶层显式指定。
set(ANDROID_ABI "arm64-v8a" CACHE STRING "Android ABI")
set(ANDROID_PLATFORM "android-26" CACHE STRING "Android API")
set(ANDROID_ARM_NEON ON CACHE BOOL "Android ARM NEON")
set(ANDROID_USE_LIBCXX ON CACHE BOOL "Android libc++")
# 选择 `c++_static` 可减少目标设备上对共享 STL 运行库分发的额外要求。
set(ANDROID_STL "c++_static" CACHE STRING "Android STL")
