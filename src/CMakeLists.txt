find_package(Vulkan REQUIRED)
add_executable(main main.cpp xel_window.cpp first_app.cpp xel_pipeline.cpp xel_device.cpp)
target_link_libraries(main PRIVATE glfw Vulkan::Vulkan)