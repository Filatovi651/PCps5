#ifndef SRC_CORE_AUDIO_BACKEND_H
#define SRC_CORE_AUDIO_BACKEND_H

#include <SDL2/SDL.h>
#include <cstdint>

namespace openhle {
namespace core {

class AudioBackend {
public:
    AudioBackend();
    ~AudioBackend();
    void play_beep(int frequency, int duration_ms);

private:
    SDL_AudioDeviceID dev = 0;
};

} // namespace core
} // namespace openhle

#endif // SRC_CORE_AUDIO_BACKEND_H
