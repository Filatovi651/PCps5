#ifndef SRC_LOADER_ELF_PARSER_H
#define SRC_LOADER_ELF_PARSER_H

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

namespace openhle {
namespace loader {

struct LoadableSegment {
    uint64_t vaddr;
    uint64_t memsz;
    uint32_t flags;
    std::vector<uint8_t> data;
};

class ElfParser {
public:
    bool load_executable(const std::string& filepath, uint64_t& out_entry_point, std::vector<LoadableSegment>& out_segments);

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

    struct Elf64_Phdr {
        uint32_t p_type;
        uint32_t p_flags;
        uint64_t p_offset;
        uint64_t p_vaddr;
        uint64_t p_paddr;
        uint64_t p_filesz;
        uint64_t p_memsz;
        uint64_t p_align;
    };
#pragma pack(pop)

    bool check_elf_header(const Elf64_Ehdr& header);
    bool read_program_headers(std::ifstream& file, const Elf64_Ehdr& header, std::vector<LoadableSegment>& out_segments);
};

} // namespace loader
} // namespace openhle

#endif // SRC_LOADER_ELF_PARSER_H
