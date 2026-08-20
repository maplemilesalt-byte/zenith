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
    assert(memory.write8(0x2002, 0xC8));
    assert(cpu.step());
    assert((cpu.rflags() & CPU::CarryFlag) != 0);
    assert(memory.write8(0x2003, 0x48));
    assert(memory.write8(0x2004, 0xFF));
    assert(memory.write8(0x2005, 0xC0));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 1);
    assert((cpu.rflags() & CPU::CarryFlag) != 0);

    cpu.setRegister(CPU::RAX, 0x8000000000000000ull);
    cpu.setRegister(CPU::RCX, 1);
    cpu.setRip(0x2010);
    assert(memory.write8(0x2010, 0x48));
    assert(memory.write8(0x2011, 0x39));
    assert(memory.write8(0x2012, 0xC8));
    assert(cpu.step());
    assert(memory.write8(0x2013, 0x48));
    assert(memory.write8(0x2014, 0xFF));
    assert(memory.write8(0x2015, 0xC8));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x7FFFFFFFFFFFFFFFull);
    assert((cpu.rflags() & CPU::CarryFlag) == 0);

    // NOT must not change flags; NEG is 0-x and sets CF iff x != 0.
    cpu.setRegister(CPU::RAX, 0);
    cpu.setRegister(CPU::RCX, 1);
    cpu.setRip(0x2020);
    assert(memory.write8(0x2020, 0x48));
    assert(memory.write8(0x2021, 0x39));
    assert(memory.write8(0x2022, 0xC8));
    assert(cpu.step());
    const auto flagsBeforeNot = cpu.rflags();
    assert(memory.write8(0x2023, 0x48));
    assert(memory.write8(0x2024, 0xF7));
    assert(memory.write8(0x2025, 0xD0));
    cpu.setRegister(CPU::RAX, 0x0F0F0F0F0F0F0F0Full);
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xF0F0F0F0F0F0F0F0ull);
    assert(cpu.rflags() == flagsBeforeNot);

    cpu.setRegister(CPU::RAX, 5);
    cpu.setRip(0x2030);
    assert(memory.write8(0x2030, 0x48));
    assert(memory.write8(0x2031, 0xF7));
    assert(memory.write8(0x2032, 0xD8));
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
    assert(memory.write8(0x2043, 0x01));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x8000000000000000ull);
    assert((cpu.rflags() & CPU::OverflowFlag) != 0);
    assert((cpu.rflags() & CPU::CarryFlag) == 0);

    cpu.setRegister(CPU::RAX, 0x8000000000000000ull);
    cpu.setRip(0x2050);
    assert(memory.write8(0x2050, 0x48));
    assert(memory.write8(0x2051, 0xC1));
    assert(memory.write8(0x2052, 0xE8));
    assert(memory.write8(0x2053, 0x01));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x4000000000000000ull);
    assert((cpu.rflags() & CPU::OverflowFlag) != 0);
    assert((cpu.rflags() & CPU::CarryFlag) != 0);

    cpu.setRegister(CPU::RAX, 0x8000000000000000ull);
    cpu.setRip(0x2060);
    assert(memory.write8(0x2060, 0x48));
    assert(memory.write8(0x2061, 0xC1));
    assert(memory.write8(0x2062, 0xF8));
    assert(memory.write8(0x2063, 0x01));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xC000000000000000ull);
    assert((cpu.rflags() & CPU::OverflowFlag) == 0);
    assert((cpu.rflags() & CPU::CarryFlag) != 0);

    cpu.setRegister(CPU::RAX, 0x123456789ABCDEF0ull);
    cpu.setRip(0x2070);
    assert(memory.write8(0x2070, 0x48));
    assert(memory.write8(0x2071, 0xC1));
    assert(memory.write8(0x2072, 0xE0));
    assert(memory.write8(0x2073, 0x00));
    const auto flagsBeforeShift0 = cpu.rflags();
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x123456789ABCDEF0ull);
    assert(cpu.rflags() == flagsBeforeShift0);

    cpu.setRegister(CPU::RAX, 1);
    cpu.setRegister(CPU::RCX, 65);
    cpu.setRip(0x2080);
    assert(memory.write8(0x2080, 0x48));
    assert(memory.write8(0x2081, 0xD3));
    assert(memory.write8(0x2082, 0xE0));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 2);

    // All implemented short Jcc conditions.
    auto testBranch = [&](std::uint8_t opcode, std::uint64_t lhs, std::uint64_t rhs,
                          std::uint8_t compareOpcode, bool take) {
        cpu.setRegister(CPU::RAX, lhs);
        cpu.setRegister(CPU::RCX, rhs);
        cpu.setRip(0x2100);
        assert(memory.write8(0x2100, 0x48));
        assert(memory.write8(0x2101, compareOpcode));
        assert(memory.write8(0x2102, 0xC8));
        assert(cpu.step());
        assert(memory.write8(0x2103, opcode));
        assert(memory.write8(0x2104, 0x02));
        assert(cpu.step());
        assert(cpu.rip() == (take ? 0x2107ull : 0x2105ull));
    };

    cpu.setRegister(CPU::RAX, 0x7FFFFFFFFFFFFFFFull);
    cpu.setRegister(CPU::RCX, 1);
    cpu.setRip(0x2110);
    assert(memory.write8(0x2110, 0x48));
    assert(memory.write8(0x2111, 0x01));
    assert(memory.write8(0x2112, 0xC8));
    assert(cpu.step());
    cpu.setRip(0x2103);
    assert(memory.write8(0x2103, 0x70));
    assert(memory.write8(0x2104, 0x02));
    assert(cpu.step());
    assert(cpu.rip() == 0x2107);
    cpu.setRip(0x2103);
    assert(memory.write8(0x2103, 0x71));
    assert(memory.write8(0x2104, 0x02));
    assert(cpu.step());
    assert(cpu.rip() == 0x2105);

    testBranch(0x72, 0, 1, 0x39, true);
    testBranch(0x73, 0, 1, 0x39, false);
    testBranch(0x74, 5, 5, 0x39, true);
    testBranch(0x75, 5, 4, 0x39, true);
    testBranch(0x76, 0, 1, 0x39, true);
    testBranch(0x77, 2, 1, 0x39, true);
    testBranch(0x78, 0, 1, 0x29, true);
    testBranch(0x79, 2, 1, 0x29, true);
    testBranch(0x7C, 0xFFFFFFFFFFFFFFFFull, 1, 0x39, true);
    testBranch(0x7D, 0xFFFFFFFFFFFFFFFFull, 1, 0x39, false);
    testBranch(0x7E, 0xFFFFFFFFFFFFFFFFull, 1, 0x39, true);
    testBranch(0x7F, 0xFFFFFFFFFFFFFFFFull, 1, 0x39, false);

    // SIB addressing: base + index*scale and signed displacement.
    cpu.setRegister(CPU::RBX, 0x8000);
    cpu.setRegister(CPU::RCX, 2);
    assert(memory.write64(0x8008, 0xAABBCCDDEEFF0011ull));
    cpu.setRip(0x2200);
    assert(memory.write8(0x2200, 0x48));
    assert(memory.write8(0x2201, 0x8B));
    assert(memory.write8(0x2202, 0x04));
    assert(memory.write8(0x2203, 0x8B));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xAABBCCDDEEFF0011ull);

    cpu.setRegister(CPU::RBX, 0x8008);
    assert(memory.write64(0x8006, 0x1122334455667788ull));
    cpu.setRip(0x2210);
    assert(memory.write8(0x2210, 0x48));
    assert(memory.write8(0x2211, 0x8B));
    assert(memory.write8(0x2212, 0x43));
    assert(memory.write8(0x2213, 0xFE));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x1122334455667788ull);

    // MOVZX/MOVSX register operands, byte and word widths.
    cpu.setRegister(CPU::RAX, 0xFFFFFFFFFFFFFF80ull);
    cpu.setRip(0x2300);
    assert(memory.write8(0x2300, 0x48));
    assert(memory.write8(0x2301, 0x0F));
    assert(memory.write8(0x2302, 0xB6));
    assert(memory.write8(0x2303, 0xC8)); // MOVZX RCX, AL
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RCX) == 0x80ull);

    cpu.setRip(0x2304);
    assert(memory.write8(0x2304, 0x48));
    assert(memory.write8(0x2305, 0x0F));
    assert(memory.write8(0x2306, 0xBE));
    assert(memory.write8(0x2307, 0xD0)); // MOVSX RDX, AL
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RDX) == 0xFFFFFFFFFFFFFF80ull);

    cpu.setRegister(CPU::RCX, 0xFFFFFFFFFFFFFFFFull);
    cpu.setRip(0x2308);
    assert(memory.write8(0x2308, 0x4C));
    assert(memory.write8(0x2309, 0x0F));
    assert(memory.write8(0x230A, 0xB7));
    assert(memory.write8(0x230B, 0xC1)); // MOVZX R8, CX
    assert(cpu.step());
    assert(cpu.getRegister(CPU::R8) == 0xFFFFull);

    cpu.setRip(0x230C);
    assert(memory.write8(0x230C, 0x4C));
    assert(memory.write8(0x230D, 0x0F));
    assert(memory.write8(0x230E, 0xBF));
    assert(memory.write8(0x230F, 0xD1)); // MOVSX R10, CX
    assert(cpu.step());
    assert(cpu.getRegister(CPU::R10) == 0xFFFFFFFFFFFFFFFFull);

    // MOVZX/MOVSX from memory must use the full ModRM/SIB address decoder.
    cpu.setRegister(CPU::RBX, 0x8000);
    cpu.setRegister(CPU::RCX, 2);
    assert(memory.write8(0x8008, 0xFE));
    cpu.setRip(0x2310);
    assert(memory.write8(0x2310, 0x48));
    assert(memory.write8(0x2311, 0x0F));
    assert(memory.write8(0x2312, 0xB6));
    assert(memory.write8(0x2313, 0x44));
    assert(memory.write8(0x2314, 0x8B));
    assert(memory.write8(0x2315, 0x00)); // [RBX + RCX*4]
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xFEull);

    assert(memory.write16(0x8006, 0xFF80));
    cpu.setRip(0x2320);
    assert(memory.write8(0x2320, 0x48));
    assert(memory.write8(0x2321, 0x0F));
    assert(memory.write8(0x2322, 0xBF));
    assert(memory.write8(0x2323, 0x43));
    assert(memory.write8(0x2324, 0xFE)); // [RBX-2]
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xFFFFFFFFFFFFFF80ull);

    // CMP must not modify operands and must produce signed overflow correctly.
    cpu.setRegister(CPU::RAX, 0x7FFFFFFFFFFFFFFFull);
    cpu.setRegister(CPU::RCX, 0xFFFFFFFFFFFFFFFFull);
    cpu.setRip(0x2330);
    assert(memory.write8(0x2330, 0x48));
    assert(memory.write8(0x2331, 0x39));
    assert(memory.write8(0x2332, 0xC8));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x7FFFFFFFFFFFFFFFull);
    assert(cpu.getRegister(CPU::RCX) == 0xFFFFFFFFFFFFFFFFull);
    assert((cpu.rflags() & CPU::OverflowFlag) != 0);
    assert((cpu.rflags() & CPU::SignFlag) != 0);
    assert((cpu.rflags() & CPU::ZeroFlag) == 0);

    // SIB with REX.X/REX.B and no base (base=5, mod=00).
    cpu.setRegister(CPU::R13, 1);
    assert(memory.write64(0x4010, 0x123456789ABCDEF0ull));
    cpu.setRip(0x2340);
    assert(memory.write8(0x2340, 0x4A)); // REX.W + X
    assert(memory.write8(0x2341, 0x8B));
    assert(memory.write8(0x2342, 0x04));
    assert(memory.write8(0x2343, 0xAD)); // [R13*4 + disp32]
    assert(memory.write32(0x2344, 0x400C));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x123456789ABCDEF0ull);

    std::cout << "CPU instruction tests passed.\n";
    return 0;
}
