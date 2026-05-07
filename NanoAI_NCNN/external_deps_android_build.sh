#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Edit these paths to your local dependency locations.
ANDROID_NDK_PATH="${SCRIPT_DIR}/3rdparty/android-ndk-r25c"
NCNN_INCLUDE_DIR="${SCRIPT_DIR}/3rdparty/ncnn/android-aarch64/install/include"
NCNN_LIBRARY_DIR="${SCRIPT_DIR}/3rdparty/ncnn/android-aarch64/install/lib"
OPENCV_DIR="${SCRIPT_DIR}/3rdparty/OpenCV-android-sdk/sdk/native/jni"
NANOAIFLOW_PREFIX="${REPO_ROOT}/out/install/flow"
NANOAIFLOW_CMAKE_DIR="${NANOAIFLOW_PREFIX}/lib/cmake/NanoAIFlow"

# PACKAGE is recommended for first validation.
# Change to PROJECTS if you want to build deploy targets.
NANOAI_NCNN_BUILD_MODE="PACKAGE"
NANOAI_NCNN_PROJECTS="car_project"

BUILD_DIR="${SCRIPT_DIR}/build_android_external"

# Ensure NanoAIFlow package is available for find_package(NanoAIFlow).
if [[ ! -f "${NANOAIFLOW_CMAKE_DIR}/NanoAIFlowConfig.cmake" ]]; then
  echo "[INFO] NanoAIFlow package not found at: ${NANOAIFLOW_CMAKE_DIR}"
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

if [[ ! -f "${NANOAIFLOW_CMAKE_DIR}/NanoAIFlowConfig.cmake" ]]; then
  echo "[ERROR] NanoAIFlowConfig.cmake still not found at: ${NANOAIFLOW_CMAKE_DIR}" >&2
  exit 1
fi

echo "[INFO] Using NanoAIFlow_DIR=${NANOAIFLOW_CMAKE_DIR}"

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
  -DNANOAIFLOW_ROOT="${NANOAIFLOW_PREFIX}" \
  -DNanoAIFlow_DIR="${NANOAIFLOW_CMAKE_DIR}" \
  -DCMAKE_PREFIX_PATH="${NANOAIFLOW_PREFIX}"

cmake --build "${BUILD_DIR}" -j

echo "Done. Build output: ${BUILD_DIR}"
