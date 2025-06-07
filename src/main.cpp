#include <iostream>
#include <stdexcept>
#include "core/Window.hpp"
#include "hal/OpenCLContext.hpp"
#include "ui/ImGuiLayer.hpp"

#include <glad/glad.h>  // Include GLAD first
#define GLFW_INCLUDE_NONE  // Tell GLFW not to include OpenGL headers
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

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
            
            // --- Simple UI without docking (fallback) ---
            
            // Status window
            ImGui::Begin("Status");
            ImGui::Text("Hello, Sirius!");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                       1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Separator();
            ImGui::Text("OpenCL Device: %s", 
                       m_OpenCLContext->GetDevice().getInfo<CL_DEVICE_NAME>().c_str());
            
            // Show docking status
            ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
                ImGui::Text("Docking: ENABLED");
                ImGui::Text("Try dragging this window by its title bar!");
            } else {
                ImGui::Text("Docking: DISABLED");
            }
            ImGui::End();
            
            // Demo window
            static bool show_demo = true;
            if (show_demo) {
                ImGui::ShowDemoWindow(&show_demo);
            }
            
            // Simple menu bar
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Exit")) {
                        glfwSetWindowShouldClose(m_Window->GetNativeWindow(), GLFW_TRUE);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("View")) {
                    ImGui::MenuItem("Demo Window", nullptr, &show_demo);
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
            
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