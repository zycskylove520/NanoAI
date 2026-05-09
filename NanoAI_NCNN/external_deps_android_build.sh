#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Edit these paths to your local dependency locations.
CMAKE_TOOLCHAIN_FILE="/yitu/zyc/deploy/3rdparty/android-ndk-r29/build/cmake/android.toolchain.cmake"
NCNN_CMAKE_DIR="/yitu/zyc/deploy/3rdparty/ncnn/android-aarch64/install/lib/cmake/ncnn"
OPENCV_CMAKE_DIR="/yitu/zyc/deploy/3rdparty/OpenCV-android-sdk/sdk/native/jni"

# NanoAI_NCNN 支持安装与可选打包，示例可选。
NANOAI_NCNN_BUILD_EXAMPLES="OFF"
NANOAI_NCNN_EXAMPLES="ALL"
NANOAI_NCNN_ENABLE_CPACK="OFF"

ENABLE_INSTALL="ON"
INSTALL_PREFIX="/usr/local"

BUILD_DIR="${SCRIPT_DIR}/build_android_external"
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

构建 NanoAI_NCNN（含依赖探测），支持 install 与可选 CPack 打包。

Options:
  -B, --build-dir <dir>    Build directory (default: ./build_android_external)
  -j, --jobs <N>           Parallel build jobs (default: nproc)
  --ncnn-dir <dir>         ncnn_DIR path (directory containing ncnnConfig.cmake)
  --install-prefix <dir>   Install prefix (default: /usr/local)
  --no-install             Build only, skip `cmake --install`
  --enable-cpack           Enable CPack package generation
  -h, --help               Show this help message and exit.

Current defaults:
  BUILD_DIR                 ${BUILD_DIR}
  JOBS                      ${JOBS}
  ENABLE_INSTALL            ${ENABLE_INSTALL}
  INSTALL_PREFIX            ${INSTALL_PREFIX}
  NANOAI_NCNN_BUILD_EXAMPLES ${NANOAI_NCNN_BUILD_EXAMPLES}
  NANOAI_NCNN_EXAMPLES      ${NANOAI_NCNN_EXAMPLES}
  NANOAI_NCNN_ENABLE_CPACK  ${NANOAI_NCNN_ENABLE_CPACK}
  CMAKE_TOOLCHAIN_FILE      ${CMAKE_TOOLCHAIN_FILE}
  NCNN_CMAKE_DIR            ${NCNN_CMAKE_DIR}
  OPENCV_CMAKE_DIR          ${OPENCV_CMAKE_DIR}

Examples:
  ./external_deps_android_build.sh
  ./external_deps_android_build.sh -B ./build_android_custom
  ./external_deps_android_build.sh -j 8
  ./external_deps_android_build.sh --ncnn-dir /path/to/ncnn/lib/cmake/ncnn
  ./external_deps_android_build.sh --install-prefix /opt/nanoai_ncnn_android
  ./external_deps_android_build.sh --enable-cpack
  ./external_deps_android_build.sh --no-install
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
    --ncnn-dir)
      NCNN_CMAKE_DIR="${2:?Missing value for $1}"
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
      NANOAI_NCNN_ENABLE_CPACK="ON"
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

# 确保 NanoAIFlow 可被 find_package(NanoAIFlow) 检测到。
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
  -DNANOAI_NCNN_ENABLE_CPACK="${NANOAI_NCNN_ENABLE_CPACK}" \
  -DNANOAI_NCNN_ANDROID_PRESET=ON \
  -DCMAKE_TOOLCHAIN_FILE="${CMAKE_TOOLCHAIN_FILE}" \
  -Dncnn_DIR="${NCNN_CMAKE_DIR}" \
  -DNANOAI_NCNN_NCNN_CONFIG_DIR="${NCNN_CMAKE_DIR}" \
  -DOPENCV_CMAKE_DIR="${OPENCV_CMAKE_DIR}" \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DNanoAIFlow_DIR="${NANOAIFLOW_CMAKE_DIR}"

cmake --build "${BUILD_DIR}" -j"${JOBS}"

if [[ "${ENABLE_INSTALL}" == "ON" ]]; then
  echo "[INFO] Installing NanoAI_NCNN to ${INSTALL_PREFIX} ..."
  cmake --install "${BUILD_DIR}"
fi

if [[ "${NANOAI_NCNN_ENABLE_CPACK}" == "ON" ]]; then
  echo "[INFO] Generating packages with CPack ..."
  cpack --config "${BUILD_DIR}/CPackConfig.cmake"
fi

echo "Done. Build output: ${BUILD_DIR}"

