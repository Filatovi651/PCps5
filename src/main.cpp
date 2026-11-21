#include "loader/elf_parser.h"
#include "core/memory_manager.h"
#include "core/video_backend.h"
#include "core/audio_backend.h"
#include "kernel/cpu_interpreter.h"
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./openhle <path_to_elf>\n";
        return 1;
    }

    try {
        openhle::loader::ElfParser parser;
        uint64_t entry_point = 0;
        std::vector<openhle::loader::LoadableSegment> segments;

        if (!parser.load_executable(*(argv + 1), entry_point, segments)) {
            return 1;
        }

        std::cout << "[OpenHLE v1.0] Loaded " << segments.size() << " segments. Entry: 0x"
                  << std::hex << entry_point << std::dec << "\n";

        openhle::MemoryManager memory(64 * 1024 * 1024);
        for (const auto& seg : segments) {
            memory.map_segment(seg.vaddr, seg.data, seg.memsz);
        }

        openhle::core::VideoBackend video;
        openhle::core::AudioBackend audio;
        openhle::kernel::CpuInterpreter cpu(&memory, &video, &audio);
        cpu.run(entry_point);

    } catch (const std::exception& e) {
        std::cerr << "[OpenHLE] Fatal Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
