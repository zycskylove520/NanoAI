# NanoAI_NCNN

English | [中文](README_zh.md)

NanoAI_NCNN is a high-performance C++ inference component for NCNN-based edge deployment. Built on top of NanoAIFlow's strongly typed concurrent pipeline model, it is suitable for complete deployment chains that include input loading, preprocessing, model inference, and postprocessing across common edge platforms such as Linux aarch64 and Android.

## Core Strengths

### 1. High-throughput inference pipelines with NanoAIFlow
- Load, preprocess, infer, and postprocess stages can be organized as one unified pipe chain
- The stage-level concurrency model of NanoAIFlow can be used to improve total throughput
- Well suited for stable-structure inference systems that need to keep multi-core CPUs busy

### 2. Multi-platform edge deployment
- Supports deployment workflows for Linux aarch64 and Android
- Lets you select the proper toolchain and dependency configuration for each target platform
- Suitable for edge vision, embedded AI, and mobile inference scenarios

### 3. Flexible engineering integration
- Supports two build modes: `PACKAGE` and `PROJECTS`
- Can be published as a reusable component or built directly as in-repository applications
- Supports selective project builds to keep build scope under control

### 4. Standard CMake package export
- Can be consumed by third-party projects through `find_package`
- Fits naturally into existing CMake toolchains and modular architectures

### 5. Clear dependency and cross-compilation workflow
- Supports explicit configuration of NCNN, OpenCV, Android NDK, and other required paths
- Linux aarch64 and Android presets are separated for better control over build paths

## Repository Layout

- `include`: public headers
- `cmake`: build scripts and third-party dependency configuration
- `projects`: example or application entry points
- `3rdparty`: third-party dependency directory
- `docs`: build and packaging documentation

## Build Modes

1. `PACKAGE`
   - Build and install a reusable package that can be consumed with `find_package`
2. `PROJECTS`
   - Build applications or examples under `projects/`

## Key Configuration Options

- `NANOAI_NCNN_BUILD_MODE`: `PACKAGE` or `PROJECTS`, default `PACKAGE`
- `NANOAI_NCNN_PROJECTS`: active only in `PROJECTS` mode, default `ALL`
- `NANOAI_NCNN_INSTALL_CMAKEDIR`: install directory for package config files
- `NANOAIFLOW_ROOT`: installation prefix of NanoAIFlow

Platform-related presets:

- `NANOAI_NCNN_LINUX_AARCH64_PRESET`
- `NANOAI_NCNN_LINUX_AARCH64_TOOLCHAIN_FILE`
- `NANOAI_NCNN_ANDROID_PRESET`
- `NANOAI_NCNN_ANDROID_NDK_PATH`

Common third-party dependency settings:

- `NANOAI_NCNN_NCNN_INCLUDE_DIR`
- `NANOAI_NCNN_NCNN_LIBRARY_DIR`
- `NANOAI_NCNN_NCNN_LIBRARY`
- `NANOAI_NCNN_OPENCV_DIR`

## Quick Start

### 1. Build and install the package

Make sure NanoAIFlow can be found by one of the following:

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

### 2. Build a selected project

```bash
cmake -S . -B build_proj \
  -DNANOAI_NCNN_BUILD_MODE=PROJECTS \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_NCNN_PROJECTS=car_project

cmake --build build_proj -j
```

### 3. Linux aarch64 cross-compilation

```bash
cmake -S . -B build_linux_aarch64 \
  -DNANOAI_NCNN_LINUX_AARCH64_PRESET=ON \
  -DNANOAI_NCNN_BUILD_MODE=PACKAGE

cmake --build build_linux_aarch64 -j
```

To use a custom toolchain:

```bash
cmake -S . -B build_linux_aarch64 \
  -DNANOAI_NCNN_LINUX_AARCH64_PRESET=ON \
  -DNANOAI_NCNN_LINUX_AARCH64_TOOLCHAIN_FILE=/path/to/your/toolchain.cmake \
  -DNANOAI_NCNN_BUILD_MODE=PACKAGE
```

### 4. Android cross-compilation

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

If `NANOAI_NCNN_ANDROID_NDK_PATH` is not set, the build will try `ANDROID_NDK_HOME` and `ANDROID_NDK_ROOT` from the environment.

You can also use [external_deps_android_build.sh](external_deps_android_build.sh) after adjusting the path variables in the script.

## Notes

- `NANOAI_NCNN_LINUX_AARCH64_PRESET` and `NANOAI_NCNN_ANDROID_PRESET` are mutually exclusive
- When a preset is enabled and `CMAKE_TOOLCHAIN_FILE` is not set manually, the matching toolchain is injected automatically

## Documentation

- [Chinese README](README_zh.md)
- [Packaging and find_package guide](docs/find_package_and_packaging_guide.md)

## License

This project is licensed under Apache License 2.0. See [LICENSE](LICENSE).