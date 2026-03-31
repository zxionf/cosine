include_directories(${PROJECT_SOURCE_DIR}/imgui)
include_directories(${PROJECT_SOURCE_DIR}/imgui/backends)
include_directories(${PROJECT_SOURCE_DIR}/interpreter)

add_executable(testglfwinit testglfwinit.cpp)
target_link_libraries(testglfwinit PRIVATE glfw)

file(GLOB_RECURSE imgui_sources 
    ${PROJECT_SOURCE_DIR}/imgui/imgui.h
    ${PROJECT_SOURCE_DIR}/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imconfig.h
    ${PROJECT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_internal.h
    ${PROJECT_SOURCE_DIR}/imgui/imstb_truetype.h
    ${PROJECT_SOURCE_DIR}/imgui/imstb_textedit.h
    ${PROJECT_SOURCE_DIR}/imgui/imstb_rectpack.h
    # Backends
    ${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.h
    ${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.h
    ${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp
    ${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3_loader.h
)
add_executable(testimgui 
    testimgui.cpp
    ${imgui_sources}
)
target_link_libraries(testimgui PRIVATE GL glfw)

add_executable(testdll testimgui.cpp ${PROJECT_SOURCE_DIR}/imgui/imgui_demo.cpp)
target_link_libraries(testdll GL imguifull glfw)

add_executable(testdocking testdocking.cpp ${PROJECT_SOURCE_DIR}/imgui/imgui_demo.cpp)
target_link_libraries(testdocking GL imguifull glfw)

file(GLOB_RECURSE interpreter_test
    ${PROJECT_SOURCE_DIR}/interpreter/*.cpp
)
list(FILTER interpreter_test EXCLUDE REGEX "main.cpp")
add_executable(test_interpreter_token test_interpreter_token.cpp ../interpreter/token.cpp ../interpreter/lexer.cpp)
add_executable(test_intepreter_tree_structure test_intepreter_tree_structure.cpp ${interpreter_test})
add_executable(test_intepreter_tree_evaluator test_intepreter_tree_evaluator.cpp ${interpreter_test})
