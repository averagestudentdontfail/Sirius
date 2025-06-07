#pragma once

namespace Sirius {
class Window;
}

namespace Sirius::UI {

class ImGuiLayer {
public:
    ImGuiLayer();
    ~ImGuiLayer();

    void OnAttach(Sirius::Window* window);
    void OnDetach();

    void Begin();
    void End();

private:
    Sirius::Window* m_Window = nullptr;
};

} // namespace Sirius::UI
