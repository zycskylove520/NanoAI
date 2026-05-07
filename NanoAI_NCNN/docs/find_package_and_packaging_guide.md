# NanoAI_NCNN 构建与选项手册（中文）

本文档用于说明：

1. 有哪些可配置项。
2. 如何构建第三方包（`PACKAGE`）。
3. 如何构建项目（`PROJECTS`）。
4. 如何选择性编译 `projects/` 子项目。

## 1. 构建模式

`NanoAI_NCNN` 通过 `NANOAI_NCNN_BUILD_MODE` 选择模式，二选一：

- `PACKAGE`：生成并安装可复用 CMake 包（支持 `find_package`）。
- `PROJECTS`：构建 `projects/` 下项目。

默认值：`PACKAGE`

## 2. 顶层 CMake 主要选项

### 2.1 模式与路径

- `NANOAI_NCNN_BUILD_MODE`：`PACKAGE` / `PROJECTS`
- `NANOAI_NCNN_PROJECTS`：`ALL` / `NONE` / `projA;projB`（仅 `PROJECTS` 模式有效）
- `NANOAIFLOW_ROOT`：NanoAIFlow 安装前缀（平台无关，可选）
- `NANOAI_NCNN_INSTALL_CMAKEDIR`：安装时导出 CMake 配置文件目录

### 2.2 交叉编译预设

- `NANOAI_NCNN_LINUX_AARCH64_PRESET`：启用 Linux aarch64 GCC 交叉编译预设（默认 `OFF`）
- `NANOAI_NCNN_LINUX_AARCH64_TOOLCHAIN_FILE`：Linux aarch64 toolchain 文件（默认 `cmake/toolchains/ncnn-aarch64-gcc.cmake`）
- `NANOAI_NCNN_ANDROID_PRESET`：启用 Android 交叉编译预设（默认 `OFF`）
- `NANOAI_NCNN_ANDROID_NDK_PATH`：Android NDK 根目录（可选，空时读取 `ANDROID_NDK_HOME`/`ANDROID_NDK_ROOT`）
- `NANOAI_NCNN_ANDROID_ABI`：Android ABI（默认 `arm64-v8a`）
- `NANOAI_NCNN_ANDROID_PLATFORM`：Android API 级别（默认 `android-26`）

注意：

- `NANOAI_NCNN_LINUX_AARCH64_PRESET` 与 `NANOAI_NCNN_ANDROID_PRESET` 互斥，不能同时开启。
- 启用任一预设且未手动指定 `CMAKE_TOOLCHAIN_FILE` 时，会自动注入对应 toolchain。
并且会自动生成项目开关，例如：

- `NANOAI_ENABLE_PROJECT_CAR_PROJECT`

```bash
cmake -S NanoAI_NCNN -B NanoAI_NCNN/build_android \

# NanoAI_NCNN 安装、find_package 与交叉编译指南

本指南说明如何始终将 NanoAI_NCNN 安装为可复用 CMake 包，并在第三方项目中通过 find_package 引入。examples 可选构建，主库始终 install。

## 1. 安装 NanoAI_NCNN（始终 install 框架）

在 NanoAI_NCNN 目录执行：

  cmake -S . -B build_pkg -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install -DCMAKE_INSTALL_PREFIX=/your/install/prefix
  cmake --build build_pkg -j
  cmake --install build_pkg

如需构建 examples，可加 -DNANOAI_NCNN_BUILD_EXAMPLES=ON

  cmake -S . -B build_example -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install -DNANOAI_NCNN_BUILD_EXAMPLES=ON
  cmake --build build_example -j

安装后会导出：

- `NanoAI_NCNNConfig.cmake`
- `NanoAI_NCNNConfigVersion.cmake`
- `NanoAI_NCNNTargets.cmake`

## 2. 第三方项目中使用 find_package

```cmake
find_package(NanoAI_NCNN CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE NanoAI_NCNN::NanoAI_NCNN)
```

若安装前缀不在系统默认搜索路径：

  cmake -S . -B build -DCMAKE_PREFIX_PATH=/your/install/prefix

## 3. 交叉编译与依赖配置

### 3.1 交叉编译预设

- `NANOAI_NCNN_LINUX_AARCH64_PRESET`：启用 Linux aarch64 交叉编译
- `NANOAI_NCNN_LINUX_AARCH64_TOOLCHAIN_FILE`：自定义 toolchain 文件
- `NANOAI_NCNN_ANDROID_PRESET`：启用 Android 交叉编译
- `NANOAI_NCNN_ANDROID_NDK_PATH`：Android NDK 根目录
- `NANOAI_NCNN_ANDROID_ABI`：Android ABI（默认 arm64-v8a）
- `NANOAI_NCNN_ANDROID_PLATFORM`：Android API 级别（默认 android-26）

注意：
- `NANOAI_NCNN_LINUX_AARCH64_PRESET` 与 `NANOAI_NCNN_ANDROID_PRESET` 互斥，不能同时开启。
- 启用任一预设且未手动指定 `CMAKE_TOOLCHAIN_FILE` 时，会自动注入对应 toolchain。

### 3.2 依赖配置

- `NANOAI_NCNN_NCNN_INCLUDE_DIR`
- `NANOAI_NCNN_NCNN_LIBRARY_DIR`
- `NANOAI_NCNN_NCNN_LIBRARY`
- `NANOAI_NCNN_OPENCV_DIR`
  -DNANOAI_NCNN_NCNN_LIBRARY_DIR=/path/to/ncnn/android-aarch64/install/lib \
  -DNANOAI_NCNN_OPENCV_DIR=/path/to/OpenCV-android-sdk/sdk/native/jni \
  -DNANOAI_NCNN_ANDROID_ABI=arm64-v8a \
  -DNANOAI_NCNN_ANDROID_PLATFORM=android-26 \
  -DNANOAI_NCNN_BUILD_MODE=PROJECTS \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_NCNN_PROJECTS=car_project

cmake --build NanoAI_NCNN/build_android -j
```

也可使用 `NanoAI_NCNN/external_deps_android_build.sh`，修改脚本顶部路径变量后一键配置与构建。

## 5. 已知行为与常见问题

### 5.1 子项目独立构建

`projects/car_project` 不支持独立配置，必须从 `NanoAI_NCNN` 根目录进入。

### 5.2 模式互斥

`PACKAGE` 与 `PROJECTS` 为互斥模式，不能同时启用。

### 5.3 预设互斥

`NANOAI_NCNN_LINUX_AARCH64_PRESET` 与 `NANOAI_NCNN_ANDROID_PRESET` 互斥，不能同时启用。

### 5.4 依赖缺失排查

如果遇到头文件或链接符号缺失，请优先检查：

1. `NANOAIFLOW_ROOT` 是否指向正确安装目录。
  也可直接传 `CMAKE_PREFIX_PATH` 或 `NanoAIFlow_DIR`。
2. `NANOAI_NCNN_NCNN_INCLUDE_DIR` 与 `NANOAI_NCNN_NCNN_LIBRARY_DIR` 是否正确。
3. `NANOAI_NCNN_OPENCV_DIR` 是否可被 `find_package(OpenCV)` 识别。
