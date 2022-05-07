add_library (glad STATIC)
target_sources (glad PRIVATE "glad/gl.cpp")
target_include_directories (glad PUBLIC "${CMAKE_SOURCE_DIR}/include/glad")

add_library (imgui STATIC)
target_sources (imgui PRIVATE
    "imgui/imgui_demo.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_impl_glfw.cpp"
    "imgui/imgui_impl_opengl3.cpp"
    "imgui/imgui_tables.cpp"
    "imgui/imgui_widgets.cpp"
    "imgui/imgui.cpp"
    "imgui/implot_demo.cpp"
    "imgui/implot_items.cpp"
    "imgui/implot.cpp"
)
target_include_directories (imgui PUBLIC "${CMAKE_SOURCE_DIR}/include/imgui")


add_executable (blockytry)

target_sources (blockytry PRIVATE
    "core/runtime.cpp"
    "core/cpu_profiler.cpp"
    "main.cpp"
)

target_link_libraries (blockytry PRIVATE
    glad
    glfw
    glm::glm
    spdlog::spdlog
    imgui
)

configure_file ("${CMAKE_SOURCE_DIR}/include/version.hpp.in"
    "${PROJECT_BINARY_DIR}/generated/version.hpp"
)

target_include_directories (blockytry PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/generated"
)

if (NOT DEFAULT_SHADER_PATH)
    set (DEFAULT_SHADER_PATH "${CMAKE_SOURCE_DIR}/shaders/")
endif ()

target_compile_definitions (blockytry PRIVATE
    "-DDEFAULT_SHADER_PATH=\"${DEFAULT_SHADER_PATH}\""
)

# target_compile_options(blockytry PRIVATE
#     $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
#     $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
# )
