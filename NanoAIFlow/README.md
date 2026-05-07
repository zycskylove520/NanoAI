# NanoAIFlow

English | [中文](README_zh.md)

NanoAIFlow is a strongly typed concurrent pipeline framework for high-performance C++ inference and data processing workloads. It is not a runtime graph editor for arbitrary dynamic nodes. Instead, it is designed for fixed business pipelines where throughput, ordering semantics, and type safety matter more than runtime mutability.

## Core Strengths

### 1. Every pipe can run concurrently
- Each pipe stage can declare its own maximum concurrency
- Multiple stages inside one pipeline can process different requests at the same time
- This makes it much easier to utilize multi-core CPUs than a purely serial chain or a coarse global-thread-pool model

### 2. Multiple pipes advance concurrently in one pipeline
- The pipeline is not a barrier model where stage B waits for stage A to finish everything
- A single `run(...)` moves through stages, while different requests can flow through different stages in parallel
- This maps naturally to AI workloads such as Load, Preprocess, Infer, and Postprocess

### 3. Stable ordering under high concurrency
- Each stage can execute concurrently
- Forwarding between stages is still released in global sequence order
- You keep high throughput without forcing downstream code to repair random out-of-order behavior

### 4. Strongly typed static pipelines
- Pipe input and output types are defined directly by `on_run(...)`
- `NanoPipeLine<P1, P2, ...>` fixes the type flow at compile time
- There is no `std::any`-style runtime dispatch in the core path, which reduces runtime uncertainty and maintenance cost

### 5. Flexible execution policies
- `shared_pool`: efficient default choice for most stages
- `dedicated_pool`: isolates critical stages from the rest of the pipeline
- `inline_run`: removes scheduling overhead for very lightweight work

### 6. Header-only and easy to integrate
- Low integration cost for existing codebases
- Installable and reusable through `find_package(NanoAIFlow CONFIG REQUIRED)`
- Suitable for monorepos, modular C++ systems, and third-party package distribution

### 7. Built for real engineering workloads
- Includes concurrency correctness tests, ordering consistency tests, and performance benchmarks
- Supports tuple-based fan-out and automatic argument expansion between stages
- Works well as the flow foundation for RKNN, NCNN, and other inference modules

## Best-Fit Scenarios

- Edge AI pipelines for CV, NLP, and speech workloads
- Fixed-structure production chains with high throughput requirements
- Systems that must exploit multi-core concurrency without losing deterministic ordering semantics

## Design Principles

- Static typing first: push type errors to compile time whenever possible
- Stage-level concurrency: each stage controls its own concurrency and execution policy
- Ordered forwarding: preserve stable and explainable behavior under heavy concurrency
- Global quota control: avoid unbounded task accumulation across the whole pipeline

For more details, see:

- [docs/nanoai_design_philosophy.md](docs/nanoai_design_philosophy.md)
- [docs/pipe_usage_guide.md](docs/pipe_usage_guide.md)

## Repository Layout

- `include`: public headers
- `3rdparty/thread-pool`: thread pool dependency
- `examples`: example programs
- `test`: unit, concurrency, and performance tests
- `docs`: design and usage documentation

## Quick Start

### 1. Build the library only

```bash
cmake -S . -B build
cmake --build build -j
```

### 2. Build with tests enabled

```bash
cmake -S . -B build_test \
  -DNANOAIFLOW_BUILD_TESTS=ON

cmake --build build_test -j
ctest --test-dir build_test --output-on-failure
```

### 3. Build with examples enabled

```bash
cmake -S . -B build_example \
  -DNANOAIFLOW_BUILD_EXAMPLES=ON

cmake --build build_example -j
```

### 4. Install as a reusable CMake package

```bash
cmake -S . -B build_install \
  -DCMAKE_INSTALL_PREFIX=/your/install/prefix

cmake --build build_install -j
cmake --install build_install
```

## CMake Options

- `NANOAIFLOW_BUILD_TESTS`: build tests, default `OFF`
- `NANOAIFLOW_BUILD_EXAMPLES`: build examples, default `OFF`
- `NANOAIFLOW_ENABLE_CPACK`: enable CPack packaging, default `OFF`

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