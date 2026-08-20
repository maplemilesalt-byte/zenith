#include "cpu/cpu.h"
#include "memory/guest_memory.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using P = GuestMemory::Permissions;
    GuestMemory memory(64ull * 1024ull * 1024ull);
    assert(memory.map(0x1000, 0, GuestMemory::PageSize * 4, P::Read | P::Write | P::Execute));
    assert(memory.map(0x8000, 0x4000, GuestMemory::PageSize, P::Read | P::Write));

    CPU cpu(memory);
    cpu.setRip(0x1000);

    // mov eax, 42
    assert(memory.write8(0x1000, 0xB8));
    assert(memory.write32(0x1001, 42));
    // mov ecx, 8
    assert(memory.write8(0x1005, 0xB9));
    assert(memory.write32(0x1006, 8));
    // add rax, rcx
    assert(memory.write8(0x100A, 0x48));
    assert(memory.write8(0x100B, 0x01));
    assert(memory.write8(0x100C, 0xC8));
    // sub rax, rcx
    assert(memory.write8(0x100D, 0x48));
    assert(memory.write8(0x100E, 0x29));
    assert(memory.write8(0x100F, 0xC8));

    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 42);
    assert(cpu.rip() == 0x1005);

    assert(cpu.step());
    assert(cpu.getRegister(CPU::RCX) == 8);

    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 50);

    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 42);

    // Separate test for 64-bit immediate MOV.
    cpu.setRip(0x1010);
    assert(memory.write8(0x1010, 0x48));
    assert(memory.write8(0x1011, 0xB8));
    assert(memory.write64(0x1012, 0x123456789ABCDEF0ull));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x123456789ABCDEF0ull);

    // Compare equal values and verify ZF, then JZ over a NOP.
    cpu.setRegister(CPU::RAX, 7);
    cpu.setRegister(CPU::RCX, 7);
    cpu.setRip(0x1020);
    assert(memory.write8(0x1020, 0x48));
    assert(memory.write8(0x1021, 0x39));
    assert(memory.write8(0x1022, 0xC8));
    assert(memory.write8(0x1023, 0x74));
    assert(memory.write8(0x1024, 0x01));
    assert(memory.write8(0x1025, 0x90));
    assert(memory.write8(0x1026, 0x90));

    assert(cpu.step());
    assert((cpu.rflags() & CPU::ZeroFlag) != 0);
    assert(cpu.step());
    assert(cpu.rip() == 0x1026);

    // PUSH/POP using the guest stack.
    cpu.setRegister(CPU::RSP, 0x8010);
    cpu.setRegister(CPU::RAX, 0x1122334455667788ull);
    cpu.setRip(0x1030);
    assert(memory.write8(0x1030, 0x50)); // push rax
    assert(memory.write8(0x1031, 0x59)); // pop rcx
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RSP) == 0x8008);
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RCX) == 0x1122334455667788ull);
    assert(cpu.getRegister(CPU::RSP) == 0x8010);

    // CALL/RET: call from 0x1040 to 0x1048; function returns to 0x1045.
    cpu.setRegister(CPU::RSP, 0x8010);
    cpu.setRip(0x1040);
    assert(memory.write8(0x1040, 0xE8));
    assert(memory.write32(0x1041, 3));
    assert(memory.write8(0x1045, 0x90));
    assert(memory.write8(0x1048, 0xC3));
    assert(cpu.step());
    assert(cpu.rip() == 0x1048);
    assert(cpu.getRegister(CPU::RSP) == 0x8008);
    assert(cpu.step());
    assert(cpu.rip() == 0x1045);
    assert(cpu.getRegister(CPU::RSP) == 0x8010);

    // Unsupported instruction must fail instead of silently executing.
    cpu.setRip(0x1050);
    assert(memory.write8(0x1050, 0xCC));
    assert(!cpu.step());
    assert(cpu.lastError() != nullptr);

    std::cout << "CPU tests passed.\n";
    return 0;
}
