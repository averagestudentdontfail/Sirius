#include <iostream>
#include <stdexcept>
#include "core/Window.hpp"
#include "hal/OpenCLContext.hpp"
#include "ui/ImGuiLayer.hpp"

#include <glad/glad.h>  // Include GLAD first
#define GLFW_INCLUDE_NONE  // Tell GLFW not to include OpenGL headers
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

class Application {
public:
    Application() {
        std::cout << "Creating Sirius Application..." << std::endl;
        m_Window = new Sirius::Window(1280, 720, "Sirius");
        m_OpenCLContext = new Sirius::HAL::OpenCLContext();
        m_ImGuiLayer = new Sirius::UI::ImGuiLayer();
        m_ImGuiLayer->OnAttach(m_Window);
        std::cout << "Application initialized successfully!" << std::endl;
    }
    
    ~Application() {
        std::cout << "Shutting down application..." << std::endl;
        m_ImGuiLayer->OnDetach();
        delete m_ImGuiLayer;
        delete m_OpenCLContext;
        delete m_Window;
    }
    
    void Run() {
        m_OpenCLContext->PrintInfo();
        
        std::cout << "Starting main loop..." << std::endl;
        int frame_count = 0;
        
        while (!m_Window->ShouldClose()) {
            frame_count++;
            
            // Clear screen
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            m_ImGuiLayer->Begin();
            
            // --- Your UI and Logic Here ---
            
            // Check if docking is available
            ImGuiIO& io = ImGui::GetIO();
            bool docking_enabled = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0;
            
            if (docking_enabled) {
                // Create dockspace if docking is enabled
                SetupDockspace();
            }
            
            // Status window
            if (ImGui::Begin("Status")) {
                ImGui::Text("Hello, Sirius!");
                ImGui::Text("Frame: %d", frame_count);
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                           1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::Separator();
                
                // OpenCL Info
                ImGui::Text("OpenCL Device: %s", 
                           m_OpenCLContext->GetDevice().getInfo<CL_DEVICE_NAME>().c_str());
                
                ImGui::Separator();
                
                // Graphics Info
                ImGui::Text("OpenGL Vendor: %s", glGetString(GL_VENDOR));
                ImGui::Text("OpenGL Renderer: %s", glGetString(GL_RENDERER));
                ImGui::Text("OpenGL Version: %s", glGetString(GL_VERSION));
                
                ImGui::Separator();
                
                // Feature Status
                ImGui::Text("Docking: %s", docking_enabled ? "ENABLED" : "DISABLED");
                ImGui::Text("Viewports: %s", (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) ? "ENABLED" : "DISABLED");
                
                if (docking_enabled) {
                    ImGui::Text("Try dragging this window by its title bar!");
                }
                
                if (ImGui::Button("Exit Application")) {
                    glfwSetWindowShouldClose(m_Window->GetNativeWindow(), GLFW_TRUE);
                }
            }
            ImGui::End();
            
            // Show demo window
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
                if (ImGui::BeginMenu("Help")) {
                    ImGui::Text("Sirius Application");
                    ImGui::Text("OpenGL + OpenCL + ImGui");
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
            
            // -----------------------------
            
            m_ImGuiLayer->End();
            m_Window->OnUpdate();
            
            // Print status every 60 frames (roughly once per second at 60fps)
            if (frame_count % 60 == 0) {
                std::cout << "Frame " << frame_count << " - FPS: " << ImGui::GetIO().Framerate << std::endl;
            }
        }
        
        std::cout << "Main loop ended after " << frame_count << " frames." << std::endl;
    }

private:
    void SetupDockspace() {
        // Create dockspace over the entire viewport
        static bool dockspaceOpen = true;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen) {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;
            
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();
        
        if (opt_fullscreen)
            ImGui::PopStyleVar(2);
            
        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        
        ImGui::End();
    }

    Sirius::Window* m_Window;
    Sirius::HAL::OpenCLContext* m_OpenCLContext;
    Sirius::UI::ImGuiLayer* m_ImGuiLayer;
};

int main() {
    std::cout << "=== Starting Sirius Application ===" << std::endl;
    
    try {
        Application app;
        app.Run();
        std::cout << "Application completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "=== Sirius Application Finished ===" << std::endl;
    return EXIT_SUCCESS;
}