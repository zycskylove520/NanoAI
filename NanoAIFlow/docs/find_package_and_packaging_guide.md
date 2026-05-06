# NanoAIFlow 安装、find_package 与打包指南

本文档说明如何将 NanoAIFlow 安装为可复用 CMake 包，并在第三方项目中通过 find_package 引入。

## 1. 本地安装 NanoAIFlow

在 NanoAIFlow 项目根目录执行：

    cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release -DNANOAIFLOW_BUILD_TESTS=OFF -DNANOAIFLOW_BUILD_EXAMPLES=OFF
    cmake --build build_release -j
    cmake --install build_release --prefix /opt/nanoaiflow

安装后关键文件位置：

- 头文件目录：/opt/nanoaiflow/include
- CMake 包目录：/opt/nanoaiflow/lib/cmake/NanoAIFlow
- 配置文件：/opt/nanoaiflow/lib/cmake/NanoAIFlow/NanoAIFlowConfig.cmake

## 2. 第三方项目中使用 find_package

第三方项目 CMakeLists.txt 示例：

    cmake_minimum_required(VERSION 3.16)
    project(MyApp LANGUAGES CXX)

    find_package(NanoAIFlow CONFIG REQUIRED)

    add_executable(my_app main.cpp)
    target_link_libraries(my_app PRIVATE NanoAI_FLOW::Flow)

配置第三方项目时，传入安装前缀：

    cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/nanoaiflow
    cmake --build build -j

## 3. 生成发布压缩包（CPack）

NanoAIFlow 已启用 CPack（默认开启，变量 NANOAIFLOW_ENABLE_CPACK=ON）。

在 NanoAIFlow 根目录执行：

    cmake -S . -B build_pkg -DCMAKE_BUILD_TYPE=Release
    cmake --build build_pkg --target package

默认会在 build_pkg 目录下生成以下格式包：

- .tar.gz
- .zip

如果只需要安装目录，不需要压缩包，可不执行 package 目标。

## 4. 版本说明

当前 NanoAIFlow CMake 项目版本为 1.0.0。

当你修改版本号后，NanoAIFlowConfigVersion.cmake 会自动按新版本生成，第三方可用 find_package(NanoAIFlow 版本号 CONFIG REQUIRED) 进行版本约束。
