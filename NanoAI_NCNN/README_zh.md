# NanoAI_NCNN

[English](README.md) | 中文



NanoAI_NCNN 是一个高性能的 NCNN 端侧推理 C++ 框架。现仅本地 INTERFACE 构建，用户可选构建 examples 进行学习和测试。所有依赖均通过 INTERFACE 链接，适合稳定高吞吐的部署链路，支持 Linux x86_64、Android 等多平台。

该模块刻意保持“薄库”形态：核心价值主要体现在公开头文件、基于 NanoAIFlow 的 pipeline 组织方式，以及端侧部署所需的显式依赖/工具链配置，而不是额外生成一层厚重的本地二进制库。


## 核心特性

1. **仅 INTERFACE 构建**：NanoAI_NCNN 以 INTERFACE 目标形式对外暴露，目的是让下游复用统一依赖面，而不是重复封装一份内部二进制库。
2. **示例可选构建**：仅在设置 `-DNANOAI_NCNN_BUILD_EXAMPLES=ON` 时构建 examples。
3. **INTERFACE 链接**：所有依赖（NanoAIFlow、OpenCV、NCNN 等）均 INTERFACE 链接，集成健壮、模块化。
4. **高吞吐推理流水线**：基于 NanoAIFlow，支持并发、有序、强类型推理链路。
5. **多平台端侧部署**：支持 Linux x86_64、Android，分别有 toolchain 与依赖配置。
6. **依赖与交叉编译流程清晰**：NCNN、OpenCV、NDK 等依赖配置明确，交叉编译预设清晰。


## 仓库结构

- `include`：对外头文件
- `cmake`：构建脚本与第三方依赖配置
- `examples`：可选示例程序（通过 `-DNANOAI_NCNN_BUILD_EXAMPLES=ON` 构建）
- `3rdparty`：第三方依赖目录
- `docs`：构建与打包文档

`examples` 与主目标导出逻辑刻意分离，这样即使示例增加了 demo 专用源码或额外可视化逻辑，也不会影响下游集成 `NanoAI_NCNN` 的稳定接口面。



## 构建

### 1. 可选构建示例

确保能找到 NanoAIFlow，可用：
- `-DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install`
- `-DCMAKE_PREFIX_PATH=/path/to/NanoAIFlow/install`
- `-DNanoAIFlow_DIR=/path/to/NanoAIFlow/install/lib/cmake/NanoAIFlow`

```bash
cmake -S . -B build_example \
  -DNANOAI_NCNN_BUILD_EXAMPLES=ON \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install
cmake --build build_example -j
```

### 2. 交叉编译预设

- `NANOAI_NCNN_LINUX_x86_64_PRESET`：启用 Linux x86_64 预设
- `NANOAI_NCNN_ANDROID_PRESET`：启用 Android 交叉编译

启用预设后，顶层 CMake 会显式暴露目标平台假设，而不是静默猜测主机侧 SDK 路径。对 Android/OpenCV/NCNN 这类容易误命中主机库的组合来说，这一点尤其重要。


常见第三方依赖配置：
- `NANOAI_NCNN_NCNN_INCLUDE_DIR`
- `NANOAI_NCNN_NCNN_LIBRARY_DIR`
- `NANOAI_NCNN_NCNN_LIBRARY`
- `OPENCV_CMAKE_DIR`


### 4. Linux aarch64/Android 交叉编译

```bash
cmake -S . -B build_linux_aarch64 \
  -DNANOAI_NCNN_LINUX_AARCH64_PRESET=ON \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install
cmake --build build_linux_aarch64 -j
```

```bash
cmake -S . -B build_android \
  -DNANOAI_NCNN_ANDROID_PRESET=ON \
  -DNANOAI_NCNN_ANDROID_NDK_PATH=/path/to/android-ndk \
  -DNANOAI_NCNN_NCNN_INCLUDE_DIR=/path/to/ncnn/android-aarch64/install/include \
  -DNANOAI_NCNN_NCNN_LIBRARY_DIR=/path/to/ncnn/android-aarch64/install/lib \
  -DOPENCV_CMAKE_DIR=/path/to/OpenCV-android-sdk/sdk/native/jni \
  -DNANOAI_NCNN_ANDROID_ABI=arm64-v8a \
  -DNANOAI_NCNN_ANDROID_PLATFORM=android-26 \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install
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