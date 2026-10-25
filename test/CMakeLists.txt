include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/interpreter)

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
target_link_libraries(test_text PRIVATE cosine_text glfw)

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
add_test(NAME text_utf8 COMMAND test_text)
