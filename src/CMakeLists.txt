add_subdirectory(shaders)
add_subdirectory(textures)
find_package(Vulkan REQUIRED)
add_executable(ex0
    main.cpp
    backend/window.cpp
    first_app.cpp
    backend/xel_pipeline.cpp
    backend/vulkan_context.cpp
    backend/swap_chain.cpp
    xel_model.cpp
    xel_renderer.cpp
    simple_render_system.cpp
)
target_link_libraries(ex0 PRIVATE glfw Vulkan::Vulkan)

add_executable(ex1
    ex1.cpp
    backend/window.cpp
    backend/vulkan_context.cpp
    backend/xel_pipeline.cpp
    xel_model.cpp
    backend/swap_chain.cpp
)
target_link_libraries(ex1 PRIVATE glfw Vulkan::Vulkan)

add_executable(ex2
    ex2.cpp
    backend/vulkan/window.cpp
    backend/vulkan/vulkan_context.cpp
    backend/vulkan/swap_chain.cpp
    backend/vulkan/ui_renderer.cpp
)
target_link_libraries(ex2 PRIVATE glfw Vulkan::Vulkan freetype)
target_include_directories(ex2 PRIVATE ${STB_INCLUDE_DIR})

add_executable(ex3 ex3.cpp)
target_link_libraries(ex3 PRIVATE freetype)

add_dependencies(ex0 shaders)
add_dependencies(ex1 shaders)
add_dependencies(ex2 shaders textures)