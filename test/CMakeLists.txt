include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/interpreter)

add_executable(test_ui_table test_ui_table.cpp)
target_link_libraries(test_ui_table PRIVATE cosine_ui)

add_executable(test_ui_menu_bar test_ui_menu_bar.cpp)
target_link_libraries(test_ui_menu_bar PRIVATE cosine_ui)

add_executable(test_ui_dialog test_ui_dialog.cpp)
target_link_libraries(test_ui_dialog PRIVATE cosine_ui)

add_executable(test_ui_tab_view test_ui_tab_view.cpp)
target_link_libraries(test_ui_tab_view PRIVATE cosine_ui)

add_executable(test_text_display test_text_display.cpp)
target_link_libraries(test_text_display PRIVATE cosine_ui)

add_executable(test_ui_text_input test_ui_text_input.cpp)
target_link_libraries(test_ui_text_input PRIVATE cosine_ui)

add_executable(test_ui_dropdown test_ui_dropdown.cpp)
target_link_libraries(test_ui_dropdown PRIVATE cosine_ui)

add_executable(test_ui_scrollbar test_ui_scrollbar.cpp)
target_link_libraries(test_ui_scrollbar PRIVATE cosine_ui)

add_executable(test_ui_widgets test_ui_widgets.cpp)
target_link_libraries(test_ui_widgets PRIVATE cosine_ui)

add_executable(test_ui_theme test_ui_theme.cpp)
target_link_libraries(test_ui_theme PRIVATE cosine_ui)

add_executable(test_ui_gallery test_ui_gallery.cpp)
target_include_directories(test_ui_gallery PRIVATE
    ${PROJECT_SOURCE_DIR}/examples/ui_gallery
    ${COSINE_GENERATED_DIR}
)
target_link_libraries(test_ui_gallery PRIVATE cosine_ui)

add_executable(test_version test_version.cpp)
target_include_directories(test_version PRIVATE ${COSINE_GENERATED_DIR})

add_executable(test_asset_path test_asset_path.cpp ${PROJECT_SOURCE_DIR}/src/backend/asset_path.cpp)
target_compile_definitions(test_asset_path PRIVATE COSINE_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets")

add_executable(test_debug_metrics test_debug_metrics.cpp)
target_link_libraries(test_debug_metrics PRIVATE cosine_ui)

add_executable(test_key_code test_key_code.cpp)

add_executable(test_ui_context test_ui_context.cpp)
target_link_libraries(test_ui_context PRIVATE cosine_ui)

add_executable(test_ui_demo test_ui_demo.cpp)
target_include_directories(test_ui_demo PRIVATE
    ${PROJECT_SOURCE_DIR}/examples/ui_demo
)
target_link_libraries(test_ui_demo PRIVATE cosine_ui)

add_executable(test_render_target test_render_target.cpp)
target_link_libraries(test_render_target PRIVATE Vulkan::Vulkan)

add_executable(testglfwinit testglfwinit.cpp)
target_link_libraries(testglfwinit PRIVATE glfw)

add_executable(test_ui_layout test_ui_layout.cpp)
target_link_libraries(test_ui_layout PRIVATE cosine_ui)

add_executable(test_ui_events test_ui_events.cpp)
target_link_libraries(test_ui_events PRIVATE cosine_ui)

add_executable(test_ui_controls test_ui_controls.cpp)
target_link_libraries(test_ui_controls PRIVATE cosine_ui)

add_executable(test_text test_text.cpp)
target_include_directories(test_text PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${STB_INCLUDE_DIR}
)
target_link_libraries(test_text PRIVATE cosine_text)

file(GLOB_RECURSE interpreter_test
    ${PROJECT_SOURCE_DIR}/interpreter/*.cpp
)
list(FILTER interpreter_test EXCLUDE REGEX "main.cpp")

add_executable(test_interpreter_token test_interpreter_token.cpp ../interpreter/token.cpp ../interpreter/lexer.cpp)
add_executable(test_intepreter_tree_structure test_intepreter_tree_structure.cpp ${interpreter_test})
add_executable(test_intepreter_tree_evaluator test_intepreter_tree_evaluator.cpp ${interpreter_test})

# 测试不使用 C++ modules；显式关闭，避免额外 CXX_SCAN 进程和 -fmodules-ts 开销。
set(COSINE_TEST_TARGETS
    testglfwinit
    test_ui_layout
    test_ui_events
    test_ui_controls
    test_text
    test_render_target
    test_ui_demo
    test_ui_context
    test_key_code
    test_debug_metrics
    test_asset_path
    test_version
    test_ui_gallery
    test_ui_theme
    test_ui_widgets
    test_ui_scrollbar
    test_ui_dropdown
    test_ui_text_input
    test_text_display
    test_ui_tab_view
    test_ui_dialog
    test_ui_menu_bar
    test_ui_table
    test_interpreter_token
    test_intepreter_tree_structure
    test_intepreter_tree_evaluator
)
set_target_properties(${COSINE_TEST_TARGETS} PROPERTIES CXX_SCAN_FOR_MODULES OFF)

add_test(NAME ui_layout COMMAND test_ui_layout)
add_test(NAME ui_events COMMAND test_ui_events)
add_test(NAME interpreter_token COMMAND test_interpreter_token)
add_test(NAME interpreter_tree_structure COMMAND test_intepreter_tree_structure)
add_test(NAME interpreter_tree_evaluator COMMAND test_intepreter_tree_evaluator)

set_tests_properties(
    interpreter_token
    interpreter_tree_structure
    interpreter_tree_evaluator
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME ui_controls COMMAND test_ui_controls)
add_test(NAME render_target COMMAND test_render_target)
add_test(NAME ui_demo COMMAND test_ui_demo)
add_test(NAME ui_context COMMAND test_ui_context)
add_test(NAME key_code COMMAND test_key_code)
add_test(NAME debug_metrics COMMAND test_debug_metrics)
add_test(NAME asset_path COMMAND test_asset_path)
add_test(NAME version COMMAND test_version)
add_test(NAME ui_gallery COMMAND test_ui_gallery)
add_test(NAME ui_theme COMMAND test_ui_theme)
add_test(NAME ui_widgets COMMAND test_ui_widgets)
add_test(NAME ui_scrollbar COMMAND test_ui_scrollbar)
add_test(NAME ui_dropdown COMMAND test_ui_dropdown)
add_test(NAME ui_text_input COMMAND test_ui_text_input)
add_test(NAME text_display COMMAND test_text_display)
add_test(NAME ui_tab_view COMMAND test_ui_tab_view)
add_test(NAME ui_dialog COMMAND test_ui_dialog)
add_test(NAME ui_menu_bar COMMAND test_ui_menu_bar)
add_test(NAME ui_table COMMAND test_ui_table)
add_test(NAME text_utf8 COMMAND test_text)

foreach(test_target IN LISTS COSINE_TEST_TARGETS)
    cosine_apply_sanitizers(${test_target})
endforeach()
