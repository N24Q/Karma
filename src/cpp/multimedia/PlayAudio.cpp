#include "AudioPlayer.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// Static player for background audio playback
static AudioPlayer* g_backgroundPlayer = nullptr;

// Play audio and wait for it to finish (with volume control)
extern "C" void PlayAudio(const char* audioPath, int volume) {
    std::cout << "[PlayAudio] Playing: " << audioPath << " at volume " << volume << "\n";

    AudioPlayer player;
    
    if (!player.load(audioPath)) {
        std::cerr << "[PlayAudio] Failed to load audio\n";
        return;
    }

    player.setVolume(volume);
    player.play();
    
    // Wait for playback to start
    int waitCount = 0;
    while (!player.isPlaying() && waitCount < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        waitCount++;
    }
    
    if (!player.isPlaying()) {
        std::cerr << "[PlayAudio] Playback never started\n";
        return;
    }
    
    std::cout << "[PlayAudio] Audio playing...\n";

    // Wait for audio to finish
    while (player.isPlaying()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[PlayAudio] Audio finished\n";
}

// Play audio in background (non-blocking) with volume control
extern "C" void PlayAudioAsync(const char* audioPath, int volume) {
    std::cout << "[PlayAudioAsync] Starting background playback: " << audioPath
              << " at volume " << volume << "\n";

    // Clean up previous player if exists
    if (g_backgroundPlayer) {
        delete g_backgroundPlayer;
    }

    g_backgroundPlayer = new AudioPlayer();

    if (!g_backgroundPlayer->load(audioPath)) {
        std::cerr << "[PlayAudioAsync] Failed to load audio\n";
        delete g_backgroundPlayer;
        g_backgroundPlayer = nullptr;
        return;
    }

    g_backgroundPlayer->setVolume(volume);
    g_backgroundPlayer->play();

    std::cout << "[PlayAudioAsync] Audio playing in background\n";
    // Returns immediately, audio plays in background
}

// Stop background audio
extern "C" void StopAudio() {
    std::cout << "[StopAudio] Stopping background audio\n";

    if (g_backgroundPlayer) {
        delete g_backgroundPlayer;
        g_backgroundPlayer = nullptr;
        std::cout << "[StopAudio] Audio stopped and cleaned up\n";
    } else {
        std::cout << "[StopAudio] No audio playing\n";
    }
}

// Cleanup background audio on program exit
extern "C" void CleanupBackgroundAudio() {
    std::cout << "[CleanupBackgroundAudio] Freeing VLC resources...\n";
    if (g_backgroundPlayer) {
        delete g_backgroundPlayer;
        g_backgroundPlayer = nullptr;
        std::cout << "[CleanupBackgroundAudio] VLC instance freed\n";
    }
}