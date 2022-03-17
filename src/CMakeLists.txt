add_executable (blockytry)

target_sources (blockytry PRIVATE
    "main.cpp"
)

target_link_libraries (blockytry PRIVATE
    glfw
    glm::glm
)

configure_file ("${CMAKE_SOURCE_DIR}/include/version.hpp.in"
    "${PROJECT_BINARY_DIR}/generated/version.hpp"
)

target_include_directories (blockytry PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/generated"
)

if (NOT DEFAULT_SHADER_PATH)
    set (DEFAULT_SHADER_PATH "${CMAKE_SOURCE_DIR}/src/shaders/")
endif ()

target_compile_definitions (blockytry PRIVATE
    "-DDEFAULT_SHADER_PATH=\"${DEFAULT_SHADER_PATH}\""
)
