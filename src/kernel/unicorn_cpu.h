#ifndef SRC_KERNEL_UNICORN_CPU_H
#define SRC_KERNEL_UNICORN_CPU_H

#include <unicorn/unicorn.h>
#include <vector>
#include <cstdint>
#include "../core/video_backend.h"
#include "../core/audio_backend.h"

namespace openhle {
namespace kernel {

class UnicornCpu {
public:
    UnicornCpu(openhle::core::VideoBackend* video = nullptr,
               openhle::core::AudioBackend* audio = nullptr);
    ~UnicornCpu();

    void map_segment(uint64_t vaddr, const std::vector<uint8_t>& data);
    void run(uint64_t entry_point);

private:
    uc_engine* uc = nullptr;
    openhle::core::VideoBackend* video_backend = nullptr;
    openhle::core::AudioBackend* audio_backend = nullptr;
    std::vector<uint8_t> frame_buffer;

    static void syscall_handler(uc_engine* uc, void* user_data);
    void handle_syscall();
};

} // namespace kernel
} // namespace openhle

#endif
