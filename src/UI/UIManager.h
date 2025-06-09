#pragma once

struct GLFWwindow;

class UIManager {
public:
    UIManager(GLFWwindow* window);
    ~UIManager();

    void beginFrame();
    void endFrame();
    void displayMainMenu(); 
};