cmake_minimum_required(VERSION 4.0.0)
project(cosvideo)

set(CMAKE_CXX_STANDARD 20)
# 项目只在 Vulkan 头（import vulkan_hpp;）使用 C++ modules。
# 默认关闭全局 module scan，避免普通测试/图形目标被 -fmodules-ts 和 CXX_SCAN 额外消耗内存；
# 需要 modules 的 target 在 src/CMakeLists.txt 里单独打开 CXX_SCAN_FOR_MODULES。
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
include(FetchContent)

# find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

enable_testing()

set(COSINE_ASSET_BUILD_DIR "${CMAKE_BINARY_DIR}/src/assets" CACHE INTERNAL "Runtime asset directory")

option(COSINE_GLFW_WAYLAND "Build GLFW with Wayland support" ON)
option(COSINE_GLFW_X11 "Build GLFW with X11 support" OFF)
if(NOT COSINE_GLFW_WAYLAND AND NOT COSINE_GLFW_X11)
    message(FATAL_ERROR "At least one of COSINE_GLFW_WAYLAND / COSINE_GLFW_X11 must be ON")
endif()

# 配置 GLFW 编译选项（避免编译示例和测试）
set(GLFW_BUILD_WAYLAND ${COSINE_GLFW_WAYLAND} CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ${COSINE_GLFW_X11} CACHE BOOL "" FORCE)
set(GLFW_USE_EGL ${COSINE_GLFW_WAYLAND} CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# 声明 FetchContent
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
# set(GLFW_BUILD_WAYLAND ON CACHE BOOL "Enable Wayland support" FORCE)
# set(GLFW_BUILD_X11 OFF CACHE BOOL "Disable X11 support" FORCE)
# 获取 GLFW
FetchContent_MakeAvailable(glfw)

set(FT_DISABLE_ZLIB       TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2      TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG        TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ   TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI     TRUE CACHE BOOL "" FORCE)
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(freetype)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 003501f6d7a8668ce8b6d875e34dd97ad0d5d5b5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)
set(STB_INCLUDE_DIR ${stb_SOURCE_DIR} CACHE INTERNAL "stb include dir")

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(interpreter)
