param(
    [string]$BuildDir = "build-ci"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root $BuildDir

cmake -S $Root -B $Build -DCMAKE_BUILD_TYPE=Debug
cmake --build $Build --config Debug
ctest --test-dir $Build -C Debug --output-on-failure
