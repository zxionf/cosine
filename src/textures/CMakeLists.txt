add_custom_target(textures ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_BINARY_DIR}/src/textures"
    COMMENT "Copying textures directory"
    VERBATIM
)