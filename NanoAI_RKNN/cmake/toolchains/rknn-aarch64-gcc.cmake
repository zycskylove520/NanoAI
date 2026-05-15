# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: rknn-aarch64-gcc.cmake
# Brief: 为 RKNN Linux aarch64 交叉编译提供最小 GCC 工具链定义。
#
# Notes:
# - 该工具链文件只声明目标系统与编译器，不擅自覆盖 sysroot、find root path 等高级策略。
# - 若部署环境切换到自定义交叉工具链，应优先复制本文件后按需扩展，而非在工程脚本里散落设置。

# Toolchain for RKNN aarch64 cross compile.
# 显式固定为 Linux/aarch64，确保 CMake 后续包发现和条件分支走到目标平台逻辑。

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
