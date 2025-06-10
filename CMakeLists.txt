# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16)
project(Sirius LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Build Options ---
option(BUILD_POCL "Build POCL from source" ON)
option(POCL_VERBOSE "Enable verbose POCL build output" OFF)

# --- Platform Definitions ---
if(WIN32)
    add_definitions(-D_WIN32 -DNOMINMAX -D_USE_MATH_DEFINES)
elseif(UNIX AND NOT APPLE)
    add_definitions(-D_GNU_SOURCE)
endif()

# === Find System Dependencies (excluding OpenCL, which we will build) ===
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# === Add Subdirectories for Dependencies ===
add_subdirectory(deps/glfw)

# === Main Executable Definition ===
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

# === POCL Build Section ===
# This section builds POCL as a standalone library suitable for direct linking.
if(BUILD_POCL AND UNIX AND NOT APPLE)
    message(STATUS "=== POCL build is enabled (standalone mode) ===")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/pocl/CMakeLists.txt")
        message(FATAL_ERROR "Pocl submodule not found. Please run: git submodule update --init --recursive")
    endif()

    find_program(LLVM_CONFIG llvm-config-16 llvm-config-15 llvm-config-14 llvm-config)
    if(NOT LLVM_CONFIG)
        message(FATAL_ERROR "LLVM not found! Install a compatible version (e.g., sudo apt install llvm-16-dev clang-16)")
    endif()

    set(POCL_BUILD_DIR "${CMAKE_BINARY_DIR}/pocl-build")
    set(POCL_INSTALL_DIR "${CMAKE_BINARY_DIR}/pocl-install")

    include(ExternalProject)
    ExternalProject_Add(pocl_external
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/pocl"
        BINARY_DIR "${POCL_BUILD_DIR}"
        INSTALL_DIR "${POCL_INSTALL_DIR}"
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${POCL_INSTALL_DIR}
            -DCMAKE_BUILD_TYPE=Release
            -DWITH_LLVM_CONFIG=${LLVM_CONFIG}
            -DENABLE_ICD=OFF # This is the key change to build a standalone library
            -DENABLE_EXAMPLES=OFF
            -DENABLE_TESTS=OFF
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --parallel
        LOG_CONFIGURE ${POCL_VERBOSE}
        LOG_BUILD ${POCL_VERBOSE}
        LOG_INSTALL ${POCL_VERBOSE}
        TIMEOUT 3600
    )

    # Ensure pocl is built before the main application
    add_dependencies(Sirius pocl_external)

    # Add the include directory from our custom POCL build
    include_directories("${POCL_INSTALL_DIR}/include")
endif()

# === Include Directories ===
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/Minkowski"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/tinyobjloader"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include"
)

# === Compiler Definitions and Flags ===
target_compile_definitions(Sirius PRIVATE
    CL_HPP_TARGET_OPENCL_VERSION=300
    CL_HPP_MINIMUM_OPENCL_VERSION=200
)

if(WIN32 AND MSVC)
    target_compile_options(Sirius PRIVATE /wd4996 /wd4267 /wd4244)
elseif(UNIX)
    target_compile_options(Sirius PRIVATE -Wall -Wextra -O3)
endif()

# === Link Libraries ===
target_link_libraries(Sirius PRIVATE
    glfw
    OpenGL::GL
    Threads::Threads
)

if(BUILD_POCL AND UNIX AND NOT APPLE)
    # Link directly to the libpocl.so we just built
    target_link_libraries(Sirius PRIVATE "${POCL_INSTALL_DIR}/lib/libpocl.so")

    # Set the RPATH. This tells the executable where to find libpocl.so at runtime,
    # eliminating the need for the environment script.
    set_target_properties(Sirius PROPERTIES
        BUILD_RPATH "${POCL_INSTALL_DIR}/lib"
        INSTALL_RPATH "${POCL_INSTALL_DIR}/lib"
    )
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(Sirius PRIVATE dl) # Dynamic linking library
endif()

# === Plugins ===
set(PLUGIN_OUTPUT_PATH "${CMAKE_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_PATH})

add_library(MinkowskiMetric SHARED plugins/Minkowski/MinkowskiMetric.cpp)
target_include_directories(MinkowskiMetric PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
set_target_properties(MinkowskiMetric PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}"
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}"
)

# === Resource Copying ===
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/kernels DESTINATION ${CMAKE_BINARY_DIR})

# === Final Status Messages ===
message(STATUS "")
message(STATUS "=== Sirius Build Configuration Summary ===")
message(STATUS "Build & Run Instructions:")
message(STATUS "1. Run: make -j$(nproc)")
message(STATUS "2. Run the application directly: ./Sirius")
message(STATUS "(The setup_pocl_env.sh script is no longer needed)")
message(STATUS "=========================================")