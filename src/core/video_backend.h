#ifndef SRC_CORE_VIDEO_BACKEND_H
#define SRC_CORE_VIDEO_BACKEND_H

#include <SDL2/SDL.h>
#include <cstdint>

namespace openhle {
namespace core {

class VideoBackend {
public:
    VideoBackend();
    ~VideoBackend();

    void present_frame(const uint8_t* framebuffer_ptr);
    void poll_input(uint64_t& out_key_mask, bool& out_quit);

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};

} // namespace core
} // namespace openhle

#endif // SRC_CORE_VIDEO_BACKEND_H
