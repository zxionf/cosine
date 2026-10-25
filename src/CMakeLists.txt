add_subdirectory(shaders)

# 字体/文本基础库：只编译一次，供 backend、UI 和测试共同链接。
add_library(cosine_text STATIC
    graphics/font_atlas.cpp
    graphics/font_atlas_builder.cpp
    graphics/font_face.cpp
)
target_include_directories(cosine_text PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(cosine_text PUBLIC glm::glm freetype)

# UI 库：布局、控件、事件、DrawList 消费端；测试和 ex4 不再各自重复编译。
add_library(cosine_ui STATIC
    ui/vbox.cpp
    ui/hbox.cpp
    ui/root.cpp
    ui/ui_system.cpp
    ui/scroll_view.cpp
)
target_include_directories(cosine_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(cosine_ui PUBLIC
    cosine_text
    glfw
    Vulkan::Vulkan
    glm::glm
)

if(MSVC)
    target_compile_options(cosine_text PRIVATE /W4)
    target_compile_options(cosine_ui PRIVATE /W4)
else()
    target_compile_options(cosine_text PRIVATE -Wall -Wextra -Wshadow)
    target_compile_options(cosine_ui PRIVATE -Wall -Wextra -Wshadow)
endif()

add_library(cosine_backend STATIC
    backend/asset_path.cpp
    backend/window.cpp
    backend/vulkan/vulkan_context.cpp
    backend/vulkan/swap_chain.cpp
    backend/vulkan/renderer.cpp
)
target_include_directories(cosine_backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(cosine_backend SYSTEM PUBLIC
    ${STB_INCLUDE_DIR}
)
target_link_libraries(cosine_backend PUBLIC
    cosine_text
    glfw
    Vulkan::Vulkan
    glm::glm
)
target_compile_definitions(cosine_backend PRIVATE COSINE_ASSET_DIR="${COSINE_ASSET_BUILD_DIR}")

if(MSVC)
    target_compile_options(cosine_backend PRIVATE /W4)
else()
    target_compile_options(cosine_backend PRIVATE -Wall -Wextra -Wshadow)
endif()

# 只有 Vulkan 头使用 C++ modules（import vulkan_hpp;），这些 target 显式开启扫描。
set_target_properties(cosine_backend PROPERTIES CXX_SCAN_FOR_MODULES ON)

# 应用层：窗口/Vulkan/UI 的装配与事件桥接，供 examples 复用。
add_library(cosine_app STATIC
    xel_application.cpp
)
target_include_directories(cosine_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(cosine_app PUBLIC
    cosine_ui
    cosine_backend
)
set_target_properties(cosine_app PROPERTIES CXX_SCAN_FOR_MODULES ON)

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "${COSINE_ASSET_BUILD_DIR}"
    COMMENT "Copying assets directory"
    VERBATIM
)

# find_package(OpenGl REQUIRED)
# add_executable(ex3 ex3.cpp)
# target_link_libraries(ex3 PRIVATE glfw OpenGL::GL)
