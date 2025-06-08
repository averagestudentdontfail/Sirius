#include <GLFW/glfw3.h>
#include <iostream>

static void error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main() {
    std::cout << "Setting up GLFW with error reporting..." << std::endl;
    
    // Set error callback before init
    glfwSetErrorCallback(error_callback);
    
    if (!glfwInit()) {
        std::cout << "GLFW init failed" << std::endl;
        return -1;
    }
    
    std::cout << "GLFW initialized successfully" << std::endl;
    
    // Try different window hints
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    
    std::cout << "Creating window..." << std::endl;
    GLFWwindow* window = glfwCreateWindow(800, 600, "Debug Test", nullptr, nullptr);
    
    if (!window) {
        std::cout << "Window creation failed with OpenGL 2.1" << std::endl;
        
        // Try without OpenGL context
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        std::cout << "Trying window without OpenGL..." << std::endl;
        window = glfwCreateWindow(800, 600, "No OpenGL Test", nullptr, nullptr);
        
        if (!window) {
            std::cout << "Even no-OpenGL window failed!" << std::endl;
            glfwTerminate();
            return -1;
        } else {
            std::cout << "SUCCESS: Window without OpenGL created!" << std::endl;
        }
    } else {
        std::cout << "SUCCESS: OpenGL window created!" << std::endl;
        glfwMakeContextCurrent(window);
        
        const char* version = (const char*)glGetString(GL_VERSION);
        if (version) std::cout << "OpenGL Version: " << version << std::endl;
    }
    
    // Keep window open briefly
    auto start = glfwGetTime();
    while (glfwGetTime() - start < 2.0 && !glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
