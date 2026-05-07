
# NanoAI

[English](README.md) | 中文

NanoAI 是一个面向高性能推理与数据处理的多模块 C++ 工程仓库，聚焦极致并发、类型安全、端侧部署与工程可复用性。仓库以 NanoAIFlow 作为统一流程基础设施，在此之上提供 RKNN 与 NCNN 两类推理模块，适合构建结构稳定、吞吐要求高、可长期维护的 AI 应用。

## 核心模块

### NanoAIFlow
- 面向推理前后处理与数据变换链路的高性能并发流程编排框架
- Header-only 设计，接入轻量
- 每个 Pipe 可独立配置并发度，多个 Pipe 可同时并发推进
- 强类型静态管线，顺序可控，适合高吞吐场景
- 详见 [NanoAIFlow/README.md](NanoAIFlow/README.md) 和 [NanoAIFlow/README_zh.md](NanoAIFlow/README_zh.md)

### NanoAI_RKNN
- 面向 Rockchip RKNN 部署场景的 C++ 推理组件
- 深度集成 NanoAIFlow，可将加载、预处理、推理、后处理纳入统一并发管线
- 支持标准 CMake 包导出与 ARM 端侧交叉编译
- 详见 [NanoAI_RKNN/README.md](NanoAI_RKNN/README.md) 和 [NanoAI_RKNN/README_zh.md](NanoAI_RKNN/README_zh.md)

### NanoAI_NCNN
- 面向 NCNN 端侧部署场景的 C++ 推理组件
- 支持 Linux aarch64 与 Android 等多平台部署
- 与 NanoAIFlow 协同构建高吞吐推理流水线
- 详见 [NanoAI_NCNN/README.md](NanoAI_NCNN/README.md) 和 [NanoAI_NCNN/README_zh.md](NanoAI_NCNN/README_zh.md)

## 推荐使用方式

1. 先单独构建并安装 NanoAIFlow。
2. 再构建 NanoAI_RKNN 或 NanoAI_NCNN，并通过 `CMAKE_PREFIX_PATH`、`NANOAIFLOW_ROOT` 或 `NanoAIFlow_DIR` 指向 Flow 安装目录。
3. 根据目标平台选择对应模块与交叉编译参数。

## Monorepo 构建入口

仓库根目录提供统一入口：

- [CMakeLists.txt](CMakeLists.txt)
- [CMakePresets.json](CMakePresets.json)

示例：

```bash
# 1. 配置并构建 NanoAIFlow
cmake --preset flow-package
cmake --build --preset flow-package

# 2. 配置并构建 RKNN
export NANOAIFLOW_ROOT=/path/to/NanoAIFlow/install
cmake --preset rknn-package
cmake --build --preset rknn-package
```

## 文档导航

- [NanoAIFlow 英文说明](NanoAIFlow/README.md)
- [NanoAIFlow 中文说明](NanoAIFlow/README_zh.md)
- [NanoAI_RKNN 英文说明](NanoAI_RKNN/README.md)
- [NanoAI_RKNN 中文说明](NanoAI_RKNN/README_zh.md)
- [NanoAI_NCNN 英文说明](NanoAI_NCNN/README.md)
- [NanoAI_NCNN 中文说明](NanoAI_NCNN/README_zh.md)

## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。

NanoAI 是一个面向高性能推理与数据处理的多模块 C++ 工程，聚焦极致并发、类型安全和工程可复用性。

## 核心模块与亮点

- **NanoAIFlow**：高性能并发流程编排框架（header-only，极致多线程并发、顺序可控、强类型静态管线，详见子模块 README）
- **NanoAI_RKNN**：RKNN 平台推理模块（高效集成 RKNN 推理能力，支持灵活管线与高性能部署）
- **NanoAI_NCNN**：NCNN 平台推理模块（高效集成 NCNN 推理能力，支持端侧多平台部署与高效 pipe 组合）

> 各模块详细优势、设计理念与用法请参见对应子模块 README。

---

## 安装与使用

1. 推荐先单独构建并安装 NanoAIFlow。
2. 构建 RKNN/NCNN 时通过 `CMAKE_PREFIX_PATH` 或 `NanoAIFlow_DIR` 指向 Flow 安装目录。
3. 具体构建命令、交叉编译和依赖配置详见各子模块文档。

---

## 文档导航

- [NanoAIFlow 说明与优势](NanoAIFlow/README.md)
- [NanoAI_RKNN 说明与优势](NanoAI_RKNN/README.md)
- [NanoAI_NCNN 说明与优势](NanoAI_NCNN/README.md)

---

## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。
---

## 子模块导航

- **NanoAIFlow**
  - [模块说明与快速命令](NanoAIFlow/README.md)
  - [打包与 find_package 指南](NanoAIFlow/docs/find_package_and_packaging_guide.md)
  - [设计理念](NanoAIFlow/docs/nanoai_design_philosophy.md)
  - [Pipe 使用说明](NanoAIFlow/docs/pipe_usage_guide.md)

- **NanoAI_RKNN**
  - [模块说明与快速命令](NanoAI_RKNN/README.md)
  - [打包与 find_package 指南](NanoAI_RKNN/docs/find_package_and_packaging_guide.md)

- **NanoAI_NCNN**
  - [模块说明与快速命令](NanoAI_NCNN/README.md)
  - [打包与 find_package 指南](NanoAI_NCNN/docs/find_package_and_packaging_guide.md)

---

## 单模块使用（推荐给外部开发者）

外部开发者可只下载目标模块目录（或单独仓库镜像）并按各模块 README 构建。

依赖关系：
- `NanoAI_RKNN` 依赖已安装的 `NanoAIFlow`
- `NanoAI_NCNN` 依赖已安装的 `NanoAIFlow`

典型流程：
1. 先构建并安装 `NanoAIFlow`
2. 构建 `NanoAI_RKNN` 或 `NanoAI_NCNN` 时，通过 `CMAKE_PREFIX_PATH` 或 `NanoAIFlow_DIR` 指向 Flow 安装目录

---

## Monorepo 统一构建入口

仓库根目录提供：
- 顶层聚合入口: [CMakeLists.txt](CMakeLists.txt)
- 标准预设: [CMakePresets.json](CMakePresets.json)

示例：
```bash
# 1) 配置并构建 NanoAIFlow
cmake --preset flow-package
cmake --build --preset flow-package

# 2) 配置并构建 RKNN（需先设置 NanoAIFlow 安装路径）
export NANOAIFLOW_ROOT=/path/to/NanoAIFlow/install
cmake --preset rknn-package
cmake --build --preset rknn-package
cmake --install build_install
```

安装后会导出 NanoAIFlowConfig.cmake 等文件，供调用方通过 find_package 查找。

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

- [打包与 find_package 指南](NanoAIFlow/docs/find_package_and_packaging_guide.md)
- [设计理念](NanoAIFlow/docs/nanoai_design_philosophy.md)
- [Pipe 使用说明](NanoAIFlow/docs/pipe_usage_guide.md)

---

## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。
