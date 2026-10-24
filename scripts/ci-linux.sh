#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${root}/build-ci"

# 离线开发机可以复用已有 build/_deps，避免每次 CI 脚本都重新 clone。
deps=()
for dep in glfw freetype stb; do
    src="${root}/build/_deps/${dep}-src"
    if [ -d "${src}" ]; then
        name="$(echo "${dep}" | tr '[:lower:]' '[:upper:]')"
        deps+=("-DFETCHCONTENT_SOURCE_DIR_${name}=${src}")
    fi
done

cmake -S "${root}" -B "${build}" -G Ninja -DCMAKE_BUILD_TYPE=Debug "${deps[@]}"
cmake --build "${build}" -j
ctest --test-dir "${build}" --output-on-failure
