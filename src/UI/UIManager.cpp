#include "UIManager.h"
#include "Core/Application.h" // Include full Application definition
#include "Core/PluginManager.h"
#include "Physics/IMetric.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <string>

// Change constructor signature
UIManager::UIManager(GLFWwindow* window, Application& app) : m_App(app) {
    // ... (rest of existing constructor code is the same)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410 core");
}

UIManager::~UIManager() { /* ... destructor is the same ... */ 
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::beginFrame() { /* ... is the same ... */ 
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
void UIManager::endFrame() { /* ... is the same ... */
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

// The new UI function
void UIManager::displayMainUI() {
    ImGui::Begin("Sirius Control");

    // Metric Selection Dropdown
    const char* currentName = m_App.m_CurrentMetric ? m_App.m_CurrentMetric->getName() : "None";
    if (ImGui::BeginCombo("Metric", currentName)) {
        auto metricNames = m_App.m_PluginManager->getMetricNames();
        for (const auto& name : metricNames) {
            bool is_selected = (name == m_App.m_CurrentMetricName);
            if (ImGui::Selectable(name.c_str(), is_selected)) {
                m_App.m_CurrentMetricName = name;
                m_App.m_CurrentMetric = m_App.m_PluginManager->getMetric(name);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    // Display info for the currently selected metric
    if (m_App.m_CurrentMetric) {
        ImGui::Text("Description: %s", m_App.m_CurrentMetric->getDescription());
        
        // Display and control metric parameters
        auto params = m_App.m_CurrentMetric->getParameters();
        if (!params.empty()) {
            ImGui::Text("Parameters:");
            for (auto const& [key, val] : params) {
                // This is a temporary way to show the value. We will add sliders later.
                ImGui::Text("  %s: %f", key.c_str(), val.value);
            }
        }
    } else {
        ImGui::Text("No metric loaded.");
    }

    ImGui::End();
}