#include "cpu.h"
#include "memory/guest_memory.h"

#include <cstdint>
#include <iostream>

CPU::CPU(GuestMemory& memory) : memory_(memory) {
    reset();
}

void CPU::reset() {
    registers_.fill(0);
    rip_ = 0;
    rflags_ = 0;
    lastError_ = nullptr;
    halted_ = false;
    exitCode_ = 0;
}

std::uint64_t CPU::getRegister(Register reg) const { return registers_[static_cast<std::size_t>(reg)]; }
void CPU::setRegister(Register reg, std::uint64_t value) { registers_[static_cast<std::size_t>(reg)] = value; }
std::uint64_t CPU::rip() const { return rip_; }
void CPU::setRip(std::uint64_t value) { rip_ = value; }
std::uint64_t CPU::rflags() const { return rflags_; }
const char* CPU::lastError() const { return lastError_; }
bool CPU::halted() const { return halted_; }
std::int64_t CPU::exitCode() const { return exitCode_; }

bool CPU::fail(const char* message) { lastError_ = message; return false; }

bool CPU::fetch8(std::uint8_t& value) {
    if (!memory_.read8(rip_, value)) return fail("instruction fetch failed");
    ++rip_;
    return true;
}

bool CPU::fetch32(std::uint32_t& value) {
    if (!memory_.read32(rip_, value)) return fail("instruction fetch failed");
    rip_ += 4;
    return true;
}

bool CPU::fetch64(std::uint64_t& value) {
    if (!memory_.read64(rip_, value)) return fail("instruction fetch failed");
    rip_ += 8;
    return true;
}

bool CPU::decodeModRM(std::uint8_t value, ModRM& modrm) const {
    modrm.mod = (value >> 6) & 0x3;
    modrm.reg = (value >> 3) & 0x7;
    modrm.rm = value & 0x7;
    return true;
}

std::size_t CPU::extendedRegister(std::uint8_t index, std::uint8_t rexBit) const {
    return static_cast<std::size_t>(index) + (((rexBit != 0) ? 1u : 0u) << 3);
}

bool CPU::resolveModRMAddress(const ModRM& modrm, std::uint8_t rex, std::uint64_t& address) {
    if (modrm.mod == 3) return fail("ModRM operand is a register, not memory");

    if (modrm.rm == 4) {
        std::uint8_t rawSIB = 0;
        if (!fetch8(rawSIB)) return false;
        const std::uint8_t scaleBits = (rawSIB >> 6) & 0x3;
        const std::uint8_t indexBits = (rawSIB >> 3) & 0x7;
        const std::uint8_t baseBits = rawSIB & 0x7;
        const std::uint64_t scale = 1ull << scaleBits;
        const bool hasIndex = !(indexBits == 4 && (rex & 0x2) == 0);
        const auto indexReg = extendedRegister(indexBits, (rex >> 1) & 0x1);

        std::int64_t displacement = 0;
        bool hasBase = true;
        if (modrm.mod == 0 && baseBits == 5) {
            hasBase = false;
            std::uint32_t raw = 0;
            if (!fetch32(raw)) return false;
            displacement = static_cast<std::int32_t>(raw);
        } else if (modrm.mod == 1) {
            std::uint8_t raw = 0;
            if (!fetch8(raw)) return false;
            displacement = static_cast<std::int8_t>(raw);
        } else if (modrm.mod == 2) {
            std::uint32_t raw = 0;
            if (!fetch32(raw)) return false;
            displacement = static_cast<std::int32_t>(raw);
        }

        std::int64_t effective = displacement;
        if (hasBase) {
            const auto baseReg = extendedRegister(baseBits, rex & 0x1);
            effective += static_cast<std::int64_t>(registers_[baseReg]);
        }
        if (hasIndex) effective += static_cast<std::int64_t>(registers_[indexReg] * scale);
        address = static_cast<std::uint64_t>(effective);
        return true;
    }

    std::int64_t displacement = 0;
    if (modrm.mod == 0) {
        if (modrm.rm == 5) {
            std::uint32_t raw = 0;
            if (!fetch32(raw)) return false;
            displacement = static_cast<std::int32_t>(raw);
            address = static_cast<std::uint64_t>(static_cast<std::int64_t>(rip_) + displacement);
            return true;
        }
    } else if (modrm.mod == 1) {
        std::uint8_t raw = 0;
        if (!fetch8(raw)) return false;
        displacement = static_cast<std::int8_t>(raw);
    } else if (modrm.mod == 2) {
        std::uint32_t raw = 0;
        if (!fetch32(raw)) return false;
        displacement = static_cast<std::int32_t>(raw);
    }

    const auto baseReg = extendedRegister(modrm.rm, rex & 0x1);
    address = static_cast<std::uint64_t>(static_cast<std::int64_t>(registers_[baseReg]) + displacement);
    return true;
}

bool CPU::readModRMR64(const ModRM& modrm, std::uint8_t rex, std::uint64_t& value) {
    if (modrm.mod == 3) {
        value = registers_[extendedRegister(modrm.rm, rex & 0x1)];
        return true;
    }
    std::uint64_t address = 0;
    if (!resolveModRMAddress(modrm, rex, address)) return false;
    if (!memory_.read64(address, value)) return fail("64-bit memory read failed");
    return true;
}

