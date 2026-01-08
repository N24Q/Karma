#include "Window.hpp"
#include <GLFW/glfw3.h>

// Exported variables
extern "C" {
    const char windowTitle[] = "Grand Gambit 64";
    void* windowPtr = nullptr;
}

// Initialize the window
extern "C" int InitialiseWindow() {
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;
    }

    // Set OpenGL version hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Borderless fullscreen window (no decorations, covers entire screen)
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    // Get the primary monitor
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
        glfwTerminate();
        return -1;
    }

    // Get the current video mode (resolution, refresh rate, etc.)
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) {
        glfwTerminate();
        return -1;
    }

    // Create true fullscreen window
    GLFWwindow* window = glfwCreateWindow(
        mode->width,
        mode->height,
        windowTitle,
        monitor,  // Fullscreen mode
        nullptr
    );

    if (!window) {
        glfwTerminate();
        return -1;
    }

    windowPtr = window;

    // Set the OpenGL context to the window
    glfwMakeContextCurrent(window);

    // Match monitor's refresh rate
    glfwSwapInterval(1);

    // Show and focus the window
    glfwShowWindow(window);
    glfwFocusWindow(window);

    // Process any pending events to ensure window is responsive
    glfwPollEvents();

    return 0;
}

// Clean up and exit
extern "C" void CleanExitWindow() {
    if (windowPtr != nullptr) {
        glfwDestroyWindow(static_cast<GLFWwindow*>(windowPtr));
        windowPtr = nullptr;
    }

    glfwTerminate();
}