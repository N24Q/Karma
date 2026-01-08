#include "AudioPlayer.hpp"
#include <iostream>

AudioPlayer::AudioPlayer() {
    std::cout << "[AudioPlayer] Initializing...\n";

    // Minimal VLC options to reduce memory usage
    const char* vlc_args[] = {
        "--no-xlib",
        "--quiet",                        // Less verbose for audio
        "--no-video",                     // Audio only, no video decoding
        "--no-stats",                     // Disable statistics
        "--no-osd",                       // Disable on-screen display
        "--no-plugins-cache",             // Don't cache plugins
        "--no-media-library",             // Disable media library
        "--no-lua"                        // Disable Lua scripting
    };

    vlcInstance = libvlc_new(8, vlc_args);

    if (!vlcInstance) {
        std::cerr << "[AudioPlayer] Failed to initialize LibVLC\n";
    } else {
        std::cout << "[AudioPlayer] LibVLC initialized\n";
    }
}

AudioPlayer::~AudioPlayer() {
    std::cout << "[AudioPlayer] Cleaning up...\n";
    
    if (mediaPlayer) {
        libvlc_media_player_stop(mediaPlayer);
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
    
    std::cout << "[AudioPlayer] Cleanup complete\n";
}

bool AudioPlayer::load(const std::string& filename) {
    if (!vlcInstance) return false;

    std::cout << "[AudioPlayer] Loading: " << filename << "\n";

    media = libvlc_media_new_path(vlcInstance, filename.c_str());
    if (!media) {
        std::cerr << "[AudioPlayer] Failed to create media\n";
        return false;
    }

    mediaPlayer = libvlc_media_player_new_from_media(media);
    if (!mediaPlayer) {
        std::cerr << "[AudioPlayer] Failed to create player\n";
        libvlc_media_release(media);
        media = nullptr;
        return false;
    }

    std::cout << "[AudioPlayer] Audio loaded\n";
    return true;
}

void AudioPlayer::play() {
    if (mediaPlayer) {
        libvlc_media_player_play(mediaPlayer);
        std::cout << "[AudioPlayer] Playing\n";
    }
}

void AudioPlayer::pause() {
    if (mediaPlayer) {
        libvlc_media_player_set_pause(mediaPlayer, 1);
        std::cout << "[AudioPlayer] Paused\n";
    }
}

void AudioPlayer::stop() {
    if (mediaPlayer) {
        libvlc_media_player_stop(mediaPlayer);
        std::cout << "[AudioPlayer] Stopped\n";
    }
}

bool AudioPlayer::isPlaying() const {
    return mediaPlayer && (libvlc_media_player_is_playing(mediaPlayer) != 0);
}

void AudioPlayer::setVolume(int volume) {
    if (mediaPlayer) {
        // Clamp volume to 0-100
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;
        libvlc_audio_set_volume(mediaPlayer, volume);
        std::cout << "[AudioPlayer] Volume set to " << volume << "\n";
    }
}