bool CPU::writeModRMR64(const ModRM& modrm, std::uint8_t rex, std::uint64_t value) {
    if (modrm.mod == 3) {
        registers_[extendedRegister(modrm.rm, rex & 0x1)] = value;
        return true;
    }
    std::uint64_t address = 0;
    if (!resolveModRMAddress(modrm, rex, address)) return false;
    if (!memory_.write64(address, value)) return fail("64-bit memory write failed");
    return true;
}

void CPU::updateAddFlags(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t result) {
    rflags_ &= ~(CarryFlag | ZeroFlag | SignFlag | OverflowFlag);
    if (result < lhs) rflags_ |= CarryFlag;
    if (result == 0) rflags_ |= ZeroFlag;
    if (result & (1ull << 63)) rflags_ |= SignFlag;
    const bool lhsSign = (lhs >> 63) != 0;
    const bool rhsSign = (rhs >> 63) != 0;
    const bool resultSign = (result >> 63) != 0;
    if (lhsSign == rhsSign && lhsSign != resultSign) rflags_ |= OverflowFlag;
}

void CPU::updateSubFlags(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t result) {
    rflags_ &= ~(CarryFlag | ZeroFlag | SignFlag | OverflowFlag);
    if (lhs < rhs) rflags_ |= CarryFlag;
    if (result == 0) rflags_ |= ZeroFlag;
    if (result & (1ull << 63)) rflags_ |= SignFlag;
    const bool lhsSign = (lhs >> 63) != 0;
    const bool rhsSign = (rhs >> 63) != 0;
    const bool resultSign = (result >> 63) != 0;
    if (lhsSign != rhsSign && lhsSign != resultSign) rflags_ |= OverflowFlag;
}

bool CPU::handleSyscall() {
    // Zenith currently exposes a tiny Linux-like console ABI to guest ELFs.
    // syscall 1 = write(fd, buffer, length), syscall 60 = exit(code).
    const auto number = registers_[RAX];

    if (number == 1) {
        const auto fd = registers_[RDI];
        const auto address = registers_[RSI];
        const auto length = registers_[RDX];
        if (fd != 1 && fd != 2) {
            registers_[RAX] = static_cast<std::uint64_t>(-9ll);
            return true;
        }
        if (length > 1024 * 1024) return fail("guest write is too large");

        for (std::uint64_t i = 0; i < length; ++i) {
            std::uint8_t byte = 0;
            if (!memory_.read8(address + i, byte)) return fail("guest write read failed");
            std::cout.put(static_cast<char>(byte));
        }
        std::cout.flush();
        registers_[RAX] = length;
        return true;
    }

    if (number == 60) {
        exitCode_ = static_cast<std::int64_t>(registers_[RDI]);
        halted_ = true;
        return true;
    }

    return fail("unsupported guest syscall");
}

