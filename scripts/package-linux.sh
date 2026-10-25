#!/usr/bin/env bash
set -euo pipefail

# 打包 Linux 可执行目录：
#   dist/cosine-<version>-linux-<arch>/
#     ui_demo  ui_gallery  assets/  run.sh  VERSION  README.md
# 以及同名 .tar.gz。
#
# 环境变量：
#   COSINE_PACKAGE_BUILD_DIR  构建目录（默认 build-release）
#   COSINE_PACKAGE_OUT_DIR    输出目录（默认 dist）
#   COSINE_JOBS               并行度（默认 2）
#   COSINE_SKIP_STRIP=1       跳过 strip

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${COSINE_PACKAGE_BUILD_DIR:-${root}/build-release}"
out="${COSINE_PACKAGE_OUT_DIR:-${root}/dist}"
jobs="${COSINE_JOBS:-2}"

deps=()
for dep in glfw freetype stb; do
    src="${root}/build/_deps/${dep}-src"
    if [ -d "${src}" ]; then
        name="$(echo "${dep}" | tr '[:lower:]' '[:upper:]')"
        deps+=("-DFETCHCONTENT_SOURCE_DIR_${name}=${src}")
    fi
done

echo "configuring ${build} (Release)"
cmake -S "${root}" -B "${build}" -G Ninja -DCMAKE_BUILD_TYPE=Release "${deps[@]}"
cmake --build "${build}" -j"${jobs}" --target ui_demo ui_gallery

version="$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "${build}/CMakeCache.txt" | head -n1)"
if [ -z "${version}" ]; then
    echo "failed to read project version from ${build}/CMakeCache.txt" >&2
    exit 1
fi

arch="$(uname -m)"
name="cosine-${version}-linux-${arch}"
stage="${out}/${name}"

rm -rf "${stage}"
mkdir -p "${stage}"

install -m 0755 "${build}/examples/ui_demo" "${stage}/ui_demo"
install -m 0755 "${build}/examples/ui_gallery" "${stage}/ui_gallery"
cp -a "${build}/src/assets" "${stage}/assets"
cp "${root}/README.md" "${stage}/README.md"
printf '%s\n' "${version}" > "${stage}/VERSION"

cat > "${stage}/run.sh" <<'RUN'
#!/usr/bin/env sh
# 从任意 cwd 启动；资源通过可执行文件旁的 assets/ 解析。
cd "$(dirname "$0")" || exit 1
exec ./ui_demo "$@"
RUN
chmod 0755 "${stage}/run.sh"

if [ "${COSINE_SKIP_STRIP:-0}" != "1" ]; then
    strip --strip-unneeded "${stage}/ui_demo" "${stage}/ui_gallery" 2>/dev/null || true
fi

"${stage}/ui_demo" --version >&2 || {
    echo "packaged ui_demo failed to run" >&2
    exit 1
}
"${stage}/ui_gallery" --version >&2 || {
    echo "packaged ui_gallery failed to run" >&2
    exit 1
}

archive="${out}/${name}.tar.gz"
tar -C "${out}" -czf "${archive}" "${name}"

echo "packaged: ${stage}"
echo "archive:  ${archive}"
