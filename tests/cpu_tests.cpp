#include "cpu/cpu.h"
#include "memory/guest_memory.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using P = GuestMemory::Permissions;
    GuestMemory memory(64ull * 1024ull * 1024ull);
    assert(memory.map(0x1000, 0, GuestMemory::PageSize * 8, P::Read | P::Write | P::Execute));
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

    cpu.setRip(0x10B0);
    assert(memory.write8(0x10B0, 0x40));
    assert(memory.write8(0x10B1, 0x49));
    assert(memory.write8(0x10B2, 0xB8));
    assert(memory.write64(0x10B3, 0xFEEDFACECAFED00Dull));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::R8) == 0xFEEDFACECAFED00Dull);

    cpu.setRegister(CPU::RAX, 0xFF00ull);
    cpu.setRegister(CPU::RCX, 0x0F0Full);
    cpu.setRip(0x10D0);
    assert(memory.write8(0x10D0, 0x48));
    assert(memory.write8(0x10D1, 0x21));
    assert(memory.write8(0x10D2, 0xC8));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x0F00ull);
    assert((cpu.rflags() & (CPU::CarryFlag | CPU::OverflowFlag | CPU::ZeroFlag | CPU::SignFlag)) == 0);

    cpu.setRegister(CPU::RAX, 0x8000000000000000ull);
    cpu.setRegister(CPU::RCX, 1ull);
    cpu.setRip(0x10E0);
    assert(memory.write8(0x10E0, 0x48));
    assert(memory.write8(0x10E1, 0x09));
    assert(memory.write8(0x10E2, 0xC8));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x8000000000000001ull);
    assert((cpu.rflags() & CPU::SignFlag) != 0);
    assert((cpu.rflags() & (CPU::CarryFlag | CPU::OverflowFlag | CPU::ZeroFlag)) == 0);

    cpu.setRegister(CPU::RAX, 0x123456789ABCDEF0ull);
    cpu.setRegister(CPU::RCX, 0x123456789ABCDEF0ull);
    cpu.setRip(0x10F0);
    assert(memory.write8(0x10F0, 0x48));
    assert(memory.write8(0x10F1, 0x31));
    assert(memory.write8(0x10F2, 0xC8));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0);
    assert((cpu.rflags() & CPU::ZeroFlag) != 0);
    assert((cpu.rflags() & (CPU::CarryFlag | CPU::SignFlag | CPU::OverflowFlag)) == 0);

    cpu.setRegister(CPU::RAX, 0x8000000000000000ull);
    cpu.setRegister(CPU::RCX, 0x8000000000000000ull);
    cpu.setRip(0x1100);
    assert(memory.write8(0x1100, 0x48));
    assert(memory.write8(0x1101, 0x85));
    assert(memory.write8(0x1102, 0xC8));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x8000000000000000ull);
    assert((cpu.rflags() & CPU::SignFlag) != 0);
    assert((cpu.rflags() & (CPU::CarryFlag | CPU::OverflowFlag | CPU::ZeroFlag)) == 0);

    cpu.setRegister(CPU::RAX, 0x10ull);
    cpu.setRegister(CPU::RCX, 0x01ull);
    cpu.setRip(0x1110);
    assert(memory.write8(0x1110, 0x48));
    assert(memory.write8(0x1111, 0x85));
    assert(memory.write8(0x1112, 0xC8));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x10ull);
    assert(cpu.getRegister(CPU::RCX) == 0x01ull);
    assert((cpu.rflags() & CPU::ZeroFlag) != 0);
    assert((cpu.rflags() & (CPU::CarryFlag | CPU::SignFlag | CPU::OverflowFlag)) == 0);

    // INC/DEC preserve CF while updating the other arithmetic flags.
    cpu.setRegister(CPU::RAX, 0x7FFFFFFFFFFFFFFFull);
    cpu.setRip(0x1120);
    cpu.setRegister(CPU::RDX, 0);
    assert(memory.write8(0x1120, 0x48));
    assert(memory.write8(0x1121, 0xFF));
    assert(memory.write8(0x1122, 0xC0));
    cpu.setRip(0x1120);
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0x8000000000000000ull);
    assert((cpu.rflags() & CPU::OverflowFlag) != 0);

    cpu.setRegister(CPU::RAX, 0);
    cpu.setRegister(CPU::RCX, CPU::CarryFlag);
    cpu.setRip(0x1123);
    assert(memory.write8(0x1123, 0x48));
    assert(memory.write8(0x1124, 0xFF));
    assert(memory.write8(0x1125, 0xC8));
    cpu.setRip(0x1123);
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xFFFFFFFFFFFFFFFFull);

    // NOT is non-destructive to flags; NEG computes 0-x.
    cpu.setRegister(CPU::RAX, 0x0F0F0F0F0F0F0F0Full);
    const auto savedFlags = cpu.rflags();
    cpu.setRip(0x1130);
    assert(memory.write8(0x1130, 0x48));
    assert(memory.write8(0x1131, 0xF7));
    assert(memory.write8(0x1132, 0xD0));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xF0F0F0F0F0F0F0F0ull);
    assert(cpu.rflags() == savedFlags);

    cpu.setRegister(CPU::RAX, 5);
    cpu.setRip(0x1133);
    assert(memory.write8(0x1133, 0x48));
    assert(memory.write8(0x1134, 0xF7));
    assert(memory.write8(0x1135, 0xD8));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xFFFFFFFFFFFFFFFBull);
    assert((cpu.rflags() & CPU::CarryFlag) != 0);

    // ADC/SBB consume CF.
    cpu.setRegister(CPU::RAX, 10);
    cpu.setRegister(CPU::RCX, 5);
    cpu.setRip(0x1140);
    assert(memory.write8(0x1140, 0x48));
    assert(memory.write8(0x1141, 0x11));
    assert(memory.write8(0x1142, 0xC8));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 15);
    cpu.setRegister(CPU::RAX, 10);
    cpu.setRegister(CPU::RCX, 5);
    cpu.setRip(0x1140);
    cpu.setRegister(CPU::RDX, 0);
    cpu.setRip(0x1143);
    assert(memory.write8(0x1143, 0x48));
    assert(memory.write8(0x1144, 0x19));
    assert(memory.write8(0x1145, 0xC8));
    cpu.setRip(0x1143);
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 5);

    // Short conditional branches cover signed and unsigned flag combinations.
    cpu.setRip(0x1150);
    cpu.setRegister(CPU::RAX, 1);
    cpu.setRegister(CPU::RCX, 2);
    assert(memory.write8(0x1150, 0x48));
    assert(memory.write8(0x1151, 0x39));
    assert(memory.write8(0x1152, 0xC8));
    assert(memory.write8(0x1153, 0x72));
    assert(memory.write8(0x1154, 0x02));
    assert(memory.write8(0x1155, 0x90));
    assert(memory.write8(0x1156, 0x90));
    assert(cpu.step());
    assert(cpu.step());
    assert(cpu.rip() == 0x1157);

    // SHL/SHR/SAR update CF/ZF/SF and implement count=1 OF semantics.
    cpu.setRegister(CPU::RAX, 1);
    cpu.setRip(0x1160);
    assert(memory.write8(0x1160, 0x48));
    assert(memory.write8(0x1161, 0xC1));
    assert(memory.write8(0x1162, 0xE0));
    assert(memory.write8(0x1163, 0x03));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 8);

    cpu.setRegister(CPU::RAX, 8);
    cpu.setRip(0x1164);
    assert(memory.write8(0x1164, 0x48));
    assert(memory.write8(0x1165, 0xC1));
    assert(memory.write8(0x1166, 0xE8));
    assert(memory.write8(0x1167, 0x03));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 1);

    cpu.setRegister(CPU::RAX, 0x8000000000000000ull);
    cpu.setRip(0x1168);
    assert(memory.write8(0x1168, 0x48));
    assert(memory.write8(0x1169, 0xC1));
    assert(memory.write8(0x116A, 0xF8));
    assert(memory.write8(0x116B, 0x01));
    assert(cpu.step());
    assert(cpu.getRegister(CPU::RAX) == 0xC000000000000000ull);

    cpu.setRip(0x10C0);
    assert(memory.write8(0x10C0, 0xCC));
    assert(!cpu.step());
    assert(cpu.lastError() != nullptr);

    std::cout << "CPU tests passed.\n";
    return 0;
}
