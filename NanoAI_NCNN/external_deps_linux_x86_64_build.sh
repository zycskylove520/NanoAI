#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Edit these paths to your local dependency locations.
TOOLCHAIN_FILE="${SCRIPT_DIR}/cmake/toolchains/ncnn-linux-x86_64-gcc.cmake"
NCNN_INCLUDE_DIR="${SCRIPT_DIR}/3rdparty/ncnn/ubuntu-x86_64/install/include"
NCNN_LIBRARY_DIR="${SCRIPT_DIR}/3rdparty/ncnn/ubuntu-x86_64/install/lib"
NANOAI_NCNN_OPENCV_DIR=""  # 不填则自动寻找linux系统上的opencv

# NanoAI_NCNN is always package-installable. Examples are optional.
NANOAI_NCNN_BUILD_EXAMPLES="OFF"
NANOAI_NCNN_EXAMPLES="ALL"

BUILD_DIR="${SCRIPT_DIR}/build_linux_external"
JOBS="$(nproc 2>/dev/null || echo 4)"

NANOAIFLOW_CMAKE_DIR=""

detect_nanoaiflow_cmake_dir() {
  local config_file=""

  for config_file in \
    "/usr/local/lib/cmake/NanoAIFlow/NanoAIFlowConfig.cmake" \
    "/usr/local/lib64/cmake/NanoAIFlow/NanoAIFlowConfig.cmake" \
    "/usr/lib/cmake/NanoAIFlow/NanoAIFlowConfig.cmake" \
    "/usr/lib64/cmake/NanoAIFlow/NanoAIFlowConfig.cmake"; do
    if [[ -f "${config_file}" ]]; then
      NANOAIFLOW_CMAKE_DIR="$(dirname "${config_file}")"
      return 0
    fi
  done

  NANOAIFLOW_CMAKE_DIR=""
  return 1
}

print_help() {
  cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Build NanoAI_NCNN Linux x86_64 external dependencies and project targets.

Options:
  -B, --build-dir <dir>    Build directory (default: ./build_linux_external)
  -j, --jobs <N>           Parallel build jobs (default: nproc)
  -h, --help               Show this help message and exit.

Current defaults:
  BUILD_DIR                 ${BUILD_DIR}
  JOBS                      ${JOBS}
  NANOAI_NCNN_BUILD_EXAMPLES ${NANOAI_NCNN_BUILD_EXAMPLES}
  NANOAI_NCNN_EXAMPLES      ${NANOAI_NCNN_EXAMPLES}
  TOOLCHAIN_FILE            ${TOOLCHAIN_FILE}
  NCNN_INCLUDE_DIR          ${NCNN_INCLUDE_DIR}
  NCNN_LIBRARY_DIR          ${NCNN_LIBRARY_DIR}
  NANOAI_NCNN_OPENCV_DIR    ${NANOAI_NCNN_OPENCV_DIR}

Examples:
  ./external_deps_linux_build.sh
  ./external_deps_linux_build.sh -B ./build_linux_custom
  ./external_deps_linux_build.sh -j 8
  ./external_deps_linux_build.sh -B ./build_linux_custom -j 8
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

# Ensure NanoAIFlow package is available for find_package(NanoAIFlow).
if ! detect_nanoaiflow_cmake_dir; then
  echo "[INFO] NanoAIFlow package config not found in common system paths."
  echo "[INFO] Trying to build/install NanoAIFlow automatically..."

  FLOW_BUILD_SCRIPT="${REPO_ROOT}/NanoAIFlow/external_deps_flow_build.sh"
  if [[ -x "${FLOW_BUILD_SCRIPT}" ]]; then
    "${FLOW_BUILD_SCRIPT}"
  else
    if [[ -f "${FLOW_BUILD_SCRIPT}" ]]; then
      chmod +x "${FLOW_BUILD_SCRIPT}"
      "${FLOW_BUILD_SCRIPT}"
    else
      echo "[ERROR] Missing NanoAIFlow build script: ${FLOW_BUILD_SCRIPT}" >&2
      echo "[ERROR] Please build NanoAIFlow manually first." >&2
      exit 1
    fi
  fi
fi

if ! detect_nanoaiflow_cmake_dir; then
  echo "[ERROR] NanoAIFlowConfig.cmake still not found after auto build/install." >&2
  echo "[ERROR] Expected one of: /usr/local/lib/cmake/NanoAIFlow or /usr/lib/cmake/NanoAIFlow (including lib64)." >&2
  exit 1
fi

echo "[INFO] Using NanoAIFlow_DIR=${NANOAIFLOW_CMAKE_DIR}"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
  -DNANOAI_NCNN_BUILD_EXAMPLES="${NANOAI_NCNN_BUILD_EXAMPLES}" \
  -DNANOAI_NCNN_EXAMPLES="${NANOAI_NCNN_EXAMPLES}" \
  -DNANOAI_NCNN_LINUX_x86_64_PRESET=ON \
  -DNANOAI_NCNN_NCNN_INCLUDE_DIR="${NCNN_INCLUDE_DIR}" \
  -DNANOAI_NCNN_NCNN_LIBRARY_DIR="${NCNN_LIBRARY_DIR}" \
  -DNANOAI_NCNN_OPENCV_DIR="${NANOAI_NCNN_OPENCV_DIR}" \
  -DNanoAIFlow_DIR="${NANOAIFLOW_CMAKE_DIR}"

cmake --build "${BUILD_DIR}" -j"${JOBS}"

echo "Done. Build output: ${BUILD_DIR}"

echo "[INFO] Installing NanoAI_NCNN package..."
cmake --install "${BUILD_DIR}"
