# 示例程序：每个都是「建树 + app.run()」的最小外壳，主循环在 Application 内。
#
# UI 展示台：注册表驱动的分区式 demo（外壳 demo.cpp + 内容 sections/*.cpp）。
# 新增展示内容 = 在 sections/ 加一个 build_xxx，并在 demo.cpp 的注册表里加一条。
add_executable(ui_demo
    ui_demo/main.cpp
    ui_demo/demo.cpp
    ui_demo/sections/basics.cpp
    ui_demo/sections/layout.cpp
    ui_demo/sections/data.cpp
    ui_demo/sections/overlay.cpp
)
target_link_libraries(ui_demo PRIVATE cosine_app)
target_include_directories(ui_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_demo/sections
)
if(COSINE_CXX_MODULE_SCAN)
    set_target_properties(ui_demo PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()
add_dependencies(ui_demo shaders copy_assets)

# 字体实验室（EX-6）：字符串预览、字号、文字调试与测量。
add_executable(font_lab
    font_lab/main.cpp
)
target_link_libraries(font_lab PRIVATE cosine_app)
target_include_directories(font_lab PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/font_lab
)
if(COSINE_CXX_MODULE_SCAN)
    set_target_properties(font_lab PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()
add_dependencies(font_lab shaders copy_assets)

# moss: 手写冒烟用例（Window → Panel → ScrollView → ColorRect 最小链路）。
add_executable(moss
    moss/main.cpp
)
target_link_libraries(moss PRIVATE cosine_app)
target_include_directories(moss PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/moss
)
if(COSINE_CXX_MODULE_SCAN)
    set_target_properties(moss PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()
add_dependencies(moss shaders copy_assets)

cosine_apply_sanitizers(ui_demo)
cosine_apply_sanitizers(font_lab)
cosine_apply_sanitizers(moss)
