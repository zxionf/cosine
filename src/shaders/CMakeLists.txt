file(GLOB SHADER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.slang"
)

if(NOT SLANGC_EXECUTABLE)
    find_program(SLANGC_EXECUTABLE slangc)
endif()
if(NOT SLANGC_EXECUTABLE)
    set(SLANGC_EXECUTABLE "/home/zxionf/tools/slang/bin/slangc")
    if(WIN32)
        set(SLANGC_EXECUTABLE "D:/Program Files/slang-2026.17.1-windows-x86_64/bin/slangc.exe")
    endif()
endif()

# 输出目录：构建目录下的 src/shaders
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/src/shaders")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

set(SPV_FILES "")

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME "${SHADER}" NAME)
    get_filename_component(SHADER_EXT "${SHADER}" EXT)

    set(SPV "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")

    if(SHADER_EXT STREQUAL ".slang")
        if(SLANGC_EXECUTABLE)
            add_custom_command(
                OUTPUT "${SPV}"
                COMMAND "${SLANGC_EXECUTABLE}"
                        -target spirv
                        -profile spirv_1_4
                        -emit-spirv-directly
                        -fvk-use-entrypoint-name
                        -entry vertMain
                        -entry fragMain
                        -o "${SPV}"
                        "${SHADER}"
                DEPENDS "${SHADER}"
                COMMENT "Compiling Slang ${SHADER_NAME}"
                VERBATIM
            )
            list(APPEND SPV_FILES "${SPV}")
        endif()
    else()
        add_custom_command(
            OUTPUT "${SPV}"
            COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${SHADER}" -o "${SPV}"
            DEPENDS "${SHADER}"
            COMMENT "Compiling ${SHADER_NAME}"
            VERBATIM
        )
        list(APPEND SPV_FILES "${SPV}")
    endif()
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_FILES})