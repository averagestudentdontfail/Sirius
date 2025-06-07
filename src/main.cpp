#include <iostream>
#include <stdexcept>
#include "core/Window.hpp"
#include "hal/OpenCLContext.hpp"
#include "ui/ImGuiLayer.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>

class Application {
public:
    Application() {
        m_Window = new Sirius::Window(1280, 720, "Sirius");
        m_OpenCLContext = new Sirius::HAL::OpenCLContext();
        m_ImGuiLayer = new Sirius::UI::ImGuiLayer();
        m_ImGuiLayer->OnAttach(m_Window);
    }
    
    ~Application() {
        m_ImGuiLayer->OnDetach();
        delete m_ImGuiLayer;
        delete m_OpenCLContext;
        delete m_Window;
    }
    
    void Run() {
        m_OpenCLContext->PrintInfo();
        
        while (!m_Window->ShouldClose()) {
            // Clear screen
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            m_ImGuiLayer->Begin();
            
            // --- Your UI and Logic Here ---
            
            // Create dockspace over the entire viewport
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
            window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            
            ImGui::Begin("DockSpace Demo", nullptr, window_flags);
            ImGui::PopStyleVar(3);
            
            // DockSpace
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
            
            ImGui::End();
            
            // Your actual windows
            if (ImGui::Begin("Status")) {
                ImGui::Text("Hello, Sirius!");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                           1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::Separator();
                ImGui::Text("OpenCL Device: %s", 
                           m_OpenCLContext->GetDevice().getInfo<CL_DEVICE_NAME>().c_str());
            }
            ImGui::End();
            
            ImGui::ShowDemoWindow();
            
            // -----------------------------
            
            m_ImGuiLayer->End();
            m_Window->OnUpdate();
        }
    }

private:
    Sirius::Window* m_Window;
    Sirius::HAL::OpenCLContext* m_OpenCLContext;
    Sirius::UI::ImGuiLayer* m_ImGuiLayer;
};

int main() {
    try {
        Application app;
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}