# UI Engine library
add_library(ui_engine
    ui_renderer.cpp
    ui_context.cpp
    ui_window.cpp
    font/font_atlas.cpp
    vertex.cpp
    layout/flex_layout.cpp
    widgets/widget.cpp
    widgets/panel.cpp
    widgets/button.cpp
    widgets/label.cpp
    widgets/slider.cpp
    widgets/text_input.cpp
)

target_include_directories(ui_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../backend
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/layout
    ${CMAKE_CURRENT_SOURCE_DIR}/theme
    ${CMAKE_CURRENT_SOURCE_DIR}/font
)

target_link_libraries(ui_engine PRIVATE glfw Vulkan::Vulkan)
target_compile_features(ui_engine PUBLIC cxx_std_20)
