#pragma once

#include <cstdint>
#include <vector>

class GuestMemory {
public:
    static constexpr std::uint64_t PageSize = 4096;
    enum class Permissions : std::uint8_t { None = 0, Read = 1, Write = 2, Execute = 4 };

    explicit GuestMemory(std::uint64_t physicalSize);
    bool map(std::uint64_t virtualAddress, std::uint64_t physicalAddress,
             std::uint64_t size, Permissions permissions);
    bool unmap(std::uint64_t virtualAddress, std::uint64_t size);
    bool read8(std::uint64_t address, std::uint8_t& value) const;
    bool read16(std::uint64_t address, std::uint16_t& value) const;
    bool read32(std::uint64_t address, std::uint32_t& value) const;
    bool read64(std::uint64_t address, std::uint64_t& value) const;
    bool write8(std::uint64_t address, std::uint8_t value);
    bool write16(std::uint64_t address, std::uint16_t value);
    bool write32(std::uint64_t address, std::uint32_t value);
    bool write64(std::uint64_t address, std::uint64_t value);

private:
    struct Page { std::uint64_t physicalPage = 0; Permissions permissions = Permissions::None; bool mapped = false; };
    bool checkRange(std::uint64_t address, std::uint64_t size, Permissions required) const;
    bool translate(std::uint64_t address, Permissions required, std::uint64_t& physicalAddress) const;
    std::vector<std::uint8_t> physicalMemory_;
    std::vector<Page> pages_;
};
