#include "loader/elf_parser.h"
#include "loader/elf_relinker.h"
#include "core/video_backend.h"
#include "core/audio_backend.h"
#include "kernel/unicorn_cpu.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:\n  ./pcps5 <path_to_elf>\n  ./pcps5 --relink <path_to_elf>\n";
        return 1;
    }

    std::string arg1 = *(argv + 1);
    if (arg1 == "--relink" && argc >= 3) {
        openhle::loader::ElfRelinker relinker;
        std::vector<openhle::loader::RelocPatch> patches;
        std::cout << "[PCps5 Static Relinker] Analyzing ELF64 imports & relocations...\n";
        if (!relinker.analyze_imports(*(argv + 2), patches)) return 1;
        std::cout << "[PCps5 Static Relinker] Total HLE patches prepared: " << patches.size() << "\n";
        return 0;
    }

    openhle::loader::ElfParser parser;
    uint64_t entry_point = 0;
    std::vector<openhle::loader::LoadableSegment> segments;

    if (!parser.load_executable(arg1, entry_point, segments)) {
        return 1;
    }

    std::cout << "[PCps5 v2.0 JIT] Loaded " << segments.size() << " segments. Entry: 0x"
              << std::hex << entry_point << std::dec << "\n";

    openhle::core::VideoBackend video;
    openhle::core::AudioBackend audio;
    openhle::kernel::UnicornCpu cpu(&video, &audio);

    for (const auto& seg : segments) {
        cpu.map_segment(seg.vaddr, seg.data);
    }

    cpu.run(entry_point);
    return 0;
}
