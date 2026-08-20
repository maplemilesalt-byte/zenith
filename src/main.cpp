#include "cpu/cpu.h"
#include "elf_loader.h"
#include "memory/guest_memory.h"
#include "renderer.h"
#include "window.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    constexpr int width = 1280;
    constexpr int height = 720;

    std::cout << "Zenith 0.3.0\n";
    std::cout << "Initializing guest memory and x86-64 CPU...\n";

    GuestMemory memory(64ull * 1024ull * 1024ull);

    if (argc >= 3 && std::string(argv[1]) == "--elf") {
        ElfLoader loader;
        ElfLoadResult result;
        std::string error;
        if (!loader.loadFile(argv[2], memory, result, error)) {
            std::cerr << "ELF load error: " << error << '\n';
            return 1;
        }

        std::cout << "ELF loaded: entry = 0x" << std::hex << result.entryPoint
                  << ", program headers = " << std::dec << result.programHeaders << '\n';

        CPU cpu(memory);
        cpu.setRip(result.entryPoint);
        // The current interpreter has no OS/syscall layer yet, so --elf is a
        // loader/CPU smoke test. Execute a small bounded number of instructions.
        constexpr int steps = 3;
        for (int i = 0; i < steps; ++i) {
            if (!cpu.step()) {
                std::cerr << "CPU error after ELF load: " << cpu.lastError() << '\n';
                return 1;
            }
        }
        std::cout << "ELF CPU smoke test: RAX = " << cpu.getRegister(CPU::RAX) << '\n';
        return 0;
    }

    constexpr std::uint64_t codeAddress = 0x1000;
    const auto permissions = GuestMemory::Permissions::Read |
                             GuestMemory::Permissions::Write |
                             GuestMemory::Permissions::Execute;
    if (!memory.map(codeAddress, 0, GuestMemory::PageSize, permissions)) {
        std::cerr << "Failed to map guest code page.\n";
        return 1;
    }

    const std::uint8_t program[] = {
        0xB8, 0x2A, 0x00, 0x00, 0x00,
        0xB9, 0x08, 0x00, 0x00, 0x00,
        0x48, 0x01, 0xC8
    };
    for (std::size_t i = 0; i < sizeof(program); ++i) {
        if (!memory.write8(codeAddress + i, program[i])) {
            std::cerr << "Failed to load guest program.\n";
            return 1;
        }
    }

    CPU cpu(memory);
    cpu.setRip(codeAddress);
    for (int i = 0; i < 3; ++i) {
        if (!cpu.step()) {
            std::cerr << "CPU error: " << cpu.lastError() << '\n';
            return 1;
        }
    }
    std::cout << "CPU self-test: RAX = " << cpu.getRegister(CPU::RAX) << '\n';

    XenithWindow window(width, height, "Zenith - Xbox Series X|S Emulator");
    Renderer renderer(width, height);
    float angle = 0.0f;
    while (window.isOpen()) {
        window.pollEvents();
        renderer.clear(0x00101018);
        renderer.drawCube(angle);
        window.present(renderer.pixels(), renderer.width(), renderer.height());
        angle += 0.01f;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::cout << "Zenith shutting down.\n";
    return 0;
}
