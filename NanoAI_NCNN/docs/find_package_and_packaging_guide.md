# NanoAI_NCNN 构建、`find_package` 与交叉编译指南

本文档说明 `NanoAI_NCNN` 当前的构建画像、主要选项，以及在第三方工程中如何理解其依赖关系。

## 1. 当前构建画像

`NanoAI_NCNN` 当前以 **INTERFACE 目标** 形式导出：

- 目标名：`NanoAI_NCNN::NanoAI_NCNN`
- 主体价值：公开头文件、NCNN/OpenCV 依赖组织、NanoAIFlow pipeline 组合方式
- 示例目录：`examples/`，仅在显式开启时参与构建

这种结构的目的是把“视觉推理组件的接口面”与“demo/验证程序”分离，避免示例工程影响下游集成体验。

## 2. 顶层 CMake 主要选项

### 2.1 基础选项

- `NANOAI_NCNN_BUILD_EXAMPLES`：是否构建示例，默认 `OFF`
- `NANOAI_NCNN_EXAMPLES`：示例选择，支持 `ALL` 或 `exampleA;exampleB`
- `NANOAIFLOW_ROOT`：NanoAIFlow 安装前缀，可选
- `NANOAI_NCNN_ENABLE_CPACK`：是否启用 CPack 打包，默认 `OFF`

说明：

- 如果只是把 `NanoAI_NCNN` 作为头文件 + 依赖聚合层接入自己的工程，通常不需要开启 examples。
- `NANOAIFLOW_ROOT` 会被追加到 `CMAKE_PREFIX_PATH`，方便顶层 `find_package(NanoAIFlow CONFIG REQUIRED)` 命中正确安装前缀。

### 2.2 交叉编译/平台预设

- `NANOAI_NCNN_LINUX_x86_64_PRESET`：启用 Linux x86_64 预设，默认 `OFF`
- `NANOAI_NCNN_ANDROID_PRESET`：启用 Android 预设，默认 `OFF`
- `CMAKE_TOOLCHAIN_FILE`：Android 预设下必须由调用方显式提供
- `ANDROID_ABI`：Android ABI，默认 `arm64-v8a`
- `ANDROID_PLATFORM`：Android API 级别，默认 `android-26`

注意：

- `NANOAI_NCNN_LINUX_x86_64_PRESET` 与 `NANOAI_NCNN_ANDROID_PRESET` 互斥。
- Android 预设不是完整 NDK toolchain，只负责补齐 ABI/STL 等常见参数；真正的 NDK toolchain 仍应由 `CMAKE_TOOLCHAIN_FILE` 指向官方脚本。

## 3. 构建 NanoAI_NCNN

### 3.1 构建并启用 examples

```bash
cmake -S . -B build_example \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_NCNN_BUILD_EXAMPLES=ON

cmake --build build_example -j
```

### 3.2 只构建指定示例

```bash
cmake -S . -B build_example \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_NCNN_BUILD_EXAMPLES=ON \
  -DNANOAI_NCNN_EXAMPLES=car_detect_example

cmake --build build_example -j
```

说明：

- examples 子目录会自动扫描直接子目录中的 `CMakeLists.txt`。
- `NANOAI_NCNN_EXAMPLES=ALL` 会启用全部示例。
- 若想完全关闭示例，请直接保持 `NANOAI_NCNN_BUILD_EXAMPLES=OFF`；无需再传 `NONE`。

## 4. 第三方项目中的依赖理解

`NanoAI_NCNN` 顶层自身会依赖：

```cmake
find_package(NanoAIFlow CONFIG REQUIRED)
```

并把以下内容聚合到 `NanoAI_NCNN::NanoAI_NCNN`：

- `NanoAI::Flow`
- `ncnn`
- `OpenCV` 相关目标或库名
- 公开头文件目录与编译定义

因此，下游项目通常只需要：

```cmake
find_package(NanoAIFlow CONFIG REQUIRED)
add_subdirectory(/path/to/NanoAI_NCNN NanoAI_NCNN-build)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE NanoAI_NCNN::NanoAI_NCNN)
```

如果 `NanoAIFlow` 安装前缀不在默认搜索路径，可显式追加：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/your/install/prefix
```

## 5. 交叉编译与依赖配置

### 5.1 Android 示例

```bash
cmake -S . -B build_android \
  -DNANOAI_NCNN_ANDROID_PRESET=ON \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/android-ndk/build/cmake/android.toolchain.cmake \
  -Dncnn_DIR=/path/to/ncnn/android/install/lib/cmake/ncnn \
  -DOPENCV_CMAKE_DIR=/path/to/OpenCV-android-sdk/sdk/native/jni \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_NCNN_BUILD_EXAMPLES=ON \
  -DNANOAI_NCNN_EXAMPLES=car_detect_example

cmake --build build_android -j
```

### 5.2 Linux x86_64 预设示例

```bash
cmake -S . -B build_linux \
  -DNANOAI_NCNN_LINUX_x86_64_PRESET=ON \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install

cmake --build build_linux -j
```

说明：

- `NCNNRuntime.cmake` 优先寻找 `ncnn` 导出的 CMake package target。
- 若历史工程仍使用兼容变量，可传 `-DNANOAI_NCNN_NCNN_CONFIG_DIR=...`，脚本会映射到 `ncnn_DIR`。
- `OpenCV.cmake` 在交叉编译场景下建议显式设置 `OPENCV_CMAKE_DIR`，避免误用主机 OpenCV。

## 6. 已知行为与常见问题

### 6.1 示例不支持 standalone configure

`examples/car_detect_example` 必须从 `NanoAI_NCNN` 根目录统一配置，不能直接进入示例子目录单独执行 `cmake -S . -B build`。

原因是：

- 顶层统一持有 `NanoAI_NCNN` 目标
- third-party 依赖与平台预设只在顶层脚本中解析
- 单独配置示例会丢失对 NCNN/OpenCV/NanoAIFlow 的统一组织

### 6.2 预设互斥

`NANOAI_NCNN_LINUX_x86_64_PRESET` 与 `NANOAI_NCNN_ANDROID_PRESET` 不能同时开启。

### 6.3 依赖缺失排查

如果遇到头文件或链接符号缺失，优先检查：

1. `NANOAIFLOW_ROOT` / `CMAKE_PREFIX_PATH` / `NanoAIFlow_DIR` 是否指向正确安装目录。
2. `ncnn_DIR` 或 `NANOAI_NCNN_NCNN_CONFIG_DIR` 是否指向有效的 `ncnnConfig.cmake`。
3. `OPENCV_CMAKE_DIR` 是否对应目标平台可用的 OpenCV 包。
4. Android 场景下 `CMAKE_TOOLCHAIN_FILE` 是否确实指向 NDK 官方 toolchain。
