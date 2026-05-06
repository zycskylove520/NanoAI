#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Edit these paths to your local dependency locations.
ANDROID_NDK_PATH="${SCRIPT_DIR}/3rdparty/android-ndk-r25c"
NCNN_INCLUDE_DIR="${SCRIPT_DIR}/3rdparty/ncnn/android-aarch64/install/include"
NCNN_LIBRARY_DIR="${SCRIPT_DIR}/3rdparty/ncnn/android-aarch64/install/lib"
OPENCV_DIR="${SCRIPT_DIR}/3rdparty/OpenCV-android-sdk/sdk/native/jni"
NANOAIFLOW_ANDROID_PREFIX="${REPO_ROOT}/out/install/flow"

# PACKAGE is recommended for first validation.
# Change to PROJECTS if you want to build deploy targets.
NANOAI_NCNN_BUILD_MODE="PACKAGE"
NANOAI_NCNN_PROJECTS="car_project"

BUILD_DIR="${SCRIPT_DIR}/build_android_external"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
  -DNANOAI_NCNN_BUILD_MODE="${NANOAI_NCNN_BUILD_MODE}" \
  -DNANOAI_NCNN_PROJECTS="${NANOAI_NCNN_PROJECTS}" \
  -DNANOAI_NCNN_ANDROID_PRESET=ON \
  -DNANOAI_NCNN_ANDROID_NDK_PATH="${ANDROID_NDK_PATH}" \
  -DNANOAI_NCNN_ANDROID_ABI=arm64-v8a \
  -DNANOAI_NCNN_ANDROID_PLATFORM=android-26 \
  -DNANOAI_NCNN_NCNN_INCLUDE_DIR="${NCNN_INCLUDE_DIR}" \
  -DNANOAI_NCNN_NCNN_LIBRARY_DIR="${NCNN_LIBRARY_DIR}" \
  -DNANOAI_NCNN_OPENCV_DIR="${OPENCV_DIR}" \
  -DNANOAIFLOW_ROOT_ANDROID="${NANOAIFLOW_ANDROID_PREFIX}"

cmake --build "${BUILD_DIR}" -j

echo "Done. Build output: ${BUILD_DIR}"
