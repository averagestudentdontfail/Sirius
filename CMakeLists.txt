cmake_minimum_required(VERSION 3.16)
project(Sirius)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Fetch Dependencies ---
include(FetchContent)

# GLFW for windowing
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLM for math
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

# ImGui for UI
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.90.1
)
FetchContent_MakeAvailable(imgui)

# Find Python3
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Create glad directory
set(GLAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/glad)
file(MAKE_DIRECTORY ${GLAD_DIR})

# Create and use Python virtual environment for glad
set(VENV_DIR ${CMAKE_CURRENT_BINARY_DIR}/venv)
set(VENV_PYTHON ${VENV_DIR}/bin/python3)
set(VENV_PIP ${VENV_DIR}/bin/pip)

# Generate glad files
add_custom_command(
  OUTPUT
    ${GLAD_DIR}/src/glad.c
    ${GLAD_DIR}/include/glad/glad.h
    ${GLAD_DIR}/include/KHR/khrplatform.h
  COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
  COMMAND ${VENV_PIP} install glad
  COMMAND ${VENV_PYTHON} -m glad --out-path=${GLAD_DIR} --api=gl=4.6 --profile=core --generator=c
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating glad files"
)

# Create glad library
add_library(glad STATIC ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)

# Create ImGui library with all necessary files
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${glfw_SOURCE_DIR}/include
)

target_link_libraries(imgui PUBLIC glfw)

# Define ImGui features
target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    IMGUI_ENABLE_DOCKING
)

# OpenCL
find_package(OpenCL REQUIRED)
if(NOT OpenCL_FOUND)
    message(FATAL_ERROR "OpenCL not found. Please install an OpenCL SDK.")
endif()

# OpenGL
find_package(OpenGL REQUIRED)

# --- Executable Definition ---
add_executable(Sirius
  src/main.cpp
  src/core/Window.cpp src/core/Window.hpp
  src/hal/OpenCLContext.cpp src/hal/OpenCLContext.hpp
  src/ui/ImGuiLayer.cpp src/ui/ImGuiLayer.hpp
)

# --- Include Directories ---
target_include_directories(Sirius PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${glfw_SOURCE_DIR}/include
  ${glm_SOURCE_DIR}
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${OpenCL_INCLUDE_DIRS}
)

# --- Link Libraries ---
target_link_libraries(Sirius PRIVATE
  glfw
  imgui
  glad
  ${OpenCL_LIBRARIES}
)

# Platform-specific linking for OpenGL
if(WIN32)
    target_link_libraries(Sirius PRIVATE opengl32)
elseif(APPLE)
    target_link_libraries(Sirius PRIVATE "-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo")
else()
    target_link_libraries(Sirius PRIVATE OpenGL::GL)
endif()

# Define for executable
target_compile_definitions(Sirius PRIVATE
  IMGUI_IMPL_OPENGL_LOADER_GLAD
  IMGUI_ENABLE_DOCKING
)