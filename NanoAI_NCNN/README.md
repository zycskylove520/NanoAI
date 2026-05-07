# NanoAI_NCNN

NanoAI_NCNN 是面向 NCNN 推理部署场景的 C++ 组件，基于 CMake 构建，并与 NanoAIFlow 协同工作。

项目目标：

- 提供可复用的 NCNN 推理接口与管线能力。
- 支持作为第三方包发布，便于被其他工程通过 CMake 复用。
- 支持在仓库内直接构建 projects 子目录中的示例/应用项目。

NanoAI_NCNN 支持两种互斥构建模式：

1. `PACKAGE`：构建并安装为可 `find_package` 的第三方包。
2. `PROJECTS`：构建 `projects/` 下可执行项目，支持按需选择子项目。

## 主要特性

- CMake 原生支持，可在 PACKAGE/PROJECTS 两种模式间切换。
- 提供标准 CMake 包导出，可被调用方通过 `find_package` 使用。
- 支持按子项目选择性编译，避免一次性构建全部项目。
- 提供基于 NanoAIFlow 的 `Load -> Preprocess -> Infer -> Postprocess` pipe 连接方式。

## 仓库结构

- include：对外头文件
- cmake：构建脚本与第三方依赖配置
- projects：可执行项目入口（当前包含 car_project）
- 3rdparty：第三方依赖目录
- docs：使用手册与构建文档

## 文档入口

- 详细构建与选项手册（中文）：[docs/find_package_and_packaging_guide.md](docs/find_package_and_packaging_guide.md)

## 核心开关

- `NANOAI_NCNN_BUILD_MODE`：`PACKAGE` 或 `PROJECTS`，默认 `PACKAGE`
- `NANOAI_NCNN_PROJECTS`：仅在 `PROJECTS` 模式生效，默认 `ALL`
- `NANOAI_NCNN_INSTALL_CMAKEDIR`：包配置文件安装目录，默认 `lib/cmake/NanoAI_NCNN`
- `NANOAIFLOW_ROOT`：NanoAIFlow 安装前缀（平台无关）

预设开关：

- `NANOAI_NCNN_LINUX_AARCH64_PRESET`：启用 Linux aarch64 交叉编译预设（默认 `OFF`）
- `NANOAI_NCNN_LINUX_AARCH64_TOOLCHAIN_FILE`：Linux aarch64 toolchain 文件路径（默认 `cmake/toolchains/ncnn-aarch64-gcc.cmake`）
- `NANOAI_NCNN_ANDROID_PRESET`：启用 Android 交叉编译预设（默认 `OFF`）

说明：

- `NANOAI_NCNN_LINUX_AARCH64_PRESET` 与 `NANOAI_NCNN_ANDROID_PRESET` 互斥，不能同时开启。
- 当任一预设开启且未手动指定 `CMAKE_TOOLCHAIN_FILE` 时，会自动注入对应 toolchain。

第三方依赖开关（位于 `cmake/NanoAINCNNThirdParty.cmake`）：

- `NANOAI_NCNN_WITH_NCNN`
- `NANOAI_NCNN_WITH_OPENCV`
- `NANOAI_NCNN_NCNN_INCLUDE_DIR`
- `NANOAI_NCNN_NCNN_LIBRARY_DIR`
- `NANOAI_NCNN_NCNN_LIBRARY`
- `NANOAI_NCNN_OPENCV_DIR`

## 快速命令

### 1. 构建并安装第三方包

先确保能找到 NanoAIFlow（任选一种）：

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

### 2. 构建 projects 中指定子项目

```bash
cmake -S . -B build_proj \
  -DNANOAI_NCNN_BUILD_MODE=PROJECTS \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_NCNN_PROJECTS=car_project

cmake --build build_proj -j
```

### 3. Linux aarch64 交叉编译（与 RKNN 风格对齐）

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

说明：Android NDK、NCNN、OpenCV 均改为外部传入，仓库不再默认提供路径。

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

若不传 `NANOAI_NCNN_ANDROID_NDK_PATH`，会依次读取环境变量 `ANDROID_NDK_HOME` 或 `ANDROID_NDK_ROOT`。

你也可以直接使用仓库内提供的脚本 [external_deps_android_build.sh](external_deps_android_build.sh)，修改顶部路径变量后执行。

## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。
