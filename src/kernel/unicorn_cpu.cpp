#include "unicorn_cpu.h"
#include <iostream>
#include <cstring>
#include <string>

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
    for (auto& kv : open_files) {
        if (kv.second) std::fclose(kv.second);
    }
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

    if (rax == 0) { // sys_read(fd, buf, count)
        auto it = open_files.find(rdi);
        if (it != open_files.end() && it->second) {
            std::vector<uint8_t> buf(rdx);
            size_t read_bytes = std::fread(buf.data(), 1, rdx, it->second);
            if (read_bytes > 0) {
                uc_mem_write(uc, rsi, buf.data(), read_bytes);
            }
            uint64_t ret = read_bytes;
            uc_reg_write(uc, UC_X86_REG_RAX, &ret);
        } else {
            uint64_t err = (uint64_t)-1;
            uc_reg_write(uc, UC_X86_REG_RAX, &err);
        }
    } else if (rax == 1) { // sys_write(fd, buf, count)
        std::vector<uint8_t> buf(rdx);
        uc_mem_read(uc, rsi, buf.data(), rdx);
        std::cout.write((const char*)buf.data(), rdx);
    } else if (rax == 2) { // sys_open(path_ptr) -> sandboxed to tests/
        char path_buf[256] = {0};
        uc_mem_read(uc, rdi, path_buf, sizeof(path_buf) - 1);
        std::string req_path(path_buf);
        if (req_path.find("..") != std::string::npos) {
            uint64_t err = (uint64_t)-1;
            uc_reg_write(uc, UC_X86_REG_RAX, &err);
            return;
        }
        std::string full_path = "tests/" + req_path;
        FILE* f = std::fopen(full_path.c_str(), "rb");
        if (!f) {
            uint64_t err = (uint64_t)-1;
            uc_reg_write(uc, UC_X86_REG_RAX, &err);
        } else {
            uint64_t fd = next_fd++;
            open_files[fd] = f;
            uc_reg_write(uc, UC_X86_REG_RAX, &fd);
        }
    } else if (rax == 3) { // sys_close(fd)
        auto it = open_files.find(rdi);
        if (it != open_files.end()) {
            if (it->second) std::fclose(it->second);
            open_files.erase(it);
        }
        uint64_t ok = 0;
        uc_reg_write(uc, UC_X86_REG_RAX, &ok);
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
