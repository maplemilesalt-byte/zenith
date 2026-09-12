#include "cpu/cpu.h"
#include "memory/guest_memory.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using P = GuestMemory::Permissions;
    GuestMemory memory(64ull * 1024ull);
    assert(memory.map(0x1000, 0, GuestMemory::PageSize, P::Read | P::Write | P::Execute));

    CPU cpu(memory);
    cpu.setRip(0x1000);
    assert(memory.write8(0x1000, 0x90));

    assert(cpu.step());
    assert(cpu.rip() == 0x1001);
    assert(cpu.lastError() == nullptr);

    std::cout << "CPU NOP test passed.\n";
    return 0;
}
