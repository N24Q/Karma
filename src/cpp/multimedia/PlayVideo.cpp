#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "VideoPlayer.hpp"
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>

extern "C" void PlayVideo(const char* videoPath, void* windowHandle) {
    std::cout << "[PlayVideo] START\n";

    GLFWwindow* window = static_cast<GLFWwindow*>(windowHandle);
    if (!window) {
        std::cerr << "[PlayVideo] ERROR: Invalid window handle!\n";
        return;
    }
    
    std::cout << "[PlayVideo] Window handle: " << window << "\n";

    // Play video
    {
        VideoPlayer player;
        player.setRenderWindow(window);
        
        if (!player.load(videoPath)) {
            std::cerr << "[PlayVideo] ERROR: Failed to load video\n";
            return;
        }

        std::cout << "[PlayVideo] Starting playback...\n";
        player.play();
        
        std::cout << "[PlayVideo] Waiting for playback to start...\n";
        int waitCount = 0;
        while (!player.isPlaying() && waitCount < 100) {
            glfwPollEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            waitCount++;
        }
        
        if (!player.isPlaying()) {
            std::cerr << "[PlayVideo] ERROR: Playback never started!\n";
            return;
        }
        
        std::cout << "[PlayVideo] Video playing...\n";

        // Wait for video to finish
        while (player.isPlaying()) {
            glfwPollEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "[PlayVideo] Video finished\n";
        std::cout << "[PlayVideo] Cleaning up VLC...\n";
        player.cleanup();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
    } // Destructor
    
    std::cout << "[PlayVideo] Player destroyed\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Clear window close flag
    std::cout << "[PlayVideo] Clearing window close flag...\n";
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    
    // Process events
    for (int i = 0; i < 20; i++) {
        glfwPollEvents();
        glfwSetWindowShouldClose(window, GLFW_FALSE);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "[PlayVideo] Restoring OpenGL context...\n";
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    // CRITICAL: Reinitialize GLAD after VLC messed with OpenGL
    std::cout << "[PlayVideo] Reinitializing OpenGL (GLAD)...\n";
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[PlayVideo] ERROR: Failed to reinitialize GLAD!\n";
        return;
    }
    std::cout << "[PlayVideo] GLAD reinitialized successfully\n";
    
    // Reset OpenGL state
    std::cout << "[PlayVideo] Resetting OpenGL state...\n";
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Check for OpenGL errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "[PlayVideo] OpenGL error after state reset: " << err << "\n";
    }
    
    std::cout << "[PlayVideo] Clearing screen...\n";
    
    // Clear screen multiple times
    for (int i = 0; i < 5; i++) {
        std::cout << "[PlayVideo] Clear " << (i+1) << "/5\n";
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "[PlayVideo] OpenGL error during clear: " << err << "\n";
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        glfwSetWindowShouldClose(window, GLFW_FALSE);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    std::cout << "[PlayVideo] COMPLETE - OpenGL ready!\n";
}