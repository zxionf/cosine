file(GLOB SHADER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
)

set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/src/shaders")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

set(SPV_FILES "")

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME "${SHADER}" NAME)
    set(SPV "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT "${SPV}"
        COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${SHADER}" -o "${SPV}"
        DEPENDS "${SHADER}"
        COMMENT "Compiling ${SHADER_NAME}"
        VERBATIM
    )
    list(APPEND SPV_FILES "${SPV}")
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_FILES})
