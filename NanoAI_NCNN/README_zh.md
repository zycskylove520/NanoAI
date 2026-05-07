# NanoAI_NCNN

[English](README.md) | 中文

NanoAI_NCNN 是面向 NCNN 端侧部署场景的高性能 C++ 推理组件。它基于 NanoAIFlow 的强类型并发管线能力，适合构建从数据输入、预处理、模型推理到结果后处理的完整部署链路，并覆盖 Linux aarch64 与 Android 等常见端侧平台。

## 核心优势

### 1. 与 NanoAIFlow 组合形成高吞吐推理流水线
- 支持将加载、预处理、推理、后处理组织为统一 Pipe 链路
- 可利用 NanoAIFlow 的阶段并发能力提升整体吞吐
- 适合结构稳定、需要持续压榨多核性能的推理系统

### 2. 面向端侧多平台部署
- 支持 Linux aarch64 与 Android 等部署场景
- 可根据目标平台选择对应的 toolchain 与依赖配置
- 适合边缘视觉、嵌入式 AI、移动端推理等场景

### 3. 工程集成方式灵活
- 支持 `PACKAGE` 与 `PROJECTS` 两种构建模式
- 既可作为可复用组件发布，也可直接构建仓库内项目
- 支持按需选择子项目，减少构建范围

### 4. 标准 CMake 包导出
- 支持通过 `find_package` 接入第三方工程
- 便于纳入现有 CMake 工具链和组件化架构

### 5. 依赖与交叉编译配置清晰
- 支持 NCNN、OpenCV、Android NDK 等依赖显式传入
- Linux aarch64 与 Android 预设分离，便于控制构建路径

## 仓库结构

- `include`：对外头文件
- `cmake`：构建脚本与第三方依赖配置
- `projects`：示例或应用项目入口
- `3rdparty`：第三方依赖目录
- `docs`：构建与打包文档

## 构建模式

1. `PACKAGE`
   - 构建并安装为可通过 `find_package` 复用的第三方包
2. `PROJECTS`
   - 构建 `projects/` 下的应用或示例项目

## 核心配置项

- `NANOAI_NCNN_BUILD_MODE`：`PACKAGE` 或 `PROJECTS`，默认 `PACKAGE`
- `NANOAI_NCNN_PROJECTS`：仅在 `PROJECTS` 模式下生效，默认 `ALL`
- `NANOAI_NCNN_INSTALL_CMAKEDIR`：包配置文件安装目录
- `NANOAIFLOW_ROOT`：NanoAIFlow 安装前缀

平台相关预设：

- `NANOAI_NCNN_LINUX_AARCH64_PRESET`
- `NANOAI_NCNN_LINUX_AARCH64_TOOLCHAIN_FILE`
- `NANOAI_NCNN_ANDROID_PRESET`
- `NANOAI_NCNN_ANDROID_NDK_PATH`

常见第三方依赖配置：

- `NANOAI_NCNN_NCNN_INCLUDE_DIR`
- `NANOAI_NCNN_NCNN_LIBRARY_DIR`
- `NANOAI_NCNN_NCNN_LIBRARY`
- `NANOAI_NCNN_OPENCV_DIR`

## 快速开始

### 1. 构建并安装第三方包

先确保能够找到 NanoAIFlow，任选一种方式：

- `-DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install`
- `-DCMAKE_PREFIX_PATH=/path/to/NanoAIFlow/install`
- `-DNanoAIFlow_DIR=/path/to/NanoAIFlow/install/lib/cmake/NanoAIFlow`

```bash
cmake -S . -B build_pkg \
  -DNANOAI_NCNN_BUILD_MODE=PACKAGE \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DCMAKE_INSTALL_PREFIX=/your/install/prefix

cmake --build build_pkg -j
cmake --install build_pkg
```

### 2. 构建指定子项目

```bash
cmake -S . -B build_proj \
  -DNANOAI_NCNN_BUILD_MODE=PROJECTS \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_NCNN_PROJECTS=car_project

cmake --build build_proj -j
```

### 3. Linux aarch64 交叉编译

```bash
cmake -S . -B build_linux_aarch64 \
  -DNANOAI_NCNN_LINUX_AARCH64_PRESET=ON \
  -DNANOAI_NCNN_BUILD_MODE=PACKAGE

cmake --build build_linux_aarch64 -j
```

如需自定义 toolchain：

```bash
cmake -S . -B build_linux_aarch64 \
  -DNANOAI_NCNN_LINUX_AARCH64_PRESET=ON \
  -DNANOAI_NCNN_LINUX_AARCH64_TOOLCHAIN_FILE=/path/to/your/toolchain.cmake \
  -DNANOAI_NCNN_BUILD_MODE=PACKAGE
```

### 4. Android 交叉编译

```bash
cmake -S . -B build_android \
  -DNANOAI_NCNN_ANDROID_PRESET=ON \
  -DNANOAI_NCNN_ANDROID_NDK_PATH=/path/to/android-ndk \
  -DNANOAI_NCNN_NCNN_INCLUDE_DIR=/path/to/ncnn/android-aarch64/install/include \
  -DNANOAI_NCNN_NCNN_LIBRARY_DIR=/path/to/ncnn/android-aarch64/install/lib \
  -DNANOAI_NCNN_OPENCV_DIR=/path/to/OpenCV-android-sdk/sdk/native/jni \
  -DNANOAI_NCNN_ANDROID_ABI=arm64-v8a \
  -DNANOAI_NCNN_ANDROID_PLATFORM=android-26 \
  -DNANOAI_NCNN_BUILD_MODE=PROJECTS \
  -DNANOAI_NCNN_PROJECTS=car_project

cmake --build build_android -j
```

若未传 `NANOAI_NCNN_ANDROID_NDK_PATH`，会尝试读取环境变量 `ANDROID_NDK_HOME` 或 `ANDROID_NDK_ROOT`。

也可以使用 [external_deps_android_build.sh](external_deps_android_build.sh) 脚本，并按需修改其中路径变量。

## 注意事项

- `NANOAI_NCNN_LINUX_AARCH64_PRESET` 与 `NANOAI_NCNN_ANDROID_PRESET` 互斥
- 预设启用后，若未手动指定 `CMAKE_TOOLCHAIN_FILE`，会自动注入对应 toolchain

## 文档导航

- [英文版 README](README.md)
- [打包与 find_package 指南](docs/find_package_and_packaging_guide.md)

## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。