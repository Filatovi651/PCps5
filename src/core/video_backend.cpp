#include "video_backend.h"
#include <iostream>

namespace openhle {
namespace core {

VideoBackend::VideoBackend() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "[Video] SDL init failed: " << SDL_GetError() << "\n";
        return;
    }

    window = SDL_CreateWindow("PCps5 Display (320x240)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    if (!window) return;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return;

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 320, 240);
}

VideoBackend::~VideoBackend() {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void VideoBackend::present_frame(const uint8_t* framebuffer_ptr) {
    if (!texture || !renderer || !framebuffer_ptr) return;
    SDL_UpdateTexture(texture, nullptr, framebuffer_ptr, 320 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void VideoBackend::poll_input(uint64_t& out_key_mask, bool& out_quit) {
    SDL_Event e;
    out_quit = false;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) out_quit = true;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) out_quit = true;
    }

    const Uint8* state = SDL_GetKeyboardState(nullptr);
    out_key_mask = 0;
    if (*(state + SDL_SCANCODE_UP))    out_key_mask |= 1;
    if (*(state + SDL_SCANCODE_DOWN))  out_key_mask |= 2;
    if (*(state + SDL_SCANCODE_LEFT))  out_key_mask |= 4;
    if (*(state + SDL_SCANCODE_RIGHT)) out_key_mask |= 8;

    SDL_Delay(16);
}

} // namespace core
} // namespace openhle
