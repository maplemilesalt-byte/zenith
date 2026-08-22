#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

class GuestMemory;

class CPU {
public:
    static constexpr std::size_t RegisterCount = 16;
    enum Register : std::size_t { RAX = 0, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15 };
    static constexpr std::uint64_t CarryFlag = 1ull << 0;
    static constexpr std::uint64_t ZeroFlag = 1ull << 6;
    static constexpr std::uint64_t SignFlag = 1ull << 7;
    static constexpr std::uint64_t OverflowFlag = 1ull << 11;
    explicit CPU(GuestMemory& memory);
    void reset(); bool step();
    std::uint64_t getRegister(Register reg) const; void setRegister(Register reg, std::uint64_t value);
    std::uint64_t rip() const; void setRip(std::uint64_t value); std::uint64_t rflags() const;
    const char* lastError() const; bool halted() const; std::int64_t exitCode() const;
    bool framePresented() const; void clearFramePresented();
    const std::string& guestTitle() const;
private:
    struct ModRM { std::uint8_t mod=0,reg=0,rm=0; };
    bool fetch8(std::uint8_t&); bool fetch32(std::uint32_t&); bool fetch64(std::uint64_t&);
    bool decodeModRM(std::uint8_t,ModRM&) const; bool readModRMR64(const ModRM&,std::uint8_t,std::uint64_t&);
    bool writeModRMR64(const ModRM&,std::uint8_t,std::uint64_t); bool resolveModRMAddress(const ModRM&,std::uint8_t,std::uint64_t&);
    std::size_t extendedRegister(std::uint8_t,std::uint8_t) const; void updateAddFlags(std::uint64_t,std::uint64_t,std::uint64_t); void updateSubFlags(std::uint64_t,std::uint64_t,std::uint64_t);
    bool handleSyscall(); bool fail(const char*);
    GuestMemory& memory_; std::array<std::uint64_t,RegisterCount> registers_{}; std::uint64_t rip_=0,rflags_=0;
    const char* lastError_=nullptr; bool halted_=false,framePresented_=false; std::int64_t exitCode_=0; std::string guestTitle_;
};
