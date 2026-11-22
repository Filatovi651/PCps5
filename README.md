# PCps5

![Language](https://shields.io)
![Platform](https://shields.io)
![Engine](https://shields.io)
![Graphics](https://shields.io)
![Progress](https://shields.io)

**PCps5** is an experimental open-source **x86-64 / ELF64 Static Relinker & High-Level Emulation (HLE) compatibility layer** written in C++20 for macOS (Apple Silicon / M-series).

> **Legal & Educational Disclaimer:** This project is strictly an educational research tool and homebrew SDK. It contains no proprietary firmware, decryption keys, or copyrighted assets, and is designed solely for compiling and running legal open-source `x86-64` ELF64 binaries on ARM64 Macs.

## Key Features

- **Hybrid Relinker + JIT Architecture:** Combines static ELF64 symbol/relocation analysis (`--relink`) with live `x86-64` to `ARM64` JIT execution via **Unicorn Engine**.
- **ELF64 Loader (`ElfParser`):** Parses 64-bit ELF headers and maps `PT_LOAD` segments into virtual memory.
- **Static Relinker (`ElfRelinker`):** Inspects `.dynsym`, `.dynstr`, and `.rela.plt` sections to resolve external function imports and prepare GOT/PLT HLE patches.
- **Native macOS Video & Audio (`SDL2`):** Provides a 320x240 RGBA software framebuffer upscaled to 640x480, real-time keyboard polling, and square-wave audio synthesis.
- **Sandboxed HLE Filesystem:** Emulates `sys_open`, `sys_read`, and `sys_close` restricted safely to the `tests/` directory so C programs can load external level maps and assets.
- **Freestanding C SDK (`sdk/pcps5_sdk.h`):** Allows writing and compiling standalone C games directly into `x86-64` ELF64 binaries.

## Building on macOS (Apple Silicon)

### 1. Install Dependencies via Homebrew
```bash
brew install sdl2 unicorn x86_64-elf-gcc
```

### 2. Compile PCps5
```bash
clang++ -std=c++20 -I/opt/homebrew/include -L/opt/homebrew/lib -lSDL2 -lunicorn \
    src/main.cpp src/loader/elf_parser.cpp src/loader/elf_relinker.cpp \
    src/core/video_backend.cpp src/core/audio_backend.cpp src/kernel/unicorn_cpu.cpp \
    -o pcps5
```

## Quick Start & Examples

### Analyze Dynamic ELF64 Imports (Static Relinker Mode)
```bash
./pcps5 --relink tests/dynamic_ps_stub.elf
```

### Compile & Run the C Maze Game (with External Map Loading)
```bash
x86_64-elf-gcc -O2 -ffreestanding -nostdlib -Wl,-Ttext=0x401000 tests/c_maze.c -o tests/c_maze.elf
./pcps5 tests/c_maze.elf
```

## Roadmap (100-Day Engineering Challenge)

- [x] **0.5%** — ELF64 `PT_LOAD` Segment Loader & Virtual Memory Mapping
- [x] **1.0%** — Unicorn JIT `x86-64` Execution + SDL2 Video, Input & Audio Backend
- [x] **1.5%** — Static Relinker (`.dynsym` / `.rela.plt` import & relocation scanner)
- [x] **2.0%** — Sandboxed HLE Filesystem (`sys_open`, `sys_read`, `sys_close`) & C Homebrew SDK
- [x] **5.0%** — Automatic GOT/PLT Trampoline Patching in Unicorn Memory
- [x] **10.0%** — Clean-Room HLE Modules (`libkernel`, `libSceSystemService`, `libc`)
- [ ] **15.0%** — Multi-threading (`pthread` HLE stubs) & Gamepad Input Support
- [ ] **40.0%** — Hardware 3D Rendering Backend via Vulkan / MoltenVK

## Contributing & Help Wanted

I am building **PCps5** as a long-term learning and open-source project on Apple Silicon! Pull Requests, code reviews, and architectural advice are very welcome, especially in:
- Automatic GOT/PLT trampoline code generation for `x86-64`
- Implementing additional POSIX / FreeBSD HLE syscalls (`sys_mmap`, `sys_clock_gettime`, `pthread`)
- Designing a **Vulkan / MoltenVK** 3D graphics pipeline
