#include "elf_relinker.h"
#include <fstream>
#include <iostream>
#include <cstring>

namespace openhle {
namespace loader {

bool ElfRelinker::analyze_imports(const std::string& filepath, std::vector<RelocPatch>& out_patches) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Relinker] Cannot open file: " << filepath << "\n";
        return false;
    }

    Elf64_Ehdr ehdr{};
    file.read(reinterpret_cast<char*>(&ehdr), sizeof(ehdr));
    if (!file || std::memcmp(ehdr.e_ident, "\x7f" "ELF", 4) != 0) {
        std::cerr << "[Relinker] Not a valid ELF64 file\n";
        return false;
    }

    if (ehdr.e_shoff == 0 || ehdr.e_shnum == 0) {
        std::cout << "[Relinker] Static binary (no section headers)\n";
        return true;
    }

    std::vector<Elf64_Shdr> shdrs(ehdr.e_shnum);
    file.seekg(ehdr.e_shoff);
    file.read(reinterpret_cast<char*>(shdrs.data()), ehdr.e_shnum * sizeof(Elf64_Shdr));

    int dynsym_idx = -1;
    for (int i = 0; i < ehdr.e_shnum; ++i) {
        if ((shdrs.data() + i)->sh_type == 11) { // SHT_DYNSYM
            dynsym_idx = i;
            break;
        }
    }

    if (dynsym_idx == -1) {
        std::cout << "[Relinker] No .dynsym section found\n";
        return true;
    }

    const auto& sym_sec = *(shdrs.data() + dynsym_idx);
    const auto& str_sec = *(shdrs.data() + sym_sec.sh_link);

    std::vector<char> dynstr(str_sec.sh_size);
    file.seekg(str_sec.sh_offset);
    file.read(dynstr.data(), str_sec.sh_size);

    size_t sym_count = sym_sec.sh_size / sizeof(Elf64_Sym);
    std::vector<Elf64_Sym> syms(sym_count);
    file.seekg(sym_sec.sh_offset);
    file.read(reinterpret_cast<char*>(syms.data()), sym_sec.sh_size);

    for (int i = 0; i < ehdr.e_shnum; ++i) {
        const auto& sec = *(shdrs.data() + i);
        if (sec.sh_type == 4) { // SHT_RELA
            size_t rela_count = sec.sh_size / sizeof(Elf64_Rela);
            std::vector<Elf64_Rela> relas(rela_count);
            file.seekg(sec.sh_offset);
            file.read(reinterpret_cast<char*>(relas.data()), sec.sh_size);

            for (const auto& r : relas) {
                uint32_t sym_idx = static_cast<uint32_t>(r.r_info >> 32);
                if (sym_idx < sym_count) {
                    const auto& s = *(syms.data() + sym_idx);
                    std::string name = (s.st_name < dynstr.size()) ? std::string(dynstr.data() + s.st_name) : "unknown";
                    std::cout << "  -> Patch GOT at 0x" << std::hex << r.r_offset
                              << std::dec << " -> Symbol: " << name << "\n";
                    out_patches.push_back({r.r_offset, name});
                }
            }
        }
    }

    return true;
}

} // namespace loader
} // namespace openhle
