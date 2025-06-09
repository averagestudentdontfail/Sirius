#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/PluginManager.h"
#include "Graphics/Renderer.h"
#include "UI/UIManager.h"
#include <glad/glad.h>  // Add this for OpenGL functions
#include <GLFW/glfw3.h>

Application::Application() {
    m_Window = std::make_unique<Window>(1280, 720, "Sirius");

    // Create the plugin manager and load plugins from the build output directory
    m_PluginManager = std::make_unique<PluginManager>();
    m_PluginManager->loadPlugins("./plugins"); // Assumes running from build dir

    // Set the initial metric if any were loaded
    auto metricNames = m_PluginManager->getMetricNames();
    if (!metricNames.empty()) {
        m_CurrentMetricName = metricNames[0];
        m_CurrentMetric = m_PluginManager->getMetric(m_CurrentMetricName);
    }

    // Create the renderer after OpenGL context is ready
    m_Renderer = std::make_unique<Renderer>(1280, 720);
    
    // UIManager now takes a reference to this Application instance
    m_UIManager = std::make_unique<UIManager>(m_Window->getNativeWindow(), *this);
}

Application::~Application() {}

void Application::run() {
    while (!m_Window->shouldClose()) {
        // Clear the framebuffer to prevent ghosting
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_Window->pollEvents();

        // Render the scene using OpenCL if we have a metric
        if (m_CurrentMetric) {
            m_Renderer->render(m_CurrentMetric);
        }

        // Render the UI
        m_UIManager->beginFrame();
        m_UIManager->displayMainUI(); 
        m_UIManager->endFrame();

        m_Window->swapBuffers();
    }
}