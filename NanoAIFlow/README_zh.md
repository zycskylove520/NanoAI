
# NanoAIFlow

[English](README.md) | 中文


NanoAIFlow 是一个高性能、header-only、强类型并发 C++ 推理与数据处理框架。始终以可安装 CMake 包（通过 `find_package`）的形式交付，用户可选构建 examples 进行学习和测试。所有依赖均通过 INTERFACE 链接，集成简单可靠。

**核心范式：**
- 始终作为框架（库）安装
- 示例可选构建（通过 `-DNANOAIFLOW_BUILD_EXAMPLES=ON`）
- 所有依赖均 INTERFACE 链接

## 核心特性

1. **每个 Pipe 可独立并发**：每个阶段可声明并发度，多阶段可并行处理不同请求，最大化多核 CPU 吞吐。
2. **多阶段流水并发推进**：非 barrier 模型，不同请求可在不同阶段并行流动，适合 AI 典型流水线（Load、Preprocess、Infer、Postprocess）。
3. **高并发下顺序可控**：阶段间始终全局顺序放行，保证输出确定性。
4. **强类型静态管线**：输入输出类型由 `on_run(...)` 定义，管线类型编译期固定，无运行时类型分发。
5. **灵活执行策略**：每阶段可选 `shared_pool`、`dedicated_pool`、`inline_run`。
6. **Header-only，INTERFACE 链接**：集成简单，无需静态/动态库，所有依赖 INTERFACE 链接。
7. **可安装可复用**：始终以 CMake 包安装，`find_package(NanoAIFlow CONFIG REQUIRED)` 复用。
8. **丰富工程测试**：并发、顺序、性能测试齐全，支持 tuple 自动展开与参数转发。

## 典型场景

- 边缘 AI 流水线（CV、NLP、语音）
- 固定结构、高吞吐生产链路
- 多核并发且需顺序确定的系统

## 设计原则

- 静态类型优先：类型错误前移到编译期
- 阶段并发：每阶段独立控制并发与策略
- 顺序放行：高并发下行为稳定可解释
- 全局配额：避免任务堆积

详细设计见：
- [docs/nanoai_design_philosophy.md](docs/nanoai_design_philosophy.md)
- [docs/pipe_usage_guide.md](docs/pipe_usage_guide.md)


## 仓库结构

- `include`：对外头文件
- `3rdparty/thread-pool`：线程池依赖
- `examples`：可选示例程序（通过 `-DNANOAIFLOW_BUILD_EXAMPLES=ON` 构建）
- `test`：单元、并发、性能测试
- `docs`：设计与使用文档


## 快速开始

### 1. 构建并安装框架（推荐）

```bash
cmake -S . -B build
cmake --build build -j
cmake --install build --prefix /your/install/prefix
```

### 2. 可选构建示例

```bash
cmake -S . -B build_example -DNANOAIFLOW_BUILD_EXAMPLES=ON
cmake --build build_example -j
```

### 3. 可选构建测试

```bash
cmake -S . -B build_test -DNANOAIFLOW_BUILD_TESTS=ON
cmake --build build_test -j
ctest --test-dir build_test --output-on-failure
```


## CMake 选项

- `NANOAIFLOW_BUILD_EXAMPLES`：是否构建示例（可选，默认 `OFF`）
- `NANOAIFLOW_BUILD_TESTS`：是否构建测试（可选，默认 `OFF`）
- `NANOAIFLOW_ENABLE_CPACK`：是否启用 CPack 打包（可选，默认 `OFF`）


## 调用方示例

```cmake
find_package(NanoAIFlow CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE NanoAI::Flow)
```


若安装前缀不在系统默认路径，可在配置时指定：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/your/install/prefix
```


## 文档导航

- [英文版 README](README.md)
- [打包与 find_package 指南](docs/find_package_and_packaging_guide.md)
- [设计理念](docs/nanoai_design_philosophy.md)
- [Pipe 使用说明](docs/pipe_usage_guide.md)


## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。

NanoAIFlow 是一个专为高性能 C++ 推理与数据处理场景设计的极致并发流程编排框架，采用 header-only 设计，基于 C++20，兼具类型安全、灵活并发与工程可复用性。

---

## 全面优势与核心特性

### 1. 极致多线程并发与资源利用
- **每个 Pipe 阶段可独立配置多线程并发**，充分利用多核资源，极大提升吞吐量。
- **多个 Pipe 可同时并发推进**，整体并发能力远超传统串行或单阶段并发模型。
- 支持共享线程池、专用线程池、内联执行三种策略，灵活适配不同业务对资源隔离和性能的需求。

### 2. 顺序可控的高并发流水线
- 阶段内部高并发，阶段间严格顺序推进，**保证输出行为稳定、可预测**。
- 适合对顺序敏感的推理/数据流场景，无需业务层额外乱序补偿。

### 3. 强类型静态管线，类型安全
- 所有 Pipe 输入输出类型在编译期确定，**避免运行时类型错误**。
- 支持 tuple 自动解包/打包，业务代码聚焦于核心逻辑。

### 4. 零依赖、Header-only、易集成
- 仅需头文件即可集成，CMake 包导出，**适合大规模工程复用与二次开发**。

### 5. 高性能基准与丰富测试
- 内置多种并发性能基准和单元测试，便于评估和验证在不同策略下的表现。

### 6. 适用场景广泛
- 特别适合 CV/NLP/语音等端侧推理前后处理的高并发流水线。

---

## 设计理念与并发模型

- 静态类型优先：所有管线结构、Pipe 输入输出均在编译期确定，类型安全、零运行时开销。
- 阶段化并发：每个 Pipe 阶段可独立配置并发度，支持全局配额与阶段隔离。
- 顺序放行：高并发下依然保证阶段间顺序一致性，输出行为可解释。
- 策略可配置：支持 shared_pool/dedicated_pool/inline_run 等多种执行策略，满足不同场景需求。

详细设计思想见 [docs/nanoai_design_philosophy.md](docs/nanoai_design_philosophy.md)。

---

## 快速开始

```bash
# 1. 构建
cmake -S . -B build
cmake --build build -j

# 2. 启用测试
cmake -S . -B build_test -DNANOAIFLOW_BUILD_TESTS=ON
cmake --build build_test -j
ctest --test-dir build_test --output-on-failure

# 3. 启用示例
cmake -S . -B build_example -DNANOAIFLOW_BUILD_EXAMPLES=ON
cmake --build build_example -j

# 4. 安装为可复用 CMake 包
cmake -S . -B build_install -DCMAKE_INSTALL_PREFIX=/your/install/prefix
cmake --build build_install -j
cmake --install build_install
```

---

## CMake 选项

- NANOAIFLOW_BUILD_TESTS: 是否构建 test 目录下测试，默认 OFF
- NANOAIFLOW_BUILD_EXAMPLES: 是否构建 examples 示例，默认 OFF
- NANOAIFLOW_ENABLE_CPACK: 是否启用 CPack 打包，默认 OFF

---

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

---

## 文档导航

- [打包与 find_package 指南](docs/find_package_and_packaging_guide.md)
- [设计理念](docs/nanoai_design_philosophy.md)
- [Pipe 使用说明](docs/pipe_usage_guide.md)

---

## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。

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
