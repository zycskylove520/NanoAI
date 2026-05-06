# NanoAI_RKNN 构建与选项手册（中文）

本文档用于说明：

1. 有哪些可配置项。
2. 如何构建第三方包（`PACKAGE`）。
3. 如何构建项目（`PROJECTS`）。
4. 如何选择性编译 `projects/` 子项目。

## 1. 构建模式

`NanoAI_RKNN` 通过 `NANOAI_RKNN_BUILD_MODE` 选择模式，二选一：

- `PACKAGE`：生成并安装可复用 CMake 包（支持 `find_package`）。
- `PROJECTS`：构建 `projects/` 下项目。

默认值：`PACKAGE`

## 2. 顶层 CMake 主要选项

### 2.1 模式与路径

- `NANOAI_RKNN_BUILD_MODE`：`PACKAGE` / `PROJECTS`
- `NANOAI_RKNN_PROJECTS`：`ALL` / `NONE` / `projA;projB`（仅 `PROJECTS` 模式有效）
- `NANOAIFLOW_ROOT`：NanoAIFlow 安装前缀（可选）
- `NANOAI_RKNN_INSTALL_CMAKEDIR`：安装时导出 CMake 配置文件目录

### 2.2 第三方依赖开关

来自 `cmake/NanoAIRKNNThirdParty.cmake`：

- `NANOAI_RKNN_WITH_RKNN_RUNTIME`
- `NANOAI_RKNN_WITH_RGA`
- `NANOAI_RKNN_WITH_OPENCV`
- `NANOAI_RKNN_WITH_STB_IMAGE`
- `NANOAI_RKNN_WITH_JPEG_TURBO`
- `NANOAI_RKNN_WITH_UTILS`

说明：当前工程仅支持 ARM（`aarch64/arm64`）交叉编译，上述开关在构建中被强制启用。

### 2.3 第三方路径变量（外部传入）

- `NANOAI_RKNN_RUNTIME_INCLUDE_DIR`
- `NANOAI_RKNN_RUNTIME_LIBRARY_DIR`
- `NANOAI_RKNN_RGA_ROOT`
- `NANOAI_RKNN_OPENCV_DIR`
- `NANOAI_RKNN_STB_IMAGE_INCLUDE_DIR`
- `NANOAI_RKNN_JPEG_TURBO_ROOT`
- `NANOAI_RKNN_UTILS_ROOT`

## 3. 方案 A：构建并安装第三方包（PACKAGE）

### 3.1 配置与安装

先准备 NanoAIFlow 依赖（任选一种）：

- 通过 `-DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install`
- 或通过 `-DCMAKE_PREFIX_PATH=/path/to/NanoAIFlow/install`
- 或通过 `-DNanoAIFlow_DIR=/path/to/NanoAIFlow/install/lib/cmake/NanoAIFlow`

```bash
cmake -S NanoAI_RKNN -B NanoAI_RKNN/build_pkg \
  -DNANOAI_RKNN_BUILD_MODE=PACKAGE \
  -DCMAKE_TOOLCHAIN_FILE=NanoAI_RKNN/cmake/toolchains/rknn-aarch64-gcc.cmake \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_RKNN_RUNTIME_INCLUDE_DIR=/path/to/librknn_api/include \
  -DNANOAI_RKNN_RUNTIME_LIBRARY_DIR=/path/to/librknn_api/aarch64 \
  -DNANOAI_RKNN_RGA_ROOT=/path/to/librga \
  -DNANOAI_RKNN_OPENCV_DIR=/path/to/opencv4/cmake \
  -DNANOAI_RKNN_STB_IMAGE_INCLUDE_DIR=/path/to/stb_image \
  -DNANOAI_RKNN_JPEG_TURBO_ROOT=/path/to/jpeg_turbo \
  -DNANOAI_RKNN_UTILS_ROOT=/path/to/utils \
  -DCMAKE_INSTALL_PREFIX=/your/install/prefix

cmake --build NanoAI_RKNN/build_pkg -j
cmake --install NanoAI_RKNN/build_pkg
```

也可使用 `NanoAI_RKNN/external_deps_rknn_build.sh`，修改脚本顶部路径变量后一键配置与构建。

安装后会导出：

- `NanoAI_RKNNConfig.cmake`
- `NanoAI_RKNNConfigVersion.cmake`
- `NanoAI_RKNNTargets.cmake`

### 3.2 调用方使用示例

```cmake
find_package(NanoAI_RKNN CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE NanoAI_RKNN::NanoAI_RKNN)
```

若安装前缀不在系统默认搜索路径：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/your/install/prefix
```

## 4. 方案 B：构建项目（PROJECTS）

### 4.1 选择性编译

`NANOAI_RKNN_PROJECTS` 可选：

- `ALL`：构建 `projects/` 下所有项目
- `NONE`：不构建任何项目
- `name1;name2`：仅构建指定项目

并且会自动生成项目开关，例如：

- `NANOAI_ENABLE_PROJECT_TEST_PROJECT`

### 4.2 示例命令

仅构建 `test_project`：

```bash
cmake -S NanoAI_RKNN -B NanoAI_RKNN/build_proj \
  -DNANOAI_RKNN_BUILD_MODE=PROJECTS \
  -DCMAKE_TOOLCHAIN_FILE=NanoAI_RKNN/cmake/toolchains/rknn-aarch64-gcc.cmake \
  -DNANOAIFLOW_ROOT=/path/to/NanoAIFlow/install \
  -DNANOAI_RKNN_PROJECTS=test_project

cmake --build NanoAI_RKNN/build_proj -j
```

构建全部项目：

```bash
cmake -S NanoAI_RKNN -B NanoAI_RKNN/build_proj_all \
  -DNANOAI_RKNN_BUILD_MODE=PROJECTS \
  -DCMAKE_TOOLCHAIN_FILE=NanoAI_RKNN/cmake/toolchains/rknn-aarch64-gcc.cmake \
  -DNANOAI_RKNN_PROJECTS=ALL

cmake --build NanoAI_RKNN/build_proj_all -j
```

## 5. 交叉编译建议

所有交叉编译参数建议在根配置阶段传入：

- `CMAKE_TOOLCHAIN_FILE`
- `CMAKE_SYSROOT`
- `CMAKE_PREFIX_PATH`
- `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER`

示例：

```bash
cmake -S NanoAI_RKNN -B NanoAI_RKNN/build_cross \
  -DNANOAI_RKNN_BUILD_MODE=PROJECTS \
  -DNANOAI_RKNN_PROJECTS=test_project \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
  -DCMAKE_SYSROOT=/path/to/sysroot \
  -DCMAKE_PREFIX_PATH=/path/to/deps

cmake --build NanoAI_RKNN/build_cross --target NanoAI_rknn_demo -j
```

## 6. 已知行为与常见问题

### 6.1 子项目独立构建

`projects/test_project` 不支持独立配置，必须从 `NanoAI_RKNN` 根目录进入。

### 6.2 模式互斥

`PACKAGE` 与 `PROJECTS` 为互斥模式，不能同时启用。

### 6.3 依赖缺失排查

如果遇到头文件或链接符号缺失，请优先检查：

1. 是否在根目录配置并传入了正确 toolchain/sysroot。
2. 第三方开关是否满足依赖关系（例如 `UTILS` 依赖 `RGA`）。
3. `NANOAIFLOW_ROOT` 是否指向正确安装目录。
  也可直接传 `CMAKE_PREFIX_PATH` 或 `NanoAIFlow_DIR`。
4. 是否使用了 `aarch64/arm64` 交叉工具链（本工程不支持非 ARM 或非交叉编译）。
