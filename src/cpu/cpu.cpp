#include "cpu.h"
#include "memory/guest_memory.h"

#include <cstdint>

CPU::CPU(GuestMemory& memory) : memory_(memory) {
    reset();
}

void CPU::reset() {
    registers_.fill(0);
    rip_ = 0;
    rflags_ = 0;
    lastError_ = nullptr;
}

std::uint64_t CPU::getRegister(Register reg) const { return registers_[static_cast<std::size_t>(reg)]; }
void CPU::setRegister(Register reg, std::uint64_t value) { registers_[static_cast<std::size_t>(reg)] = value; }
std::uint64_t CPU::rip() const { return rip_; }
void CPU::setRip(std::uint64_t value) { rip_ = value; }
std::uint64_t CPU::rflags() const { return rflags_; }
const char* CPU::lastError() const { return lastError_; }

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

bool CPU::readModRMR8(const ModRM& modrm, std::uint8_t rex, std::uint8_t& value) {
    if (modrm.mod == 3) {
        const auto reg = extendedRegister(modrm.rm, rex & 0x1);
        value = static_cast<std::uint8_t>(registers_[reg]);
        return true;
    }
    std::uint64_t address = 0;
    if (!resolveModRMAddress(modrm, rex, address)) return false;
    if (!memory_.read8(address, value)) return fail("8-bit memory read failed");
    return true;
}

