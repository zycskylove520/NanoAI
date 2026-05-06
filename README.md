# NanoAI

NanoAI 是一个多模块 C++ 推理工程仓库，包含：

- `NanoAIFlow`：流程编排基础组件（header-only）
- `NanoAI_RKNN`：RKNN 平台模块
- `NanoAI_NCNN`：NCNN 平台模块

设计目标：开发者可以只下载单个模块使用，也可以使用本仓库进行统一管理与构建。

说明：对外发布范围不包含 NanoAI_MNN。

## 子模块导航

- NanoAIFlow
  - 模块说明与快速命令: [NanoAIFlow/README.md](NanoAIFlow/README.md)
  - 打包与 find_package 指南: [NanoAIFlow/docs/find_package_and_packaging_guide.md](NanoAIFlow/docs/find_package_and_packaging_guide.md)
  - 设计理念: [NanoAIFlow/docs/nanoai_design_philosophy.md](NanoAIFlow/docs/nanoai_design_philosophy.md)
  - Pipe 使用说明: [NanoAIFlow/docs/pipe_usage_guide.md](NanoAIFlow/docs/pipe_usage_guide.md)

- NanoAI_RKNN
  - 模块说明与快速命令: [NanoAI_RKNN/README.md](NanoAI_RKNN/README.md)
  - 打包与 find_package 指南: [NanoAI_RKNN/docs/find_package_and_packaging_guide.md](NanoAI_RKNN/docs/find_package_and_packaging_guide.md)

- NanoAI_NCNN
  - 模块说明与快速命令: [NanoAI_NCNN/README.md](NanoAI_NCNN/README.md)
  - 打包与 find_package 指南: [NanoAI_NCNN/docs/find_package_and_packaging_guide.md](NanoAI_NCNN/docs/find_package_and_packaging_guide.md)

## 单模块使用（推荐给外部开发者）

外部开发者可只下载目标模块目录（或单独仓库镜像）并按各模块 README 构建。

依赖关系：

- `NanoAI_RKNN` 依赖已安装的 `NanoAIFlow`
- `NanoAI_NCNN` 依赖已安装的 `NanoAIFlow`

典型流程：

1. 先构建并安装 `NanoAIFlow`。
2. 构建 `NanoAI_RKNN` 或 `NanoAI_NCNN` 时，通过 `CMAKE_PREFIX_PATH` 或 `NanoAIFlow_DIR` 指向 Flow 安装目录。

## Monorepo 统一构建入口

仓库根目录提供了：

- 顶层聚合入口: [CMakeLists.txt](CMakeLists.txt)
- 标准预设: [CMakePresets.json](CMakePresets.json)

示例：

```bash
# 1) 配置并构建 NanoAIFlow
cmake --preset flow-package
cmake --build --preset flow-package

# 2) 配置并构建 RKNN（需先设置 NanoAIFlow 安装路径）
export NANOAIFLOW_ROOT=/path/to/NanoAIFlow/install
cmake --preset rknn-package
cmake --build --preset rknn-package
```

注意：`rknn-package`、`ncnn-package-host`、`ncnn-projects-android` 预设都依赖外部三方路径变量。推荐优先使用模块内脚本：

- `NanoAI_RKNN/external_deps_rknn_build.sh`
- `NanoAI_NCNN/external_deps_android_build.sh`

## 建议阅读顺序

1. 对应子模块 README（快速命令与构建入口）
2. 对应子模块 docs（详细选项与排障）

## 开源协作

- 贡献指南: [CONTRIBUTING.md](CONTRIBUTING.md)
- Issue 模板: [.github/ISSUE_TEMPLATE](.github/ISSUE_TEMPLATE)
- PR 模板: [.github/PULL_REQUEST_TEMPLATE.md](.github/PULL_REQUEST_TEMPLATE.md)
