#include "Core/Application.h"
#include "Core/Window.h"
#include "UI/UIManager.h"
#include <glad/glad.h>  // Add this for OpenGL functions

Application::Application() {
    m_Window = std::make_unique<Window>(1280, 720, "Sirius");
    m_UIManager = std::make_unique<UIManager>(m_Window->getNativeWindow());
    
    // Set a nice background color (dark gray)
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

Application::~Application() {
    // RAII will handle cleanup of unique_ptrs
}

void Application::run() {
    while (!m_Window->shouldClose()) {
        m_Window->pollEvents();

        // Clear the framebuffer before rendering
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_UIManager->beginFrame();

        // --- Render UI here ---
        m_UIManager->displayMainMenu();

        m_UIManager->endFrame();

        m_Window->swapBuffers();
    }
}