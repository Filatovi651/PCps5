#ifndef SRC_HLE_SCE_LIBS_H
#define SRC_HLE_SCE_LIBS_H

#include <string>
#include <unordered_map>
#include <cstdint>
#include <vector>

namespace openhle {
namespace hle {

class SceLibRegistry {
public:
    SceLibRegistry();
    uint32_t get_syscall_id(const std::string& symbol_name) const;
    static std::vector<uint8_t> build_trampoline(uint32_t syscall_id);

private:
    std::unordered_map<std::string, uint32_t> symbol_map;
};

} // namespace hle
} // namespace openhle

#endif
