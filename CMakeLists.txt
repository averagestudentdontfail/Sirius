# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16)
project(Sirius LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build configuration
option(BUILD_POCL "Build POCL from source" ON)
option(POCL_VERBOSE "Enable verbose POCL build output" OFF)
option(FORCE_SYSTEM_OPENCL "Force use of system OpenCL even on Linux" OFF)

# Platform detection
if(WIN32)
    message(STATUS "Building for Windows - using system OpenCL")
    set(USE_SYSTEM_OPENCL ON)
    set(BUILD_POCL OFF)
    add_definitions(-D_WIN32 -DNOMINMAX -D_USE_MATH_DEFINES)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux")
    add_definitions(-D_GNU_SOURCE)
    if(FORCE_SYSTEM_OPENCL)
        set(BUILD_POCL OFF)
        message(STATUS "Forced to use system OpenCL")
    endif()
endif()

# Find system packages
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# === POCL BUILD SECTION ===
if(BUILD_POCL AND UNIX AND NOT APPLE)
    message(STATUS "=== Attempting to build POCL 7.0+ from source ===")
    
    # Check if POCL submodule exists
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/pocl/CMakeLists.txt")
        message(WARNING "POCL submodule not found! Run: git submodule update --init --recursive")
        message(STATUS "Falling back to system OpenCL...")
        set(BUILD_POCL OFF)
    else()
        # Check for required dependencies
        find_program(LLVM_CONFIG llvm-config-16 llvm-config-15 llvm-config-14 llvm-config)
        find_program(CLANG_EXECUTABLE clang-16 clang-15 clang-14 clang)
        
        if(NOT LLVM_CONFIG)
            message(WARNING "LLVM not found! Install: sudo apt install llvm-16-dev clang-16")
            message(STATUS "Falling back to system OpenCL...")
            set(BUILD_POCL OFF)
        elseif(NOT CLANG_EXECUTABLE)
            message(WARNING "Clang not found! Install: sudo apt install clang-16 libclang-16-dev")
            message(STATUS "Falling back to system OpenCL...")
            set(BUILD_POCL OFF)
        else()
            # Check for clang-cpp library
            execute_process(
                COMMAND ${LLVM_CONFIG} --libdir
                OUTPUT_VARIABLE LLVM_LIBDIR
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            
            set(CLANG_CPP_LIB "${LLVM_LIBDIR}/libclang-cpp.so")
            if(NOT EXISTS "${CLANG_CPP_LIB}")
                message(WARNING "libclang-cpp.so not found at ${CLANG_CPP_LIB}")
                message(STATUS "Install missing packages: sudo apt install libclang-cpp16-dev libclang-16-dev")
                message(STATUS "Falling back to system OpenCL...")
                set(BUILD_POCL OFF)
            else()
                message(STATUS "✅ POCL dependencies found")
                message(STATUS "Using LLVM: ${LLVM_CONFIG}")
                message(STATUS "Using Clang: ${CLANG_EXECUTABLE}")
                message(STATUS "Found clang-cpp: ${CLANG_CPP_LIB}")
            endif()
        endif()
    endif()
endif()

# Build POCL if dependencies are satisfied
if(BUILD_POCL AND UNIX AND NOT APPLE)
    message(STATUS "=== Building POCL 7.0+ from source ===")
    
    # POCL build configuration
    set(POCL_BUILD_DIR "${CMAKE_BINARY_DIR}/pocl-build")
    set(POCL_INSTALL_DIR "${CMAKE_BINARY_DIR}/pocl-install")
    
    # Create directories
    file(MAKE_DIRECTORY "${POCL_INSTALL_DIR}/include")
    file(MAKE_DIRECTORY "${POCL_INSTALL_DIR}/lib")
    
    # Configure POCL as ExternalProject with robust error handling
    include(ExternalProject)
    ExternalProject_Add(pocl_external
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/pocl"
        BINARY_DIR "${POCL_BUILD_DIR}"
        INSTALL_DIR "${POCL_INSTALL_DIR}"
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${POCL_INSTALL_DIR}
            -DCMAKE_BUILD_TYPE=Release
            -DWITH_LLVM_CONFIG=${LLVM_CONFIG}
            -DENABLE_ICD=ON
            -DENABLE_LOADABLE_DRIVERS=ON
            -DPOCL_DEBUG_MESSAGES=OFF
            -DENABLE_TESTS=OFF
            -DENABLE_EXAMPLES=OFF
            -DENABLE_DOCS=OFF
            -DDIRECT_LINKAGE=OFF
            -DENABLE_POCL_BUILDING=ON
            -DSTATIC_LLVM=OFF
            -DENABLE_LLVM=ON
            -DENABLE_SPIR=ON
            -DENABLE_SPIRV=ON
            -DWITH_LLVM_CONFIG=${LLVM_CONFIG}
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --parallel
        LOG_CONFIGURE ${POCL_VERBOSE}
        LOG_BUILD ${POCL_VERBOSE}
        LOG_INSTALL ${POCL_VERBOSE}
        # Add timeout and error handling
        TIMEOUT 3600  # 1 hour timeout
    )
    
    # Set OpenCL paths
    set(OpenCL_FOUND TRUE)
    set(OpenCL_INCLUDE_DIRS "${POCL_INSTALL_DIR}/include")
    set(OpenCL_LIBRARIES "${POCL_INSTALL_DIR}/lib/libOpenCL.so")
    set(POCL_LIBRARY "${POCL_INSTALL_DIR}/lib/libpocl.so")
    
    message(STATUS "POCL will be built to: ${POCL_INSTALL_DIR}")
    
else()
    # Use system OpenCL
    message(STATUS "=== Using system OpenCL ===")
    
    find_package(OpenCL QUIET)
    if(NOT OpenCL_FOUND)
        # Manual OpenCL detection
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
            message(STATUS "OpenCL found manually: ${OPENCL_LIBRARY}")
        else()
            message(WARNING "OpenCL not found! Install: sudo apt install pocl-opencl-icd opencl-headers")
            message(STATUS "Building without OpenCL support (fallback rendering only)")
            set(OpenCL_FOUND FALSE)
        endif()
    endif()
endif()

# === DEPENDENCIES ===
add_subdirectory(deps/glfw)

# === INCLUDE DIRECTORIES ===
set(INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/Minkowski"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/tinyobjloader"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include"
)

if(OpenCL_FOUND)
    list(APPEND INCLUDE_DIRS ${OpenCL_INCLUDE_DIRS})
endif()

include_directories(${INCLUDE_DIRS})

# === MAIN EXECUTABLE ===
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

# === COMPILER FLAGS ===
target_compile_definitions(Sirius PRIVATE
    CL_HPP_TARGET_OPENCL_VERSION=300
    CL_HPP_MINIMUM_OPENCL_VERSION=200
)

if(OpenCL_FOUND)
    target_compile_definitions(Sirius PRIVATE OPENCL_AVAILABLE=1)
    if(BUILD_POCL)
        target_compile_definitions(Sirius PRIVATE USING_BUILT_POCL=1)
    endif()
else()
    target_compile_definitions(Sirius PRIVATE NO_OPENCL=1)
    message(STATUS "⚠️  Building without OpenCL - only fallback rendering available")
endif()

if(WIN32 AND MSVC)
    target_compile_options(Sirius PRIVATE /wd4996 /wd4267 /wd4244)
elseif(UNIX)
    target_compile_options(Sirius PRIVATE -Wall -Wextra -O3)
endif()

# === LINK LIBRARIES ===
target_link_libraries(Sirius PRIVATE
    glfw
    OpenGL::GL
    Threads::Threads
)

# Link OpenCL if available
if(OpenCL_FOUND)
    if(BUILD_POCL AND UNIX AND NOT APPLE)
        # For built POCL
        add_dependencies(Sirius pocl_external)
        target_link_libraries(Sirius PRIVATE "${OpenCL_LIBRARIES}")
        set_target_properties(Sirius PROPERTIES
            BUILD_RPATH "${POCL_INSTALL_DIR}/lib"
            INSTALL_RPATH "${POCL_INSTALL_DIR}/lib"
        )
    else()
        # For system OpenCL
        if(TARGET OpenCL::OpenCL)
            target_link_libraries(Sirius PRIVATE OpenCL::OpenCL)
        else()
            target_link_libraries(Sirius PRIVATE ${OpenCL_LIBRARIES})
        endif()
    endif()
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(Sirius PRIVATE dl)
endif()

# === PLUGINS ===
set(PLUGIN_OUTPUT_PATH "${CMAKE_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_PATH})

add_library(MinkowskiMetric SHARED plugins/Minkowski/MinkowskiMetric.cpp)
target_include_directories(MinkowskiMetric PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
set_target_properties(MinkowskiMetric PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}"
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}")

# === RESOURCE COPYING ===
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/kernels DESTINATION ${CMAKE_BINARY_DIR})

# === POST-BUILD SETUP FOR POCL ===
if(BUILD_POCL AND UNIX AND NOT APPLE AND OpenCL_FOUND)
    # Create OpenCL ICD configuration after POCL is built
    add_custom_command(TARGET Sirius POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/etc/OpenCL/vendors"
        COMMAND ${CMAKE_COMMAND} -E echo "${POCL_INSTALL_DIR}/lib/libpocl.so" > "${CMAKE_BINARY_DIR}/etc/OpenCL/vendors/pocl.icd"
        COMMENT "Setting up POCL ICD configuration"
        DEPENDS pocl_external
    )
    
    # Create environment setup script
    set(ENV_SETUP_SCRIPT "${CMAKE_BINARY_DIR}/setup_pocl_env.sh")
    file(GENERATE OUTPUT "${ENV_SETUP_SCRIPT}"
        CONTENT "#!/bin/bash
# POCL Environment Setup Script
export OPENCL_VENDOR_PATH=\"${CMAKE_BINARY_DIR}/etc/OpenCL/vendors\"
export LD_LIBRARY_PATH=\"${POCL_INSTALL_DIR}/lib:\$LD_LIBRARY_PATH\"
export POCL_DEVICES=cpu
echo \"✅ POCL environment configured!\"
echo \"📍 OpenCL Vendor Path: \$OPENCL_VENDOR_PATH\"
echo \"📚 Library Path: ${POCL_INSTALL_DIR}/lib\"
echo \"🎯 Ready to run: ./Sirius\"
"
    )
    
    # Make setup script executable
    add_custom_command(TARGET Sirius POST_BUILD
        COMMAND chmod +x "${ENV_SETUP_SCRIPT}"
        COMMENT "Making POCL environment script executable"
    )
endif()

# === STATUS MESSAGES ===
message(STATUS "")
message(STATUS "=== Sirius Build Configuration ===")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(BUILD_POCL AND OpenCL_FOUND)
    message(STATUS "OpenCL: Building POCL 7.0+ from source ✅")
    message(STATUS "📋 Build Instructions:")
    message(STATUS "1. make -j$(nproc)  # 15-45 minutes for POCL build")
    message(STATUS "2. source setup_pocl_env.sh")
    message(STATUS "3. ./Sirius")
    message(STATUS "")
    message(STATUS "⚠️  If POCL build fails, reconfigure with: -DFORCE_SYSTEM_OPENCL=ON")
elseif(OpenCL_FOUND)
    message(STATUS "OpenCL: Using system OpenCL ✅")
    message(STATUS "Build: make -j$(nproc) && ./Sirius")
else()
    message(STATUS "OpenCL: Not available ❌")
    message(STATUS "Build: make -j$(nproc) && ./Sirius (fallback rendering only)")
    message(STATUS "")
    message(STATUS "💡 To enable OpenCL, install: sudo apt install pocl-opencl-icd opencl-headers")
endif()

message(STATUS "=========================================")
message(STATUS "")