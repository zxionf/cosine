# M2 验收 demo：最小可交互面板。
add_executable(ui_demo
    ui_demo/main.cpp
)
target_link_libraries(ui_demo PRIVATE cosine_app)
target_include_directories(ui_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_demo
)
if(COSINE_CXX_MODULE_SCAN)
    set_target_properties(ui_demo PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()
add_dependencies(ui_demo shaders copy_assets)

# UI gallery：分区展示全部控件与交互，M3.5 起作为 UI 打磨的验收对象。
add_executable(ui_gallery
    ui_gallery/main.cpp
)
target_link_libraries(ui_gallery PRIVATE cosine_app)
target_include_directories(ui_gallery PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_gallery
)
if(COSINE_CXX_MODULE_SCAN)
    set_target_properties(ui_gallery PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()
add_dependencies(ui_gallery shaders copy_assets)

cosine_apply_sanitizers(ui_demo)
cosine_apply_sanitizers(ui_gallery)
