# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16)
project(Sirius LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform configuration
if(WIN32)
    message(STATUS "Building for Windows")
    add_definitions(-D_WIN32 -DNOMINMAX -D_USE_MATH_DEFINES)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux")
    add_definitions(-D_GNU_SOURCE)
endif()

# Find packages
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# Find OpenCL
find_package(OpenCL QUIET)
if(NOT OpenCL_FOUND)
    if(WIN32)
        find_path(OPENCL_INCLUDE_DIR NAMES CL/cl.h PATHS
            "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/*/include"
            "C:/Program Files (x86)/Intel/OpenCL SDK/*/include"
            "$ENV{CUDA_PATH}/include")
        find_library(OPENCL_LIBRARY NAMES OpenCL PATHS
            "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/*/lib/x64"
            "C:/Program Files (x86)/Intel/OpenCL SDK/*/lib/x64"
            "$ENV{CUDA_PATH}/lib/x64")
    else()
        find_path(OPENCL_INCLUDE_DIR NAMES CL/cl.h PATHS /usr/include /usr/local/include)
        find_library(OPENCL_LIBRARY NAMES OpenCL pocl PATHS /usr/lib /usr/lib/x86_64-linux-gnu)
    endif()
    
    if(OPENCL_INCLUDE_DIR AND OPENCL_LIBRARY)
        set(OpenCL_INCLUDE_DIRS ${OPENCL_INCLUDE_DIR})
        set(OpenCL_LIBRARIES ${OPENCL_LIBRARY})
        set(OpenCL_FOUND TRUE)
        message(STATUS "OpenCL found manually")
    else()
        message(FATAL_ERROR "OpenCL not found. Install NVIDIA drivers or POCL.")
    endif()
endif()

# Dependencies
add_subdirectory(deps/glfw)

# Include directories
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/Minkowski"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/tinyobjloader"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include"
    ${OpenCL_INCLUDE_DIRS}
)

# Main executable
add_executable(Sirius
    src/main.cpp
    src/Core/Application.cpp
    src/Core/Window.cpp
    src/Core/PluginManager.cpp
    src/Graphics/Renderer.cpp
    src/UI/UIManager.cpp
    deps/glad/src/glad.c
    deps/imgui/imgui.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui/imgui_demo.cpp
    deps/imgui/backends/imgui_impl_glfw.cpp
    deps/imgui/backends/imgui_impl_opengl3.cpp
)

# Compiler flags
target_compile_definitions(Sirius PRIVATE
    CL_HPP_TARGET_OPENCL_VERSION=300
    CL_HPP_MINIMUM_OPENCL_VERSION=200
)

if(WIN32 AND MSVC)
    target_compile_options(Sirius PRIVATE /wd4996 /wd4267 /wd4244)
elseif(UNIX)
    target_compile_options(Sirius PRIVATE -Wall -Wextra -O3)
endif()

# Link libraries
target_link_libraries(Sirius PRIVATE
    glfw
    OpenGL::GL
    ${OpenCL_LIBRARIES}
    Threads::Threads
)

if(UNIX AND NOT APPLE)
    target_link_libraries(Sirius PRIVATE dl)
endif()

# Plugin
set(PLUGIN_OUTPUT_PATH "${CMAKE_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_PATH})

add_library(MinkowskiMetric SHARED plugins/Minkowski/MinkowskiMetric.cpp)
target_include_directories(MinkowskiMetric PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
set_target_properties(MinkowskiMetric PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}"
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}")

# Copy kernels
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/kernels DESTINATION ${CMAKE_BINARY_DIR})

message(STATUS "Configuration complete")
message(STATUS "OpenCL: ${OpenCL_LIBRARIES}")