find_package(Vulkan REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backend)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ui/widgets)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ui/layout)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ui/theme)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ui/font)

add_subdirectory(shaders)
add_subdirectory(ui)

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
target_link_libraries(ex0 PRIVATE glfw Vulkan::Vulkan ui_engine)
add_dependencies(ex0 shaders)

add_executable(ex1
    ex1.cpp
    backend/window.cpp
    backend/device.cpp
    backend/xel_pipeline.cpp
    xel_model.cpp
    backend/swap_chain.cpp
)
target_link_libraries(ex1 PRIVATE glfw Vulkan::Vulkan ui_engine)
add_dependencies(ex1 shaders)

add_executable(ex_minimal
    ex_minimal.cpp
    backend/window.cpp
    backend/device.cpp
    backend/swap_chain.cpp
)
target_link_libraries(ex_minimal PRIVATE glfw Vulkan::Vulkan)
add_dependencies(ex_minimal shaders)
