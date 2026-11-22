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
