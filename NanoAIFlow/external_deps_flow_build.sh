#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) NanoAI
#
# File: external_deps_flow_build.sh
# Brief: 一键构建并安装 NanoAIFlow（支持测试/示例/打包开关）。

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# 默认构建参数。
BUILD_DIR="${SCRIPT_DIR}/build_external"
INSTALL_PREFIX="${REPO_ROOT}/out/install/flow"
BUILD_TESTS="OFF"
BUILD_EXAMPLES="OFF"
ENABLE_CPACK="OFF"
JOBS="$(nproc 2>/dev/null || echo 4)"

print_help() {
  cat <<'EOF'
Usage: ./external_deps_flow_build.sh [options]

Options:
  -B, --build-dir <dir>       Build directory (default: ./build_external)
  -I, --install-prefix <dir>  Install prefix (default: ../out/install/flow)
      --tests                 Enable NANOAIFLOW_BUILD_TESTS=ON
      --examples              Enable NANOAIFLOW_BUILD_EXAMPLES=ON
      --cpack                 Enable NANOAIFLOW_ENABLE_CPACK=ON
  -j, --jobs <N>              Parallel build jobs (default: nproc)
  -h, --help                  Show this help message

Examples:
  ./external_deps_flow_build.sh
  ./external_deps_flow_build.sh --tests
  ./external_deps_flow_build.sh --examples -j 8
  ./external_deps_flow_build.sh --tests --examples --cpack
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -B|--build-dir)
      BUILD_DIR="${2:?Missing value for $1}"
      shift 2
      ;;
    -I|--install-prefix)
      INSTALL_PREFIX="${2:?Missing value for $1}"
      shift 2
      ;;
    --tests)
      BUILD_TESTS="ON"
      shift
      ;;
    --examples)
      BUILD_EXAMPLES="ON"
      shift
      ;;
    --cpack)
      ENABLE_CPACK="ON"
      shift
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

mkdir -p "${BUILD_DIR}" "${INSTALL_PREFIX}"

# 配置阶段：生成构建系统并写入目标开关。
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DNANOAIFLOW_BUILD_TESTS="${BUILD_TESTS}" \
  -DNANOAIFLOW_BUILD_EXAMPLES="${BUILD_EXAMPLES}" \
  -DNANOAIFLOW_ENABLE_CPACK="${ENABLE_CPACK}"

# 编译与安装阶段。
cmake --build "${BUILD_DIR}" -j"${JOBS}"
cmake --install "${BUILD_DIR}"

# 若开启测试，执行 CTest。
if [[ "${BUILD_TESTS}" == "ON" ]]; then
  ctest --test-dir "${BUILD_DIR}" --output-on-failure
fi

echo "Done. Build output: ${BUILD_DIR}"
echo "Installed to: ${INSTALL_PREFIX}"
