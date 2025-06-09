#include "Core/Application.h"
#include "Core/Window.h"
#include "UI/UIManager.h"

Application::Application() {
    m_Window = std::make_unique<Window>(1280, 720, "Sirius");
    m_UIManager = std::make_unique<UIManager>(m_Window->getNativeWindow());
}

Application::~Application() {
    // RAII will handle cleanup of unique_ptrs
}

void Application::run() {
    while (!m_Window->shouldClose()) {
        m_Window->pollEvents();

        m_UIManager->beginFrame();

        // --- Render UI here ---
        // Example: ImGui::ShowDemoWindow();
        m_UIManager->displayMainMenu();


        m_UIManager->endFrame();

        m_Window->swapBuffers();
    }
}