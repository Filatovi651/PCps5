#include "../src/core/memory_manager.h"
#include "../src/kernel/cpu_interpreter.h"
#include <iostream>
#include <vector>
#include <string>

int main() {
    try {
        openhle::MemoryManager mm(64 * 1024 * 1024); // Выделяем 64 МБ

        // 1. Кладем строку "Hello OpenHLE!\n" в память по адресу 0x402000
        std::string msg = "Hello OpenHLE!\n";
        std::vector<uint8_t> str_data(msg.begin(), msg.end());
        mm.map_segment(0x402000, str_data, 0x1000);

        // 2. Машинный код x86-64 по адресу 0x401000:
        // mov rax, 1        (sys_write)
        // mov rdi, 1        (stdout)
        // mov rsi, 0x402000 (адрес строки)
        // mov rdx, 15       (длина строки)
        // syscall
        // mov rax, 60       (sys_exit)
        // mov rdi, 0        (код возврата 0)
        // syscall
        std::vector<uint8_t> code = {
            0x48, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x48, 0xBF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x48, 0xBE, 0x00, 0x20, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x48, 0xBA, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x0F, 0x05,
            0x48, 0xB8, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x48, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x0F, 0x05
        };
        mm.map_segment(0x401000, code, 0x1000);

        // 3. Запускаем процессор!
        openhle::kernel::CpuInterpreter cpu(&mm);
        cpu.run(0x401000);

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
