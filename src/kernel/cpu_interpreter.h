#ifndef SRC_KERNEL_CPU_INTERPRETER_H
#define SRC_KERNEL_CPU_INTERPRETER_H

#include <cstdint>
#include <cstddef>
#include "../core/memory_manager.h"
#include "../core/video_backend.h"

namespace openhle {
namespace kernel {

class CpuInterpreter {
public:
    CpuInterpreter(openhle::MemoryManager* mem_manager, openhle::core::VideoBackend* video_backend = nullptr);
    void run(uint64_t start_address);

private:
    openhle::MemoryManager* mem_manager;
    openhle::core::VideoBackend* video_backend;
    uint64_t rax = 0;
    uint64_t rcx = 0;
    uint64_t rdx = 0;
    uint64_t rdi = 0;
    uint64_t rsi = 0;
    uint64_t rip = 0;
    bool zf = false;
    bool running = false;

    void fetch(uint8_t* buffer, size_t buffer_size);
    void decode_and_execute(const uint8_t* buffer);
    void handle_syscall();
};

} // namespace kernel
} // namespace openhle

#endif // SRC_KERNEL_CPU_INTERPRETER_H
