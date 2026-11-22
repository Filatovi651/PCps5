# PCps5 — Hybrid Static Relinker & HLE Compatibility Layer for Apple Silicon

**PCps5** is an experimental open-source **x86-64 / ELF64 Static Relinker & High-Level Emulation (HLE) engine** written in C++20 for macOS (Apple Silicon / M-series).

It combines two architectural approaches:
1. **Static ELF64 Relinker (`--relink`):** Parses `.dynsym`, `.dynstr`, and `.rela.plt` sections of 64-bit ELF executables to resolve external symbol imports and prepare GOT/PLT patches for HLE stubs.
2. **Unicorn JIT Execution + HLE Kernel:** Translates `x86-64` machine code on the fly to `ARM64` (Apple Silicon) and intercepts POSIX/custom syscalls to provide native **SDL2** graphics, audio synthesis, input polling, and sandboxed filesystem I/O.

> **Disclaimer:** This project is strictly an educational research and homebrew development tool. It does not include any proprietary firmware, keys, or copyrighted assets, and is intended solely for running legal open-source ELF64 binaries.

## Architecture

- `src/loader/elf_parser.*` — ELF64 header and `PT_LOAD` segment loader.
- `src/loader/elf_relinker.*` — Dynamic symbol (`.dynsym`) and relocation (`.rela`) analyzer.
- `src/kernel/unicorn_cpu.*` — Unicorn Engine (`x86-64`) wrapper with HLE syscall dispatcher (`sys_open`, `sys_read`, `sys_write`, `sys_close`, `sys_present_frame`, `sys_play_sound`).
- `src/core/video_backend.*` — 320x240 RGBA framebuffer renderer and keyboard input handler via SDL2.
- `src/core/audio_backend.*` — Real-time square-wave audio synthesizer via SDL2 Audio.
- `sdk/pcps5_sdk.h` — Freestanding C header for compiling homebrew `x86-64` ELF64 games.

## Building on macOS (Apple Silicon)

### Prerequisites
```bash
brew install sdl2 unicorn x86_64-elf-gcc
```

### Compile PCps5
```bash
clang++ -std=c++20 -I/opt/homebrew/include -L/opt/homebrew/lib -lSDL2 -lunicorn \
    src/main.cpp src/loader/elf_parser.cpp src/loader/elf_relinker.cpp \
    src/core/video_backend.cpp src/core/audio_backend.cpp src/kernel/unicorn_cpu.cpp \
    -o pcps5
```

## Usage & Homebrew Examples

### 1. Analyze ELF64 Dynamic Imports (Static Relinker Mode)
```bash
./pcps5 --relink tests/dynamic_ps_stub.elf
```

### 2. Compile & Run a C Homebrew Game (Maze with External Map Loading)
```bash
x86_64-elf-gcc -O2 -ffreestanding -nostdlib -Wl,-Ttext=0x401000 tests/c_maze.c -o tests/c_maze.elf
./pcps5 tests/c_maze.elf
```

## Roadmap (100-Day Engineering Challenge)
- [x] ELF64 `PT_LOAD` Loader & Virtual Memory Mapping
- [x] Static Relinker (`.dynsym` / `.rela.plt` symbol resolution)
- [x] Unicorn JIT `x86-64` -> `ARM64` Execution Backend
- [x] SDL2 Video Framebuffer, Keyboard Input & Audio Synthesizer
- [x] Sandboxed HLE Filesystem (`sys_open`, `sys_read`, `sys_close`)
- [x] Freestanding C SDK (`pcps5_sdk.h`) & Sample Homebrew Games
- [ ] Automatic GOT/PLT Trampoline Patching in Unicorn Memory
- [ ] Multi-threading (`pthread` HLE stubs) & Gamepad Support
- [ ] Vulkan / MoltenVK Hardware 3D Rendering Backend

## Contributing
Contributions, bug reports, and ideas for HLE syscall implementations or Vulkan/MoltenVK integration are very welcome! Feel free to open an Issue or submit a Pull Request.
