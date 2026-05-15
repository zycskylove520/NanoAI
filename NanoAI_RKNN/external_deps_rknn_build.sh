#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Edit these paths to your local dependency locations.
TOOLCHAIN_FILE="${SCRIPT_DIR}/cmake/toolchains/rknn-aarch64-gcc.cmake"

RKNN_RUNTIME_INCLUDE_DIR="${SCRIPT_DIR}/3rdparty/rknn/librknn_api/include"
RKNN_RUNTIME_LIBRARY_DIR="${SCRIPT_DIR}/3rdparty/rknn/librknn_api/aarch64"
RGA_ROOT="${SCRIPT_DIR}/3rdparty/rknn/librga"
OPENCV_DIR="${SCRIPT_DIR}/3rdparty/opencv/build_linux_aarch64/install/lib/cmake/opencv4"
STB_IMAGE_INCLUDE_DIR="${SCRIPT_DIR}/3rdparty/rknn/stb_image"
JPEG_TURBO_ROOT="${SCRIPT_DIR}/3rdparty/rknn/jpeg_turbo"
UTILS_ROOT="${SCRIPT_DIR}/3rdparty/rknn/utils"

BUILD_DIR="${SCRIPT_DIR}/build_external"
NANOAI_RKNN_BUILD_EXAMPLES="ON"
NANOAI_RKNN_EXAMPLES="ALL"
NANOAI_RKNN_ENABLE_CPACK="OFF"
ENABLE_INSTALL="ON"
INSTALL_PREFIX="/usr/local"
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

Build NanoAI_RKNN external dependencies and project targets.

Options:
  -B, --build-dir <dir>    Build directory (default: ./build_external)
  -j, --jobs <N>           Parallel build jobs (default: nproc)
  --install-prefix <dir>   Install prefix (default: /usr/local)
  --no-install             Build only, skip `cmake --install`
  --enable-cpack           Enable CPack package generation
  -h, --help               Show this help message and exit.

Current defaults:
  BUILD_DIR                   ${BUILD_DIR}
  JOBS                        ${JOBS}
  ENABLE_INSTALL              ${ENABLE_INSTALL}
  INSTALL_PREFIX              ${INSTALL_PREFIX}
  NANOAI_RKNN_BUILD_EXAMPLES  ${NANOAI_RKNN_BUILD_EXAMPLES}
  NANOAI_RKNN_EXAMPLES        ${NANOAI_RKNN_EXAMPLES}
  NANOAI_RKNN_ENABLE_CPACK    ${NANOAI_RKNN_ENABLE_CPACK}
  TOOLCHAIN_FILE              ${TOOLCHAIN_FILE}
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
  ./external_deps_rknn_build.sh --install-prefix /opt/nanoai_rknn
  ./external_deps_rknn_build.sh --enable-cpack
  ./external_deps_rknn_build.sh --no-install
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
    --install-prefix)
      INSTALL_PREFIX="${2:?Missing value for $1}"
      shift 2
      ;;
    --no-install)
      ENABLE_INSTALL="OFF"
      shift
      ;;
    --enable-cpack)
      NANOAI_RKNN_ENABLE_CPACK="ON"
      shift
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
  -DNANOAI_RKNN_BUILD_EXAMPLES="${NANOAI_RKNN_BUILD_EXAMPLES}" \
  -DNANOAI_RKNN_EXAMPLES="${NANOAI_RKNN_EXAMPLES}" \
  -DNANOAI_RKNN_ENABLE_CPACK="${NANOAI_RKNN_ENABLE_CPACK}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DNanoAIFlow_DIR="${NANOAIFLOW_CMAKE_DIR}" \
  -DNANOAI_RKNN_RUNTIME_INCLUDE_DIR="${RKNN_RUNTIME_INCLUDE_DIR}" \
  -DNANOAI_RKNN_RUNTIME_LIBRARY_DIR="${RKNN_RUNTIME_LIBRARY_DIR}" \
  -DNANOAI_RKNN_RGA_ROOT="${RGA_ROOT}" \
  -DNANOAI_RKNN_OPENCV_DIR="${OPENCV_DIR}" \
  -DNANOAI_RKNN_STB_IMAGE_INCLUDE_DIR="${STB_IMAGE_INCLUDE_DIR}" \
  -DNANOAI_RKNN_JPEG_TURBO_ROOT="${JPEG_TURBO_ROOT}" \
  -DNANOAI_RKNN_UTILS_ROOT="${UTILS_ROOT}"

cmake --build "${BUILD_DIR}" -j"${JOBS}"

echo "Done. Build output: ${BUILD_DIR}"

if [[ "${ENABLE_INSTALL}" == "ON" ]]; then
  echo "[INFO] Installing NanoAI_RKNN to ${INSTALL_PREFIX} ..."
  cmake --install "${BUILD_DIR}"
fi

if [[ "${NANOAI_RKNN_ENABLE_CPACK}" == "ON" ]]; then
  echo "[INFO] Generating packages with CPack ..."
  cpack --config "${BUILD_DIR}/CPackConfig.cmake"
fi
