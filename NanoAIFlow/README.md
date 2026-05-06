# NanoAIFlow

NanoAIFlow 是一个面向 C++ 推理与数据处理场景的轻量级流程编排组件，基于 C++20 构建，当前以头文件库形式提供。

项目目标：

- 提供简洁的 Pipe/Pipeline 组合能力，便于搭建可复用处理链。
- 提供标准 CMake 包导出，方便在外部工程通过 find_package 复用。
- 保持低接入成本，适合作为上层 AI/推理模块的流程基础设施。

## 主要特性

- Header-only 设计，接入简单。
- 基于 C++20，类型推导与模板能力更完整。
- 支持并发相关测试场景，便于验证高并发稳定性。
- 提供示例与测试目标，便于快速验证与性能评估。

## 目录说明

- include: NanoAIFlow 对外头文件
- 3rdparty/thread-pool: 线程池头文件依赖
- examples: 示例程序
- test: 单元与并发相关测试
- docs: 设计与使用文档

## 快速开始

### 1. 仅构建库配置（默认）

```bash
cmake -S . -B build
cmake --build build -j
```

### 2. 启用测试

```bash
cmake -S . -B build_test \
  -DNANOAIFLOW_BUILD_TESTS=ON

cmake --build build_test -j
ctest --test-dir build_test --output-on-failure
```

### 3. 启用示例

```bash
cmake -S . -B build_example \
  -DNANOAIFLOW_BUILD_EXAMPLES=ON

cmake --build build_example -j
```

### 4. 安装为可复用 CMake 包

```bash
cmake -S . -B build_install \
  -DCMAKE_INSTALL_PREFIX=/your/install/prefix

cmake --build build_install -j
cmake --install build_install
```

安装后会导出 NanoAIFlowConfig.cmake 等文件，供调用方通过 find_package 查找。

## CMake 选项

- NANOAIFLOW_BUILD_TESTS: 是否构建 test 目录下测试，默认 OFF
- NANOAIFLOW_BUILD_EXAMPLES: 是否构建 examples 示例，默认 OFF
- NANOAIFLOW_ENABLE_CPACK: 是否启用 CPack 打包，默认 OFF

## 调用方使用示例

在调用方 CMakeLists.txt 中：

```cmake
find_package(NanoAIFlow CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE NanoAI::Flow)
```

若安装前缀不在系统默认路径，可在调用方配置时指定：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/your/install/prefix
```

## 文档导航

- 打包与 find_package 指南: docs/find_package_and_packaging_guide.md
- 设计理念: docs/nanoai_design_philosophy.md
- Pipe 使用说明: docs/pipe_usage_guide.md

## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。
