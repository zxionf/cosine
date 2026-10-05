cmake_minimum_required(VERSION 4.0.0)
project(cosvideo)

set(CMAKE_CXX_STANDARD 20)
include(FetchContent)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# file(GLOB_RECURSE SOURES "")

# 配置 GLFW 编译选项（避免编译示例和测试）
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
set(GLFW_USE_EGL ON CACHE BOOL "" FORCE)  # 重要：Wayland 需要 EGL
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
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)
set(STB_INCLUDE_DIR ${stb_SOURCE_DIR} CACHE INTERNAL "stb include dir")

add_subdirectory(test)

add_subdirectory(movy)
add_subdirectory(interpreter)

add_subdirectory(src)
