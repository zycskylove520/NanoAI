
# NanoAI

[English](README.md) | 中文

NanoAI 是一个多模块高性能推理与数据处理 C++ 工程仓库。所有子模块均始终以可安装 CMake 包（通过 `find_package`）的形式交付，用户可选构建 examples 进行学习和测试。所有依赖均 INTERFACE 链接，构建/安装/示例范式在各子模块中完全统一。

## 核心模块

### NanoAIFlow
- 高性能、header-only 并发流程编排框架，适合推理/数据处理
- 当前运行时基于 C++20 标准并发原语实现，不依赖第三方线程池
- `NanoPipeLine` 支持拷贝构造与移动构造
- 始终以 CMake 包安装，examples 可选（`-DNANOAIFLOW_BUILD_EXAMPLES=ON`）
- 所有依赖 INTERFACE 链接
- 详见 [NanoAIFlow/README.md](NanoAIFlow/README.md) 和 [NanoAIFlow/README_zh.md](NanoAIFlow/README_zh.md)

### NanoAI_RKNN
- Rockchip RKNN 推理框架，深度集成 NanoAIFlow
- 始终以 CMake 包安装，examples 可选
- 所有依赖 INTERFACE 链接
- 详见 [NanoAI_RKNN/README.md](NanoAI_RKNN/README.md) 和 [NanoAI_RKNN/README_zh.md](NanoAI_RKNN/README_zh.md)

### NanoAI_NCNN
- NCNN 端侧推理框架，支持多平台（Linux aarch64、Android）
- 始终以 CMake 包安装，examples 可选
- 所有依赖 INTERFACE 链接
- 详见 [NanoAI_NCNN/README.md](NanoAI_NCNN/README.md) 和 [NanoAI_NCNN/README_zh.md](NanoAI_NCNN/README_zh.md)

## 推荐用法

1. 先构建并安装 NanoAIFlow。
2. 再构建并安装 NanoAI_RKNN 或 NanoAI_NCNN，设置 `CMAKE_PREFIX_PATH`、`NANOAIFLOW_ROOT` 或 `NanoAIFlow_DIR` 指向 Flow 安装目录。
3. 如需示例，使用 `-D<模块>_BUILD_EXAMPLES=ON` 启用。
4. 所有依赖均 INTERFACE 链接，使用 CMake 包模式无需手动链接系统库。
5. 按需选择目标平台和 toolchain 配置。

## Monorepo 构建入口

仓库根目录提供统一入口：

- [CMakeLists.txt](CMakeLists.txt)
- [CMakePresets.json](CMakePresets.json)

示例：

```bash
# 1. 配置并构建 NanoAIFlow
cmake --preset flow-package
cmake --build --preset flow-package

# 2. 配置并构建 RKNN（需设置 NanoAIFlow 路径）
export NANOAIFLOW_ROOT=/path/to/NanoAIFlow/install
cmake --preset rknn-package
cmake --build --preset rknn-package
```

## 文档导航

- [通用注释规范](docs/comment_guidelines.md)
- [NanoAIFlow 英文说明](NanoAIFlow/README.md)
- [NanoAIFlow 中文说明](NanoAIFlow/README_zh.md)
- [NanoAI_RKNN 英文说明](NanoAI_RKNN/README.md)
- [NanoAI_RKNN 中文说明](NanoAI_RKNN/README_zh.md)
- [NanoAI_NCNN 英文说明](NanoAI_NCNN/README.md)
- [NanoAI_NCNN 中文说明](NanoAI_NCNN/README_zh.md)

## 许可证

本项目采用 Apache License 2.0，详见 [LICENSE](LICENSE)。
