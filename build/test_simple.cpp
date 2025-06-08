#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    std::cout << "Testing GLFW window creation..." << std::endl;
    
    if (!glfwInit()) {
        std::cout << "GLFW init failed" << std::endl;
        return -1;
    }
    
    // Try with no specific OpenGL version
    glfwDefaultWindowHints();
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "Simple Test", nullptr, nullptr);
    
    if (!window) {
        std::cout << "Window creation failed" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Get OpenGL info directly
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    
    std::cout << "SUCCESS! Window created." << std::endl;
    if (version) std::cout << "OpenGL Version: " << version << std::endl;
    if (vendor) std::cout << "OpenGL Vendor: " << vendor << std::endl;
    if (renderer) std::cout << "OpenGL Renderer: " << renderer << std::endl;
    
    // Keep window open for 3 seconds
    auto start = glfwGetTime();
    while (glfwGetTime() - start < 3.0 && !glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.2f, 0.7f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
