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
# vertex.hpp 公开包含 <vulkan/vulkan.hpp>，因此头文件路径要传递给所有消费者。
target_link_libraries(cosine_text PUBLIC glm::glm freetype Vulkan::Headers)

# 动画内核：缓动、属性、时间轴；只依赖 glm，不依赖 UI。
add_library(cosine_anim STATIC
    anim/easing.cpp
    anim/scene.cpp
    anim/property.cpp
    anim/timeline.cpp
    anim/player.cpp
    anim/serialize.cpp
    anim/command.cpp
)
target_include_directories(cosine_anim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(cosine_anim PUBLIC glm::glm)

# UI 库：布局、控件、事件、DrawList 消费端；测试和 ex4 不再各自重复编译。
add_library(cosine_ui STATIC
    ui/vbox.cpp
    ui/hbox.cpp
    ui/grid.cpp
    ui/wrap.cpp
    ui/root.cpp
    ui/ui_system.cpp
    ui/scroll_view.cpp
)
target_include_directories(cosine_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(cosine_ui
    PUBLIC
        cosine_text
        cosine_anim
        glm::glm
    PRIVATE
        glfw
        Vulkan::Vulkan
)

if(MSVC)
    target_compile_options(cosine_text PRIVATE /W4)
    target_compile_options(cosine_ui PRIVATE /W4)
    target_compile_options(cosine_anim PRIVATE /W4)
else()
    target_compile_options(cosine_text PRIVATE -Wall -Wextra -Wshadow)
    target_compile_options(cosine_ui PRIVATE -Wall -Wextra -Wshadow)
    target_compile_options(cosine_anim PRIVATE -Wall -Wextra -Wshadow)
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

# 只有 Vulkan 头使用 C++ modules（import vulkan_hpp;），编译器支持扫描时才开启。
if(COSINE_CXX_MODULE_SCAN)
    set_target_properties(cosine_backend PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()

# 应用层：窗口/Vulkan/UI 的装配与事件桥接，供 examples 复用。
add_library(cosine_app STATIC
    xel_application.cpp
)
target_include_directories(cosine_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COSINE_GENERATED_DIR}
)
target_link_libraries(cosine_app PUBLIC
    cosine_ui
    cosine_backend
)
if(COSINE_CXX_MODULE_SCAN)
    set_target_properties(cosine_app PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()

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

cosine_apply_sanitizers(cosine_text)
cosine_apply_sanitizers(cosine_ui)
cosine_apply_sanitizers(cosine_backend)
cosine_apply_sanitizers(cosine_app)
