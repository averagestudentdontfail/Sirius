#include "ImGuiLayer.hpp"
#include "core/Window.hpp"

#include <glad/glad.h>  // Include GLAD first
#define GLFW_INCLUDE_NONE  // Tell GLFW not to include OpenGL headers
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>

namespace Sirius::UI {

ImGuiLayer::ImGuiLayer() {}
ImGuiLayer::~ImGuiLayer() {}

void ImGuiLayer::OnAttach(Sirius::Window* window) {
    m_Window = window;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Enable docking and viewports
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Determine appropriate GLSL version based on OpenGL version
    const char* glsl_version = nullptr;
    
    // Get OpenGL version
    int major = 0, minor = 0;
    const char* version_string = (const char*)glGetString(GL_VERSION);
    
    // Try to get version via glGetIntegerv (OpenGL 3.0+)
    if (glGetIntegerv) {
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
    }
    
    // If that didn't work, parse the version string
    if (major == 0 && version_string) {
        sscanf(version_string, "%d.%d", &major, &minor);
    }
    
    std::cout << "Detected OpenGL " << major << "." << minor << std::endl;
    
    // Choose appropriate GLSL version based on OpenGL version
    if (major > 4 || (major == 4 && minor >= 6)) {
        glsl_version = "#version 460";
    } else if (major == 4 && minor >= 5) {
        glsl_version = "#version 450";
    } else if (major == 4 && minor >= 3) {
        glsl_version = "#version 430";
    } else if (major == 4 && minor >= 1) {
        glsl_version = "#version 410";
    } else if (major == 3 && minor >= 3) {
        glsl_version = "#version 330";
    } else if (major == 3 && minor >= 2) {
        glsl_version = "#version 150";
    } else if (major == 3 && minor >= 1) {
        glsl_version = "#version 140";
    } else if (major == 3 && minor >= 0) {
        glsl_version = "#version 130";
    } else if (major == 2 && minor >= 1) {
        glsl_version = "#version 120";
    } else {
        // Fallback for very old OpenGL
        glsl_version = "#version 110";
    }
    
    std::cout << "Using GLSL version: " << glsl_version << std::endl;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window->GetNativeWindow(), true);
    
    // Initialize OpenGL3 backend with detected version
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend!" << std::endl;
        throw std::runtime_error("Failed to initialize ImGui OpenGL3 backend");
    }
    
    std::cout << "ImGui initialized successfully with OpenGL " << major << "." << minor << std::endl;
}

void ImGuiLayer::OnDetach() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::Begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::End() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)m_Window->GetWidth(), (float)m_Window->GetHeight());

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // Note: In headless mode, viewports might not work properly
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

} // namespace Sirius::UI