bool CPU::step() {
    if (halted_) return true;
    lastError_ = nullptr;
    std::uint8_t opcode = 0;
    if (!fetch8(opcode)) return false;
    if (opcode == 0x90) return true;

    // In 64-bit mode a REX prefix is 0x40-0x4F. If multiple REX prefixes
    // occur, the final one supplies the effective REX bits for the opcode.
    std::uint8_t rex = 0;
    while ((opcode & 0xF0) == 0x40) {
        rex = opcode & 0x0F;
        if (!fetch8(opcode)) return false;
    }

    // MOV r32, imm32 (B8+rd). A 32-bit write zero-extends in 64-bit mode.
    if (!(rex & 0x8) && opcode >= 0xB8 && opcode <= 0xBF) {
        std::uint32_t immediate = 0;
        if (!fetch32(immediate)) return false;
        registers_[extendedRegister(opcode - 0xB8, rex & 0x1)] = immediate;
        return true;
    }

    // REX.W + MOV r64, imm64.
    if ((rex & 0x8) && opcode >= 0xB8 && opcode <= 0xBF) {
        std::uint64_t immediate = 0;
        if (!fetch64(immediate)) return false;
        registers_[extendedRegister(opcode - 0xB8, rex & 0x1)] = immediate;
        return true;
    }

    // PUSH r64 / POP r64. REX.B extends the opcode register to R8-R15.
    if (opcode >= 0x50 && opcode <= 0x57) {
        const auto reg = extendedRegister(opcode - 0x50, rex & 0x1);
        const std::uint64_t newRsp = registers_[RSP] - 8;
        if (!memory_.write64(newRsp, registers_[reg])) return fail("stack push failed");
        registers_[RSP] = newRsp;
        return true;
    }

    if (opcode >= 0x58 && opcode <= 0x5F) {
        const auto reg = extendedRegister(opcode - 0x58, rex & 0x1);
        const std::uint64_t oldRsp = registers_[RSP];
        std::uint64_t value = 0;
        if (!memory_.read64(oldRsp, value)) return fail("stack pop failed");
        registers_[reg] = value;
        registers_[RSP] = oldRsp + 8;
        return true;
    }

    if (rex & 0x8) {
        // LEA r64, m. LEA calculates an effective address without reading memory.
        if (opcode == 0x8D) {
            std::uint8_t rawModRM = 0;
            if (!fetch8(rawModRM)) return false;
            ModRM modrm;
            decodeModRM(rawModRM, modrm);
            if (modrm.mod == 3) return fail("LEA requires a memory operand");
            const auto reg = extendedRegister(modrm.reg, (rex >> 2) & 0x1);
            std::uint64_t address = 0;
            if (!resolveModRMAddress(modrm, rex, address)) return false;
            registers_[reg] = address;
            return true;
        }

        if (opcode == 0x89 || opcode == 0x8B) {
            std::uint8_t rawModRM = 0;
            if (!fetch8(rawModRM)) return false;
            ModRM modrm;
            decodeModRM(rawModRM, modrm);
            const auto reg = extendedRegister(modrm.reg, (rex >> 2) & 0x1);
            if (opcode == 0x89) return writeModRMR64(modrm, rex, registers_[reg]);
            std::uint64_t value = 0;
            if (!readModRMR64(modrm, rex, value)) return false;
            registers_[reg] = value;
            return true;
        }

        if (opcode == 0x01 || opcode == 0x29 || opcode == 0x39) {
            std::uint8_t rawModRM = 0;
            if (!fetch8(rawModRM)) return false;
            ModRM modrm;
            decodeModRM(rawModRM, modrm);
            const auto reg = extendedRegister(modrm.reg, (rex >> 2) & 0x1);
            std::uint64_t lhs = 0;
            if (!readModRMR64(modrm, rex, lhs)) return false;
            const auto rhs = registers_[reg];
            if (opcode == 0x01) {
                const auto result = lhs + rhs;
                if (!writeModRMR64(modrm, rex, result)) return false;
                updateAddFlags(lhs, rhs, result);
            } else if (opcode == 0x29) {
                const auto result = lhs - rhs;
                if (!writeModRMR64(modrm, rex, result)) return false;
                updateSubFlags(lhs, rhs, result);
            } else {
                updateSubFlags(lhs, rhs, lhs - rhs);
            }
            return true;
        }

        // AND / OR / XOR r/m64, r64.
        if (opcode == 0x21 || opcode == 0x09 || opcode == 0x31) {
            std::uint8_t rawModRM = 0;
            if (!fetch8(rawModRM)) return false;
            ModRM modrm;
            decodeModRM(rawModRM, modrm);
            const auto reg = extendedRegister(modrm.reg, (rex >> 2) & 0x1);
            std::uint64_t lhs = 0;
            if (!readModRMR64(modrm, rex, lhs)) return false;
            const auto rhs = registers_[reg];

            std::uint64_t result = 0;
            if (opcode == 0x21) result = lhs & rhs;
            else if (opcode == 0x09) result = lhs | rhs;
            else result = lhs ^ rhs;

            if (!writeModRMR64(modrm, rex, result)) return false;
            rflags_ &= ~(CarryFlag | OverflowFlag | ZeroFlag | SignFlag);
            if (result == 0) rflags_ |= ZeroFlag;
            if (result & (1ull << 63)) rflags_ |= SignFlag;
            return true;
        }
    }

    // syscall is 0F 05. It is handled by Zenith's guest syscall shim.
    if (opcode == 0x0F) {
        std::uint8_t second = 0;
        if (!fetch8(second)) return false;
        if (second == 0x05) return handleSyscall();
        return fail("unsupported 0F opcode");
    }

    if (opcode == 0xE8) {
        std::uint32_t rawDisplacement = 0;
        if (!fetch32(rawDisplacement)) return false;
        const auto returnAddress = rip_;
        const auto target = static_cast<std::uint64_t>(static_cast<std::int64_t>(rip_) + static_cast<std::int32_t>(rawDisplacement));
        const auto newRsp = registers_[RSP] - 8;
        if (!memory_.write64(newRsp, returnAddress)) return fail("call stack write failed");
        registers_[RSP] = newRsp;
        rip_ = target;
        return true;
    }

    if (opcode == 0xC3) {
        const auto oldRsp = registers_[RSP];
        std::uint64_t returnAddress = 0;
        if (!memory_.read64(oldRsp, returnAddress)) return fail("return stack read failed");
        registers_[RSP] = oldRsp + 8;
        rip_ = returnAddress;
        return true;
    }

    if (opcode == 0xEB) {
        std::uint8_t displacement = 0;
        if (!fetch8(displacement)) return false;
        rip_ += static_cast<std::int64_t>(static_cast<std::int8_t>(displacement));
        return true;
    }

    if (opcode == 0x74 || opcode == 0x75) {
        std::uint8_t displacement = 0;
        if (!fetch8(displacement)) return false;
        const bool zero = (rflags_ & ZeroFlag) != 0;
        const bool take = opcode == 0x74 ? zero : !zero;
        if (take) rip_ += static_cast<std::int64_t>(static_cast<std::int8_t>(displacement));
        return true;
    }

    return fail("unsupported opcode");
}
