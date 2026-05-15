# NanoAI_RKNN

[English](README.md) | 中文

NanoAI_RKNN 是面向 Rockchip RKNN 端侧部署场景的高性能 C++ 推理组件。它在 NanoAIFlow 的强类型并发管线基础之上封装 RKNN 推理能力，适合构建从数据加载、预处理、模型推理到后处理的完整工程链路。

## 核心优势

### 1. 与 NanoAIFlow 深度协同
- 可将 Load、Preprocess、Infer、Postprocess 等阶段纳入统一并发管线
- 适合固定结构、高吞吐的端侧推理系统
- 便于将推理前后处理与模型执行统一管理

### 2. 面向 RKNN 端侧部署优化
- 聚焦 Rockchip 平台的 RKNN Runtime 集成
- 面向 `aarch64` 和 `arm64` 端侧部署流程设计
- 适合工业视觉、边缘 AI、嵌入式推理等场景

### 3. 工程集成方式灵活
- 始终支持导出/安装为可复用 CMake 包
- 可按需开启 examples 示例用于学习和集成验证
- 支持按示例选择性构建，减少不必要的编译开销

### 4. 标准 CMake 包导出
- 支持通过 `find_package` 接入上层工程
- 便于纳入现有 CMake 工程体系
- 适合组件化拆分、二次封装与持续交付

### 5. 第三方依赖路径可控
- RKNN Runtime、RGA、OpenCV、stb_image、jpeg_turbo、utils 等依赖均通过外部路径显式传入
- 更适合复杂交叉编译环境与企业内部依赖管理流程

## 仓库结构

- `include`：对外头文件
- `cmake`：构建脚本与第三方依赖配置
- `examples`：示例或应用项目入口
- `3rdparty`：第三方依赖目录
- `docs`：构建与打包文档

## 构建行为

1. 始终支持打包安装
  - 构建并安装为可通过 `find_package` 复用的第三方包
2. 示例按需构建
  - 通过 CMake 选项开启 `examples/` 下的应用或示例项目

这样拆分的目的是：在很多 RKNN 交叉编译流程里，主要诉求是产出可复用库包，而示例通常只在联调、验收或新成员接入时才需要开启。

## 核心配置项

- `NANOAI_RKNN_BUILD_EXAMPLES`：`ON` 或 `OFF`，默认 `OFF`
- `NANOAI_RKNN_EXAMPLES`：仅在 `NANOAI_RKNN_BUILD_EXAMPLES=ON` 时生效，默认 `ALL`
- `NANOAI_RKNN_INSTALL_CMAKEDIR`：包配置文件安装目录
- `NANOAIFLOW_ROOT`：NanoAIFlow 安装前缀

常见第三方依赖配置：

- `NANOAI_RKNN_RUNTIME_INCLUDE_DIR`
- `NANOAI_RKNN_RUNTIME_LIBRARY_DIR`
- `NANOAI_RKNN_RGA_ROOT`
- `NANOAI_RKNN_OPENCV_DIR`
- `NANOAI_RKNN_STB_IMAGE_INCLUDE_DIR`
- `NANOAI_RKNN_JPEG_TURBO_ROOT`
- `NANOAI_RKNN_UTILS_ROOT`

这些路径都应指向目标架构依赖。当前构建逻辑不会主动猜测主机环境中的 SDK 安装位置，以避免交叉编译时误用 x86 依赖。

## 快速开始

### 1. 构建并安装第三方包

先确保能够找到 NanoAIFlow，任选一种方式：

- `-DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install`
- `-DCMAKE_PREFIX_PATH=/path/to/NanoAIFlow/install`
- `-DNanoAIFlow_DIR=/path/to/NanoAIFlow/install/lib/cmake/NanoAIFlow`

```bash
cmake -S . -B build_pkg \
  -DNANOAI_RKNN_BUILD_EXAMPLES=OFF \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/rknn-aarch64-gcc.cmake \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_RKNN_RUNTIME_INCLUDE_DIR=/path/to/librknn_api/include \
  -DNANOAI_RKNN_RUNTIME_LIBRARY_DIR=/path/to/librknn_api/aarch64 \
  -DNANOAI_RKNN_RGA_ROOT=/path/to/librga \
  -DNANOAI_RKNN_OPENCV_DIR=/path/to/opencv4/cmake \
  -DNANOAI_RKNN_STB_IMAGE_INCLUDE_DIR=/path/to/stb_image \
  -DNANOAI_RKNN_JPEG_TURBO_ROOT=/path/to/jpeg_turbo \
  -DNANOAI_RKNN_UTILS_ROOT=/path/to/utils \
  -DCMAKE_INSTALL_PREFIX=/your/install/prefix

cmake --build build_pkg -j
cmake --install build_pkg
```

也可以使用 [external_deps_rknn_build.sh](external_deps_rknn_build.sh) 脚本，并按需修改其中路径变量。

### 2. 构建指定示例

推荐优先通过 `NANOAI_RKNN_EXAMPLES` 指定示例目录。构建系统内部还会派生每个示例自己的开关，主要用于在 CMake GUI 或 IDE 中展示最终选择结果。

```bash
cmake -S . -B build_proj \
  -DNANOAI_RKNN_BUILD_EXAMPLES=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/rknn-aarch64-gcc.cmake \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_RKNN_EXAMPLES=test_project

cmake --build build_proj -j
```

### 3. 交叉编译示例

```bash
cmake -S . -B build_cross \
  -DNANOAI_RKNN_BUILD_EXAMPLES=ON \
  -DNANOAI_RKNN_EXAMPLES=test_project \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
  -DCMAKE_SYSROOT=/path/to/sysroot \
  -DCMAKE_PREFIX_PATH=/path/to/deps

cmake --build build_cross --target NanoAI_rknn_demo -j
```

## 注意事项

- `examples/test_project` 需要从当前仓库入口构建，不建议独立配置
- 当前主要面向 `aarch64/arm64` 交叉编译场景
- 如果 IDE 只是为了代码补全而进行本机 `configure`，看到“跳过依赖检查”通常属于预期行为

## 文档导航

- [英文版 README](README.md)
- [打包与 find_package 指南](docs/find_package_and_packaging_guide.md)

## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。