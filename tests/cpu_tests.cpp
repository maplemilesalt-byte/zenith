#include "cpu/cpu.h"
#include "memory/guest_memory.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using P = GuestMemory::Permissions;
    GuestMemory memory(64ull * 1024ull * 1024ull);
    assert(memory.map(0x1000, 0, GuestMemory::PageSize, P::Read | P::Write | P::Execute));
    assert(memory.map(0x8000, 0x4000, GuestMemory::PageSize * 2, P::Read | P::Write));

    CPU cpu(memory);
    cpu.setRip(0x1000);

    assert(memory.write8(0x1000, 0xB8));
    assert(memory.write32(0x1001, 42));
    assert(memory.write8(0x1005, 0xB9));
    assert(memory.write32(0x1006, 8));
    assert(memory.write8(0x100A, 0x48));
    assert(memory.write8(0x100B, 0x01));
    assert(memory.write8(0x100C, 0xC8));
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

    cpu.setRip(0x1010);
    assert(memory.write8(0x1010, 0x48));
    assert(memory.write8(0x1011, 0xB8));
    assert(memory.write64(0x1012, 0x123456789ABCDEF0ull));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x123456789ABCDEF0ull);

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

    cpu.setRegister(CPU::RSP, 0x8010);
    cpu.setRegister(CPU::RAX, 0x1122334455667788ull);
    cpu.setRip(0x1030);
    assert(memory.write8(0x1030, 0x50));
    assert(memory.write8(0x1031, 0x59));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RSP) == 0x8008);
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RCX) == 0x1122334455667788ull);
    assert(cpu.getRegister(CPU::RSP) == 0x8010);

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

    assert(memory.write64(0x8000, 0x1122334455667788ull));
    cpu.setRegister(CPU::RBX, 0x8000);
    cpu.setRip(0x1060);
    assert(memory.write8(0x1060, 0x48));
    assert(memory.write8(0x1061, 0x8B));
    assert(memory.write8(0x1062, 0x03));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x1122334455667788ull);

    cpu.setRegister(CPU::RAX, 0xAABBCCDDEEFF0011ull);
    cpu.setRip(0x1063);
    assert(memory.write8(0x1063, 0x48));
    assert(memory.write8(0x1064, 0x89));
    assert(memory.write8(0x1065, 0x43));
    assert(memory.write8(0x1066, 0x08));
    assert(cpu.step());
    std::uint64_t stored = 0;
    assert(memory.read64(0x8008, stored));
    assert(stored == 0xAABBCCDDEEFF0011ull);

    assert(memory.write64(0x8010, 0xCAFEBABE12345678ull));
    cpu.setRip(0x1070);
    assert(memory.write8(0x1070, 0x48));
    assert(memory.write8(0x1071, 0x8B));
    assert(memory.write8(0x1072, 0x0D));
    assert(memory.write32(0x1073, static_cast<std::uint32_t>(0x8010 - 0x1077)));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RCX) == 0xCAFEBABE12345678ull);

    cpu.setRip(0x1080);
    assert(memory.write8(0x1080, 0x49));
    assert(memory.write8(0x1081, 0xB8));
    assert(memory.write64(0x1082, 0x0102030405060708ull));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::R8) == 0x0102030405060708ull);
    assert(memory.write8(0x108A, 0x4C));
    assert(memory.write8(0x108B, 0x89));
    assert(memory.write8(0x108C, 0x03));
    assert(cpu.step());
    assert(memory.read64(0x8000, stored));
    assert(stored == 0x0102030405060708ull);

    assert(memory.write64(0x8028, 0xDEADBEEFCAFEBABEull));
    cpu.setRegister(CPU::RBX, 0x8000);
    cpu.setRegister(CPU::RCX, 8);
    cpu.setRip(0x1090);
    assert(memory.write8(0x1090, 0x48));
    assert(memory.write8(0x1091, 0x8B));
    assert(memory.write8(0x1092, 0x44));
    assert(memory.write8(0x1093, 0x8B));
    assert(memory.write8(0x1094, 0x08));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xDEADBEEFCAFEBABEull);

    cpu.setRegister(CPU::R12, 0x8040);
    cpu.setRegister(CPU::R13, 4);
    cpu.setRegister(CPU::RAX, 0x1122334455667788ull);
    cpu.setRip(0x1095);
    assert(memory.write8(0x1095, 0x4B));
    assert(memory.write8(0x1096, 0x89));
    assert(memory.write8(0x1097, 0x04));
    assert(memory.write8(0x1098, 0x6C));
    assert(memory.write64(0x8048, 0));
    assert(cpu.step());
    assert(memory.read64(0x8048, stored));
    assert(stored == 0x1122334455667788ull);

    assert(memory.write64(0x8060, 0xA1B2C3D4E5F60718ull));
    cpu.setRegister(CPU::RCX, 4);
    cpu.setRip(0x1099);
    assert(memory.write8(0x1099, 0x48));
    assert(memory.write8(0x109A, 0x8B));
    assert(memory.write8(0x109B, 0x04));
    assert(memory.write8(0x109C, 0xCD));
    assert(memory.write32(0x109D, 0x8040));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xA1B2C3D4E5F60718ull);

    // LEA: RAX = RBX + RCX*4 + 8. It must not read the target memory.
    cpu.setRegister(CPU::RBX, 0x9000);
    cpu.setRegister(CPU::RCX, 3);
    cpu.setRip(0x10A0);
    assert(memory.write8(0x10A0, 0x48));
    assert(memory.write8(0x10A1, 0x8D));
    assert(memory.write8(0x10A2, 0x44));
    assert(memory.write8(0x10A3, 0x8B));
    assert(memory.write8(0x10A4, 0x08));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x9014);

    // REX.B on PUSH/POP: move R8 through the extended opcode register.
    cpu.setRegister(CPU::R8, 0x8877665544332211ull);
    cpu.setRegister(CPU::RSP, 0x8010);
    cpu.setRip(0x10A5);
    assert(memory.write8(0x10A5, 0x41));
    assert(memory.write8(0x10A6, 0x50));
    assert(memory.write8(0x10A7, 0x41));
    assert(memory.write8(0x10A8, 0x58));
    assert(cpu.step());
    assert(cpu.step());
    assert(cpu.getRegister(CPU::R8) == 0x8877665544332211ull);
    assert(cpu.getRegister(CPU::RSP) == 0x8010);

    // Multiple REX prefixes: the final REX prefix supplies the effective bits.
    cpu.setRip(0x10B0);
    assert(memory.write8(0x10B0, 0x40));
    assert(memory.write8(0x10B1, 0x49));
    assert(memory.write8(0x10B2, 0xB8));
    assert(memory.write64(0x10B3, 0xFEEDFACECAFED00Dull));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::R8) == 0xFEEDFACECAFED00Dull);

    cpu.setRip(0x10C0);
    assert(memory.write8(0x10C0, 0xCC));
    assert(!cpu.step());
    assert(cpu.lastError() != nullptr);

    std::cout << "CPU tests passed.\n";
    return 0;
}
