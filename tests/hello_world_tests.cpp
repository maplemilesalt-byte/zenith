#include "cpu/cpu.h"
#include "memory/guest_memory.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

int main() {
    using P = GuestMemory::Permissions;
    GuestMemory memory(64ull * 1024ull * 1024ull);
    assert(memory.map(0x1000, 0, GuestMemory::PageSize, P::Read | P::Write | P::Execute));
    assert(memory.map(0x2000, GuestMemory::PageSize, GuestMemory::PageSize, P::Read | P::Write));

    constexpr char message[] = "Hello, world!\n";
    for (std::size_t i = 0; i < sizeof(message) - 1; ++i) {
        assert(memory.write8(0x2000 + i, static_cast<std::uint8_t>(message[i])));
    }

    // Linux x86-64 ABI: write(1, 0x2000, 14), then SYSCALL.
    // REX.W is accepted here by the CPU while the syscall itself uses 0F 05.
    assert(memory.write8(0x1000, 0x48));
    assert(memory.write8(0x1001, 0xB8));
    assert(memory.write64(0x1002, 1));              // RAX = SYS_write
    assert(memory.write8(0x100A, 0x48));
    assert(memory.write8(0x100B, 0xBF));
    assert(memory.write64(0x100C, 1));              // RDI = stdout
    assert(memory.write8(0x1014, 0x48));
    assert(memory.write8(0x1015, 0xBE));
    assert(memory.write64(0x1016, 0x2000));         // RSI = buffer
    assert(memory.write8(0x101E, 0x48));
    assert(memory.write8(0x101F, 0xBA));
    assert(memory.write64(0x1020, sizeof(message) - 1)); // RDX = length
    assert(memory.write8(0x1028, 0x48));
    assert(memory.write8(0x1029, 0x0F));
    assert(memory.write8(0x102A, 0x05));            // SYSCALL

    CPU cpu(memory);
    cpu.setRip(0x1000);
    for (int i = 0; i < 5; ++i) assert(cpu.step());
    assert(cpu.rip() == 0x102B);
    assert(cpu.getRegister(CPU::RAX) == sizeof(message) - 1);

    std::cout << "hello_world_tests: passed\n";
    return 0;
}
