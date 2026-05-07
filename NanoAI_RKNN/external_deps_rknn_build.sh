#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Edit these paths to your local dependency locations.
TOOLCHAIN_FILE="${SCRIPT_DIR}/cmake/toolchains/rknn-aarch64-gcc.cmake"
NANOAIFLOW_ROOT="${REPO_ROOT}/out/install/nanoai_flow"

RKNN_RUNTIME_INCLUDE_DIR="${SCRIPT_DIR}/3rdparty/rknn/librknn_api/include"
RKNN_RUNTIME_LIBRARY_DIR="${SCRIPT_DIR}/3rdparty/rknn/librknn_api/aarch64"
RGA_ROOT="${SCRIPT_DIR}/3rdparty/rknn/librga"
OPENCV_DIR="${SCRIPT_DIR}/3rdparty/opencv/build_linux_aarch64/install/lib/cmake/opencv4"
STB_IMAGE_INCLUDE_DIR="${SCRIPT_DIR}/3rdparty/rknn/stb_image"
JPEG_TURBO_ROOT="${SCRIPT_DIR}/3rdparty/rknn/jpeg_turbo"
UTILS_ROOT="${SCRIPT_DIR}/3rdparty/rknn/utils"

BUILD_DIR="${SCRIPT_DIR}/build_external"
NANOAI_RKNN_BUILD_EXAMPLES="OFF"
NANOAI_RKNN_EXAMPLES="ALL"
JOBS="$(nproc 2>/dev/null || echo 4)"

print_help() {
  cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Build NanoAI_RKNN external dependencies and project targets.

Options:
  -B, --build-dir <dir>    Build directory (default: ./build_external)
  -j, --jobs <N>           Parallel build jobs (default: nproc)
  -h, --help               Show this help message and exit.

Current defaults:
  BUILD_DIR                   ${BUILD_DIR}
  JOBS                        ${JOBS}
  NANOAI_RKNN_BUILD_EXAMPLES  ${NANOAI_RKNN_BUILD_EXAMPLES}
  NANOAI_RKNN_EXAMPLES        ${NANOAI_RKNN_EXAMPLES}
  TOOLCHAIN_FILE              ${TOOLCHAIN_FILE}
  NANOAIFLOW_ROOT             ${NANOAIFLOW_ROOT}
  RKNN_RUNTIME_INCLUDE_DIR    ${RKNN_RUNTIME_INCLUDE_DIR}
  RKNN_RUNTIME_LIBRARY_DIR    ${RKNN_RUNTIME_LIBRARY_DIR}
  RGA_ROOT                    ${RGA_ROOT}
  OPENCV_DIR                  ${OPENCV_DIR}
  STB_IMAGE_INCLUDE_DIR       ${STB_IMAGE_INCLUDE_DIR}
  JPEG_TURBO_ROOT             ${JPEG_TURBO_ROOT}
  UTILS_ROOT                  ${UTILS_ROOT}

Examples:
  ./external_deps_rknn_build.sh
  ./external_deps_rknn_build.sh -B ./build_external_custom
  ./external_deps_rknn_build.sh -j 8
  ./external_deps_rknn_build.sh -B ./build_external_custom -j 8
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -B|--build-dir)
      BUILD_DIR="${2:?Missing value for $1}"
      shift 2
      ;;
    -j|--jobs)
      JOBS="${2:?Missing value for $1}"
      shift 2
      ;;
    -h|--help)
      print_help
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      print_help
      exit 1
      ;;
  esac
done

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
  -DNANOAI_RKNN_BUILD_EXAMPLES="${NANOAI_RKNN_BUILD_EXAMPLES}" \
  -DNANOAI_RKNN_EXAMPLES="${NANOAI_RKNN_EXAMPLES}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DNANOAIFLOW_ROOT="${NANOAIFLOW_ROOT}" \
  -DNANOAI_RKNN_RUNTIME_INCLUDE_DIR="${RKNN_RUNTIME_INCLUDE_DIR}" \
  -DNANOAI_RKNN_RUNTIME_LIBRARY_DIR="${RKNN_RUNTIME_LIBRARY_DIR}" \
  -DNANOAI_RKNN_RGA_ROOT="${RGA_ROOT}" \
  -DNANOAI_RKNN_OPENCV_DIR="${OPENCV_DIR}" \
  -DNANOAI_RKNN_STB_IMAGE_INCLUDE_DIR="${STB_IMAGE_INCLUDE_DIR}" \
  -DNANOAI_RKNN_JPEG_TURBO_ROOT="${JPEG_TURBO_ROOT}" \
  -DNANOAI_RKNN_UTILS_ROOT="${UTILS_ROOT}"

cmake --build "${BUILD_DIR}" -j"${JOBS}"

echo "Done. Build output: ${BUILD_DIR}"

echo "[INFO] Installing NanoAI_RKNN package..."
cmake --install "${BUILD_DIR}"