bool CPU::readModRMR16(const ModRM& modrm, std::uint8_t rex, std::uint16_t& value) {
    if (modrm.mod == 3) {
        const auto reg = extendedRegister(modrm.rm, rex & 0x1);
        value = static_cast<std::uint16_t>(registers_[reg]);
        return true;
    }
    std::uint64_t address = 0;
    if (!resolveModRMAddress(modrm, rex, address)) return false;
    if (!memory_.read16(address, value)) return fail("16-bit memory read failed");
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

bool CPU::step() {
    lastError_ = nullptr;
    std::uint8_t opcode = 0;
    if (!fetch8(opcode)) return false;
    if (opcode == 0x90) return true;

    std::uint8_t rex = 0;
    while ((opcode & 0xF0) == 0x40) {
        rex = opcode & 0x0F;
        if (!fetch8(opcode)) return false;
    }

    if (!(rex & 0x8) && opcode >= 0xB8 && opcode <= 0xBF) {
        std::uint32_t immediate = 0;
        if (!fetch32(immediate)) return false;
        registers_[extendedRegister(opcode - 0xB8, rex & 0x1)] = immediate;
        return true;
    }

    if ((rex & 0x8) && opcode >= 0xB8 && opcode <= 0xBF) {
        std::uint64_t immediate = 0;
        if (!fetch64(immediate)) return false;
        registers_[extendedRegister(opcode - 0xB8, rex & 0x1)] = immediate;
        return true;
    }

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

        // MOVZX/MOVSX r64, r/m8/r/m16.
        if (opcode == 0x0F) {
            std::uint8_t opcode2 = 0;
            if (!fetch8(opcode2)) return false;
            if (opcode2 == 0xB6 || opcode2 == 0xB7 || opcode2 == 0xBE || opcode2 == 0xBF) {
                std::uint8_t rawModRM = 0;
                if (!fetch8(rawModRM)) return false;
                ModRM modrm;
                decodeModRM(rawModRM, modrm);
                const auto reg = extendedRegister(modrm.reg, (rex >> 2) & 0x1);
                if (opcode2 == 0xB6 || opcode2 == 0xBE) {
                    std::uint8_t value = 0;
                    if (!readModRMR8(modrm, rex, value)) return false;
                    registers_[reg] = (opcode2 == 0xB6)
                        ? static_cast<std::uint64_t>(value)
                        : static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int8_t>(value)));
                } else {
                    std::uint16_t value = 0;
                    if (!readModRMR16(modrm, rex, value)) return false;
                    registers_[reg] = (opcode2 == 0xB7)
                        ? static_cast<std::uint64_t>(value)
                        : static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int16_t>(value)));
                }
                return true;
            }
        }

        if (opcode == 0x01 || opcode == 0x11 || opcode == 0x21 ||
            opcode == 0x09 || opcode == 0x19 || opcode == 0x29 ||
            opcode == 0x31 || opcode == 0x39 || opcode == 0x85) {
            std::uint8_t rawModRM = 0;
            if (!fetch8(rawModRM)) return false;
            ModRM modrm;
            decodeModRM(rawModRM, modrm);
            const auto reg = extendedRegister(modrm.reg, (rex >> 2) & 0x1);
            std::uint64_t lhs = 0;
            if (!readModRMR64(modrm, rex, lhs)) return false;
            const auto rhs = registers_[reg];
            const auto carry = (rflags_ & CarryFlag) ? 1ull : 0ull;

            if (opcode == 0x01) {
                const auto result = lhs + rhs;
                if (!writeModRMR64(modrm, rex, result)) return false;
                updateAddFlags(lhs, rhs, result);
            } else if (opcode == 0x11) {
                const auto result = lhs + rhs + carry;
                if (!writeModRMR64(modrm, rex, result)) return false;
                rflags_ &= ~(CarryFlag | ZeroFlag | SignFlag | OverflowFlag);
                if (result < lhs || (carry && result == lhs)) rflags_ |= CarryFlag;
                if (result == 0) rflags_ |= ZeroFlag;
                if (result & (1ull << 63)) rflags_ |= SignFlag;
                const bool a = (lhs >> 63) != 0;
                const bool b = (rhs >> 63) != 0;
                const bool r = (result >> 63) != 0;
                if (a == b && a != r) rflags_ |= OverflowFlag;
            } else if (opcode == 0x29) {
                const auto result = lhs - rhs;
                if (!writeModRMR64(modrm, rex, result)) return false;
                updateSubFlags(lhs, rhs, result);
            } else if (opcode == 0x19) {
                const auto result = lhs - rhs - carry;
                if (!writeModRMR64(modrm, rex, result)) return false;
                rflags_ &= ~(CarryFlag | ZeroFlag | SignFlag | OverflowFlag);
                if (lhs < rhs || (carry && lhs - rhs == 0)) rflags_ |= CarryFlag;
                if (result == 0) rflags_ |= ZeroFlag;
                if (result & (1ull << 63)) rflags_ |= SignFlag;
                const bool a = (lhs >> 63) != 0;
                const bool b = (rhs >> 63) != 0;
                const bool r = (result >> 63) != 0;
                if (a != b && a != r) rflags_ |= OverflowFlag;
            } else if (opcode == 0x39) {
                updateSubFlags(lhs, rhs, lhs - rhs);
            } else {
                const auto result = (opcode == 0x21) ? (lhs & rhs) :
                                    (opcode == 0x09) ? (lhs | rhs) :
                                    (opcode == 0x31) ? (lhs ^ rhs) :
                                    (lhs & rhs);
                if (opcode != 0x85) {
                    if (!writeModRMR64(modrm, rex, result)) return false;
                }
                rflags_ &= ~(CarryFlag | OverflowFlag | ZeroFlag | SignFlag);
                if (result == 0) rflags_ |= ZeroFlag;
                if (result & (1ull << 63)) rflags_ |= SignFlag;
            }
            return true;
        }

        if (opcode == 0xFF || opcode == 0xF7) {
            std::uint8_t rawModRM = 0;
            if (!fetch8(rawModRM)) return false;
            ModRM modrm;
            decodeModRM(rawModRM, modrm);
            std::uint64_t value = 0;
            if (!readModRMR64(modrm, rex, value)) return false;
            const auto oldCarry = rflags_ & CarryFlag;
            if (opcode == 0xFF && (modrm.reg == 0 || modrm.reg == 1)) {
                const auto result = modrm.reg == 0 ? value + 1 : value - 1;
                if (!writeModRMR64(modrm, rex, result)) return false;
                if (modrm.reg == 0) updateAddFlags(value, 1, result);
                else updateSubFlags(value, 1, result);
                rflags_ = (rflags_ & ~CarryFlag) | oldCarry;
                return true;
            }
            if (opcode == 0xF7 && (modrm.reg == 2 || modrm.reg == 3)) {
                if (modrm.reg == 2) return writeModRMR64(modrm, rex, ~value);
                const auto result = 0ull - value;
                if (!writeModRMR64(modrm, rex, result)) return false;
                updateSubFlags(0, value, result);
                return true;
            }
            return fail("unsupported unary opcode");
        }

        if (opcode == 0xC1 || opcode == 0xD3) {
            std::uint8_t rawModRM = 0;
            if (!fetch8(rawModRM)) return false;
            ModRM modrm;
            decodeModRM(rawModRM, modrm);
            if (modrm.reg != 4 && modrm.reg != 5 && modrm.reg != 7) return fail("unsupported shift opcode");
            std::uint8_t count = 0;
            if (opcode == 0xC1) {
                if (!fetch8(count)) return false;
            } else {
                count = static_cast<std::uint8_t>(registers_[RCX] & 0xFF);
            }
            count &= 0x3F;
            std::uint64_t value = 0;
            if (!readModRMR64(modrm, rex, value)) return false;
            if (count == 0) return true;
            const auto original = value;
            std::uint64_t result = 0;
            bool carry = false;
            if (modrm.reg == 4) {
                result = value << count;
                carry = ((value >> (64 - count)) & 1ull) != 0;
            } else if (modrm.reg == 5) {
                result = value >> count;
                carry = ((value >> (count - 1)) & 1ull) != 0;
            } else {
                result = static_cast<std::uint64_t>(static_cast<std::int64_t>(value) >> count);
                carry = ((value >> (count - 1)) & 1ull) != 0;
            }
            if (!writeModRMR64(modrm, rex, result)) return false;
            rflags_ &= ~(CarryFlag | ZeroFlag | SignFlag | OverflowFlag);
            if (carry) rflags_ |= CarryFlag;
            if (result == 0) rflags_ |= ZeroFlag;
            if (result & (1ull << 63)) rflags_ |= SignFlag;
            if (count == 1) {
                bool overflow = false;
                if (modrm.reg == 4) overflow = ((result >> 63) & 1ull) != (carry ? 1ull : 0ull);
                else if (modrm.reg == 5) overflow = (original >> 63) != 0;
                if (overflow) rflags_ |= OverflowFlag;
            }
            return true;
        }
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

    if ((opcode & 0xF0) == 0x70) {
        std::uint8_t displacement = 0;
        if (!fetch8(displacement)) return false;
        const bool cf = (rflags_ & CarryFlag) != 0;
        const bool zf = (rflags_ & ZeroFlag) != 0;
        const bool sf = (rflags_ & SignFlag) != 0;
        const bool of = (rflags_ & OverflowFlag) != 0;
        bool take = false;
        switch (opcode) {
            case 0x70: take = of; break;
            case 0x71: take = !of; break;
            case 0x72: take = cf; break;
            case 0x73: take = !cf; break;
            case 0x74: take = zf; break;
            case 0x75: take = !zf; break;
            case 0x76: take = cf || zf; break;
            case 0x77: take = !cf && !zf; break;
            case 0x78: take = sf; break;
            case 0x79: take = !sf; break;
            case 0x7C: take = sf != of; break;
            case 0x7D: take = sf == of; break;
            case 0x7E: take = zf || (sf != of); break;
            case 0x7F: take = !zf && (sf == of); break;
            default: return fail("unsupported conditional branch");
        }
        if (take) rip_ += static_cast<std::int64_t>(static_cast<std::int8_t>(displacement));
        return true;
    }

    return fail("unsupported opcode");
}
