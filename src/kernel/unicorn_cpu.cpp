#include "unicorn_cpu.h"
#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <chrono>

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

void UnicornCpu::apply_relinker_patches(const std::vector<openhle::loader::RelocPatch>& patches) {
    uint64_t trampoline_addr = 0x900000;
    for (const auto& p : patches) {
        uint32_t sys_id = sce_libs.get_syscall_id(p.symbol_name);
        if (sys_id == 0) {
            std::cerr << "[HLE Warning] Unimplemented import: " << p.symbol_name << "\n";
            continue;
        }
        auto code = openhle::hle::SceLibRegistry::build_trampoline(sys_id);
        uc_mem_write(uc, trampoline_addr, code.data(), code.size());
        uc_mem_write(uc, p.offset, &trampoline_addr, sizeof(uint64_t));
        std::cout << "[HLE Relinker] Linked " << p.symbol_name << " -> GOT[0x"
                  << std::hex << p.offset << "] = Trampoline(0x" << trampoline_addr << ")"
                  << std::dec << "\n";
        trampoline_addr += 16;
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

    if (rax == 0) { // sys_read
        auto it = open_files.find(rdi);
        if (it != open_files.end() && it->second) {
            std::vector<uint8_t> buf(rdx);
            size_t read_bytes = std::fread(buf.data(), 1, rdx, it->second);
            if (read_bytes > 0) uc_mem_write(uc, rsi, buf.data(), read_bytes);
            uint64_t ret = read_bytes;
            uc_reg_write(uc, UC_X86_REG_RAX, &ret);
        } else {
            uint64_t err = (uint64_t)-1;
            uc_reg_write(uc, UC_X86_REG_RAX, &err);
        }
    } else if (rax == 1) { // sys_write
        std::vector<uint8_t> buf(rdx);
        uc_mem_read(uc, rsi, buf.data(), rdx);
        std::cout.write((const char*)buf.data(), rdx);
    } else if (rax == 2) { // sys_open (sandboxed to tests/)
        std::string req_path(256, 0);
        uc_mem_read(uc, rdi, req_path.data(), 255);
        req_path = std::string(req_path.c_str());
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
    } else if (rax == 3) { // sys_close
        auto it = open_files.find(rdi);
        if (it != open_files.end()) {
            if (it->second) std::fclose(it->second);
            open_files.erase(it);
        }
        uint64_t ok = 0;
        uc_reg_write(uc, UC_X86_REG_RAX, &ok);
    } else if (rax == 60) { // sys_exit
        SDL_Delay(200);
        uc_emu_stop(uc);
    } else if (rax == 100 || rax == 201) { // sys_present_frame / sceVideoOutSubmitFlip
        if (video_backend) {
            uint64_t fb_ptr = (rdi != 0) ? rdi : 0x500000;
            uc_mem_read(uc, fb_ptr, frame_buffer.data(), frame_buffer.size());
            video_backend->present_frame(frame_buffer.data());
            uint64_t keys = 0;
            bool quit = false;
            video_backend->poll_input(keys, quit);
            uc_reg_write(uc, UC_X86_REG_RAX, &keys);
            if (quit) uc_emu_stop(uc);
        }
    } else if (rax == 102 || rax == 203) { // sys_play_sound / sceAudioOutOutput
        if (audio_backend) {
            audio_backend->play_beep((int)rdi, (int)rsi);
        }
    } else if (rax == 200 || rax == 202 || rax == 204) { // sceVideoOutOpen / AudioInit / PadInit
        uint64_t handle = 1;
        uc_reg_write(uc, UC_X86_REG_RAX, &handle);
    } else if (rax == 205) { // scePadReadState
        if (video_backend) {
            uint64_t keys = 0;
            bool quit = false;
            video_backend->poll_input(keys, quit);
            uc_reg_write(uc, UC_X86_REG_RAX, &keys);
            if (quit) uc_emu_stop(uc);
        }
    } else if (rax == 206) { // sceKernelUsleep
        usleep((useconds_t)rdi);
    } else if (rax == 207) { // sceKernelGetProcessTime (микросекунды)
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
        uc_reg_write(uc, UC_X86_REG_RAX, &us);
    } else if (rax == 208) { // malloc / sceKernelAllocateDirectMemory
        static uint64_t heap_ptr = 0xA00000;
        uint64_t size = (rdi > 0) ? rdi : 4096;
        uint64_t allocated = heap_ptr;
        heap_ptr += (size + 15) & ~15ULL;
        uc_reg_write(uc, UC_X86_REG_RAX, &allocated);
    } else if (rax == 209) { // sceSystemServiceHideSplashScreen
        uint64_t ok = 0;
        uc_reg_write(uc, UC_X86_REG_RAX, &ok);
    } else if (rax == 210) { // puts(const char* s)
        std::string str(512, 0);
        uc_mem_read(uc, rdi, str.data(), 511);
        std::cout << str.c_str() << "\n";
        uint64_t ok = 0;
        uc_reg_write(uc, UC_X86_REG_RAX, &ok);
    } else if (rax == 211) { // memset(dst, val, size)
        if (rdx > 0) {
            std::vector<uint8_t> buf(rdx, (uint8_t)(rsi & 0xFF));
            uc_mem_write(uc, rdi, buf.data(), rdx);
        }
        uc_reg_write(uc, UC_X86_REG_RAX, &rdi);
    } else if (rax == 212) { // memcpy(dst, src, size)
        if (rdx > 0) {
            std::vector<uint8_t> buf(rdx);
            uc_mem_read(uc, rsi, buf.data(), rdx);
            uc_mem_write(uc, rdi, buf.data(), rdx);
        }
        uc_reg_write(uc, UC_X86_REG_RAX, &rdi);
    }
}

} // namespace kernel
} // namespace openhle
