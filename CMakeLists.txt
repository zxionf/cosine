cmake_minimum_required(VERSION 4.0.0)
project(cosvideo VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(COSINE_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated" CACHE INTERNAL "Generated headers")
file(MAKE_DIRECTORY "${COSINE_GENERATED_DIR}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp.in"
               "${COSINE_GENERATED_DIR}/xel/version.hpp" @ONLY)
# 项目只在 Vulkan 头（import vulkan_hpp;）使用 C++ modules。
# 默认关闭全局 module scan，避免普通测试/图形目标被 -fmodules-ts 和 CXX_SCAN 额外消耗内存；
# 需要 modules 的 target 在 src/CMakeLists.txt 里单独打开 CXX_SCAN_FOR_MODULES。
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# 只有编译器提供 import graph 依赖扫描时才打开 CXX_SCAN_FOR_MODULES，
# 否则 CMake 生成阶段会直接报错（例如 Ubuntu 24.04 的 GCC 13）。
# 已知支持：GCC >= 14、Clang >= 16、MSVC >= 19.34（VS 17.4）。
set(COSINE_CXX_MODULE_SCAN OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
    set(COSINE_CXX_MODULE_SCAN ON)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16)
    set(COSINE_CXX_MODULE_SCAN ON)
elseif(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.34)
    set(COSINE_CXX_MODULE_SCAN ON)
endif()
message(STATUS "cosine: C++ module scan ${COSINE_CXX_MODULE_SCAN}")
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

# 可选 sanitizer（address/undefined/thread），只作用于自有 target，不污染第三方依赖。
set(COSINE_SANITIZE "" CACHE STRING "Sanitizers for project targets: address;undefined;thread")
set_property(CACHE COSINE_SANITIZE PROPERTY STRINGS "" address undefined thread)

function(cosine_apply_sanitizers target)
    if(NOT COSINE_SANITIZE)
        return()
    endif()
    foreach(sanitizer IN LISTS COSINE_SANITIZE)
        if(NOT sanitizer MATCHES "^(address|undefined|thread|leak)$")
            message(FATAL_ERROR "Unsupported sanitizer: ${sanitizer}")
        endif()
    endforeach()
    set(flags -fsanitize=${COSINE_SANITIZE} -fno-omit-frame-pointer -g)
    target_compile_options(${target} PUBLIC ${flags})
    target_link_options(${target} PUBLIC ${flags})
endfunction()

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
add_subdirectory(examples)
add_subdirectory(test)
add_subdirectory(interpreter)
