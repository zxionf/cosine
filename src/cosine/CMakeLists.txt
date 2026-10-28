add_library(cosine_editor STATIC
    editor.cpp
    texture_cache.cpp
)
target_include_directories(cosine_editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(cosine_editor PUBLIC
    cosine_app
    cosine_anim
)

if(MSVC)
    target_compile_options(cosine_editor PRIVATE /W4)
else()
    target_compile_options(cosine_editor PRIVATE -Wall -Wextra -Wshadow)
endif()
cosine_apply_sanitizers(cosine_editor)

# 最终产品可执行：默认构建，可用 -DCOSINE_BUILD_EDITOR=OFF 关掉（只构建库/测试）。
if(COSINE_BUILD_EDITOR)
    add_executable(cosine main.cpp)
    target_link_libraries(cosine PRIVATE cosine_editor)
    if(COSINE_CXX_MODULE_SCAN)
        set_target_properties(cosine PROPERTIES CXX_SCAN_FOR_MODULES ON)
    endif()
    add_dependencies(cosine shaders copy_assets)
    cosine_apply_sanitizers(cosine)
endif()
