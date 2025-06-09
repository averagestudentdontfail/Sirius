#include "UIManager.h"
#include "Core/Application.h" // Include full Application definition
#include "Core/PluginManager.h"
#include "Physics/IMetric.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>  // Add this include for GLFW functions
#include <string>

UIManager::UIManager(GLFWwindow* window, Application& app) : m_App(app) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410 core");
}

UIManager::~UIManager() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::endFrame() {
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