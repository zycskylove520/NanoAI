
# NanoAIFlow

[English](README.md) | 中文

NanoAIFlow 是一个专为高性能 C++ 推理与数据处理场景设计的强类型并发流程编排框架。它不是运行时随意拼接节点的工作流系统，而是面向固定业务链路、强调吞吐、顺序语义和类型安全的工程化流水线基础设施。

## 核心优势

### 1. 每个 Pipe 都能独立并发
- 每个 Pipe 阶段都可单独配置最大并发度
- 同一条 Pipeline 中，多个 Pipe 可同时处理不同序号的数据
- 相比单线程串行链路或只有整体线程池的模型，更容易压榨多核 CPU 吞吐

### 2. 多个 Pipe 一起并发，更适合高吞吐推理链路
- Pipeline 不是“前一阶段全部做完，后一阶段再开始”
- 一次 `run(...)` 会按阶段推进，而不同请求会在不同阶段并行流动
- 这非常适合 Load、Preprocess、Infer、Postprocess 一类典型 AI 推理流水线

### 3. 高并发下仍然保证顺序语义稳定
- 阶段内部允许并发执行
- 阶段之间按全局序号顺序放行
- 这样既保留高吞吐，又避免下游出现不可控乱序行为

### 4. 强类型静态管线，避免运行时类型错误
- Pipe 的输入输出由 `on_run(...)` 签名直接定义
- `NanoPipeLine<P1, P2, ...>` 在编译期确定类型流转
- 不依赖 `std::any` 一类动态分发路径，减少运行时不确定性与维护成本

### 5. 灵活执行策略，兼顾吞吐与隔离
- `shared_pool`：适合大多数常规阶段，共享线程资源
- `dedicated_pool`：适合关键阶段资源隔离，避免互相干扰
- `inline_run`：适合极轻量逻辑，减少调度开销

### 6. Header-only 与标准 CMake 包导出
- 引入成本低，便于集成到已有工程
- 支持安装后通过 `find_package(NanoAIFlow CONFIG REQUIRED)` 复用
- 适合 monorepo、组件化工程和第三方发布场景

### 7. 为真实工程而设计
- 提供并发正确性测试、顺序一致性测试、性能基准测试
- 支持 tuple 自动展开，适合复杂阶段间数据流转
- 适合作为 RKNN、NCNN、MNN 等推理模块的流程基础设施

## 适用场景

- CV、NLP、语音等端侧推理前后处理流水线
- 固定结构、高吞吐、低容错的工业链路
- 需要严格控制顺序语义，同时又要发挥多核并发能力的业务系统

## 设计要点

- 静态类型优先：把类型问题尽量前移到编译期
- 阶段化并发：每个阶段独立控制并发与执行策略
- 顺序放行：高并发下保持输出行为稳定、可解释
- 全局配额控制：避免整体任务失控堆积

详细设计可参考：

- [docs/nanoai_design_philosophy.md](docs/nanoai_design_philosophy.md)
- [docs/pipe_usage_guide.md](docs/pipe_usage_guide.md)

## 仓库结构

- `include`：对外头文件
- `3rdparty/thread-pool`：线程池依赖
- `examples`：示例程序
- `test`：单元测试、并发测试、性能测试
- `docs`：设计与使用文档

## 快速开始

### 1. 仅构建库

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

## CMake 选项

- `NANOAIFLOW_BUILD_TESTS`：是否构建测试，默认 `OFF`
- `NANOAIFLOW_BUILD_EXAMPLES`：是否构建示例，默认 `OFF`
- `NANOAIFLOW_ENABLE_CPACK`：是否启用 CPack 打包，默认 `OFF`

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
