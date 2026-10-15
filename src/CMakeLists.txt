add_subdirectory(shaders)
add_subdirectory(textures)
find_package(Vulkan REQUIRED)
add_executable(ex0
    main.cpp
)
target_link_libraries(ex0 PRIVATE glfw Vulkan::Vulkan)

add_executable(ex4
    ex4.cpp
    backend/window.cpp
    backend/vulkan/vulkan_context.cpp
    backend/vulkan/swap_chain.cpp
    backend/vulkan/renderer.cpp
    xel_application.cpp
    ui/vbox.cpp
    ui/root.cpp
    ui/ui_system.cpp
)
target_include_directories(ex4 PRIVATE ${STB_INCLUDE_DIR})
target_link_libraries(ex4 PRIVATE glfw Vulkan::Vulkan)

add_executable(ex2
    ex2.cpp
    backend/window.cpp
    backend/vulkan/vulkan_context.cpp
    backend/vulkan/swap_chain.cpp
    backend/vulkan/renderer.cpp
    backend/vulkan/font_renderer.cpp
)
target_include_directories(ex2 PRIVATE ${STB_INCLUDE_DIR})
target_link_libraries(ex2 PRIVATE glfw Vulkan::Vulkan freetype)

# find_package(OpenGl REQUIRED)
# add_executable(ex3 ex3.cpp)
# target_link_libraries(ex3 PRIVATE glfw OpenGL::GL)

add_dependencies(ex0 shaders)
add_dependencies(ex4 shaders textures)
add_dependencies(ex2 shaders textures)