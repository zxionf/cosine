#!/usr/bin/env bash
set -euo pipefail

# 用 lavapipe（Mesa 软件 Vulkan）做一次有限时长的渲染冒烟：
# - 需要可用的 ICD 与显示环境（Wayland/X11）；缺失时 skip。
# - 运行 ui_demo 固定秒数，检查日志不含 validation error。

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${COSINE_BUILD_DIR:-${root}/build-ci}"
demo="${build}/examples/ui_demo"
seconds="${COSINE_SMOKE_SECONDS:-10}"

if [ ! -x "${demo}" ]; then
    echo "skip: ${demo} not built"
    exit 0
fi

if [ -z "${WAYLAND_DISPLAY:-}" ] && [ -z "${DISPLAY:-}" ]; then
    echo "skip: no WAYLAND_DISPLAY or DISPLAY"
    exit 0
fi

find_lavapipe() {
    if [ -n "${COSINE_LAVAPIPE_ICD:-}" ] && [ -e "${COSINE_LAVAPIPE_ICD}" ]; then
        printf '%s\n' "${COSINE_LAVAPIPE_ICD}"
        return 0
    fi
    for candidate in \
        /usr/share/vulkan/icd.d/lvp_icd.x86_64.json \
        /usr/share/vulkan/icd.d/lvp_icd.aarch64.json \
        /usr/share/vulkan/icd.d/lvp_icd.json; do
        if [ -e "${candidate}" ]; then
            printf '%s\n' "${candidate}"
            return 0
        fi
    done
    return 1
}

icd="$(find_lavapipe || true)"
lib_dir="${COSINE_LAVAPIPE_LIB_DIR:-}"
if [ -z "${icd}" ] && [ -n "${lib_dir}" ] && [ -e "${lib_dir}/libvulkan_lvp.so" ]; then
    generated="${build}/lvp_icd.json"
    mkdir -p "$(dirname "${generated}")"
    printf '{"file_format_version":"1.0.0","ICD":{"library_path":"%s/libvulkan_lvp.so","api_version":"1.3.0"}}\n' \
        "${lib_dir}" > "${generated}"
    icd="${generated}"
fi

if [ -z "${icd}" ]; then
    echo "skip: lavapipe ICD not found (set COSINE_LAVAPIPE_ICD or COSINE_LAVAPIPE_LIB_DIR)"
    exit 0
fi

log="$(mktemp)"
trap 'rm -f "${log}"' EXIT

echo "running ${demo} for ${seconds}s with lavapipe (${icd})"
set +e
(
    cd "$(dirname "${demo}")"
    if [ -n "${lib_dir}" ]; then
        LD_LIBRARY_PATH="${lib_dir}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
            VK_ICD_FILENAMES="${icd}" timeout "${seconds}" ./ui_demo
    else
        VK_ICD_FILENAMES="${icd}" timeout "${seconds}" ./ui_demo
    fi
) > "${log}" 2>&1
status=$?
set -e

# timeout 结束进程是预期的正常路径。
if [ "${status}" -ne 0 ] && [ "${status}" -ne 124 ]; then
    echo "fail: ui_demo exited with ${status}"
    cat "${log}"
    exit 1
fi

if grep -Eiq 'VUID-|validation (error|warning)|Validation Error' "${log}"; then
    echo "fail: validation messages found"
    cat "${log}"
    exit 1
fi

echo "lavapipe smoke passed"
