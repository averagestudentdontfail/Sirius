#include "UIManager.h"
#include "Core/Application.h"
#include "Core/PluginManager.h"
#include "Graphics/Renderer.h"
#include "Physics/IMetric.h"
#include <glad/glad.h>  // Must come BEFORE any OpenGL includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <string>
#include <chrono>

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
    // Create dockspace for the entire window
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    // Create the dockspace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();

    // Render viewport window
    displayViewport();
    
    // Render control panel
    displayControlPanel();
}

void UIManager::displayViewport() {
    ImGui::Begin("Viewport");

    Renderer* renderer = m_App.getRenderer();
    if (renderer && m_App.m_CurrentMetric) {
        unsigned int textureID = renderer->getOutputTexture();
        
        // Get the content region available for the image
        ImVec2 contentRegion = ImGui::GetContentRegionAvail();
        
        // Ensure we have a valid texture and content region
        if (textureID > 0 && contentRegion.x > 0 && contentRegion.y > 0) {
            // Cast texture ID directly to ImTextureID (which is uint64_t in this build)
            ImTextureID texID = static_cast<ImTextureID>(textureID);
            ImGui::Image(texID, contentRegion, ImVec2(0, 1), ImVec2(1, 0)); // Flip Y coordinate for OpenGL
            
            // Display render info on hover
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Metric: %s", m_App.m_CurrentMetric->getName());
                ImGui::Text("Resolution: %.0fx%.0f", contentRegion.x, contentRegion.y);
                ImGui::Text("Texture ID: %u", textureID);
                ImGui::EndTooltip();
            }
        } else {
            ImGui::Text("Waiting for texture...");
            ImGui::Text("Texture ID: %u", textureID);
            ImGui::Text("Content Region: %.1fx%.1f", contentRegion.x, contentRegion.y);
        }
    } else {
        ImGui::Text("No metric selected or renderer not available");
    }

    ImGui::End();
}

void UIManager::displayControlPanel() {
    ImGui::Begin("Control Panel");

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
                // Add sliders for parameter control
                float value = static_cast<float>(val.value);
                float min_val = static_cast<float>(val.min);
                float max_val = static_cast<float>(val.max);
                
                if (ImGui::SliderFloat(key.c_str(), &value, min_val, max_val)) {
                    m_App.m_CurrentMetric->setParameter(key, static_cast<double>(value));
                }
            }
        }
        
        ImGui::Separator();
        
        // Render statistics
        if (ImGui::CollapsingHeader("Render Info")) {
            Renderer* renderer = m_App.getRenderer();
            if (renderer) {
                ImGui::Text("Renderer: OpenCL Ray Tracer (POCL CPU)");
                ImGui::Text("Platform: Portable Computing Language");
                ImGui::Text("Device: AMD Ryzen 7 7800X3D (16 cores)");
                ImGui::Text("Output Texture ID: %u", renderer->getOutputTexture());
                ImGui::Text("Resolution: %dx%d", 1280, 720);
                ImGui::Text("Total Rays: %d", 1280 * 720);
                
                // Add frame timing info
                static float frameTime = 0.0f;
                static int frameCount = 0;
                static auto lastTime = std::chrono::high_resolution_clock::now();
                
                frameCount++;
                auto currentTime = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);
                
                if (elapsed.count() >= 1000) { // Update every second
                    frameTime = elapsed.count() / float(frameCount);
                    frameCount = 0;
                    lastTime = currentTime;
                }
                
                if (frameTime > 0) {
                    ImGui::Text("Frame Time: %.1f ms", frameTime);
                    ImGui::Text("FPS: %.1f", 1000.0f / frameTime);
                }
            }
        }
    } else {
        ImGui::Text("No metric loaded.");
        
        if (ImGui::Button("Reload Plugins")) {
            m_App.m_PluginManager->loadPlugins("./plugins");
            auto metricNames = m_App.m_PluginManager->getMetricNames();
            if (!metricNames.empty()) {
                m_App.m_CurrentMetricName = metricNames[0];
                m_App.m_CurrentMetric = m_App.m_PluginManager->getMetric(m_App.m_CurrentMetricName);
            }
        }
    }

    ImGui::End();
}