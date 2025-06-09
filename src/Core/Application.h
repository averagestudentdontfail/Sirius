#pragma once

#include <memory>
#include <string>

// Forward-declarations
class Window;
class UIManager;
class PluginManager;
class IMetric;
class Renderer;

class Application {
public:
    Application();
    ~Application();

    void run();

    // Getters for UI access
    Renderer* getRenderer() const { return m_Renderer.get(); }

private:
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<UIManager> m_UIManager;
    std::unique_ptr<PluginManager> m_PluginManager;
    std::unique_ptr<Renderer> m_Renderer;

    IMetric* m_CurrentMetric = nullptr;
    std::string m_CurrentMetricName;

    friend class UIManager; // Allow UIManager to access Application's state
};