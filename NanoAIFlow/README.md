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
6. **Pipeline value semantics**: `NanoPipeLine` supports copy and move construction, making pass-by-value and factory returns straightforward.
7. **C++20 standard-library threading**: Uses a native C++20 thread-pool runtime built on standard synchronization primitives, with no third-party thread-pool dependency.
8. **Header-only, INTERFACE linkage**: Easy integration, no static/dynamic library required. All dependencies are INTERFACE linked.
9. **Installable and reusable**: Always installed as a CMake package, reusable via `find_package(NanoAIFlow CONFIG REQUIRED)`.
10. **Backpressure control**: Shared/dedicated pools support bounded queues and submit policies (`block`, `timeout`, `reject`) to prevent unbounded memory growth.
11. **Observability built in**: Runtime stats and event callbacks expose rejections, timeouts, and ordered-waiter overflow for production diagnosis.
12. **Rich engineering tests**: Includes correctness, ordering, performance, backpressure, and observability tests.

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
- `include/nanoai_flow/core/thread_pool.hpp`: native C++ thread-pool runtime
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

## Performance Snapshot

The current implementation was validated on the same machine used during the migration away from `BS::thread_pool`. Compared with the original third-party pool version, the current C++20 runtime improved representative benchmark throughput as follows:

- `test_pool_performance/shared_pool_all`: `31746.03 -> 70796.46` QPS
- `test_pool_performance/dedicated_pool_all`: `100000.00 -> 140350.88` QPS
- `test_pool_performance/mixed_pool_shared_dedicated_shared`: `38834.95 -> 84210.53` QPS
- `perf_benchmark/shared_pool_all`: `31347.96 -> 73800.74` QPS
- `perf_benchmark/dedicated_pool_all`: `109289.62 -> 151515.15` QPS
- `perf_benchmark/mode_unordered`: `49751.24 -> 150375.94` QPS

These numbers are hardware-dependent, but they confirm that the final native runtime outperforms both the original `BS::thread_pool` integration and the first replacement prototype on the validation host.


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