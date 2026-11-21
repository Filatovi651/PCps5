#include "elf_parser.h"
#include <cstring>
#include <iostream>

namespace openhle {
namespace loader {

bool ElfParser::load_executable(const std::string& filepath, uint64_t& out_entry_point, std::vector<LoadableSegment>& out_segments) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Loader] Failed to open file: " << filepath << "\n";
        return false;
    }

    Elf64_Ehdr header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file || !check_elf_header(header)) {
        std::cerr << "[Loader] Invalid ELF64 x86-64 header\n";
        return false;
    }

    out_entry_point = header.e_entry;
    return read_program_headers(file, header, out_segments);
}

bool ElfParser::check_elf_header(const Elf64_Ehdr& header) {
    const uint8_t* id = header.e_ident;
    return *(id + 0) == 0x7f &&
           *(id + 1) == 'E' &&
           *(id + 2) == 'L' &&
           *(id + 3) == 'F' &&
           *(id + 4) == 2 &&      // 64-bit
           *(id + 5) == 1 &&      // Little Endian
           header.e_machine == 0x3e; // x86-64
}

bool ElfParser::read_program_headers(std::ifstream& file, const Elf64_Ehdr& header, std::vector<LoadableSegment>& out_segments) {
    Elf64_Phdr phdr;
    for (uint16_t i = 0; i < header.e_phnum; ++i) {
        file.seekg(header.e_phoff + i * sizeof(Elf64_Phdr));
        file.read(reinterpret_cast<char*>(&phdr), sizeof(phdr));
        if (!file) return false;

        if (phdr.p_type == 1) { // PT_LOAD
            std::vector<uint8_t> data(phdr.p_filesz);
            if (phdr.p_filesz > 0) {
                file.seekg(phdr.p_offset);
                file.read(reinterpret_cast<char*>(data.data()), phdr.p_filesz);
                if (!file) return false;
            }
            out_segments.push_back({phdr.p_vaddr, phdr.p_memsz, phdr.p_flags, std::move(data)});
        }
    }
    return true;
}

} // namespace loader
} // namespace openhle
