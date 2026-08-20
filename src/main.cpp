#include "cpu/cpu.h"
#include "elf_loader.h"
#include "memory/guest_memory.h"
#include "renderer.h"
#include "window.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

int main() {
    constexpr int width = 1280;
    constexpr int height = 720;

    std::cout << "Zenith 0.2.0\n";
    std::cout << "Initializing guest memory, ELF loader and x86-64 CPU...\n";

    GuestMemory memory(64ull * 1024ull * 1024ull);
    ElfLoader loader;
    std::uint64_t entry = 0;
    const char* loaderError = nullptr;

    if (!loader.load("helloworld.elf", memory, entry, loaderError)) {
        std::cerr << "ELF loader error: " << (loaderError ? loaderError : "unknown error") << '\n';
        return 1;
    }

    CPU cpu(memory);
    cpu.setRip(entry);

    // helloworld.elf contains five instructions ending in the guest write syscall.
    for (int i = 0; i < 5; ++i) {
        if (!cpu.step()) {
            std::cerr << "Guest CPU error: " << cpu.lastError() << '\n';
            return 1;
        }
    }

    std::cout << "ELF self-test: entry = 0x" << std::hex << entry << std::dec << '\n';

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
