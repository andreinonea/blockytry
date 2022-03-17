cmake_minimum_required (VERSION 3.14)

project ("blockytry"
    VERSION 0.1.1
    LANGUAGES CXX
    DESCRIPTION "Blocky game for OpenGL learning purposes"
    HOMEPAGE_URL "https://github.com/andreinonea/blockytry"
)

# Policy regarding use of 'project' VERSION.
# https://cmake.org/cmake/help/latest/policy/CMP0048.html#policy:CMP0048
cmake_policy (SET CMP0048 NEW)

# Policy regarding searching packages in <name>_ROOT.
# https://cmake.org/cmake/help/latest/policy/CMP0074.html
cmake_policy (SET CMP0074 NEW)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED True)

if (UNIX)
    set (OpenGL_GL_PREFERENCE GLVND)
endif (UNIX)

find_package (glfw3 REQUIRED)
find_package (glm REQUIRED)

# Set build type
if (PROJECT_VERSION VERSION_LESS 1.0.0)
    set (blockytry_BUILD_TYPE "Beta")
endif()
if (PROJECT_VERSION VERSION_LESS 0.1.0)
    set (blockytry_BUILD_TYPE "Alpha")
endif()


add_subdirectory (src)
