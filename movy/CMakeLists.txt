include_directories(${PROJECT_SOURCE_DIR}/imgui)
include_directories(${PROJECT_SOURCE_DIR}/imgui/backends)
include_directories(${PROJECT_SOURCE_DIR}/ImGuiColorTextEdit)

file(GLOB_RECURSE imgui_sources 
    ${PROJECT_SOURCE_DIR}/imgui/imgui.h
    ${PROJECT_SOURCE_DIR}/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imconfig.h
    # ${PROJECT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_internal.h
    ${PROJECT_SOURCE_DIR}/imgui/imstb_truetype.h
    ${PROJECT_SOURCE_DIR}/imgui/imstb_textedit.h
    ${PROJECT_SOURCE_DIR}/imgui/imstb_rectpack.h
    # backend
    ${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.h
    ${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.h
    ${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp
    ${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3_loader.h
)
add_library(imguifull SHARED ${imgui_sources})
target_link_libraries(imguifull PRIVATE ${PROJECT_SOURCE_DIR}/libs/glfw/libglfw3dll.a)

add_executable(movy 
    main.cpp
    layout.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/ImGuiColorTextEdit/ImTextEditor.cpp
)
target_link_libraries(movy opengl32 imguifull ${PROJECT_SOURCE_DIR}/libs/glfw/libglfw3dll.a)
