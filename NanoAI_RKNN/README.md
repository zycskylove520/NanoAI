# NanoAI_RKNN

English | [中文](README_zh.md)

NanoAI_RKNN is a high-performance C++ inference component for Rockchip RKNN edge deployment. It builds RKNN inference capability on top of NanoAIFlow's strongly typed concurrent pipeline model, making it suitable for complete engineering chains that include loading, preprocessing, inference, and postprocessing.

## Core Strengths

### 1. Deep integration with NanoAIFlow
- Load, Preprocess, Infer, and Postprocess stages can be placed in one unified concurrent pipeline
- Well suited for fixed-structure, high-throughput edge inference systems
- Lets you manage model execution and surrounding data flow in one consistent framework

### 2. Optimized for RKNN edge deployment
- Focused on integrating Rockchip RKNN Runtime into practical C++ systems
- Designed for `aarch64` and `arm64` edge deployment workflows
- Suitable for industrial vision, embedded AI, and edge inference applications

### 3. Flexible engineering integration
- Always exports/installable as a reusable CMake package
- Examples can be enabled optionally for learning and integration validation
- Supports selective example builds to avoid unnecessary compilation cost

### 4. Standard CMake package export
- Can be consumed by upper-layer projects through `find_package`
- Fits naturally into existing CMake-based engineering systems
- Works well for modular delivery, secondary packaging, and CI/CD workflows

### 5. Explicit third-party dependency control
- RKNN Runtime, RGA, OpenCV, stb_image, jpeg_turbo, and utils paths are passed explicitly from outside the repository
- Better suited for complex cross-compilation environments and enterprise dependency management

## Repository Layout

- `include`: public headers
- `cmake`: build scripts and third-party dependency configuration
- `examples`: example or application entry points
- `3rdparty`: third-party dependency directory
- `docs`: build and packaging documentation

## Build Behavior

1. Package export/install is always available
  - Build and install a reusable package that can be consumed with `find_package`
2. Examples are optional
  - Enable examples under `examples/` with CMake options

## Key Configuration Options

- `NANOAI_RKNN_BUILD_EXAMPLES`: `ON` or `OFF`, default `OFF`
- `NANOAI_RKNN_EXAMPLES`: active when `NANOAI_RKNN_BUILD_EXAMPLES=ON`, default `ALL`
- `NANOAI_RKNN_INSTALL_CMAKEDIR`: install directory for package config files
- `NANOAIFLOW_ROOT`: installation prefix of NanoAIFlow

Common third-party dependency settings:

- `NANOAI_RKNN_RUNTIME_INCLUDE_DIR`
- `NANOAI_RKNN_RUNTIME_LIBRARY_DIR`
- `NANOAI_RKNN_RGA_ROOT`
- `NANOAI_RKNN_OPENCV_DIR`
- `NANOAI_RKNN_STB_IMAGE_INCLUDE_DIR`
- `NANOAI_RKNN_JPEG_TURBO_ROOT`
- `NANOAI_RKNN_UTILS_ROOT`

## Quick Start

### 1. Build and install the package

Make sure NanoAIFlow can be found by one of the following:

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

You can also use [external_deps_rknn_build.sh](external_deps_rknn_build.sh) after adjusting the path variables in the script.

### 2. Build selected examples

```bash
cmake -S . -B build_proj \
  -DNANOAI_RKNN_BUILD_EXAMPLES=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/rknn-aarch64-gcc.cmake \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_RKNN_EXAMPLES=test_project

cmake --build build_proj -j
```

### 3. Cross-compilation example

```bash
cmake -S . -B build_cross \
  -DNANOAI_RKNN_BUILD_EXAMPLES=ON \
  -DNANOAI_RKNN_EXAMPLES=test_project \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
  -DCMAKE_SYSROOT=/path/to/sysroot \
  -DCMAKE_PREFIX_PATH=/path/to/deps

cmake --build build_cross --target NanoAI_rknn_demo -j
```

## Notes

- `examples/test_project` should be built from this repository entry rather than configured as a standalone project
- The current workflow primarily targets `aarch64/arm64` cross-compilation

## Documentation

- [Chinese README](README_zh.md)
- [Packaging and find_package guide](docs/find_package_and_packaging_guide.md)

## License

This project is licensed under Apache License 2.0. See [LICENSE](LICENSE).