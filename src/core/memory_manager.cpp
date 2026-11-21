#include "memory_manager.h"
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <stdexcept>

namespace openhle {

MemoryManager::MemoryManager(size_t size) : mem_size(size) {
    base_addr = mmap(nullptr, mem_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base_addr == MAP_FAILED) {
        throw std::runtime_error("Failed to allocate virtual memory with mmap");
    }
}

MemoryManager::~MemoryManager() {
    if (base_addr && base_addr != MAP_FAILED) {
        if (munmap(base_addr, mem_size) == -1) {
            std::cerr << "Warning: munmap failed\n";
        }
    }
}

void MemoryManager::map_segment(uint64_t vaddr, const std::vector<uint8_t>& data, uint64_t memsz) {
    if (vaddr + memsz > mem_size || data.size() > memsz) {
        throw std::out_of_range("Segment exceeds allocated virtual memory bounds");
    }

    uint8_t* target_addr = static_cast<uint8_t*>(base_addr) + vaddr;
    if (!data.empty()) {
        std::memcpy(target_addr, data.data(), data.size());
    }
    if (memsz > data.size()) {
        std::memset(target_addr + data.size(), 0, memsz - data.size());
    }
}

void MemoryManager::dump_memory(uint64_t vaddr, size_t size) const {
    if (vaddr + size > mem_size) {
        throw std::out_of_range("Dump exceeds virtual memory bounds");
    }

    const uint8_t* start_addr = static_cast<const uint8_t*>(base_addr) + vaddr;
    std::cout << "Memory dump at 0x" << std::hex << vaddr << ": ";
    for (size_t i = 0; i < size; ++i) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(start_addr[i]) << " ";
    }
    std::cout << std::dec << "\n";
}

uint8_t* MemoryManager::get_pointer(uint64_t vaddr) const {
    if (vaddr >= mem_size) {
        throw std::out_of_range("Virtual address out of bounds");
    }
    return static_cast<uint8_t*>(base_addr) + vaddr;
}

} // namespace openhle
