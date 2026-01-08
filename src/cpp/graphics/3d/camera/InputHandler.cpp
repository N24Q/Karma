#include "InputHandler.h"

namespace Karma {

InputHandler* InputHandler::s_instance = nullptr;

InputHandler::InputHandler()
    : m_keyW(false)
    , m_keyA(false)
    , m_keyS(false)
    , m_keyD(false)
{
    s_instance = this;
}

void InputHandler::RegisterCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, KeyCallback);
}

void InputHandler::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!s_instance) return;

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_W) s_instance->m_keyW = true;
        if (key == GLFW_KEY_A) s_instance->m_keyA = true;
        if (key == GLFW_KEY_S) s_instance->m_keyS = true;
        if (key == GLFW_KEY_D) s_instance->m_keyD = true;
    } else if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_W) s_instance->m_keyW = false;
        if (key == GLFW_KEY_A) s_instance->m_keyA = false;
        if (key == GLFW_KEY_S) s_instance->m_keyS = false;
        if (key == GLFW_KEY_D) s_instance->m_keyD = false;
    }
}

bool InputHandler::IsKeyPressed(int key) const {
    switch (key) {
        case GLFW_KEY_W: return m_keyW;
        case GLFW_KEY_A: return m_keyA;
        case GLFW_KEY_S: return m_keyS;
        case GLFW_KEY_D: return m_keyD;
        default: return false;
    }
}

void InputHandler::GetMovementInput(bool& forward, bool& backward, bool& left, bool& right) const {
    forward = m_keyW;
    backward = m_keyS;
    left = m_keyA;
    right = m_keyD;
}

} // namespace Karma
