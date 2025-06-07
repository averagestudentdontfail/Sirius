#pragma once

#include <string>
#include <functional>

struct GLFWwindow;

namespace Sirius {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    void OnUpdate();
    bool ShouldClose() const;
    void Shutdown();

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    GLFWwindow* GetNativeWindow() const { return m_Window; }

private:
    void Init();

    GLFWwindow* m_Window;
    int m_Width;
    int m_Height;
    std::string m_Title;
};

} // namespace Sirius
