#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build-coverage}"
COVERAGE_DIR="${COVERAGE_DIR:-${PROJECT_ROOT}/coverage}"

if ! command -v gcovr >/dev/null 2>&1; then
    echo "gcovr is required. Install it with: python3 -m pip install gcovr" >&2
    exit 1
fi

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage -O0 -g" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
    -DCMAKE_SHARED_LINKER_FLAGS="--coverage"
find "${BUILD_DIR}" -type f -name '*.gcda' -delete
cmake --build "${BUILD_DIR}" --parallel
ctest --test-dir "${BUILD_DIR}" --output-on-failure

mkdir -p "${COVERAGE_DIR}"
gcovr --root "${PROJECT_ROOT}" \
    --filter "${PROJECT_ROOT}/Core_engine/" \
    --html-details "${COVERAGE_DIR}/index.html" \
    --print-summary \
    --exclude-unreachable-branches \
    --exclude-throw-branches \
    --fail-under-line 100 \
    --fail-under-function 100 \
    "${BUILD_DIR}"
