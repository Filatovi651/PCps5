#ifndef SRC_LOADER_ELF_RELINKER_H
#define SRC_LOADER_ELF_RELINKER_H

#include <cstdint>
#include <string>
#include <vector>

namespace openhle {
namespace loader {

struct RelocPatch {
    uint64_t offset;
    std::string symbol_name;
};

class ElfRelinker {
public:
    bool analyze_imports(const std::string& filepath, std::vector<RelocPatch>& out_patches);

private:
#pragma pack(push, 1)
    struct Elf64_Ehdr {
        uint8_t e_ident[16];
        uint16_t e_type;
        uint16_t e_machine;
        uint32_t e_version;
        uint64_t e_entry;
        uint64_t e_phoff;
        uint64_t e_shoff;
        uint32_t e_flags;
        uint16_t e_ehsize;
        uint16_t e_phentsize;
        uint16_t e_phnum;
        uint16_t e_shentsize;
        uint16_t e_shnum;
        uint16_t e_shstrndx;
    };

    struct Elf64_Shdr {
        uint32_t sh_name;
        uint32_t sh_type;
        uint64_t sh_flags;
        uint64_t sh_addr;
        uint64_t sh_offset;
        uint64_t sh_size;
        uint32_t sh_link;
        uint32_t sh_info;
        uint64_t sh_addralign;
        uint64_t sh_entsize;
    };

    struct Elf64_Sym {
        uint32_t st_name;
        uint8_t st_info;
        uint8_t st_other;
        uint16_t st_shndx;
        uint64_t st_value;
        uint64_t st_size;
    };

    struct Elf64_Rela {
        uint64_t r_offset;
        uint64_t r_info;
        int64_t r_addend;
    };
#pragma pack(pop)
};

} // namespace loader
} // namespace openhle

#endif
