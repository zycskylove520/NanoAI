# NanoAI

English | [中文](README_zh.md)

NanoAI is a multi-module C++ repository for high-performance inference and data processing. It focuses on extreme concurrency, strong typing, edge deployment, and reusable engineering workflows. The repository uses NanoAIFlow as its common pipeline foundation and builds RKNN and NCNN inference modules on top of it for production-style AI applications with stable graph structure and high throughput requirements.

## Core Modules

### NanoAIFlow
- A high-performance concurrent pipeline framework for inference pre-processing, post-processing, and data transformation chains
- Header-only design with low integration cost
- Per-pipe concurrency control, with multiple pipes advancing concurrently in the same pipeline
- Strongly typed static pipelines with ordered forwarding under high concurrency
- See [NanoAIFlow/README.md](NanoAIFlow/README.md) and [NanoAIFlow/README_zh.md](NanoAIFlow/README_zh.md)

### NanoAI_RKNN
- A C++ inference component for Rockchip RKNN deployment
- Deep integration with NanoAIFlow so load, preprocess, infer, and postprocess stages can run inside one unified concurrent pipeline
- Standard CMake package export and ARM edge cross-compilation support
- See [NanoAI_RKNN/README.md](NanoAI_RKNN/README.md) and [NanoAI_RKNN/README_zh.md](NanoAI_RKNN/README_zh.md)

### NanoAI_NCNN
- A C++ inference component for NCNN-based edge deployment
- Supports multi-platform deployment such as Linux aarch64 and Android
- Designed to work with NanoAIFlow for high-throughput inference pipelines
- See [NanoAI_NCNN/README.md](NanoAI_NCNN/README.md) and [NanoAI_NCNN/README_zh.md](NanoAI_NCNN/README_zh.md)

## Recommended Usage

1. Build and install NanoAIFlow first.
2. Build NanoAI_RKNN or NanoAI_NCNN and point `CMAKE_PREFIX_PATH`, `NANOAIFLOW_ROOT`, or `NanoAIFlow_DIR` to the NanoAIFlow install location.
3. Choose the proper module and toolchain configuration for the target platform.

## Monorepo Entry Points

The repository provides unified build entry points:

- [CMakeLists.txt](CMakeLists.txt)
- [CMakePresets.json](CMakePresets.json)

Example:

```bash
# 1. Configure and build NanoAIFlow
cmake --preset flow-package
cmake --build --preset flow-package

# 2. Configure and build RKNN
export NANOAIFLOW_ROOT=/path/to/NanoAIFlow/install
cmake --preset rknn-package
cmake --build --preset rknn-package
```

## Documentation

- [NanoAIFlow English README](NanoAIFlow/README.md)
- [NanoAIFlow Chinese README](NanoAIFlow/README_zh.md)
- [NanoAI_RKNN English README](NanoAI_RKNN/README.md)
- [NanoAI_RKNN Chinese README](NanoAI_RKNN/README_zh.md)
- [NanoAI_NCNN English README](NanoAI_NCNN/README.md)
- [NanoAI_NCNN Chinese README](NanoAI_NCNN/README_zh.md)

## License

This project is licensed under Apache License 2.0. See [LICENSE](LICENSE).