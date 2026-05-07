# NanoAIFlow

English | [中文](README_zh.md)


NanoAIFlow is a high-performance, header-only, strongly typed concurrent pipeline framework for C++ inference and data processing. It is always installed as a reusable CMake package (via `find_package`), and users can optionally build examples for learning and testing. All dependencies are linked via INTERFACE, making integration simple and robust.

**Key paradigm:**
- Always install as a framework (library)
- Examples are optional (enabled by `-DNANOAIFLOW_BUILD_EXAMPLES=ON`)
- All dependencies are INTERFACE linked

## Core Features

1. **Every pipe can run concurrently**: Each stage can declare its own concurrency, and multiple stages can process different requests in parallel, maximizing multi-core CPU utilization.
2. **Multiple pipes advance concurrently**: The pipeline is not a barrier model; different requests can flow through different stages in parallel, ideal for AI workloads (Load, Preprocess, Infer, Postprocess).
3. **Stable ordering under high concurrency**: Forwarding between stages is always in global sequence order, ensuring deterministic output.
4. **Strongly typed static pipelines**: Input/output types are defined by `on_run(...)`, and the pipeline type is fixed at compile time. No runtime type dispatch.
5. **Flexible execution policies**: Choose between `shared_pool`, `dedicated_pool`, and `inline_run` for each stage.
6. **Header-only, INTERFACE linkage**: Easy integration, no static/dynamic library required. All dependencies are INTERFACE linked.
7. **Installable and reusable**: Always installed as a CMake package, reusable via `find_package(NanoAIFlow CONFIG REQUIRED)`.
8. **Rich engineering tests**: Includes correctness, ordering, and performance tests; supports tuple-based fan-out and argument expansion.

## Typical Scenarios

- Edge AI pipelines (CV, NLP, speech)
- Fixed-structure, high-throughput production chains
- Multi-core concurrency with deterministic ordering

## Design Principles

- Static typing first: push type errors to compile time
- Stage-level concurrency: each stage controls its own concurrency and execution policy
- Ordered forwarding: stable, explainable behavior under concurrency
- Global quota control: avoid unbounded task accumulation

For more details, see:
- [docs/nanoai_design_philosophy.md](docs/nanoai_design_philosophy.md)
- [docs/pipe_usage_guide.md](docs/pipe_usage_guide.md)


## Repository Layout

- `include`: public headers
- `3rdparty/thread-pool`: thread pool dependency
- `examples`: optional example programs (build with `-DNANOAIFLOW_BUILD_EXAMPLES=ON`)
- `test`: unit, concurrency, and performance tests
- `docs`: design and usage documentation


## Quick Start

### 1. Build and install the framework only

```bash
cmake -S . -B build
cmake --build build -j
cmake --install build --prefix /your/install/prefix
```

### 2. Build with examples (optional)

```bash
cmake -S . -B build_example -DNANOAIFLOW_BUILD_EXAMPLES=ON
cmake --build build_example -j
```

### 3. Build with tests (optional)

```bash
cmake -S . -B build_test -DNANOAIFLOW_BUILD_TESTS=ON
cmake --build build_test -j
ctest --test-dir build_test --output-on-failure
```


## CMake Options

- `NANOAIFLOW_BUILD_EXAMPLES`: build examples (optional, default `OFF`)
- `NANOAIFLOW_BUILD_TESTS`: build tests (optional, default `OFF`)
- `NANOAIFLOW_ENABLE_CPACK`: enable CPack packaging (optional, default `OFF`)


## Consumer Example

```cmake
find_package(NanoAIFlow CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE NanoAI::Flow)
```


If the install prefix is not in a default search path:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/your/install/prefix
```


## Documentation

- [Chinese README](README_zh.md)
- [Packaging and find_package guide](docs/find_package_and_packaging_guide.md)
- [Design philosophy](docs/nanoai_design_philosophy.md)
- [Pipe usage guide](docs/pipe_usage_guide.md)


## License

This project is licensed under Apache License 2.0. See [LICENSE](LICENSE).