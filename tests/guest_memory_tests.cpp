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

    // Cross-page accesses must work when both pages are mapped.
    assert(memory.map(0x3000, 0x2000, GuestMemory::PageSize, P::Read | P::Write));
    assert(memory.map(0x4000, 0x3000, GuestMemory::PageSize, P::Read | P::Write));
    assert(memory.write64(0x3FFC, 0xAABBCCDDEEFF0011ull));
    assert(memory.read64(0x3FFC, value));
    assert(value == 0xAABBCCDDEEFF0011ull);

    // A multi-byte access must fail if the second page is unmapped.
    assert(memory.unmap(0x4000, GuestMemory::PageSize));
    assert(!memory.read64(0x3FFC, value));

    std::cout << "GuestMemory tests passed.\n";
    return 0;
}
