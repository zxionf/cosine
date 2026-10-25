#!/usr/bin/env bash
set -euo pipefail

# 用 lavapipe（Mesa 软件 Vulkan）做一次有限时长的渲染冒烟：
# - 需要可用的 ICD 与显示环境（Wayland/X11）；缺失时 skip。
# - 依次运行 examples/ 下的 demo，检查日志不含 validation error。

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${COSINE_BUILD_DIR:-${root}/build-ci}"
seconds="${COSINE_SMOKE_SECONDS:-10}"
demos=(ui_demo ui_gallery)

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

ran=0
for demo in "${demos[@]}"; do
    binary="${build}/examples/${demo}"
    if [ ! -x "${binary}" ]; then
        echo "skip: ${binary} not built"
        continue
    fi

    log="$(mktemp)"
    echo "running ${binary} for ${seconds}s with lavapipe (${icd})"
    set +e
    (
        cd "$(dirname "${binary}")"
        if [ -n "${lib_dir}" ]; then
            LD_LIBRARY_PATH="${lib_dir}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
                VK_ICD_FILENAMES="${icd}" timeout "${seconds}" "./${demo}"
        else
            VK_ICD_FILENAMES="${icd}" timeout "${seconds}" "./${demo}"
        fi
    ) > "${log}" 2>&1
    status=$?
    set -e

    # timeout 结束进程是预期的正常路径。
    if [ "${status}" -ne 0 ] && [ "${status}" -ne 124 ]; then
        echo "fail: ${demo} exited with ${status}"
        cat "${log}"
        rm -f "${log}"
        exit 1
    fi

    if grep -Eiq 'VUID-|validation (error|warning)|Validation Error' "${log}"; then
        echo "fail: ${demo} produced validation messages"
        cat "${log}"
        rm -f "${log}"
        exit 1
    fi

    rm -f "${log}"
    ran=$((ran + 1))
done

if [ "${ran}" -eq 0 ]; then
    echo "skip: no demo binaries built under ${build}/examples"
    exit 0
fi

echo "lavapipe smoke passed (${ran} demo(s))"
