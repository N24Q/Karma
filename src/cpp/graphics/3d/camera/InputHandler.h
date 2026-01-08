#pragma once

#include <GLFW/glfw3.h>

namespace Karma {

class InputHandler {
public:
    InputHandler();
    ~InputHandler() = default;

    void RegisterCallbacks(GLFWwindow* window);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    bool IsKeyPressed(int key) const;
    void GetMovementInput(bool& forward, bool& backward, bool& left, bool& right) const;

    static InputHandler* GetInstance() { return s_instance; }

private:
    static InputHandler* s_instance;

    bool m_keyW;
    bool m_keyA;
    bool m_keyS;
    bool m_keyD;
};

} // namespace Karma
