#pragma once

#include <cstdint>
#include <string>

class GuestMemory;

class ElfLoader {
public:
    bool load(const std::string& path, GuestMemory& memory, std::uint64_t& entry, const char*& error);

private:
    const char* fail(const char* message, const char*& error) const;
};
