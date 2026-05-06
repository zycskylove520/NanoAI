# gaotiepaiwu (NanoAI_NCNN)

This directory is an NCNN migration of a legacy Android project.

It supports:

- Linux demo executable for image inference (pipeline style: Load/Preprocess/Infer/Postprocess)
- Android JNI shared library for Bitmap inference

Namespace used in this migration:

- NanoAI_NCNN::Projects::GaoTiePaiWu

## Layout

- include/nanoai_ncnn/projects/gaotiepaiwu
: detector headers and detection types.
- src/gaotiepaiwu_demo_main.cpp
: Linux demo entry.
- include/nanoai_ncnn/projects/gaotiepaiwu/paiwu_pipeline.hpp
: pipeline assembly for Linux demo.
- include/nanoai_ncnn/projects/gaotiepaiwu/paiwu_postprocess.hpp
: postprocess decode and NMS for pipeline output.
- src/paiwu_jni.h
: JNI interface declarations.
- src/paiwu_jni.cpp
: JNI interface implementation.
- src/paiwu_detector.cpp
: Linux detector wrapper based on pipeline.
- src/paiwu_detector_android.cpp
: Android detector implementation used by JNI build.
- model/
: model files (paiwu_detect.param and paiwu_detect.bin).

If you are migrating from another repository, copy the original model files into the local `model/` directory.

## Build (Linux demo)

Configure from NanoAI_NCNN root:

```bash
cmake -S . -B build_proj \
  -DNANOAI_NCNN_BUILD_MODE=PROJECTS \
  -DNANOAI_NCNN_PROJECTS=gaotiepaiwu

cmake --build build_proj -j
```

## Run (Linux demo)

```bash
./build_proj/bin/NanoAI_ncnn_gaotiepaiwu_project \
  --param /path/to/paiwu_detect.param \
  --bin /path/to/paiwu_detect.bin \
  --image /path/to/input.jpg \
  --output /path/to/output.jpg
```
