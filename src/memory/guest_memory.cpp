#include "guest_memory.h"

#include <cstring>
#include <limits>

namespace {
constexpr std::uint64_t GuestAddressSpaceSize = 256ull * 1024ull * 1024ull;

bool hasPermission(GuestMemory::Permissions value, GuestMemory::Permissions required) {
    const auto v = static_cast<std::uint8_t>(value);
    const auto r = static_cast<std::uint8_t>(required);
    return (v & r) == r;
}
}

GuestMemory::GuestMemory(std::uint64_t physicalSize)
    : physicalMemory_(static_cast<std::size_t>(physicalSize), 0),
      pages_(static_cast<std::size_t>(GuestAddressSpaceSize / PageSize)) {}

bool GuestMemory::map(std::uint64_t virtualAddress, std::uint64_t physicalAddress,
                      std::uint64_t size, Permissions permissions) {
    if (!size || virtualAddress % PageSize || physicalAddress % PageSize || size % PageSize) return false;
    if (size > GuestAddressSpaceSize || size > physicalMemory_.size()) return false;
    if (virtualAddress > GuestAddressSpaceSize - size || physicalAddress > physicalMemory_.size() - size) return false;

    const auto firstPage = virtualAddress / PageSize;
    const auto pageCount = size / PageSize;
    for (std::uint64_t i = 0; i < pageCount; ++i) {
        if (pages_[static_cast<std::size_t>(firstPage + i)].mapped) return false;
    }
    for (std::uint64_t i = 0; i < pageCount; ++i) {
        auto& page = pages_[static_cast<std::size_t>(firstPage + i)];
        page.physicalPage = physicalAddress / PageSize + i;
        page.permissions = permissions;
        page.mapped = true;
    }
    return true;
}

bool GuestMemory::protect(std::uint64_t virtualAddress, std::uint64_t size, Permissions permissions) {
    if (!size || virtualAddress % PageSize || size % PageSize || size > GuestAddressSpaceSize) return false;
    if (virtualAddress > GuestAddressSpaceSize - size) return false;

    const auto firstPage = virtualAddress / PageSize;
    const auto pageCount = size / PageSize;
    for (std::uint64_t i = 0; i < pageCount; ++i) {
        if (!pages_[static_cast<std::size_t>(firstPage + i)].mapped) return false;
    }
    for (std::uint64_t i = 0; i < pageCount; ++i) {
        pages_[static_cast<std::size_t>(firstPage + i)].permissions = permissions;
    }
    return true;
}

bool GuestMemory::unmap(std::uint64_t virtualAddress, std::uint64_t size) {
    if (!size || virtualAddress % PageSize || size % PageSize || size > GuestAddressSpaceSize) return false;
    if (virtualAddress > GuestAddressSpaceSize - size) return false;

    const auto firstPage = virtualAddress / PageSize;
    const auto pageCount = size / PageSize;
    for (std::uint64_t i = 0; i < pageCount; ++i) {
        if (!pages_[static_cast<std::size_t>(firstPage + i)].mapped) return false;
    }
    for (std::uint64_t i = 0; i < pageCount; ++i) {
        pages_[static_cast<std::size_t>(firstPage + i)] = {};
    }
    return true;
}

bool GuestMemory::translate(std::uint64_t address, Permissions required, std::uint64_t& physicalAddress) const {
    if (address >= GuestAddressSpaceSize) return false;
    const auto pageIndex = address / PageSize;
    const auto offset = address % PageSize;
    const auto& page = pages_[static_cast<std::size_t>(pageIndex)];
    if (!page.mapped || !hasPermission(page.permissions, required)) return false;
    physicalAddress = page.physicalPage * PageSize + offset;
    return physicalAddress < physicalMemory_.size();
}

bool GuestMemory::checkRange(std::uint64_t address, std::uint64_t size, Permissions required) const {
    if (!size || address > std::numeric_limits<std::uint64_t>::max() - (size - 1)) return false;
    std::uint64_t physical{};
    return translate(address, required, physical) && translate(address + size - 1, required, physical);
}

bool GuestMemory::read8(std::uint64_t address, std::uint8_t& value) const {
    std::uint64_t physical{};
    if (!translate(address, Permissions::Read, physical)) return false;
    value = physicalMemory_[static_cast<std::size_t>(physical)];
    return true;
}

bool GuestMemory::read16(std::uint64_t address, std::uint16_t& value) const {
    if (!checkRange(address, sizeof(value), Permissions::Read)) return false;
    std::uint8_t bytes[2]{};
    for (std::size_t i = 0; i < sizeof(value); ++i) if (!read8(address + i, bytes[i])) return false;
    std::memcpy(&value, bytes, sizeof(value));
    return true;
}

bool GuestMemory::read32(std::uint64_t address, std::uint32_t& value) const {
    if (!checkRange(address, sizeof(value), Permissions::Read)) return false;
    std::uint8_t bytes[4]{};
    for (std::size_t i = 0; i < sizeof(value); ++i) if (!read8(address + i, bytes[i])) return false;
    std::memcpy(&value, bytes, sizeof(value));
    return true;
}

bool GuestMemory::read64(std::uint64_t address, std::uint64_t& value) const {
    if (!checkRange(address, sizeof(value), Permissions::Read)) return false;
    std::uint8_t bytes[8]{};
    for (std::size_t i = 0; i < sizeof(value); ++i) if (!read8(address + i, bytes[i])) return false;
    std::memcpy(&value, bytes, sizeof(value));
    return true;
}

bool GuestMemory::write8(std::uint64_t address, std::uint8_t value) {
    std::uint64_t physical{};
    if (!translate(address, Permissions::Write, physical)) return false;
    physicalMemory_[static_cast<std::size_t>(physical)] = value;
    return true;
}

bool GuestMemory::write16(std::uint64_t address, std::uint16_t value) {
    if (!checkRange(address, sizeof(value), Permissions::Write)) return false;
    std::uint8_t bytes[2]{};
    std::memcpy(bytes, &value, sizeof(value));
    for (std::size_t i = 0; i < sizeof(value); ++i) if (!write8(address + i, bytes[i])) return false;
    return true;
}

bool GuestMemory::write32(std::uint64_t address, std::uint32_t value) {
    if (!checkRange(address, sizeof(value), Permissions::Write)) return false;
    std::uint8_t bytes[4]{};
    std::memcpy(bytes, &value, sizeof(value));
    for (std::size_t i = 0; i < sizeof(value); ++i) if (!write8(address + i, bytes[i])) return false;
    return true;
}

bool GuestMemory::write64(std::uint64_t address, std::uint64_t value) {
    if (!checkRange(address, sizeof(value), Permissions::Write)) return false;
    std::uint8_t bytes[8]{};
    std::memcpy(bytes, &value, sizeof(value));
    for (std::size_t i = 0; i < sizeof(value); ++i) if (!write8(address + i, bytes[i])) return false;
    return true;
}
