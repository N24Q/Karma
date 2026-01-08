#ifndef AUDIOPLAYER_HPP
#define AUDIOPLAYER_HPP

#include <vlc/vlc.h>
#include <string>

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    bool load(const std::string& filename);
    void play();
    void pause();
    void stop();
    bool isPlaying() const;
    void setVolume(int volume); // 0-100

private:
    libvlc_instance_t* vlcInstance = nullptr;
    libvlc_media_t* media = nullptr;
    libvlc_media_player_t* mediaPlayer = nullptr;
};

#endif