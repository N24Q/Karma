#include "VideoPlayer.hpp"
#include <iostream>
#include <windows.h>
#include <string>
#include <thread>
#include <chrono>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

VideoPlayer::VideoPlayer() {
    std::cout << "[VideoPlayer] Constructor\n";

    // Minimal VLC options to reduce memory usage
    const char* vlc_args[] = {
        "--no-xlib",
        "--verbose=0",                    // Disable verbose logging
        "--no-video-title-show",
        "--no-stats",                     // Disable statistics
        "--no-snapshot-preview",          // Disable snapshot preview
        "--no-osd",                       // Disable on-screen display
        "--avcodec-fast",                 // Use fast codec mode
        "--no-plugins-cache",             // Don't cache plugins
        "--no-media-library",             // Disable media library
        "--no-lua"                        // Disable Lua scripting
    };

    vlcInstance = libvlc_new(10, vlc_args);

    if (!vlcInstance) {
        std::cerr << "[VideoPlayer] Failed to initialize LibVLC\n";
    } else {
        std::cout << "[VideoPlayer] LibVLC initialized\n";
    }
}

VideoPlayer::~VideoPlayer() {
    std::cout << "[VideoPlayer] Destructor - cleaning up\n";
    
    if (mediaPlayer) {
        std::cout << "[VideoPlayer] Detaching window from VLC\n";
        // CRITICAL: Detach VLC from the window before stopping
        libvlc_media_player_set_hwnd(mediaPlayer, nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "[VideoPlayer] Stopping media player\n";
        libvlc_media_player_stop(mediaPlayer);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::cout << "[VideoPlayer] Releasing media player\n";
        libvlc_media_player_release(mediaPlayer);
        mediaPlayer = nullptr;
    }
    
    if (media) {
        libvlc_media_release(media);
        media = nullptr;
    }
    
    if (vlcInstance) {
        libvlc_release(vlcInstance);
        vlcInstance = nullptr;
    }
    
    std::cout << "[VideoPlayer] Cleanup complete\n";
}

void VideoPlayer::cleanup() {
    std::cout << "[VideoPlayer] Explicit cleanup - detaching window\n";
    if (mediaPlayer) {
        // Detach from window first
        libvlc_media_player_set_hwnd(mediaPlayer, nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

bool VideoPlayer::load(const std::string& filename) {
    if (!vlcInstance) return false;

    std::cout << "[VideoPlayer] Loading: " << filename << "\n";

    media = libvlc_media_new_path(vlcInstance, filename.c_str());
    if (!media) {
        std::cerr << "[VideoPlayer] Failed to create media\n";
        return false;
    }

    mediaPlayer = libvlc_media_player_new_from_media(media);
    if (!mediaPlayer) {
        std::cerr << "[VideoPlayer] Failed to create player\n";
        libvlc_media_release(media);
        media = nullptr;
        return false;
    }

    if (renderWindow) {
        HWND hwnd = glfwGetWin32Window(renderWindow);
        libvlc_media_player_set_hwnd(mediaPlayer, hwnd);
    }

    std::cout << "[VideoPlayer] Media loaded\n";
    return true;
}

void VideoPlayer::setRenderWindow(GLFWwindow* window) {
    renderWindow = window;
    if (mediaPlayer && window) {
        HWND hwnd = glfwGetWin32Window(window);
        libvlc_media_player_set_hwnd(mediaPlayer, hwnd);
    }
}

void VideoPlayer::play() {
    if (mediaPlayer) {
        libvlc_media_player_play(mediaPlayer);
    }
}

void VideoPlayer::pause() {
    if (mediaPlayer) {
        libvlc_media_player_set_pause(mediaPlayer, 1);
    }
}

void VideoPlayer::stop() {
    if (mediaPlayer) {
        std::cout << "[VideoPlayer] Stopping playback\n";
        libvlc_media_player_stop(mediaPlayer);
    }
}

bool VideoPlayer::isPlaying() const {
    return mediaPlayer && (libvlc_media_player_is_playing(mediaPlayer) != 0);
}