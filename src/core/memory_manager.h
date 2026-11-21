#ifndef SRC_CORE_MEMORY_MANAGER_H
#define SRC_CORE_MEMORY_MANAGER_H

#include <cstdint>
#include <vector>
#include <cstddef>

namespace openhle {

class MemoryManager {
public:
    explicit MemoryManager(size_t size);
    ~MemoryManager();

    // Запрещаем копирование памяти, чтобы не вызвать двойной munmap
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    void map_segment(uint64_t vaddr, const std::vector<uint8_t>& data, uint64_t memsz);
    void dump_memory(uint64_t vaddr, size_t size) const;
    uint8_t* get_pointer(uint64_t vaddr) const;

private:
    void* base_addr = nullptr;
    size_t mem_size = 0;
};

} // namespace openhle

#endif // SRC_CORE_MEMORY_MANAGER_H
