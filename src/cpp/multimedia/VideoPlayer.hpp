#ifndef VIDEOPLAYER_HPP
#define VIDEOPLAYER_HPP

#include <vlc/vlc.h>
#include <string>

struct GLFWwindow;

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    bool load(const std::string& filename);
    void setRenderWindow(GLFWwindow* window);
    void play();
    void pause();
    void stop();
    bool isPlaying() const;
    void cleanup();  // Explicit cleanup method

private:
    libvlc_instance_t* vlcInstance = nullptr;
    libvlc_media_t* media = nullptr;
    libvlc_media_player_t* mediaPlayer = nullptr;
    GLFWwindow* renderWindow = nullptr;
};

#endif