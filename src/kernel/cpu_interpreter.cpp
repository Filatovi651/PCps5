#include "cpu_interpreter.h"
#include <array>
#include <cstring>
#include <iostream>
#include <unistd.h>

namespace openhle {
namespace kernel {

CpuInterpreter::CpuInterpreter(openhle::MemoryManager* mem_manager, openhle::core::VideoBackend* video_backend)
    : mem_manager(mem_manager), video_backend(video_backend) {}

void CpuInterpreter::push64(uint64_t value) {
    rsp -= 8;
    uint8_t* ptr = mem_manager->get_pointer(rsp);
    std::memcpy(ptr, &value, sizeof(uint64_t));
}

uint64_t CpuInterpreter::pop64() {
    const uint8_t* ptr = mem_manager->get_pointer(rsp);
    uint64_t value = 0;
    std::memcpy(&value, ptr, sizeof(uint64_t));
    rsp += 8;
    return value;
}

void CpuInterpreter::run(uint64_t start_address) {
    rip = start_address;
    rsp = initial_rsp;
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
        if (op1 == 0xBB) { rbx = imm64; rip += 10; return; }      // mov rbx, imm64
        if (op1 == 0xBE) { rsi = imm64; rip += 10; return; }      // mov rsi, imm64
        if (op1 == 0xBF) { rdi = imm64; rip += 10; return; }      // mov rdi, imm64

        if (op1 == 0x83 && op2 == 0xC3) {                         // add rbx, imm8
            rbx = static_cast<uint64_t>(static_cast<int64_t>(rbx) + static_cast<int8_t>(op3));
            rip += 4;
            return;
        }
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
    } else if (op0 == 0x49) {
        if (op1 == 0xB8) {                                        // mov r8, imm64
            uint64_t imm64 = 0;
            std::memcpy(&imm64, buffer + 2, sizeof(uint64_t));
            r8 = imm64;
            rip += 10;
            return;
        }
        if (op1 == 0x83 && op2 == 0xC0) {                         // add r8, imm8
            r8 = static_cast<uint64_t>(static_cast<int64_t>(r8) + static_cast<int8_t>(op3));
            rip += 4;
            return;
        }
    } else if (op0 == 0xA8) {                                     // test al, imm8
        uint8_t al = static_cast<uint8_t>(rax & 0xFF);
        zf = ((al & op1) == 0);
        rip += 2;
        return;
    } else if (op0 == 0x74) {                                     // jz rel8
        int8_t offset = static_cast<int8_t>(op1);
        if (zf) {
            rip = static_cast<uint64_t>(static_cast<int64_t>(rip + 2) + offset);
        } else {
            rip += 2;
        }
        return;
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
    } else if (op0 == 0xE9) {                                     // jmp rel32
        int32_t offset = 0;
        std::memcpy(&offset, buffer + 1, sizeof(int32_t));
        rip = static_cast<uint64_t>(static_cast<int64_t>(rip + 5) + offset);
        return;
    } else if (op0 == 0xEB) {                                     // jmp rel8
        int8_t offset = static_cast<int8_t>(op1);
        rip = static_cast<uint64_t>(static_cast<int64_t>(rip + 2) + offset);
        return;
    } else if (op0 == 0xE8) {                                     // call rel32
        int32_t offset = 0;
        std::memcpy(&offset, buffer + 1, sizeof(int32_t));
        push64(rip + 5);
        rip = static_cast<uint64_t>(static_cast<int64_t>(rip + 5) + offset);
        return;
    } else if (op0 == 0xC3) {                                     // ret
        if (rsp >= initial_rsp) {
            std::cout << "[CPU] Main returned cleanly. Halting.\n";
            running = false;
        } else {
            rip = pop64();
        }
        return;
    } else if (op0 == 0xF3 && op1 == 0xA5) {                      // rep movsd
        if (rcx > 0) {
            uint8_t* dst = mem_manager->get_pointer(rdi);
            const uint8_t* src = mem_manager->get_pointer(rsi);
            std::memcpy(dst, src, rcx * 4);
            rdi += rcx * 4;
            rsi += rcx * 4;
            rcx = 0;
        }
        rip += 2;
        return;
    } else if (op0 == 0x0F && op1 == 0x05) {                      // syscall
        handle_syscall();
        rip += 2;
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
            rax = keys;
            if (quit) {
                std::cout << "[HLE] Window closed by user.\n";
                running = false;
            }
        }
    } else if (rax == 101) { // sys_draw_rect: rdi=fb, rbx=x, r8=y, rdx=color
        uint8_t* fb = mem_manager->get_pointer(rdi);
        int x0 = static_cast<int>(rbx);
        int y0 = static_cast<int>(r8);
        uint32_t color = static_cast<uint32_t>(rdx & 0xFFFFFFFF);
        for (int dy = 0; dy < 16; ++dy) {
            for (int dx = 0; dx < 16; ++dx) {
                int px = x0 + dx;
                int py = y0 + dy;
                if (px >= 0 && px < 320 && py >= 0 && py < 240) {
                    std::memcpy(fb + (py * 320 + px) * 4, &color, 4);
                }
            }
        }
    } else {
        std::cerr << "[HLE] Unimplemented syscall: " << rax << "\n";
    }
}

} // namespace kernel
} // namespace openhle
