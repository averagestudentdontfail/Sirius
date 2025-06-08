#include <GLFW/glfw3.h>
#include <iostream>

static void error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main() {
    std::cout << "Testing minimal OpenGL..." << std::endl;
    
    glfwSetErrorCallback(error_callback);
    
    if (!glfwInit()) {
        std::cout << "GLFW init failed" << std::endl;
        return -1;
    }
    
    // Test 1: Absolute minimal OpenGL
    std::cout << "Test 1: Minimal OpenGL context..." << std::endl;
    glfwDefaultWindowHints();
    GLFWwindow* window = glfwCreateWindow(800, 600, "Minimal OpenGL", nullptr, nullptr);
    
    if (!window) {
        // Test 2: Explicit legacy OpenGL
        std::cout << "Test 2: Legacy OpenGL 2.1..." << std::endl;
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        
        window = glfwCreateWindow(800, 600, "Legacy OpenGL", nullptr, nullptr);
    }
    
    if (!window) {
        // Test 3: OpenGL ES
        std::cout << "Test 3: OpenGL ES..." << std::endl;
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        
        window = glfwCreateWindow(800, 600, "OpenGL ES", nullptr, nullptr);
    }
    
    if (!window) {
        std::cout << "❌ All OpenGL attempts failed" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    
    std::cout << "✅ SUCCESS!" << std::endl;
    if (version) std::cout << "OpenGL Version: " << version << std::endl;
    if (vendor) std::cout << "OpenGL Vendor: " << vendor << std::endl;
    if (renderer) std::cout << "OpenGL Renderer: " << renderer << std::endl;
    
    // Keep window open
    auto start = glfwGetTime();
    while (glfwGetTime() - start < 3.0 && !glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.8f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
