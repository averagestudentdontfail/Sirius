# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16)
project(Sirius LANGUAGES CXX C)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find Required Packages -------------------------------------------------
# Find OpenGL and OpenCL, which should be available on your system
find_package(OpenGL REQUIRED)
find_package(OpenCL REQUIRED)
find_package(Threads REQUIRED)

# --- Add Dependencies (from the 'deps' folder) -----------------------------
# Add the subdirectories for our dependencies. CMake will build them with our project.
# We will add these dependencies in the next steps.
add_subdirectory(deps/glfw)

# --- Configure Include Directories ------------------------------------------
# This allows our project to find the header files from our dependencies and source.
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
)

# --- Link Libraries to the Main Application ---------------------------------
target_link_libraries(Sirius PRIVATE
    # Our dependencies
    glfw
    # imgui removed, as we now add sources directly

    # System libraries
    OpenGL::GL
    OpenCL::OpenCL
    Threads::Threads
)

# --- Define a Metric Plugin (Example) ---------------------------------------
# This section demonstrates how we will build our metric plugins.
# We create a SHARED library for each metric. This allows the main application
# to load them at runtime without needing to be recompiled.

# Set a variable for where plugins will be output
set(PLUGIN_OUTPUT_PATH "${CMAKE_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_PATH})

# Example for a Schwarzschild plugin
add_library(schwarzschild_plugin SHARED
    plugins/schwarzschild/schwarzschild.cpp
)

# Ensure the plugin is built to the correct directory
set_target_properties(schwarzschild_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}"
)

# ----------------------------------------------------------------------------

message(STATUS "CMake configured for Sirius project.")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Plugin output directory: ${PLUGIN_OUTPUT_PATH}")