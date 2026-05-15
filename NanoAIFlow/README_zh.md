
# NanoAIFlow

[English](README.md) | 中文

NanoAIFlow 是一个高性能、header-only、强类型并发 C++ 推理与数据处理框架。当前版本基于 C++20 标准并发原语实现运行时调度，不再依赖第三方线程池库，并保持可安装、可复用的 CMake 包交付方式。

**核心范式：**
- 始终作为框架（库）安装
- 示例可选构建（通过 `-DNANOAIFLOW_BUILD_EXAMPLES=ON`）
- 所有依赖均 INTERFACE 链接

## 核心特性

说明：以下特性列表既描述“能做什么”，也隐含该框架的工程边界——它更偏向固定拓扑、高吞吐、可观测的生产型流水线，而不是运行时任意拼装节点的 DAG 系统。

1. **每个 Pipe 可独立并发**：每个阶段可声明并发度，多阶段可并行处理不同请求，最大化多核 CPU 吞吐。
2. **多阶段流水并发推进**：非 barrier 模型，不同请求可在不同阶段并行流动，适合 AI 典型流水线（Load、Preprocess、Infer、Postprocess）。
3. **高并发下顺序可控**：阶段间始终全局顺序放行，保证输出确定性。
4. **强类型静态管线**：输入输出类型由 `on_run(...)` 定义，管线类型编译期固定，无运行时类型分发。
5. **灵活执行策略**：每阶段可选 `shared_pool`、`dedicated_pool`、`inline_run`。
6. **管线值语义友好**：`NanoPipeLine` 支持拷贝构造与移动构造，便于按值传递与工厂返回。
7. **C++20 原生线程并发实现**：当前运行时基于标准库同步原语实现，无第三方线程池依赖。
8. **Header-only，INTERFACE 链接**：集成简单，无需静态/动态库，所有依赖 INTERFACE 链接。
9. **可安装可复用**：始终以 CMake 包安装，`find_package(NanoAIFlow CONFIG REQUIRED)` 复用。
10. **背压控制能力**：共享/专用线程池支持有界队列与提交策略（`block`/`timeout`/`reject`），防止任务无限堆积。
11. **可观测能力**：提供运行时统计与事件回调，可追踪拒绝、超时、ordered waiters 溢出等关键问题。
12. **丰富工程测试**：并发、顺序、性能、背压与可观测测试齐全，支持 tuple 自动展开与参数转发。

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
- `include/nanoai_flow/core/thread_pool.hpp`：C++20 标准库线程运行时
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

## 性能快照

说明：这些数据主要用于展示版本演进方向与默认配置下的大致性能画像，不应直接替代业务现场 benchmark。实际选型时仍建议结合目标机器、输入分布与线程绑定策略重新测量。

以下数据来自同一台验证主机，对比对象为最初基于 `BS::thread_pool` 的实现与当前最终 C++20 运行时：

- `test_pool_performance/shared_pool_all`：`31746.03 -> 70796.46` QPS
- `test_pool_performance/dedicated_pool_all`：`100000.00 -> 140350.88` QPS
- `test_pool_performance/mixed_pool_shared_dedicated_shared`：`38834.95 -> 84210.53` QPS
- `perf_benchmark/shared_pool_all`：`31347.96 -> 73800.74` QPS
- `perf_benchmark/dedicated_pool_all`：`109289.62 -> 151515.15` QPS
- `perf_benchmark/mode_unordered`：`49751.24 -> 150375.94` QPS

具体数值会随硬件与负载变化，但当前最终版本在验证主机上稳定优于原始第三方线程池实现。

## 调用方示例

说明：由于 `NanoAIFlow` 是 header-only 的 `INTERFACE` 包，调用方通常只需 `find_package(...)` 并链接 `NanoAI::Flow`，无需关心额外二进制库部署。

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
