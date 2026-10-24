file(GLOB SHADER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.slang"
)

if(NOT SLANGC_EXECUTABLE)
    find_program(SLANGC_EXECUTABLE
        NAMES slangc
        HINTS
            "$ENV{HOME}/tools/slang/bin"
            "$ENV{ProgramFiles}/slang/bin"
    )
endif()
if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc not found. Install Slang or pass -DSLANGC_EXECUTABLE=<path>")
endif()

if(NOT Vulkan_GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Install the Vulkan SDK or pass -DVulkan_GLSLC_EXECUTABLE=<path>")
endif()

set(SHADER_OUTPUT_DIR "${COSINE_ASSET_BUILD_DIR}/shaders")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

set(SPV_FILES "")

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME "${SHADER}" NAME)
    get_filename_component(SHADER_EXT "${SHADER}" EXT)

    set(SPV "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")

    if(SHADER_EXT STREQUAL ".slang")
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
    else()
        add_custom_command(
            OUTPUT "${SPV}"
            COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${SHADER}" -o "${SPV}"
            DEPENDS "${SHADER}"
            COMMENT "Compiling ${SHADER_NAME}"
            VERBATIM
        )
    endif()
    list(APPEND SPV_FILES "${SPV}")
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_FILES})
