# NanoAI

English | [中文](README_zh.md)


NanoAI is a multi-module C++ repository for high-performance inference and data processing. All modules are always installed as reusable CMake packages (via `find_package`), and users can optionally build examples for learning and testing. All dependencies are linked via INTERFACE, and the build/install/example paradigm is unified across all submodules.


## Core Modules

### NanoAIFlow
- High-performance, header-only concurrent pipeline framework for inference/data processing
- Current runtime is implemented with C++20 standard concurrency primitives, with no third-party thread-pool dependency
- `NanoPipeLine` supports copy and move construction for value-style pipeline usage
- Always installed as a CMake package, examples are optional (`-DNANOAIFLOW_BUILD_EXAMPLES=ON`)
- All dependencies INTERFACE linked
- See [NanoAIFlow/README.md](NanoAIFlow/README.md) and [NanoAIFlow/README_zh.md](NanoAIFlow/README_zh.md)

### NanoAI_RKNN
- C++ inference framework for Rockchip RKNN deployment
- Deep integration with NanoAIFlow, always installed as a CMake package, examples optional
- All dependencies INTERFACE linked
- See [NanoAI_RKNN/README.md](NanoAI_RKNN/README.md) and [NanoAI_RKNN/README_zh.md](NanoAI_RKNN/README_zh.md)

### NanoAI_NCNN
- C++ inference framework for NCNN-based edge deployment
- Multi-platform (Linux aarch64, Android), always installed as a CMake package, examples optional
- All dependencies INTERFACE linked
- See [NanoAI_NCNN/README.md](NanoAI_NCNN/README.md) and [NanoAI_NCNN/README_zh.md](NanoAI_NCNN/README_zh.md)


## Recommended Usage

1. Build and install NanoAIFlow first (always install as a framework).
2. Build and install NanoAI_RKNN or NanoAI_NCNN, setting `CMAKE_PREFIX_PATH`, `NANOAIFLOW_ROOT`, or `NanoAIFlow_DIR` to the NanoAIFlow install location.
3. Enable examples if needed via `-D<module>_BUILD_EXAMPLES=ON`.
4. All dependencies are INTERFACE linked; no need to manually link system libraries if using CMake package mode.
5. Choose the proper toolchain configuration for the target platform as needed.


## Monorepo Entry Points

The repository provides unified build entry points:

- [CMakeLists.txt](CMakeLists.txt)
- [CMakePresets.json](CMakePresets.json)

Example:

```bash
# 1. Configure and build NanoAIFlow (install framework)
cmake --preset flow-package
cmake --build --preset flow-package

# 2. Configure and build RKNN (install framework, set NanoAIFlow path)
export NANOAIFLOW_ROOT=/path/to/NanoAIFlow/install
cmake --preset rknn-package
cmake --build --preset rknn-package
```


## Documentation

- [General Comment Guidelines](docs/comment_guidelines.md)
- [NanoAIFlow English README](NanoAIFlow/README.md)
- [NanoAIFlow Chinese README](NanoAIFlow/README_zh.md)
- [NanoAI_RKNN English README](NanoAI_RKNN/README.md)
- [NanoAI_RKNN Chinese README](NanoAI_RKNN/README_zh.md)
- [NanoAI_NCNN English README](NanoAI_NCNN/README.md)
- [NanoAI_NCNN Chinese README](NanoAI_NCNN/README_zh.md)

## License

This project is licensed under Apache License 2.0. See [LICENSE](LICENSE).