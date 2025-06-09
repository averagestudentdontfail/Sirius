# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16)
project(Sirius LANGUAGES CXX C)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find Required Packages -------------------------------------------------
find_package(OpenGL REQUIRED)
find_package(OpenCL REQUIRED)
find_package(Threads REQUIRED)

# --- Add Dependencies -------------------------------------------------------
add_subdirectory(deps/glfw)

# --- Configure Include Directories ------------------------------------------
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/tinyobjloader"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include"
)

# --- Add Application Source Files --------------------------------------------
add_executable(Sirius
    src/main.cpp
    src/Core/Application.cpp
    src/Core/Window.cpp
    src/UI/UIManager.cpp
    # Add glad.c from our deps folder
    deps/glad/src/glad.c
    # Add ImGui source files directly
    deps/imgui/imgui.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui/imgui_demo.cpp
    deps/imgui/backends/imgui_impl_glfw.cpp
    deps/imgui/backends/imgui_impl_opengl3.cpp
)

# --- Link Libraries to the Main Application ---------------------------------
target_link_libraries(Sirius PRIVATE
    glfw
    imgui
    OpenGL::GL
    OpenCL::OpenCL
    Threads::Threads
)

# Add platform-specific library for dynamic loading
if(UNIX AND NOT APPLE)
    target_link_libraries(Sirius PRIVATE dl)
endif()

# --- Define a Metric Plugin -------------------------------------------------

# Set a variable for where plugins will be output
set(PLUGIN_OUTPUT_PATH "${CMAKE_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_PATH})

# Add the Minkowski metric plugin
add_library(minkowski_plugin SHARED
    plugins/minkowski/MinkowskiPlugin.cpp
)

set_target_properties(minkowski_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}"
)

# ----------------------------------------------------------------------------

message(STATUS "CMake configured for Sirius project.")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Plugin output directory: ${PLUGIN_OUTPUT_PATH}")