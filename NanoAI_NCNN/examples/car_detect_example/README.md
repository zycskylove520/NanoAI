# car_detect_example (NanoAI_NCNN)

该目录是车检示例的 NCNN 版本，采用 NanoAIFlow 的 pipe 方式组织：

- LoadPipe：加载 NCNN 模型
- PreprocessPipe：图像 letterbox + RGB + normalize
- InferPipe：执行 NCNN 推理
- PostprocessPipe：将检测结果解码为车辆结构

命名空间统一使用 `NanoAI_NCNN`。

## 目录结构

- include/
  业务类型定义、后处理与 pipeline 组装。
- src/main.cpp
  示例入口，读取图片并保存绘制结果。
- model/
  模型目录（放置 `car_detect.param` 与 `car_detect.bin`）。

## 构建

从 `NanoAI_NCNN` 根目录执行：

```bash
cmake -S . -B build_example \
  -DNANOAI_NCNN_BUILD_EXAMPLES=ON \
  -DNANOAI_NCNN_EXAMPLES=car_detect_example

cmake --build build_example -j
```

## 运行

可执行文件名为 `NanoAI_ncnn_car_detect_example`。

```bash
./build_example/bin/NanoAI_ncnn_car_detect_example \
  --param /path/to/car_detect.param \
  --bin /path/to/car_detect.bin \
  --image /path/to/input.jpg \
  --output /path/to/output.jpg
```

