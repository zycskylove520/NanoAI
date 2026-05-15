# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: ncnn-linux-x86_64-gcc.cmake
# Brief: 提供 NanoAI_NCNN 面向 Linux x86_64 的最小 toolchain 预设包装。
#
# Notes:
# - 该文件只声明目标系统与架构，不强绑编译器路径，方便在不同主机环境下复用。
# - 如果需要真正交叉到非本机 x86_64 工具链，可在外部补充编译器、sysroot 与查找路径策略。

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 保留注释形式的编译器示例，提醒调用方可在必要时显式固定 gcc/g++ 路径。
# set(CMAKE_C_COMPILER /usr/bin/x86_64-linux-gnu-gcc)
# set(CMAKE_CXX_COMPILER /usr/bin/x86_64-linux-gnu-g++)