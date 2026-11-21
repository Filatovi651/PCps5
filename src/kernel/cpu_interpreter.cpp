#include "cpu_interpreter.h"
#include <array>
#include <cstring>
#include <iostream>
#include <unistd.h>

namespace openhle {
namespace kernel {

CpuInterpreter::CpuInterpreter(openhle::MemoryManager* mem_manager, openhle::core::VideoBackend* video_backend)
    : mem_manager(mem_manager), video_backend(video_backend) {}

void CpuInterpreter::run(uint64_t start_address) {
    rip = start_address;
    running = true;

    while (running) {
        std::array<uint8_t, 15> buffer{};
        fetch(buffer.data(), buffer.size());
        decode_and_execute(buffer.data());
    }
}

void CpuInterpreter::fetch(uint8_t* buffer, size_t buffer_size) {
    const uint8_t* mem = mem_manager->get_pointer(rip);
    std::memcpy(buffer, mem, buffer_size);
}

void CpuInterpreter::decode_and_execute(const uint8_t* buffer) {
    uint8_t op0 = *(buffer + 0);
    uint8_t op1 = *(buffer + 1);
    uint8_t op2 = *(buffer + 2);
    uint8_t op3 = *(buffer + 3);

    if (op0 == 0x48) {
        uint64_t imm64 = 0;
        std::memcpy(&imm64, buffer + 2, sizeof(uint64_t));

        if (op1 == 0xB8) { rax = imm64; rip += 10; return; }      // mov rax, imm64
        if (op1 == 0xB9) { rcx = imm64; rip += 10; return; }      // mov rcx, imm64
        if (op1 == 0xBA) { rdx = imm64; rip += 10; return; }      // mov rdx, imm64
        if (op1 == 0xBE) { rsi = imm64; rip += 10; return; }      // mov rsi, imm64
        if (op1 == 0xBF) { rdi = imm64; rip += 10; return; }      // mov rdi, imm64

        if (op1 == 0x83 && op2 == 0xC7 && op3 == 0x04) {          // add rdi, 4
            rdi += 4;
            rip += 4;
            return;
        }
        if (op1 == 0xFF && op2 == 0xC9) {                         // dec rcx
            rcx -= 1;
            zf = (rcx == 0);
            rip += 3;
            return;
        }
    } else if (op0 == 0x89 && op1 == 0x07) {                      // mov dword ptr [rdi], eax
        uint32_t val = static_cast<uint32_t>(rax & 0xFFFFFFFF);
        uint8_t* dst = mem_manager->get_pointer(rdi);
        std::memcpy(dst, &val, sizeof(uint32_t));
        rip += 2;
        return;
    } else if (op0 == 0x05) {                                     // add eax, imm32
        uint32_t imm32 = 0;
        std::memcpy(&imm32, buffer + 1, sizeof(uint32_t));
        uint32_t eax = static_cast<uint32_t>(rax & 0xFFFFFFFF) + imm32;
        rax = (rax & 0xFFFFFFFF00000000ULL) | eax;
        rip += 5;
        return;
    } else if (op0 == 0x75) {                                     // jnz rel8
        int8_t offset = static_cast<int8_t>(op1);
        if (!zf) {
            rip = static_cast<uint64_t>(static_cast<int64_t>(rip + 2) + offset);
        } else {
            rip += 2;
        }
        return;
    } else if (op0 == 0xEB) {                                     // jmp rel8 (безусловный переход)
        int8_t offset = static_cast<int8_t>(op1);
        rip = static_cast<uint64_t>(static_cast<int64_t>(rip + 2) + offset);
        return;
    } else if (op0 == 0x0F && op1 == 0x05) {                      // syscall
        handle_syscall();
        rip += 2;
        return;
    } else if (op0 == 0xC3) {                                     // ret
        running = false;
        return;
    }

    std::cerr << "[CPU] Unknown instruction 0x" << std::hex << static_cast<int>(op0)
              << " at 0x" << rip << std::dec << "\n";
    running = false;
}

void CpuInterpreter::handle_syscall() {
    if (rax == 1) { // sys_write
        if (rdi == 1) {
            const char* str_ptr = reinterpret_cast<const char*>(mem_manager->get_pointer(rsi));
            ssize_t written = write(STDOUT_FILENO, str_ptr, rdx);
            (void)written;
        }
    } else if (rax == 60) { // sys_exit
        std::cout << "[HLE] sys_exit called with code: " << rdi << "\n";
        running = false;
    } else if (rax == 100) { // sys_present_frame (320x240) + poll_input
        if (video_backend) {
            const uint8_t* fb = mem_manager->get_pointer(rdi);
            video_backend->present_frame(fb);
            uint64_t keys = 0;
            bool quit = false;
            video_backend->poll_input(keys, quit);
            rax = keys; // Возвращаем нажатые кнопки в RAX
            if (quit) {
                std::cout << "[HLE] Window closed by user.\n";
                running = false;
            }
        }
    } else {
        std::cerr << "[HLE] Unimplemented syscall: " << rax << "\n";
    }
}

} // namespace kernel
} // namespace openhle
