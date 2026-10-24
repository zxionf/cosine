add_subdirectory(shaders)

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
target_link_libraries(cosine_backend PUBLIC glfw Vulkan::Vulkan glm::glm)
target_compile_definitions(cosine_backend PRIVATE COSINE_ASSET_DIR="${COSINE_ASSET_BUILD_DIR}")

if(MSVC)
    target_compile_options(cosine_backend PRIVATE /W4)
else()
    target_compile_options(cosine_backend PRIVATE -Wall -Wextra -Wshadow)
endif()

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "${COSINE_ASSET_BUILD_DIR}"
    COMMENT "Copying assets directory"
    VERBATIM
)

add_executable(ex4
    ex4.cpp
    xel_application.cpp
    ui/vbox.cpp
    ui/hbox.cpp
    ui/root.cpp
    ui/ui_system.cpp
    ui/scroll_view.cpp
)
target_link_libraries(ex4 PRIVATE cosine_backend glfw Vulkan::Vulkan glm::glm)
add_dependencies(ex4 shaders copy_assets)

add_executable(ex2
    ex2.cpp
)
target_link_libraries(ex2 PRIVATE cosine_backend glfw Vulkan::Vulkan glm::glm)
add_dependencies(ex2 shaders copy_assets)

# find_package(OpenGl REQUIRED)
# add_executable(ex3 ex3.cpp)
# target_link_libraries(ex3 PRIVATE glfw OpenGL::GL)
