# NanoAI_RKNN

NanoAI_RKNN 是面向 RKNN 推理部署场景的 C++ 组件，基于 CMake 构建，并与 NanoAIFlow 协同工作。

项目目标：

- 提供可复用的 RKNN 推理相关接口与管线能力。
- 支持作为第三方包发布，便于被其他工程通过 CMake 复用。
- 支持在仓库内直接构建 projects 子目录中的示例/应用项目。

NanoAI_RKNN 支持两种互斥构建模式：

1. `PACKAGE`：构建并安装为可 `find_package` 的第三方包。
2. `PROJECTS`：构建 `projects/` 下可执行项目，支持按需选择子项目。

## 主要特性

- CMake 原生支持，可在 PACKAGE/PROJECTS 两种模式间切换。
- 提供标准 CMake 包导出，可被调用方通过 `find_package` 使用。
- 支持按子项目选择性编译，避免一次性构建全部项目。
- 仅支持 ARM 平台交叉编译（aarch64/arm64），交叉编译参数从根 CMake 统一传入。

## 仓库结构

- include：对外头文件
- cmake：构建脚本与第三方依赖配置
- projects：可执行项目入口（如 test_project）
- 3rdparty：第三方依赖目录
- docs：使用手册与构建文档

## 文档入口

- 详细构建与选项手册（中文）：[docs/find_package_and_packaging_guide.md](docs/find_package_and_packaging_guide.md)

## 核心开关

- `NANOAI_RKNN_BUILD_MODE`：`PACKAGE` 或 `PROJECTS`，默认 `PACKAGE`
- `NANOAI_RKNN_PROJECTS`：仅在 `PROJECTS` 模式生效，默认 `ALL`
- `NANOAI_RKNN_INSTALL_CMAKEDIR`：包配置文件安装目录，默认 `lib/cmake/NanoAI_RKNN`
- `NANOAIFLOW_ROOT`：NanoAIFlow 安装前缀（用于 `find_package(NanoAIFlow)`）

第三方依赖开关（位于 `cmake/NanoAIRKNNThirdParty.cmake`）：

- `NANOAI_RKNN_WITH_RKNN_RUNTIME`
- `NANOAI_RKNN_WITH_RGA`
- `NANOAI_RKNN_WITH_OPENCV`
- `NANOAI_RKNN_WITH_STB_IMAGE`
- `NANOAI_RKNN_WITH_JPEG_TURBO`
- `NANOAI_RKNN_WITH_UTILS`

说明：以上开关在 `aarch64/arm64` 下默认更偏向开启 RKNN/RGA 相关依赖，在非 ARM 平台默认更保守。
当前实现中这些开关由工程强制启用，不支持关闭。

第三方路径说明：

- RKNN 的第三方依赖改为外部路径传入，仓库不再提供默认内置路径。
- 必传路径包括：RKNN Runtime、RGA、OpenCV、stb_image、jpeg_turbo、utils。

## 快速命令

### 1. 构建并安装第三方包

先确保能找到 NanoAIFlow（任选一种）：

- `-DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install`
- `-DCMAKE_PREFIX_PATH=/path/to/NanoAIFlow/install`
- `-DNanoAIFlow_DIR=/path/to/NanoAIFlow/install/lib/cmake/NanoAIFlow`

```bash
cmake -S . -B build_pkg \
  -DNANOAI_RKNN_BUILD_MODE=PACKAGE \
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

你也可以直接使用仓库内脚本 [external_deps_rknn_build.sh](external_deps_rknn_build.sh)，修改顶部路径变量后执行。

### 2. 构建 projects 中指定子项目

```bash
cmake -S . -B build_proj \
  -DNANOAI_RKNN_BUILD_MODE=PROJECTS \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/rknn-aarch64-gcc.cmake \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_RKNN_PROJECTS=test_project

cmake --build build_proj -j
```

### 3. 交叉编译示例

```bash
cmake -S . -B build_cross \
  -DNANOAI_RKNN_BUILD_MODE=PROJECTS \
  -DNANOAI_RKNN_PROJECTS=test_project \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
  -DCMAKE_SYSROOT=/path/to/sysroot \
  -DCMAKE_PREFIX_PATH=/path/to/deps

cmake --build build_cross --target NanoAI_rknn_demo -j
```

## 注意事项

- `projects/test_project` 不支持独立配置，必须从仓库根 CMake 进入。
- `PACKAGE` 与 `PROJECTS` 为互斥模式，不能同时启用。
- 仅允许交叉编译，且目标处理器必须为 `aarch64/arm64`。

## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。
