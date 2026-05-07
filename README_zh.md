
# NanoAI

[English](README.md) | 中文


NanoAI 是一个多模块高性能推理与数据处理 C++ 工程仓库。所有子模块均始终以可安装 CMake 包（通过 `find_package`）的形式交付，用户可选构建 examples 进行学习和测试。所有依赖均 INTERFACE 链接，构建/安装/示例范式在各子模块中完全统一。


## 核心模块

### NanoAIFlow
- 高性能、header-only 并发流程编排框架，适合推理/数据处理
- 始终以 CMake 包安装，examples 可选（`-DNANOAIFLOW_BUILD_EXAMPLES=ON`）
- 所有依赖 INTERFACE 链接
- 详见 [NanoAIFlow/README.md](NanoAIFlow/README.md) 和 [NanoAIFlow/README_zh.md](NanoAIFlow/README_zh.md)

### NanoAI_RKNN
- Rockchip RKNN 推理框架，深度集成 NanoAIFlow
- 始终以 CMake 包安装，examples 可选
- 所有依赖 INTERFACE 链接
- 详见 [NanoAI_RKNN/README.md](NanoAI_RKNN/README.md) 和 [NanoAI_RKNN/README_zh.md](NanoAI_RKNN/README_zh.md)

### NanoAI_NCNN
- NCNN 端侧推理框架，支持多平台（Linux aarch64、Android）
- 始终以 CMake 包安装，examples 可选
- 所有依赖 INTERFACE 链接
- 详见 [NanoAI_NCNN/README.md](NanoAI_NCNN/README.md) 和 [NanoAI_NCNN/README_zh.md](NanoAI_NCNN/README_zh.md)


## 推荐用法

1. 先构建并安装 NanoAIFlow（始终 install 框架）。
2. 再构建并安装 NanoAI_RKNN 或 NanoAI_NCNN，设置 `CMAKE_PREFIX_PATH`、`NANOAIFLOW_ROOT` 或 `NanoAIFlow_DIR` 指向 Flow 安装目录。
3. 如需示例，使用 `-D<模块>_BUILD_EXAMPLES=ON` 启用。
4. 所有依赖均 INTERFACE 链接，使用 CMake 包模式无需手动链接系统库。
5. 按需选择目标平台和 toolchain 配置。


## Monorepo 构建入口

仓库根目录提供统一入口：

- [CMakeLists.txt](CMakeLists.txt)
- [CMakePresets.json](CMakePresets.json)

示例：

```bash
# 1. 配置并构建 NanoAIFlow（install 框架）
cmake --preset flow-package
cmake --build --preset flow-package

# 2. 配置并构建 RKNN（install 框架，需设置 NanoAIFlow 路径）
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
