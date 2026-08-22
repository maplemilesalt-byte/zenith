#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class GuestMemory;

class CPU {
public:
    static constexpr std::size_t RegisterCount = 16;

    enum Register : std::size_t {
        RAX = 0, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
        R8, R9, R10, R11, R12, R13, R14, R15
    };

    static constexpr std::uint64_t CarryFlag = 1ull << 0;
    static constexpr std::uint64_t ZeroFlag = 1ull << 6;
    static constexpr std::uint64_t SignFlag = 1ull << 7;
    static constexpr std::uint64_t OverflowFlag = 1ull << 11;

    explicit CPU(GuestMemory& memory);

    void reset();
    bool step();

    std::uint64_t getRegister(Register reg) const;
    void setRegister(Register reg, std::uint64_t value);

    std::uint64_t rip() const;
    void setRip(std::uint64_t value);
    std::uint64_t rflags() const;
    const char* lastError() const;
    bool halted() const;
    std::int64_t exitCode() const;

private:
    struct ModRM {
        std::uint8_t mod = 0;
        std::uint8_t reg = 0;
        std::uint8_t rm = 0;
    };

    bool fetch8(std::uint8_t& value);
    bool fetch32(std::uint32_t& value);
    bool fetch64(std::uint64_t& value);
    bool decodeModRM(std::uint8_t value, ModRM& modrm) const;
    bool readModRMR64(const ModRM& modrm, std::uint8_t rex, std::uint64_t& value);
    bool writeModRMR64(const ModRM& modrm, std::uint8_t rex, std::uint64_t value);
    bool resolveModRMAddress(const ModRM& modrm, std::uint8_t rex, std::uint64_t& address);
    std::size_t extendedRegister(std::uint8_t index, std::uint8_t rexBit) const;
    void updateAddFlags(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t result);
    void updateSubFlags(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t result);
    bool handleSyscall();
    bool fail(const char* message);

    GuestMemory& memory_;
    std::array<std::uint64_t, RegisterCount> registers_{};
    std::uint64_t rip_ = 0;
    std::uint64_t rflags_ = 0;
    const char* lastError_ = nullptr;
    bool halted_ = false;
    std::int64_t exitCode_ = 0;
};
