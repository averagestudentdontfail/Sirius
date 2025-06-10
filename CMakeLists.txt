# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16)
project(Sirius LANGUAGES CXX C)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection and configuration
if(WIN32)
    message(STATUS "Building for Windows - GPU OpenCL expected")
    set(PLATFORM_TYPE "Windows")
    # Add Windows-specific definitions
    add_definitions(-D_WIN32 -DNOMINMAX)
    # Define M_PI for MSVC
    add_definitions(-D_USE_MATH_DEFINES)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux - POCL CPU OpenCL expected")
    set(PLATFORM_TYPE "Linux")
    # Add Linux-specific definitions
    add_definitions(-D_GNU_SOURCE)
endif()

# --- Find Required Packages -------------------------------------------------
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# OpenCL handling with better error messages
find_package(OpenCL QUIET)
if(OpenCL_FOUND)
    message(STATUS "OpenCL found: ${OpenCL_VERSION_STRING}")
    message(STATUS "OpenCL include dir: ${OpenCL_INCLUDE_DIRS}")
    message(STATUS "OpenCL libraries: ${OpenCL_LIBRARIES}")
else()
    message(WARNING "OpenCL not found by CMake. Trying manual detection...")
    
    if(WIN32)
        # Windows: Look for NVIDIA, Intel, or AMD OpenCL
        find_path(OPENCL_INCLUDE_DIR 
            NAMES CL/cl.h CL/opencl.h opencl.h
            PATHS
                "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/*/include"
                "C:/Program Files (x86)/Intel/OpenCL SDK/*/include"
                "C:/Program Files/AMD APP SDK/*/include"
                "$ENV{CUDA_PATH}/include"
                "$ENV{OPENCL_ROOT}/include"
        )
        
        find_library(OPENCL_LIBRARY
            NAMES OpenCL
            PATHS
                "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/*/lib/x64"
                "C:/Program Files (x86)/Intel/OpenCL SDK/*/lib/x64"
                "C:/Program Files/AMD APP SDK/*/lib/x86_64"
                "$ENV{CUDA_PATH}/lib/x64"
                "$ENV{OPENCL_ROOT}/lib"
        )
    else()
        # Linux: Look for POCL or other OpenCL implementations
        find_path(OPENCL_INCLUDE_DIR 
            NAMES CL/cl.h
            PATHS
                /usr/include
                /usr/local/include
                /opt/pocl/include
        )
        
        find_library(OPENCL_LIBRARY
            NAMES OpenCL pocl
            PATHS
                /usr/lib
                /usr/lib/x86_64-linux-gnu
                /usr/local/lib
                /opt/pocl/lib
        )
    endif()
    
    if(OPENCL_INCLUDE_DIR AND OPENCL_LIBRARY)
        message(STATUS "Manual OpenCL detection successful:")
        message(STATUS "  Include: ${OPENCL_INCLUDE_DIR}")
        message(STATUS "  Library: ${OPENCL_LIBRARY}")
        set(OpenCL_INCLUDE_DIRS ${OPENCL_INCLUDE_DIR})
        set(OpenCL_LIBRARIES ${OPENCL_LIBRARY})
        set(OpenCL_FOUND TRUE)
    else()
        message(FATAL_ERROR "OpenCL not found! Please install:\n"
                           "  Windows: NVIDIA CUDA, Intel OpenCL SDK, or AMD APP SDK\n"
                           "  Linux: sudo apt install pocl-opencl-icd opencl-headers")
    endif()
endif()

# --- Add Dependencies -------------------------------------------------------
add_subdirectory(deps/glfw)

# --- Configure Include Directories ------------------------------------------
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

# --- Add Application Source Files --------------------------------------------
add_executable(Sirius
    src/main.cpp
    src/Core/Application.cpp
    src/Core/Window.cpp
    src/Core/PluginManager.cpp
    src/Graphics/Renderer.cpp
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

# --- Platform-specific compiler flags ---------------------------------------
if(WIN32)
    # Windows-specific flags
    target_compile_definitions(Sirius PRIVATE 
        _WIN32
        NOMINMAX
        _USE_MATH_DEFINES
        CL_HPP_TARGET_OPENCL_VERSION=120
    )
    if(MSVC)
        # Disable specific MSVC warnings
        target_compile_options(Sirius PRIVATE /wd4996 /wd4267 /wd4244)
    endif()
elseif(UNIX)
    # Linux-specific flags
    target_compile_definitions(Sirius PRIVATE
        _GNU_SOURCE
        CL_HPP_TARGET_OPENCL_VERSION=120
    )
    target_compile_options(Sirius PRIVATE -Wall -Wextra)
endif()

# --- Link Libraries to the Main Application ---------------------------------
target_link_libraries(Sirius PRIVATE
    glfw
    OpenGL::GL
    ${OpenCL_LIBRARIES}
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
add_library(MinkowskiMetric SHARED
    plugins/Minkowski/MinkowskiMetric.cpp
)

# Add include directories for the plugin
target_include_directories(MinkowskiMetric PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set_target_properties(MinkowskiMetric PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}"
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}"  # For Windows DLLs
)

# Copy kernels directory to build output
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/kernels 
     DESTINATION ${CMAKE_BINARY_DIR})

# ----------------------------------------------------------------------------

message(STATUS "CMake configured for Sirius project (${PLATFORM_TYPE})")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Plugin output directory: ${PLUGIN_OUTPUT_PATH}")
message(STATUS "Kernels copied to: ${CMAKE_BINARY_DIR}/kernels")
message(STATUS "OpenCL libraries: ${OpenCL_LIBRARIES}")

# Print OpenCL detection instructions
if(WIN32)
    message(STATUS "Windows build: Expecting GPU OpenCL (NVIDIA/Intel/AMD)")
    message(STATUS "Make sure you have NVIDIA drivers or Intel OpenCL runtime installed")
else()
    message(STATUS "Linux build: Expecting POCL CPU OpenCL")
    message(STATUS "If OpenCL fails, run: sudo apt install pocl-opencl-icd opencl-headers clinfo")
endif()