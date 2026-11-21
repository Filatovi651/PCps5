cmake_minimum_required(VERSION 3.16)
project(PCps5 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/opt/homebrew/include)
link_directories(/opt/homebrew/lib)

add_executable(pcps5
    src/main.cpp
    src/loader/elf_parser.cpp
    src/core/memory_manager.cpp
    src/core/video_backend.cpp
    src/kernel/cpu_interpreter.cpp
)

target_link_libraries(pcps5 SDL2)
