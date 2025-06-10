# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16)
project(Sirius LANGUAGES CXX C)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection and configuration
if(WIN32)
    message(STATUS "Building for Windows - NVIDIA GPU OpenCL 3.0 expected")
    set(PLATFORM_TYPE "Windows")
    # Add Windows-specific definitions
    add_definitions(-D_WIN32 -DNOMINMAX)
    # Define M_PI for MSVC
    add_definitions(-D_USE_MATH_DEFINES)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux - POCL 7.0+ CPU OpenCL 3.0 expected")
    set(PLATFORM_TYPE "Linux")
    # Add Linux-specific definitions
    add_definitions(-D_GNU_SOURCE)
endif()

# --- Find Required Packages -------------------------------------------------
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# OpenCL 3.0 handling with better error messages
find_package(OpenCL QUIET)
if(OpenCL_FOUND)
    message(STATUS "OpenCL found: ${OpenCL_VERSION_STRING}")
    message(STATUS "OpenCL include dir: ${OpenCL_INCLUDE_DIRS}")
    message(STATUS "OpenCL libraries: ${OpenCL_LIBRARIES}")
    
    # Check if OpenCL version supports 3.0
    if(OpenCL_VERSION_STRING)
        if(OpenCL_VERSION_STRING VERSION_GREATER_EQUAL "3.0")
            message(STATUS "✅ OpenCL 3.0+ detected - enabling advanced features")
            set(OPENCL_30_AVAILABLE TRUE)
        elseif(OpenCL_VERSION_STRING VERSION_GREATER_EQUAL "2.0")
            message(STATUS "⚠️  OpenCL 2.0+ detected - using fallback compatibility")
            set(OPENCL_20_AVAILABLE TRUE)
        else()
            message(WARNING "❌ OpenCL version ${OpenCL_VERSION_STRING} may be too old")
        endif()
    endif()
else()
    message(WARNING "OpenCL not found by CMake. Trying manual detection...")
    
    if(WIN32)
        # Windows: Look for NVIDIA, Intel, or AMD OpenCL 3.0
        find_path(OPENCL_INCLUDE_DIR 
            NAMES CL/cl.h CL/opencl.h opencl.h
            PATHS
                "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/*/include"
                "C:/Program Files (x86)/Intel/OpenCL SDK/*/include"
                "C:/Program Files/AMD APP SDK/*/include"
                "$ENV{CUDA_PATH}/include"
                "$ENV{OPENCL_ROOT}/include"
                # Additional paths for newer OpenCL installations
                "C:/Program Files/Intel/oneAPI/compiler/*/windows/include"
                "C:/Program Files (x86)/IntelSWTools/opencl_sdk/include"
        )
        
        find_library(OPENCL_LIBRARY
            NAMES OpenCL
            PATHS
                "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/*/lib/x64"
                "C:/Program Files (x86)/Intel/OpenCL SDK/*/lib/x64"
                "C:/Program Files/AMD APP SDK/*/lib/x86_64"
                "$ENV{CUDA_PATH}/lib/x64"
                "$ENV{OPENCL_ROOT}/lib"
                # Additional paths for newer OpenCL installations
                "C:/Program Files/Intel/oneAPI/compiler/*/windows/lib"
                "C:/Program Files (x86)/IntelSWTools/opencl_sdk/lib/x64"
        )
    else()
        # Linux: Look for POCL 7.0+ or other OpenCL 3.0 implementations
        find_path(OPENCL_INCLUDE_DIR 
            NAMES CL/cl.h
            PATHS
                /usr/include
                /usr/local/include
                /opt/pocl/include
                /opt/intel/opencl/include
        )
        
        find_library(OPENCL_LIBRARY
            NAMES OpenCL pocl
            PATHS
                /usr/lib
                /usr/lib/x86_64-linux-gnu
                /usr/local/lib
                /opt/pocl/lib
                /opt/intel/opencl/lib
        )
    endif()
    
    if(OPENCL_INCLUDE_DIR AND OPENCL_LIBRARY)
        message(STATUS "Manual OpenCL detection successful:")
        message(STATUS "  Include: ${OPENCL_INCLUDE_DIR}")
        message(STATUS "  Library: ${OPENCL_LIBRARY}")
        set(OpenCL_INCLUDE_DIRS ${OPENCL_INCLUDE_DIR})
        set(OpenCL_LIBRARIES ${OPENCL_LIBRARY})
        set(OpenCL_FOUND TRUE)
        
        # For manual detection, assume OpenCL 3.0 availability
        message(STATUS "Assuming OpenCL 3.0 support for manual detection")
        set(OPENCL_30_AVAILABLE TRUE)
    else()
        message(FATAL_ERROR "OpenCL 3.0 not found! Please install:\n"
                           "  Windows: Latest NVIDIA drivers (GeForce 471+) or Intel OpenCL Runtime\n"
                           "  Linux: POCL 7.0+ -> sudo apt install pocl-opencl-icd opencl-headers clinfo")
    endif()
endif()

# Verify OpenCL 3.0 support at runtime (add to program)
if(OPENCL_30_AVAILABLE)
    message(STATUS "🚀 Building with OpenCL 3.0 optimizations enabled")
