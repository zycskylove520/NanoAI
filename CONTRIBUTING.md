# Contributing to NanoAI

感谢你为 NanoAI 做贡献。

本仓库由多个模块组成：

- NanoAIFlow
- NanoAI_RKNN
- NanoAI_NCNN

请优先保证改动最小、可复现、可验证。

## 开始之前

1. Fork 并克隆仓库。
2. 在分支上开发，不要直接在 main 提交。
3. 阅读模块对应 README 与 docs，确认改动范围。

建议分支命名：

- feat/<short-topic>
- fix/<short-topic>
- docs/<short-topic>
- ci/<short-topic>

## 提交规范

建议采用简洁的 Conventional Commits 风格：

- feat: add xxx
- fix: resolve xxx
- docs: update xxx
- ci: adjust xxx
- refactor: improve xxx

每次提交建议只解决一个问题，避免混入无关改动。

## 本地验证

至少完成以下检查中的相关项：

1. Flow 可配置与可构建。
2. RKNN 在提供 NanoAIFlow 前缀后可完成 configure。
3. NCNN 在提供 NanoAIFlow 前缀后可完成 configure。

参考命令：

1) Flow:

cmake --preset flow-package
cmake --build --preset flow-package -j
cmake --install out/build/flow-package --prefix /path/to/flow-install

2) RKNN configure:

cmake -S NanoAI_RKNN -B out/build/rknn-local \
  -DNANOAI_RKNN_BUILD_MODE=PACKAGE \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/rknn-aarch64-gcc.cmake \
  -DNANOAIFLOW_ROOT=/path/to/flow-install

3) NCNN configure:

cmake -S NanoAI_NCNN -B out/build/ncnn-local \
  -DNANOAI_NCNN_BUILD_MODE=PACKAGE \
  -DNANOAIFLOW_ROOT=/path/to/flow-install

## 代码与文档要求

1. 不要引入机器私有绝对路径。
2. 保持模块独立可用，不破坏单模块构建方式。
3. 文档示例使用通用占位路径，例如 /path/to/xxx。
4. 修改构建逻辑时，更新对应 README 或 docs。

## Pull Request 要求

1. 说明改动目的与影响范围。
2. 给出本地验证步骤与结果。
3. 若涉及构建选项变更，列出新增或修改的 CMake 变量。
4. 若涉及行为变化，补充迁移说明。

感谢你的贡献。
