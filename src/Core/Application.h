#pragma once

#include <memory>

// Forward-declare to avoid including heavy headers here
class Window;
class UIManager;

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<UIManager> m_UIManager;
};