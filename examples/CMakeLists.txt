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

# 动画演示：现有 UI 控件 + cosine_anim 的缓动驱动位置与进度。
add_executable(ui_anim_demo
    ui_anim_demo/main.cpp
)
target_link_libraries(ui_anim_demo PRIVATE cosine_app cosine_anim)
target_include_directories(ui_anim_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_anim_demo
)
if(COSINE_CXX_MODULE_SCAN)
    set_target_properties(ui_anim_demo PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()
add_dependencies(ui_anim_demo shaders copy_assets)

# 动画能力全面展示（M4.5）：缓动、层级、属性动画、事件与播放控制。
add_executable(ui_anim_showcase
    ui_anim_showcase/main.cpp
)
target_link_libraries(ui_anim_showcase PRIVATE cosine_app cosine_anim)
target_include_directories(ui_anim_showcase PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_anim_showcase
)
if(COSINE_CXX_MODULE_SCAN)
    set_target_properties(ui_anim_showcase PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()
add_dependencies(ui_anim_showcase shaders copy_assets)

# 高级 UI 实验室（EX-2）：探索未定 API 的组合控件，不参与正式验收。
add_executable(ui_lab
    ui_lab/main.cpp
)
target_link_libraries(ui_lab PRIVATE cosine_app)
target_include_directories(ui_lab PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_lab
)
if(COSINE_CXX_MODULE_SCAN)
    set_target_properties(ui_lab PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()
add_dependencies(ui_lab shaders copy_assets)

cosine_apply_sanitizers(ui_demo)
cosine_apply_sanitizers(ui_gallery)
cosine_apply_sanitizers(ui_anim_demo)
cosine_apply_sanitizers(ui_anim_showcase)
cosine_apply_sanitizers(ui_lab)
