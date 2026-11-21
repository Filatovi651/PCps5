#include "audio_backend.h"
#include <vector>
#include <cmath>

namespace openhle {
namespace core {

AudioBackend::AudioBackend() {
    SDL_AudioSpec want{};
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048;
    dev = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
    if (dev != 0) {
        SDL_PauseAudioDevice(dev, 0);
    }
}

AudioBackend::~AudioBackend() {
    if (dev != 0) {
        SDL_CloseAudioDevice(dev);
    }
}

void AudioBackend::play_beep(int frequency, int duration_ms) {
    if (dev == 0 || frequency <= 0 || duration_ms <= 0) return;
    int sample_count = (44100 * duration_ms) / 1000;
    std::vector<int16_t> samples(sample_count);
    for (int i = 0; i < sample_count; ++i) {
        double t = static_cast<double>(i) / 44100.0;
        samples[i] = (std::sin(2.0 * M_PI * frequency * t) >= 0.0) ? 4000 : -4000;
    }
    SDL_ClearQueuedAudio(dev);
    SDL_QueueAudio(dev, samples.data(), samples.size() * sizeof(int16_t));
}

} // namespace core
} // namespace openhle
