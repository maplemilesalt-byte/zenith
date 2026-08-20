#include "cpu/cpu.h"
#include "memory/guest_memory.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using P = GuestMemory::Permissions;
    GuestMemory memory(64ull * 1024ull * 1024ull);
    assert(memory.map(0x2000, 0, GuestMemory::PageSize * 4, P::Read | P::Write | P::Execute));
    assert(memory.map(0x8000, 0x4000, GuestMemory::PageSize * 2, P::Read | P::Write));

    CPU cpu(memory);

    // INC/DEC update arithmetic flags but preserve CF.
    cpu.setRegister(CPU::RAX, 0);
    cpu.setRegister(CPU::RCX, 1);
    cpu.setRip(0x2000);
    assert(memory.write8(0x2000, 0x48));
    assert(memory.write8(0x2001, 0x39));
    assert(memory.write8(0x2002, 0xC8)); // CMP RAX,RCX -> CF=1
    assert(cpu.step());
    assert((cpu.rflags() & CPU::CarryFlag) != 0);
    assert(memory.write8(0x2003, 0x48));
    assert(memory.write8(0x2004, 0xFF));
    assert(memory.write8(0x2005, 0xC0)); // INC RAX
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 1);
    assert((cpu.rflags() & CPU::CarryFlag) != 0);

    cpu.setRegister(CPU::RAX, 0x8000000000000000ull);
    cpu.setRegister(CPU::RCX, 1);
    cpu.setRip(0x2010);
    assert(memory.write8(0x2010, 0x48));
    assert(memory.write8(0x2011, 0x39));
    assert(memory.write8(0x2012, 0xC8)); // CF=0
    assert(cpu.step());
    assert(memory.write8(0x2013, 0x48));
    assert(memory.write8(0x2014, 0xFF));
    assert(memory.write8(0x2015, 0xC8)); // DEC RAX
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x7FFFFFFFFFFFFFFFull);
    assert((cpu.rflags() & CPU::CarryFlag) == 0);

    // NOT must not change flags; NEG is 0-x and sets CF iff x != 0.
    cpu.setRegister(CPU::RAX, 0);
    cpu.setRegister(CPU::RCX, 1);
    cpu.setRip(0x2020);
    assert(memory.write8(0x2020, 0x48));
    assert(memory.write8(0x2021, 0x39));
    assert(memory.write8(0x2022, 0xC8)); // CF=1
    assert(cpu.step());
    const auto flagsBeforeNot = cpu.rflags();
    assert(memory.write8(0x2023, 0x48));
    assert(memory.write8(0x2024, 0xF7));
    assert(memory.write8(0x2025, 0xD0)); // NOT RAX
    cpu.setRegister(CPU::RAX, 0x0F0F0F0F0F0F0F0Full);
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xF0F0F0F0F0F0F0F0ull);
    assert(cpu.rflags() == flagsBeforeNot);

    cpu.setRegister(CPU::RAX, 5);
    cpu.setRip(0x2030);
    assert(memory.write8(0x2030, 0x48));
    assert(memory.write8(0x2031, 0xF7));
    assert(memory.write8(0x2032, 0xD8)); // NEG RAX
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xFFFFFFFFFFFFFFFBull);
    assert((cpu.rflags() & CPU::CarryFlag) != 0);

    cpu.setRegister(CPU::RAX, 0);
    cpu.setRip(0x2033);
    assert(memory.write8(0x2033, 0x48));
    assert(memory.write8(0x2034, 0xF7));
    assert(memory.write8(0x2035, 0xD8));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0);
    assert((cpu.rflags() & CPU::CarryFlag) == 0);
    assert((cpu.rflags() & CPU::ZeroFlag) != 0);

    // SHL/SHR/SAR count=1 flag semantics, plus count=0 preserving flags/value.
    cpu.setRegister(CPU::RAX, 0x4000000000000000ull);
    cpu.setRip(0x2040);
    assert(memory.write8(0x2040, 0x48));
    assert(memory.write8(0x2041, 0xC1));
    assert(memory.write8(0x2042, 0xE0));
    assert(memory.write8(0x2043, 0x01)); // SHL RAX,1: OF must be 1
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x8000000000000000ull);
    assert((cpu.rflags() & CPU::OverflowFlag) != 0);
    assert((cpu.rflags() & CPU::CarryFlag) == 0);

    cpu.setRegister(CPU::RAX, 0x8000000000000000ull);
    cpu.setRip(0x2050);
    assert(memory.write8(0x2050, 0x48));
    assert(memory.write8(0x2051, 0xC1));
    assert(memory.write8(0x2052, 0xE8));
    assert(memory.write8(0x2053, 0x01)); // SHR RAX,1: OF follows original sign
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x4000000000000000ull);
    assert((cpu.rflags() & CPU::OverflowFlag) != 0);
    assert((cpu.rflags() & CPU::CarryFlag) != 0);

    cpu.setRegister(CPU::RAX, 0x8000000000000000ull);
    cpu.setRip(0x2060);
    assert(memory.write8(0x2060, 0x48));
    assert(memory.write8(0x2061, 0xC1));
    assert(memory.write8(0x2062, 0xF8));
    assert(memory.write8(0x2063, 0x01)); // SAR RAX,1: OF must be 0
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xC000000000000000ull);
    assert((cpu.rflags() & CPU::OverflowFlag) == 0);
    assert((cpu.rflags() & CPU::CarryFlag) != 0);

    cpu.setRegister(CPU::RAX, 0x123456789ABCDEF0ull);
    cpu.setRegister(CPU::RCX, 0);
    cpu.setRip(0x2070);
    assert(memory.write8(0x2070, 0x48));
    assert(memory.write8(0x2071, 0xC1));
    assert(memory.write8(0x2072, 0xE0));
    assert(memory.write8(0x2073, 0x00)); // SHL RAX,0
    cpu.setRegister(CPU::RCX, 0x1234);
    const auto flagsBeforeShift0 = cpu.rflags();
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x123456789ABCDEF0ull);
    assert(cpu.rflags() == flagsBeforeShift0);

    // Count from CL is masked to six bits.
    cpu.setRegister(CPU::RAX, 1);
    cpu.setRegister(CPU::RCX, 65);
    cpu.setRip(0x2080);
    assert(memory.write8(0x2080, 0x48));
    assert(memory.write8(0x2081, 0xD3));
    assert(memory.write8(0x2082, 0xE0)); // SHL RAX,CL -> effective count 1
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 2);

    // All implemented short Jcc conditions are exercised with matching flags.
    struct BranchCase { std::uint8_t opcode; std::uint64_t flags; bool take; };
    const BranchCase branches[] = {
        {0x70, CPU::OverflowFlag, true}, {0x71, CPU::OverflowFlag, false},
        {0x72, CPU::CarryFlag, true}, {0x73, CPU::CarryFlag, false},
        {0x74, CPU::ZeroFlag, true}, {0x75, CPU::ZeroFlag, false},
        {0x76, CPU::CarryFlag, true}, {0x77, CPU::CarryFlag, false},
        {0x78, CPU::SignFlag, true}, {0x79, CPU::SignFlag, false},
        {0x7C, CPU::SignFlag, true}, {0x7D, CPU::SignFlag, false},
        {0x7E, CPU::ZeroFlag, true}, {0x7F, CPU::ZeroFlag, false}
    };
    for (const auto& branch : branches) {
        cpu.setRip(0x2100);
        cpu.setRegister(CPU::RAX, 0);
        cpu.setRegister(CPU::RCX, 0);
        assert(memory.write8(0x2100, branch.opcode));
        assert(memory.write8(0x2101, 0x02));
        cpu.setRegister(CPU::RDX, branch.flags);
        // Establish the requested flags using the CPU's public state indirectly:
        // use arithmetic for the common cases below; then overwrite only when zero.
        if (branch.flags == CPU::OverflowFlag) {
            cpu.setRegister(CPU::RAX, 0x7FFFFFFFFFFFFFFFull);
            cpu.setRegister(CPU::RCX, 1);
            cpu.setRip(0x2110);
            assert(memory.write8(0x2110, 0x48));
            assert(memory.write8(0x2111, 0x01));
            assert(memory.write8(0x2112, 0xC8));
            assert(cpu.step());
            cpu.setRip(0x2100);
        } else if (branch.flags == CPU::CarryFlag) {
            cpu.setRegister(CPU::RAX, 0);
            cpu.setRegister(CPU::RCX, 1);
            cpu.setRip(0x2110);
            assert(memory.write8(0x2110, 0x48));
            assert(memory.write8(0x2111, 0x39));
            assert(memory.write8(0x2112, 0xC8));
            assert(cpu.step());
            cpu.setRip(0x2100);
        } else if (branch.flags == CPU::ZeroFlag) {
            cpu.setRegister(CPU::RAX, 1);
            cpu.setRegister(CPU::RCX, 1);
            cpu.setRip(0x2110);
            assert(memory.write8(0x2110, 0x48));
            assert(memory.write8(0x2111, 0x39));
            assert(memory.write8(0x2112, 0xC8));
            assert(cpu.step());
            cpu.setRip(0x2100);
        } else {
            cpu.setRegister(CPU::RAX, 0);
            cpu.setRegister(CPU::RCX, 1);
            cpu.setRip(0x2110);
            assert(memory.write8(0x2110, 0x48));
            assert(memory.write8(0x2111, 0x29));
            assert(memory.write8(0x2112, 0xC8));
            assert(cpu.step());
            cpu.setRip(0x2100);
        }
        assert(cpu.step());
        const auto expectedRip = branch.take ? 0x2104ull : 0x2102ull;
        assert(cpu.rip() == expectedRip);
    }

    // SIB addressing: base + index*scale and signed displacement.
    cpu.setRegister(CPU::RBX, 0x8000);
    cpu.setRegister(CPU::RCX, 2);
    assert(memory.write64(0x8008, 0xAABBCCDDEEFF0011ull));
    cpu.setRip(0x2200);
    assert(memory.write8(0x2200, 0x48));
    assert(memory.write8(0x2201, 0x8B));
    assert(memory.write8(0x2202, 0x04));
    assert(memory.write8(0x2203, 0x8B)); // [RBX + RCX*4]
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xAABBCCDDEEFF0011ull);

    assert(memory.write64(0x7FFE, 0x1122334455667788ull));
    cpu.setRip(0x2210);
    assert(memory.write8(0x2210, 0x48));
    assert(memory.write8(0x2211, 0x8B));
    assert(memory.write8(0x2212, 0x43));
    assert(memory.write8(0x2213, 0xFE)); // [RBX-2]
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x1122334455667788ull);

    std::cout << "CPU instruction tests passed.\n";
    return 0;
}
