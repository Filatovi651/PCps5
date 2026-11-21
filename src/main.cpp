#include "loader/elf_parser.h"
#include "core/video_backend.h"
#include "core/audio_backend.h"
#include "kernel/unicorn_cpu.h"
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./pcps5 <path_to_elf>\n";
        return 1;
    }

    pcps5::loader::ElfParser parser;
    uint64_t entry_point = 0;
    std::vector<pcps5::loader::LoadableSegment> segments;

    if (!parser.load_executable(*(argv + 1), entry_point, segments)) {
        return 1;
    }

    std::cout << "[PCps5 v2.0 JIT] Loaded " << segments.size() << " segments. Entry: 0x"
              << std::hex << entry_point << std::dec << "\n";

    pcps5::core::VideoBackend video;
    pcps5::core::AudioBackend audio;
    pcps5::kernel::UnicornCpu cpu(&video, &audio);

    for (const auto& seg : segments) {
        cpu.map_segment(seg.vaddr, seg.data);
    }

    cpu.run(entry_point);
    return 0;
}
