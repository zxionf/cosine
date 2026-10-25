#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${COSINE_BUILD_DIR:-${root}/build-ci}"

# CI 默认限制并行度，避免 32 核机器上同时启动过多 cc1plus 导致内存耗尽。
# 本地内存充足时可用 COSINE_JOBS=8 ./scripts/ci-linux.sh 覆盖。
jobs="${COSINE_JOBS:-2}"
if ! [[ "${jobs}" =~ ^[1-9][0-9]*$ ]]; then
    echo "COSINE_JOBS must be a positive integer, got: ${jobs}" >&2
    exit 2
fi

# 可选 sanitizer：COSINE_SANITIZE=address / undefined / thread
sanitize="${COSINE_SANITIZE:-}"
sanitize_args=()
if [ -n "${sanitize}" ]; then
    sanitize_args+=("-DCOSINE_SANITIZE=${sanitize}")
fi

# 离线开发机可以复用已有 build/_deps，避免每次 CI 脚本都重新 clone。
deps=()
for dep in glfw freetype stb; do
    src="${root}/build/_deps/${dep}-src"
    if [ -d "${src}" ]; then
        name="$(echo "${dep}" | tr '[:lower:]' '[:upper:]')"
        deps+=("-DFETCHCONTENT_SOURCE_DIR_${name}=${src}")
    fi
done

cmake -S "${root}" -B "${build}" -G Ninja -DCMAKE_BUILD_TYPE=Debug "${deps[@]}" "${sanitize_args[@]}"
echo "building ${build} with ${jobs} job(s)${sanitize:+ (sanitize=${sanitize})}"
cmake --build "${build}" -j"${jobs}"
ctest --test-dir "${build}" --output-on-failure
