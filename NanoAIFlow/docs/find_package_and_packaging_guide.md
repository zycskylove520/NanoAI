
# NanoAIFlow 安装、find_package 与打包指南

本指南说明如何始终将 NanoAIFlow 安装为可复用 CMake 包，并在第三方项目中通过 find_package 引入。examples 可选构建，主库始终 install。当前版本安装产物仅包含 NanoAIFlow 自有头文件与 CMake 配置文件，不再携带第三方线程池头文件。

设计意图：把“框架构建”和“业务接入”严格分离，保证上游仓库只需暴露稳定的头文件与 CMake package 接口，而不会把示例、测试或历史第三方依赖泄漏给调用方。

## 1. 安装 NanoAIFlow（始终 install 框架）

在 NanoAIFlow 项目根目录执行：

    cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release
    cmake --build build_release -j
    cmake --install build_release --prefix /opt/nanoaiflow

如需构建 examples，可加 -DNANOAIFLOW_BUILD_EXAMPLES=ON

    cmake -S . -B build_example -DNANOAIFLOW_BUILD_EXAMPLES=ON
    cmake --build build_example -j

安装后关键文件位置：

- 头文件目录：/opt/nanoaiflow/include
- CMake 包目录：/opt/nanoaiflow/lib/cmake/NanoAIFlow
- 配置文件：/opt/nanoaiflow/lib/cmake/NanoAIFlow/NanoAIFlowConfig.cmake

线程运行时相关头文件位于：

- /opt/nanoaiflow/include/nanoai_flow/core/thread_pool.hpp

## 2. 第三方项目中使用 find_package

推荐优先使用这种集成方式，而不是直接把 `include/` 目录硬编码进业务工程。原因是 `find_package` 能同时传递版本、导出目标、未来可能新增的编译定义与安装布局信息，后期维护成本更低。

第三方项目 CMakeLists.txt 示例：

    cmake_minimum_required(VERSION 3.16)
    project(MyApp LANGUAGES CXX)

    find_package(NanoAIFlow CONFIG REQUIRED)

    add_executable(my_app main.cpp)
    target_link_libraries(my_app PRIVATE NanoAI::Flow)

配置第三方项目时，传入安装前缀：

    cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/nanoaiflow
    cmake --build build -j

## 3. 生成发布压缩包（CPack 可选）

NanoAIFlow 支持 CPack（变量 NANOAIFLOW_ENABLE_CPACK=ON）。

在 NanoAIFlow 根目录执行：

    cmake -S . -B build_pkg -DCMAKE_BUILD_TYPE=Release
    cmake --build build_pkg --target package

默认会在 build_pkg 目录下生成 .tar.gz/.zip 包。

如只需安装目录，可不执行 package 目标。

## 4. 版本说明

当框架 API、安装目录结构或导出目标发生兼容性变化时，应同步更新版本号，并让调用方通过版本约束显式感知升级风险。

当前 NanoAIFlow CMake 项目版本为 1.0.0。

当你修改版本号后，NanoAIFlowConfigVersion.cmake 会自动按新版本生成，第三方可用 find_package(NanoAIFlow 版本号 CONFIG REQUIRED) 进行版本约束。
