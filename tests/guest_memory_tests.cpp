#include "memory/guest_memory.h"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using P = GuestMemory::Permissions;
    GuestMemory memory(64ull * 1024ull * 1024ull);

    assert(memory.map(0x1000, 0x0000, GuestMemory::PageSize, P::Read | P::Write));
    assert(memory.write64(0x1000, 0x123456789ABCDEF0ull));
    std::uint64_t value = 0;
    assert(memory.read64(0x1000, value));
    assert(value == 0x123456789ABCDEF0ull);

    assert(!memory.map(0x1000, 0x1000, GuestMemory::PageSize, P::Read));
    assert(memory.map(0x2000, 0x1000, GuestMemory::PageSize, P::Read));
    assert(!memory.write8(0x2000, 0x42));
    assert(memory.unmap(0x2000, GuestMemory::PageSize));

    std::uint8_t unmappedValue = 0;
    assert(!memory.read8(0x2000, unmappedValue));
    std::cout << "GuestMemory tests passed.\n";
    return 0;
}
