# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16)
project(Sirius LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build configuration
option(BUILD_POCL "Build POCL from source" ON)
option(POCL_VERBOSE "Enable verbose POCL build output" OFF)

# Platform detection
if(WIN32)
    message(STATUS "Building for Windows - using system OpenCL")
    set(USE_SYSTEM_OPENCL ON)
    set(BUILD_POCL OFF)  # Don't build POCL on Windows
    add_definitions(-D_WIN32 -DNOMINMAX -D_USE_MATH_DEFINES)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux - will build POCL from source")
    set(USE_SYSTEM_OPENCL OFF)
    add_definitions(-D_GNU_SOURCE)
endif()

# Find system packages
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# === POCL BUILD SECTION ===
if(BUILD_POCL AND UNIX AND NOT APPLE)
    message(STATUS "=== Building POCL 7.0+ from source ===")
    
    # Check if POCL submodule exists
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/pocl/CMakeLists.txt")
        message(FATAL_ERROR "POCL submodule not found! Run: git submodule update --init --recursive")
    endif()
    
    # POCL build configuration
    set(POCL_BUILD_DIR "${CMAKE_BINARY_DIR}/pocl-build")
    set(POCL_INSTALL_DIR "${CMAKE_BINARY_DIR}/pocl-install")
    
    # Required LLVM version for POCL
    find_program(LLVM_CONFIG llvm-config-16 llvm-config-15 llvm-config-14 llvm-config)
    if(NOT LLVM_CONFIG)
        message(FATAL_ERROR "LLVM not found! Install: sudo apt install llvm-16-dev clang-16")
    endif()
    
    message(STATUS "Using LLVM: ${LLVM_CONFIG}")
    
    # Configure POCL as ExternalProject
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
            -DDIRECT_LINKAGE=OFF
            -DENABLE_POCL_BUILDING=ON
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --parallel
        LOG_CONFIGURE ${POCL_VERBOSE}
        LOG_BUILD ${POCL_VERBOSE}
        LOG_INSTALL ${POCL_VERBOSE}
    )
    
    # Set OpenCL paths to use built POCL
    set(OpenCL_FOUND TRUE)
    set(OpenCL_INCLUDE_DIRS "${POCL_INSTALL_DIR}/include")
    set(OpenCL_LIBRARIES "${POCL_INSTALL_DIR}/lib/libOpenCL.so")
    set(POCL_LIBRARY "${POCL_INSTALL_DIR}/lib/libpocl.so")
    
    # Create imported target for OpenCL
    add_library(OpenCL::OpenCL SHARED IMPORTED)
    set_target_properties(OpenCL::OpenCL PROPERTIES
        IMPORTED_LOCATION "${OpenCL_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${OpenCL_INCLUDE_DIRS}"
    )
    
    # Ensure POCL is built before main target
    add_dependencies(OpenCL::OpenCL pocl_external)
    
    message(STATUS "POCL will be built to: ${POCL_INSTALL_DIR}")
    
else()
    # Use system OpenCL (Windows or when BUILD_POCL=OFF)
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
            message(FATAL_ERROR "OpenCL not found!")
        endif()
    endif()
endif()

# === DEPENDENCIES ===
add_subdirectory(deps/glfw)

# === INCLUDE DIRECTORIES ===
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

if(BUILD_POCL)
    target_compile_definitions(Sirius PRIVATE USING_BUILT_POCL=1)
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

# Link OpenCL (either built POCL or system OpenCL)
if(BUILD_POCL AND TARGET OpenCL::OpenCL)
    target_link_libraries(Sirius PRIVATE OpenCL::OpenCL)
    add_dependencies(Sirius pocl_external)
else()
    target_link_libraries(Sirius PRIVATE ${OpenCL_LIBRARIES})
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
if(BUILD_POCL AND UNIX AND NOT APPLE)
    # Create OpenCL ICD configuration
    add_custom_target(setup_pocl_icd ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/etc/OpenCL/vendors"
        COMMAND ${CMAKE_COMMAND} -E echo "${POCL_INSTALL_DIR}/lib/libpocl.so" > "${CMAKE_BINARY_DIR}/etc/OpenCL/vendors/pocl.icd"
        DEPENDS pocl_external
        COMMENT "Setting up POCL ICD configuration"
    )
    
    # Create a script to set up environment variables
    file(WRITE "${CMAKE_BINARY_DIR}/setup_pocl_env.sh"
        "#!/bin/bash\n"
        "export OPENCL_VENDOR_PATH=\"${CMAKE_BINARY_DIR}/etc/OpenCL/vendors\"\n"
        "export LD_LIBRARY_PATH=\"${POCL_INSTALL_DIR}/lib:\$LD_LIBRARY_PATH\"\n"
        "export POCL_DEVICES=cpu\n"
        "echo \"POCL environment configured!\"\n"
        "echo \"Run: source ${CMAKE_BINARY_DIR}/setup_pocl_env.sh\"\n"
        "echo \"Then: ./Sirius\"\n"
    )
    
    # Make the script executable
    file(COPY "${CMAKE_BINARY_DIR}/setup_pocl_env.sh"
         DESTINATION "${CMAKE_BINARY_DIR}"
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    
    add_dependencies(Sirius setup_pocl_icd)
endif()

# === STATUS MESSAGES ===
message(STATUS "")
message(STATUS "=== Sirius Build Configuration ===")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}")

if(BUILD_POCL)
    message(STATUS "OpenCL: Building POCL 7.0+ from source")
    message(STATUS "POCL install dir: ${POCL_INSTALL_DIR}")
    message(STATUS "")
    message(STATUS "📋 Build Instructions:")
    message(STATUS "1. mkdir build && cd build")
    message(STATUS "2. cmake .. -DCMAKE_BUILD_TYPE=Release")
    message(STATUS "3. make -j$(nproc)  # This will take 10-30 minutes")
    message(STATUS "4. source setup_pocl_env.sh")
    message(STATUS "5. ./Sirius")
else()
    message(STATUS "OpenCL: Using system installation")
    message(STATUS "OpenCL library: ${OpenCL_LIBRARIES}")
endif()

message(STATUS "=====================================")
message(STATUS "")