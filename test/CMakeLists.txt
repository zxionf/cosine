include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/interpreter)

add_executable(testglfwinit testglfwinit.cpp)
target_link_libraries(testglfwinit PRIVATE glfw)

add_executable(test_ui_layout
    test_ui_layout.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/vbox.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/root.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/scroll_view.cpp
)
target_link_libraries(test_ui_layout PRIVATE glm::glm)

file(GLOB_RECURSE interpreter_test
    ${PROJECT_SOURCE_DIR}/interpreter/*.cpp
)
list(FILTER interpreter_test EXCLUDE REGEX "main.cpp")

add_executable(test_interpreter_token test_interpreter_token.cpp ../interpreter/token.cpp ../interpreter/lexer.cpp)
add_executable(test_intepreter_tree_structure test_intepreter_tree_structure.cpp ${interpreter_test})
add_executable(test_intepreter_tree_evaluator test_intepreter_tree_evaluator.cpp ${interpreter_test})

add_test(NAME ui_layout COMMAND test_ui_layout)
add_test(NAME interpreter_token COMMAND test_interpreter_token)
add_test(NAME interpreter_tree_structure COMMAND test_intepreter_tree_structure)
add_test(NAME interpreter_tree_evaluator COMMAND test_intepreter_tree_evaluator)

set_tests_properties(
    interpreter_token
    interpreter_tree_structure
    interpreter_tree_evaluator
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
