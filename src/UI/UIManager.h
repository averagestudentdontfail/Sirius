#pragma once

struct GLFWwindow;
class Application; // Forward-declare Application

class UIManager {
public:
    // Constructor now takes a reference to the Application
    UIManager(GLFWwindow* window, Application& app);
    ~UIManager();

    void beginFrame();
    void endFrame();
    void displayMainUI();

private:
    void displayViewport();
    void displayControlPanel();
    
    Application& m_App; // Store a reference to the main application
};