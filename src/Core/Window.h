#pragma once

#include <string>

// Forward-declare GLFW types
struct GLFWwindow;

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool shouldClose() const;
    void pollEvents() const;
    void swapBuffers() const;

    GLFWwindow* getNativeWindow() const { return m_Window; }

private:
    GLFWwindow* m_Window;
};