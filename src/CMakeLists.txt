add_subdirectory(shaders)
find_package(Vulkan REQUIRED)
add_executable(ex0
    main.cpp
    backend/window.cpp
    first_app.cpp
    backend/xel_pipeline.cpp
    backend/device.cpp
    backend/swap_chain.cpp
    xel_model.cpp
    xel_renderer.cpp
    simple_render_system.cpp
)
target_link_libraries(ex0 PRIVATE glfw Vulkan::Vulkan)

add_executable(ex1
    ex1.cpp
    backend/window.cpp
    backend/device.cpp
    backend/xel_pipeline.cpp
    xel_model.cpp
    backend/swap_chain.cpp
    ui/renderer.cpp
)
target_link_libraries(ex1 PRIVATE glfw Vulkan::Vulkan)

add_executable(ex2
    ex2.cpp
    backend/window.cpp
    backend/device.cpp
    backend/xel_pipeline.cpp
    ui/renderer.cpp
)
target_link_libraries(ex2 PRIVATE glfw Vulkan::Vulkan)

add_dependencies(ex0 shaders)
add_dependencies(ex1 shaders)
add_dependencies(ex2 shaders)