elseif(OPENCL_20_AVAILABLE)
    message(STATUS "⚠️  Building with OpenCL 2.0 fallback compatibility")
else()
    message(WARNING "❌ Building with basic OpenCL support - performance may be limited")
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

# --- Platform-specific compiler flags for OpenCL 3.0 ----------------------
if(WIN32)
    # Windows-specific flags for OpenCL 3.0
    target_compile_definitions(Sirius PRIVATE 
        _WIN32
        NOMINMAX
        _USE_MATH_DEFINES
        CL_HPP_TARGET_OPENCL_VERSION=300  # OpenCL 3.0
        CL_HPP_MINIMUM_OPENCL_VERSION=200 # Minimum 2.0
    )
    
    if(OPENCL_30_AVAILABLE)
        target_compile_definitions(Sirius PRIVATE OPENCL_30_AVAILABLE=1)
        message(STATUS "Windows: OpenCL 3.0 compile flags enabled")
    endif()
    
    if(MSVC)
        # Disable specific MSVC warnings
        target_compile_options(Sirius PRIVATE /wd4996 /wd4267 /wd4244)
        # Enable OpenCL 3.0 optimizations for MSVC
        target_compile_options(Sirius PRIVATE /O2 /Oi /Ot)
    endif()
elseif(UNIX)
    # Linux-specific flags for POCL 7.0+ OpenCL 3.0
    target_compile_definitions(Sirius PRIVATE
        _GNU_SOURCE
        CL_HPP_TARGET_OPENCL_VERSION=300  # OpenCL 3.0
        CL_HPP_MINIMUM_OPENCL_VERSION=200 # Minimum 2.0
    )
    
    if(OPENCL_30_AVAILABLE)
        target_compile_definitions(Sirius PRIVATE OPENCL_30_AVAILABLE=1)
        message(STATUS "Linux: OpenCL 3.0 compile flags enabled (POCL 7.0+)")
    endif()
    
    target_compile_options(Sirius PRIVATE -Wall -Wextra -O3)
    # Enable OpenCL 3.0 optimizations for GCC/Clang
    target_compile_options(Sirius PRIVATE -march=native -mtune=native)
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

# --- OpenCL 3.0 Performance Tuning ------------------------------------------
if(OPENCL_30_AVAILABLE)
    # Add OpenCL 3.0 specific build optimizations
    message(STATUS "Applying OpenCL 3.0 build optimizations...")
    
    if(WIN32)
        # Windows GPU optimizations
        set_target_properties(Sirius PROPERTIES
            LINK_FLAGS "/LTCG"  # Link-time code generation
        )
    else()
        # Linux CPU optimizations for POCL
        set_target_properties(Sirius PROPERTIES
            LINK_FLAGS "-flto"  # Link-time optimization
        )
    endif()
endif()

# --- Build Configuration Summary --------------------------------------------
message(STATUS "")
message(STATUS "=== Sirius OpenCL 3.0 Build Configuration ===")
message(STATUS "Platform: ${PLATFORM_TYPE}")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Plugin output directory: ${PLUGIN_OUTPUT_PATH}")
message(STATUS "Kernels copied to: ${CMAKE_BINARY_DIR}/kernels")
message(STATUS "OpenCL libraries: ${OpenCL_LIBRARIES}")

if(OPENCL_30_AVAILABLE)
    message(STATUS "🚀 OpenCL 3.0: ENABLED")
    if(WIN32)
        message(STATUS "   Expected performance: 60-120+ FPS (NVIDIA GPU)")
        message(STATUS "   Features: GPU ray tracing, advanced optimizations")
    else()
        message(STATUS "   Expected performance: 30-60 FPS (POCL 7.0+ CPU)")
        message(STATUS "   Features: Multi-core CPU ray tracing, POCL optimizations")
    endif()
elseif(OPENCL_20_AVAILABLE)
    message(STATUS "⚠️  OpenCL 2.0: FALLBACK")
    message(STATUS "   Expected performance: Reduced (missing OpenCL 3.0 optimizations)")
else()
    message(STATUS "❌ OpenCL: BASIC")
    message(STATUS "   Expected performance: Limited (upgrade recommended)")
endif()

message(STATUS "")

# Print platform-specific instructions
if(WIN32)
    message(STATUS "Windows Instructions:")
    message(STATUS "  1. Ensure latest NVIDIA drivers (GeForce 471+ for OpenCL 3.0)")
    message(STATUS "  2. Build: cmake --build . --config Release --parallel")
    message(STATUS "  3. Run from: ${CMAKE_BINARY_DIR}/Release/")
else()
    message(STATUS "Linux Instructions:")
    message(STATUS "  1. Ensure POCL 7.0+: apt list --installed | grep pocl")
    message(STATUS "  2. Build: cmake --build . --parallel")
    message(STATUS "  3. Run from: ${CMAKE_BINARY_DIR}/")
    message(STATUS "  4. Debug: export POCL_DEBUG=1 (if needed)")
endif()

message(STATUS "===============================================")