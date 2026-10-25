add_executable(ui_demo
    ui_demo/main.cpp
)
target_link_libraries(ui_demo PRIVATE cosine_app)
target_include_directories(ui_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_demo
)
set_target_properties(ui_demo PROPERTIES CXX_SCAN_FOR_MODULES ON)
add_dependencies(ui_demo shaders copy_assets)

cosine_apply_sanitizers(ui_demo)
