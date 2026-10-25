param(
    [string]$BuildDir = "build-release",
    [string]$OutDir = "dist",
    [int]$Jobs = 2,
    [switch]$SkipStrip
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root $BuildDir
$Out = Join-Path $Root $OutDir

cmake -S $Root -B $Build -DCMAKE_BUILD_TYPE=Release
cmake --build $Build --config Release --parallel $Jobs --target ui_demo

$cache = Join-Path $Build "CMakeCache.txt"
$version = (Select-String -Path $cache -Pattern '^CMAKE_PROJECT_VERSION:\w+=(.+)$').Matches.Groups[1].Value
if (-not $version) {
    throw "failed to read project version from $cache"
}

$name = "cosine-$version-windows-x64"
$stage = Join-Path $Out $name

if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Path $stage | Out-Null

$exeCandidates = @(
    (Join-Path $Build "examples/Release/ui_demo.exe"),
    (Join-Path $Build "examples/ui_demo.exe"),
    (Join-Path $Build "examples/Debug/ui_demo.exe")
)
$exe = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $exe) {
    throw "ui_demo.exe not found under $Build/examples"
}

Copy-Item $exe (Join-Path $stage "ui_demo.exe")
Copy-Item -Recurse (Join-Path $Build "src/assets") (Join-Path $stage "assets")
Copy-Item (Join-Path $Root "README.md") (Join-Path $stage "README.md")
Set-Content -Path (Join-Path $stage "VERSION") -Value $version -NoNewline

@'
@echo off
cd /d "%~dp0"
ui_demo.exe %*
'@ | Set-Content -Path (Join-Path $stage "run.cmd") -Encoding ASCII

if (-not $SkipStrip) {
    # MSVC 没有 strip；用 /DEBUG:NONE 构建即可，这里保留分支便于后续替换符号发布策略。
    Write-Host "skip strip: MSVC binaries are not stripped"
}

& (Join-Path $stage "ui_demo.exe") --version

$archive = Join-Path $Out "$name.zip"
if (Test-Path $archive) { Remove-Item -Force $archive }
Compress-Archive -Path $stage -DestinationPath $archive

Write-Host "packaged: $stage"
Write-Host "archive:  $archive"
