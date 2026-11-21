#include "unicorn_cpu.h"
#include <iostream>
#include <cstring>

namespace openhle {
namespace kernel {

UnicornCpu::UnicornCpu(openhle::core::VideoBackend* video,
                       openhle::core::AudioBackend* audio)
    : video_backend(video), audio_backend(audio), frame_buffer(320 * 240 * 4, 0) {
    if (uc_open(UC_ARCH_X86, UC_MODE_64, &uc) != UC_ERR_OK) {
        std::cerr << "[Unicorn] Failed to open engine\n";
        return;
    }

    if (uc_mem_map(uc, 0x400000, 16 * 1024 * 1024, UC_PROT_ALL) != UC_ERR_OK) {
        std::cerr << "[Unicorn] Failed to map memory\n";
        return;
    }

    uint64_t rsp = 0x800000;
    uc_reg_write(uc, UC_X86_REG_RSP, &rsp);

    uc_hook syscall_hook;
    if (uc_hook_add(uc, &syscall_hook, UC_HOOK_INSN, (void*)UnicornCpu::syscall_handler,
                    this, 1, 0, UC_X86_INS_SYSCALL) != UC_ERR_OK) {
        std::cerr << "[Unicorn] Failed to add syscall hook\n";
    }
}

UnicornCpu::~UnicornCpu() {
    if (uc) uc_close(uc);
}

void UnicornCpu::map_segment(uint64_t vaddr, const std::vector<uint8_t>& data) {
    if (!data.empty()) {
        uc_mem_write(uc, vaddr, data.data(), data.size());
    }
}

void UnicornCpu::run(uint64_t entry_point) {
    uc_err err = uc_emu_start(uc, entry_point, 0x1400000, 0, 0);
    if (err != UC_ERR_OK) {
        std::cerr << "[Unicorn] Emulation stopped: " << uc_strerror(err) << "\n";
    }
}

void UnicornCpu::syscall_handler(uc_engine* uc, void* user_data) {
    ((UnicornCpu*)user_data)->handle_syscall();
}

void UnicornCpu::handle_syscall() {
    uint64_t rax = 0, rdi = 0, rsi = 0, rdx = 0;
    uc_reg_read(uc, UC_X86_REG_RAX, &rax);
    uc_reg_read(uc, UC_X86_REG_RDI, &rdi);
    uc_reg_read(uc, UC_X86_REG_RSI, &rsi);
    uc_reg_read(uc, UC_X86_REG_RDX, &rdx);

    if (rax == 1) { // sys_write
        std::vector<uint8_t> buf(rdx);
        uc_mem_read(uc, rsi, buf.data(), rdx);
        std::cout.write((const char*)buf.data(), rdx);
    } else if (rax == 60) { // sys_exit
        uc_emu_stop(uc);
    } else if (rax == 100) { // sys_present_frame + poll_input
        if (video_backend) {
            uc_mem_read(uc, rdi, frame_buffer.data(), frame_buffer.size());
            video_backend->present_frame(frame_buffer.data());
            uint64_t keys = 0;
            bool quit = false;
            video_backend->poll_input(keys, quit);
            uc_reg_write(uc, UC_X86_REG_RAX, &keys);
            if (quit) uc_emu_stop(uc);
        }
    } else if (rax == 102) { // sys_play_sound
        if (audio_backend) {
            audio_backend->play_beep((int)rdi, (int)rsi);
        }
    }
}

} // namespace kernel
} // namespace openhle
