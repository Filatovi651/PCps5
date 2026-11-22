#ifndef PCPS5_SDK_H
#define PCPS5_SDK_H

#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 240
#define FB_ADDR  ((uint32_t*)0x500000)

#define KEY_UP    1
#define KEY_DOWN  2
#define KEY_LEFT  4
#define KEY_RIGHT 8

static inline int64_t pcps5_open(const char* filename) {
    int64_t fd = -1;
    asm volatile(
        "mov $2, %%rax\n"
        "mov %1, %%rdi\n"
        "syscall\n"
        "mov %%rax, %0\n"
        : "=r"(fd)
        : "r"(filename)
        : "rax", "rdi", "rcx", "r11", "memory"
    );
    return fd;
}

static inline int64_t pcps5_read(int64_t fd, void* buf, uint64_t count) {
    int64_t bytes = -1;
    asm volatile(
        "mov $0, %%rax\n"
        "mov %1, %%rdi\n"
        "mov %2, %%rsi\n"
        "mov %3, %%rdx\n"
        "syscall\n"
        "mov %%rax, %0\n"
        : "=r"(bytes)
        : "r"(fd), "r"(buf), "r"(count)
        : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
    );
    return bytes;
}

static inline void pcps5_close(int64_t fd) {
    asm volatile(
        "mov $3, %%rax\n"
        "mov %0, %%rdi\n"
        "syscall\n"
        :
        : "r"(fd)
        : "rax", "rdi", "rcx", "r11"
    );
}

static inline uint64_t pcps5_present_frame(void) {
    uint64_t keys = 0;
    uint64_t fb = 0x500000;
    asm volatile(
        "mov $100, %%rax\n"
        "mov %1, %%rdi\n"
        "syscall\n"
        "mov %%rax, %0\n"
        : "=r"(keys)
        : "r"(fb)
        : "rax", "rdi", "rcx", "r11", "memory"
    );
    return keys;
}

static inline void pcps5_play_sound(int freq, int ms) {
    asm volatile(
        "mov $102, %%rax\n"
        "mov %0, %%rdi\n"
        "mov %1, %%rsi\n"
        "syscall\n"
        :
        : "r"((uint64_t)freq), "r"((uint64_t)ms)
        : "rax", "rdi", "rsi", "rcx", "r11"
    );
}

static inline void pcps5_exit(int code) {
    asm volatile(
        "mov $60, %%rax\n"
        "mov %0, %%rdi\n"
        "syscall\n"
        :
        : "r"((uint64_t)code)
        : "rax", "rdi", "rcx", "r11"
    );
}

#endif
