#include "Core/Window.h"
#include <stdexcept>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLFW error callback function
static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

Window::Window(int width, int height, const std::string& title) {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Request OpenGL 4.1 context.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_Window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    // Print OpenGL info for debugging
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;

    glfwSwapInterval(1); // Enable V-Sync
}

Window::~Window() {
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

void Window::pollEvents() const {
    glfwPollEvents();
}

void Window::swapBuffers() const {
    glfwSwapBuffers(m_Window);
}