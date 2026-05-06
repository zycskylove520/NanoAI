#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Edit these paths to your local dependency locations.
TOOLCHAIN_FILE="${SCRIPT_DIR}/cmake/toolchains/rknn-aarch64-gcc.cmake"
NANOAIFLOW_ROOT="${REPO_ROOT}/out/install/flow"

RKNN_RUNTIME_INCLUDE_DIR="${SCRIPT_DIR}/3rdparty/rknn/librknn_api/include"
RKNN_RUNTIME_LIBRARY_DIR="${SCRIPT_DIR}/3rdparty/rknn/librknn_api/aarch64"
RGA_ROOT="${SCRIPT_DIR}/3rdparty/rknn/librga"
OPENCV_DIR="${SCRIPT_DIR}/3rdparty/opencv/build_linux_aarch64/install/lib/cmake/opencv4"
STB_IMAGE_INCLUDE_DIR="${SCRIPT_DIR}/3rdparty/rknn/stb_image"
JPEG_TURBO_ROOT="${SCRIPT_DIR}/3rdparty/rknn/jpeg_turbo"
UTILS_ROOT="${SCRIPT_DIR}/3rdparty/rknn/utils"

BUILD_DIR="${SCRIPT_DIR}/build_external"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
  -DNANOAI_RKNN_BUILD_MODE=PACKAGE \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DNANOAIFLOW_ROOT="${NANOAIFLOW_ROOT}" \
  -DNANOAI_RKNN_RUNTIME_INCLUDE_DIR="${RKNN_RUNTIME_INCLUDE_DIR}" \
  -DNANOAI_RKNN_RUNTIME_LIBRARY_DIR="${RKNN_RUNTIME_LIBRARY_DIR}" \
  -DNANOAI_RKNN_RGA_ROOT="${RGA_ROOT}" \
  -DNANOAI_RKNN_OPENCV_DIR="${OPENCV_DIR}" \
  -DNANOAI_RKNN_STB_IMAGE_INCLUDE_DIR="${STB_IMAGE_INCLUDE_DIR}" \
  -DNANOAI_RKNN_JPEG_TURBO_ROOT="${JPEG_TURBO_ROOT}" \
  -DNANOAI_RKNN_UTILS_ROOT="${UTILS_ROOT}"

cmake --build "${BUILD_DIR}" -j

echo "Done. Build output: ${BUILD_DIR}"
