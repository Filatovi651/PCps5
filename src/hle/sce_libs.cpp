#include "sce_libs.h"
#include <cstring>

namespace openhle {
namespace hle {

SceLibRegistry::SceLibRegistry() {
    symbol_map.emplace("sceVideoOutOpen", 200);
    symbol_map.emplace("sceVideoOutSubmitFlip", 201);
    symbol_map.emplace("sceAudioOutInit", 202);
    symbol_map.emplace("sceAudioOutOutput", 203);
    symbol_map.emplace("scePadInit", 204);
    symbol_map.emplace("scePadReadState", 205);
    symbol_map.emplace("sceKernelUsleep", 206);
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
