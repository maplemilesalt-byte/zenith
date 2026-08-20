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

std::uint64_t CPU::getRegister(Register reg) const {
    return registers_[static_cast<std::size_t>(reg)];
}

void CPU::setRegister(Register reg, std::uint64_t value) {
    registers_[static_cast<std::size_t>(reg)] = value;
}

std::uint64_t CPU::rip() const { return rip_; }
void CPU::setRip(std::uint64_t value) { rip_ = value; }
std::uint64_t CPU::rflags() const { return rflags_; }
const char* CPU::lastError() const { return lastError_; }

bool CPU::fail(const char* message) {
    lastError_ = message;
    return false;
}

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

bool CPU::decodeModRM(std::uint8_t modrm, std::uint8_t& reg, std::uint8_t& rm) const {
    const std::uint8_t mod = (modrm >> 6) & 0x3;
    if (mod != 0x3) return false; // Register-direct form only for now.
    reg = (modrm >> 3) & 0x7;
    rm = modrm & 0x7;
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

    if (opcode == 0x90) { // NOP
        return true;
    }

    // MOV r32, imm32 (B8+rd id). Writing a 32-bit register zero-extends it.
    if (opcode >= 0xB8 && opcode <= 0xBF) {
        std::uint32_t immediate = 0;
        if (!fetch32(immediate)) return false;
        const auto reg = static_cast<Register>(opcode - 0xB8);
        registers_[static_cast<std::size_t>(reg)] = immediate;
        return true;
    }

    // REX.W + MOV r64, imm64.
    if (opcode == 0x48) {
        std::uint8_t next = 0;
        if (!fetch8(next)) return false;
        if (next >= 0xB8 && next <= 0xBF) {
            std::uint64_t immediate = 0;
            if (!fetch64(immediate)) return false;
            registers_[static_cast<std::size_t>(next - 0xB8)] = immediate;
            return true;
        }

        // REX.W + ADD/SUB/CMP r/m64, r64 with register-direct ModRM.
        if (next == 0x01 || next == 0x29 || next == 0x39) {
            std::uint8_t modrm = 0;
            if (!fetch8(modrm)) return false;
            std::uint8_t reg = 0, rm = 0;
            if (!decodeModRM(modrm, reg, rm)) return fail("unsupported ModRM addressing mode");
            const auto lhs = registers_[rm];
            const auto rhs = registers_[reg];
            if (next == 0x01) {
                const auto result = lhs + rhs;
                registers_[rm] = result;
                updateAddFlags(lhs, rhs, result);
            } else if (next == 0x29) {
                const auto result = lhs - rhs;
                registers_[rm] = result;
                updateSubFlags(lhs, rhs, result);
            } else {
                updateSubFlags(lhs, rhs, lhs - rhs);
            }
            return true;
        }

        return fail("unsupported REX.W instruction");
    }

    // Short unconditional jump.
    if (opcode == 0xEB) {
        std::uint8_t displacement = 0;
        if (!fetch8(displacement)) return false;
        rip_ += static_cast<std::int64_t>(static_cast<std::int8_t>(displacement));
        return true;
    }

    // Short conditional jumps: JZ / JNZ.
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
