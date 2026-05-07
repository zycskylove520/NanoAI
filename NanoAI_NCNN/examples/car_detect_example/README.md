# car_project (NanoAI_NCNN)

该目录是 Android 车检项目的 NCNN 迁移版，采用 NanoAIFlow 的 pipe 方式组织为：

- LoadPipe：加载 NCNN 模型
- PreprocessPipe：图像 letterbox + RGB + normalize
- InferPipe：执行 NCNN 推理
- PostprocessPipe：解码检测框到车辆结构

命名空间统一使用：`NanoAI_NCNN`。

## 目录结构

- include/nanoai_ncnn/projects/car_project
: 业务类型定义、后处理、pipeline 组装。
- src/car_demo_main.cpp
: 示例入口，读取图片并保存绘制结果。
- model/
: 模型目录（需放置 `car_detect.param` 与 `car_detect.bin`）。

如果你从其他仓库迁移该项目，请将原项目对应模型文件复制到当前 `model/` 目录。

## 构建

从 `NanoAI_NCNN` 根目录执行：

```bash
cmake -S . -B build_proj \
  -DNANOAI_NCNN_BUILD_MODE=PROJECTS \
  -DNANOAI_NCNN_PROJECTS=car_project

cmake --build build_proj -j
```

## 运行

```bash
./build_proj/bin/NanoAI_ncnn_car_project \
  --param /path/to/car_detect.param \
  --bin /path/to/car_detect.bin \
  --image /path/to/input.jpg \
  --output /path/to/output.jpg
```

