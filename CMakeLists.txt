# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16)
project(Sirius LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Platform-Specific Setup ---
if(WIN32)
    set(IS_WINDOWS TRUE)
    add_definitions(-D_WIN32 -DNOMINMAX -D_USE_MATH_DEFINES)
else()
    set(IS_WINDOWS FALSE)
    add_definitions(-D_GNU_SOURCE)
endif()

# --- Build Options (Linux-only for now) ---
if(NOT IS_WINDOWS)
    option(BUILD_POCL "Build POCL from source (Linux only)" ON)
    option(POCL_VERBOSE "Enable verbose POCL build output" OFF)
endif()

# === Find System Dependencies ===
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

if(IS_WINDOWS)
    # On Windows, find the system OpenCL SDK (e.g., from NVIDIA CUDA Toolkit)
    message(STATUS "Windows: Looking for OpenCL SDK...")
    find_package(OpenCL)
    if(NOT OpenCL_FOUND)
        message(STATUS "find_package(OpenCL) failed, searching manually...")
        find_path(OpenCL_INCLUDE_DIRS CL/cl.h
            PATHS "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v*/include"
                  "C:/Program Files (x86)/Intel/OpenCL SDK/include"
                  "$ENV{CUDA_PATH}/include"
        )
        find_library(OpenCL_LIBRARIES OpenCL
            PATHS "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v*/lib/x64"
                  "C:/Program Files (x86)/Intel/OpenCL SDK/lib/x64"
                  "$ENV{CUDA_PATH}/lib/x64"
        )
        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(OpenCL DEFAULT_MSG OpenCL_LIBRARIES OpenCL_INCLUDE_DIRS)
    endif()
    if(NOT OpenCL_FOUND)
        message(FATAL_ERROR "OpenCL SDK not found on Windows. Please install the NVIDIA CUDA Toolkit or another OpenCL SDK.")
    endif()
else()
    # On Linux, use the existing logic for finding system OpenCL or building POCL
    if(NOT BUILD_POCL)
        find_package(OpenCL REQUIRED)
    endif()
endif()

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
if(OpenCL_FOUND OR (NOT IS_WINDOWS AND BUILD_POCL))
    # Add OpenCL includes
    target_include_directories(Sirius PRIVATE ${OpenCL_INCLUDE_DIRS})
endif()

# === Compiler Definitions and Flags ===
if(OpenCL_FOUND OR (NOT IS_WINDOWS AND BUILD_POCL))
    target_compile_definitions(Sirius PRIVATE
        CL_HPP_TARGET_OPENCL_VERSION=300
        CL_HPP_MINIMUM_OPENCL_VERSION=200
    )
endif()

if(IS_WINDOWS)
    target_compile_options(Sirius PRIVATE /W4 /wd4996 /wd4267 /wd4244)
else()
    target_compile_options(Sirius PRIVATE -Wall -Wextra -O3)
endif()

# === Link Libraries ===
target_link_libraries(Sirius PRIVATE
    glfw
    OpenGL::GL
    Threads::Threads
)

if(IS_WINDOWS)
    if(OpenCL_FOUND)
        target_link_libraries(Sirius PRIVATE ${OpenCL_LIBRARIES})
    endif()
else() # Linux linking logic
    if(BUILD_POCL)
        # Link directly to the libOpenCL.so we built with POCL
        add_dependencies(Sirius pocl_external) # Make sure POCL builds first
        target_link_libraries(Sirius PRIVATE "${POCL_INSTALL_DIR}/lib/libOpenCL.so")
        set_target_properties(Sirius PROPERTIES BUILD_RPATH "${POCL_INSTALL_DIR}/lib")
    else()
        # Link to system OpenCL if not building POCL
        target_link_libraries(Sirius PRIVATE OpenCL::OpenCL)
    endif()
    target_link_libraries(Sirius PRIVATE dl) # Dynamic linking library
endif()

# === POCL Build Section (Linux only) ===
if(NOT IS_WINDOWS AND BUILD_POCL)
    set(POCL_BUILD_DIR "${CMAKE_BINARY_DIR}/pocl-build")
    set(POCL_INSTALL_DIR "${CMAKE_BINARY_DIR}/pocl-install")
    include(ExternalProject)
    ExternalProject_Add(pocl_external
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/pocl"
        BINARY_DIR "${POCL_BUILD_DIR}"
        INSTALL_DIR "${POCL_INSTALL_DIR}"
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${POCL_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release -DWITH_LLVM_CONFIG=${LLVM_CONFIG} -DENABLE_ICD=OFF -DENABLE_EXAMPLES=OFF -DENABLE_TESTS=OFF
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --parallel
        LOG_CONFIGURE ${POCL_VERBOSE} LOG_BUILD ${POCL_VERBOSE} LOG_INSTALL ${POCL_VERBOSE}
        TIMEOUT 3600
    )
    target_include_directories(Sirius PRIVATE "${POCL_INSTALL_DIR}/include")
endif()

# === Plugins ===
set(PLUGIN_OUTPUT_PATH "${CMAKE_BINARY_DIR}/plugins")
if(IS_WINDOWS)
    set(PLUGIN_OUTPUT_PATH "${CMAKE_BINARY_DIR}/plugins/Release") # For multi-config generators
endif()
file(MAKE_DIRECTORY ${PLUGIN_OUTPUT_PATH})

add_library(MinkowskiMetric SHARED plugins/Minkowski/MinkowskiMetric.cpp)
target_include_directories(MinkowskiMetric PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
set_target_properties(MinkowskiMetric PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}" RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_PATH}")

# === Resource Copying ===
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/kernels DESTINATION ${CMAKE_BINARY_DIR})

# === Final Status Messages ===
message(STATUS "Build & Run Instructions:")
if(IS_WINDOWS)
    message(STATUS "1. Open this project in Visual Studio and build the 'Sirius' target.")
    message(STATUS "2. Or run: cmake --build . --config Release")
    message(STATUS "3. The executable will be in the 'Release' subfolder.")
else()
    message(STATUS "1. Run: make -j$(nproc)")
    message(STATUS "2. Run the application directly: ./Sirius")
endif()
message(STATUS "=========================================")