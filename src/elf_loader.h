#pragma once

#include <cstdint>
#include <string>
#include <vector>

class GuestMemory;

struct ElfLoadResult {
    std::uint64_t entryPoint = 0;
    std::uint16_t machine = 0;
    std::uint16_t programHeaders = 0;
};

class ElfLoader {
public:
    bool loadFile(const std::string& path, GuestMemory& memory,
                  ElfLoadResult& result, std::string& error) const;
    bool loadImage(const std::vector<std::uint8_t>& image, GuestMemory& memory,
                   ElfLoadResult& result, std::string& error) const;
};
