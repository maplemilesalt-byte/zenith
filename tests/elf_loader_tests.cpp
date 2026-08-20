#include "elf_loader.h"
#include "memory/guest_memory.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "../helloworld.elf";

    GuestMemory memory(64ull * 1024ull * 1024ull);
    ElfLoader loader;
    std::uint64_t entry = 0;
    const char* error = nullptr;

    assert(loader.load(path, memory, entry, error));
    assert(error == nullptr);
    assert(entry == 0x400000);

    std::uint8_t first = 0;
    assert(memory.read8(entry, first));
    assert(first == 0xB8);

    std::uint8_t message = 0;
    assert(memory.read8(0x400020, message));
    assert(message == 'H');

    std::cout << "elf_loader_tests: passed\n";
    return 0;
}
