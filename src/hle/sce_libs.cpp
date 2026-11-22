#include "sce_libs.h"
#include <cstring>

namespace openhle {
namespace hle {

SceLibRegistry::SceLibRegistry() {
    // 1. libSceVideoOut & libSceAudioOut & libScePad
    symbol_map.emplace("sceVideoOutOpen", 200);
    symbol_map.emplace("sceVideoOutSubmitFlip", 201);
    symbol_map.emplace("sceAudioOutInit", 202);
    symbol_map.emplace("sceAudioOutOutput", 203);
    symbol_map.emplace("scePadInit", 204);
    symbol_map.emplace("scePadReadState", 205);

    // 2. libkernel (Память, время, потоки и управление процессом)
    symbol_map.emplace("sceKernelUsleep", 206);
    symbol_map.emplace("sceKernelGetProcessTime", 207);
    symbol_map.emplace("sceKernelAllocateDirectMemory", 208);
    symbol_map.emplace("sceKernelExitProcess", 60);

    // 3. libSceSystemService (Системные сервисы консоли)
    symbol_map.emplace("sceSystemServiceHideSplashScreen", 209);
    symbol_map.emplace("sceSystemServiceGetStatus", 209);

    // 4. libScePthread (Многопоточность и синхронизация)
    symbol_map.emplace("scePthreadCreate", 220);
    symbol_map.emplace("pthread_create", 220);
    symbol_map.emplace("scePthreadJoin", 221);
    symbol_map.emplace("pthread_join", 221);
    symbol_map.emplace("scePthreadMutexInit", 222);
    symbol_map.emplace("scePthreadMutexLock", 222);
    symbol_map.emplace("scePthreadMutexUnlock", 222);

    // 5. libc (Стандартные функции C для динамических бинарников)
    symbol_map.emplace("malloc", 208);
    symbol_map.emplace("puts", 210);
    symbol_map.emplace("memset", 211);
    symbol_map.emplace("memcpy", 212);
}

uint32_t SceLibRegistry::get_syscall_id(const std::string& symbol_name) const {
    auto it = symbol_map.find(symbol_name);
    return (it != symbol_map.end()) ? it->second : 0;
}

std::vector<uint8_t> SceLibRegistry::build_trampoline(uint32_t syscall_id) {
    std::vector<uint8_t> code(8, 0);
    uint8_t* p = code.data();
    *(p + 0) = 0xB8;
    std::memcpy(p + 1, &syscall_id, sizeof(uint32_t));
    *(p + 5) = 0x0F;
    *(p + 6) = 0x05;
    *(p + 7) = 0xC3;
    return code;
}

} // namespace hle
} // namespace openhle
