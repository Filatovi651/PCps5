#include "cpu_interpreter.h"
#include <cstring>
#include <iostream>
#include <unistd.h>

namespace openhle {
namespace kernel {

CpuInterpreter::CpuInterpreter(openhle::MemoryManager* mem_manager)
    : mem_manager(mem_manager) {}

void CpuInterpreter::run(uint64_t start_address) {
    rip = start_address;
    running = true;

    while (running) {
        uint8_t buffer[15] = {0};
        fetch(buffer, sizeof(buffer));
        decode_and_execute(buffer);
    }
}

void CpuInterpreter::fetch(uint8_t* buffer, size_t buffer_size) {
    const uint8_t* mem = mem_manager->get_pointer(rip);
    std::memcpy(buffer, mem, buffer_size);
}

void CpuInterpreter::decode_and_execute(const uint8_t* buffer) {
    if (buffer[0] == 0x48) {
        uint64_t imm64 = 0;
        std::memcpy(&imm64, buffer + 2, sizeof(uint64_t));

        switch (buffer[1]) {
            case 0xB8: // mov rax, imm64
                rax = imm64;
                rip += 10;
                return;
            case 0xBF: // mov rdi, imm64
                rdi = imm64;
                rip += 10;
                return;
            case 0xBE: // mov rsi, imm64
                rsi = imm64;
                rip += 10;
                return;
            case 0xBA: // mov rdx, imm64
                rdx = imm64;
                rip += 10;
                return;
        }
    } else if (buffer[0] == 0x0F && buffer[1] == 0x05) { // syscall
        handle_syscall();
        rip += 2;
        return;
    } else if (buffer[0] == 0xC3) { // ret
        std::cout << "[CPU] Reached RET instruction. Halting execution.\n";
        running = false;
        return;
    }

    std::cerr << "[CPU] Unknown instruction 0x" << std::hex << static_cast<int>(buffer[0])
              << " at 0x" << rip << std::dec << "\n";
    running = false;
}

void CpuInterpreter::handle_syscall() {
    if (rax == 1) { // sys_write
        if (rdi == 1) { // stdout
            const char* str_ptr = reinterpret_cast<const char*>(mem_manager->get_pointer(rsi));
            ssize_t written = write(STDOUT_FILENO, str_ptr, rdx);
            if (written == -1) {
                std::cerr << "[HLE] sys_write failed\n";
            }
        }
    } else if (rax == 60) { // sys_exit
        std::cout << "[HLE] sys_exit called with code: " << rdi << "\n";
        running = false;
    } else {
        std::cerr << "[HLE] Unimplemented syscall: " << rax << "\n";
    }
}

} // namespace kernel
} // namespace openhle
