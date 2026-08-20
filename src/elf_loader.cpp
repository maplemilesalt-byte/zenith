#include "elf_loader.h"
#include "memory/guest_memory.h"

#include <cstdint>
#include <fstream>
#include <limits>
#include <vector>

namespace {
#pragma pack(push, 1)
struct Elf64Header {
    unsigned char ident[16];
    std::uint16_t type;
    std::uint16_t machine;
    std::uint32_t version;
    std::uint64_t entry;
    std::uint64_t phoff;
    std::uint64_t shoff;
    std::uint32_t flags;
    std::uint16_t ehsize;
    std::uint16_t phentsize;
    std::uint16_t phnum;
    std::uint16_t shentsize;
    std::uint16_t shnum;
    std::uint16_t shstrndx;
};

struct Elf64ProgramHeader {
    std::uint32_t type;
    std::uint32_t flags;
    std::uint64_t offset;
    std::uint64_t vaddr;
    std::uint64_t paddr;
    std::uint64_t filesz;
    std::uint64_t memsz;
    std::uint64_t align;
};
#pragma pack(pop)

constexpr std::uint32_t PT_LOAD = 1;
constexpr std::uint32_t PF_X = 1;
constexpr std::uint32_t PF_W = 2;
constexpr std::uint32_t PF_R = 4;
constexpr std::uint16_t EM_X86_64 = 62;
constexpr std::uint16_t ET_EXEC = 2;
constexpr std::uint16_t ET_DYN = 3;

bool rangeFits(std::uint64_t offset, std::uint64_t size, std::uint64_t total) {
    return offset <= total && size <= total - offset;
}

std::uint64_t alignDown(std::uint64_t value, std::uint64_t alignment) {
    return value & ~(alignment - 1);
}

std::uint64_t alignUp(std::uint64_t value, std::uint64_t alignment) {
    if (value > std::numeric_limits<std::uint64_t>::max() - (alignment - 1)) return 0;
    return (value + alignment - 1) & ~(alignment - 1);
}
}

const char* ElfLoader::fail(const char* message, const char*& error) const {
    error = message;
    return message;
}

bool ElfLoader::load(const std::string& path, GuestMemory& memory,
                     std::uint64_t& entry, const char*& error) {
    error = nullptr;
    entry = 0;

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        fail("failed to open ELF", error);
        return false;
    }
    const auto end = file.tellg();
    if (end < 0) {
        fail("failed to determine ELF size", error);
        return false;
    }
    const auto size = static_cast<std::uint64_t>(end);
    if (size < sizeof(Elf64Header)) {
        fail("ELF file is too small", error);
        return false;
    }
    file.seekg(0);
    std::vector<std::uint8_t> image(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(image.data()), static_cast<std::streamsize>(image.size()))) {
        fail("failed to read ELF", error);
        return false;
    }

    const auto* header = reinterpret_cast<const Elf64Header*>(image.data());
    if (header->ident[0] != 0x7F || header->ident[1] != 'E' ||
        header->ident[2] != 'L' || header->ident[3] != 'F') {
        fail("invalid ELF magic", error);
        return false;
    }
    if (header->ident[4] != 2 || header->ident[5] != 1) {
        fail("ELF is not 64-bit little-endian", error);
        return false;
    }
    if (header->machine != EM_X86_64) {
        fail("ELF is not x86-64", error);
        return false;
    }
    if (header->type != ET_EXEC && header->type != ET_DYN) {
        fail("unsupported ELF type", error);
        return false;
    }
    if (header->phentsize != sizeof(Elf64ProgramHeader) || header->phnum == 0) {
        fail("unsupported ELF program header layout", error);
        return false;
    }
    if (header->phoff > size || header->phnum > (size - header->phoff) / header->phentsize) {
        fail("ELF program headers are out of bounds", error);
        return false;
    }

    constexpr std::uint64_t pageSize = GuestMemory::PageSize;
    std::uint64_t physicalCursor = 0;
    bool loadedSegment = false;

    for (std::uint16_t i = 0; i < header->phnum; ++i) {
        const auto* ph = reinterpret_cast<const Elf64ProgramHeader*>(
            image.data() + header->phoff + static_cast<std::uint64_t>(i) * header->phentsize);
        if (ph->type != PT_LOAD) continue;
        if (ph->filesz > ph->memsz || !rangeFits(ph->offset, ph->filesz, size)) {
            fail("ELF load segment is out of bounds", error);
            return false;
        }
        if (ph->vaddr > std::numeric_limits<std::uint64_t>::max() - ph->memsz) {
            fail("ELF load segment address overflow", error);
            return false;
        }

        const auto pageVaddr = alignDown(ph->vaddr, pageSize);
        const auto pageOffset = ph->vaddr - pageVaddr;
        const auto mappedSize = alignUp(pageOffset + ph->memsz, pageSize);
        if (!mappedSize) {
            fail("ELF load segment size overflow", error);
            return false;
        }
        if (physicalCursor > std::numeric_limits<std::uint64_t>::max() - mappedSize) {
            fail("ELF physical memory allocation overflow", error);
            return false;
        }

        GuestMemory::Permissions permissions = GuestMemory::Permissions::None;
        if (ph->flags & PF_R) permissions = permissions | GuestMemory::Permissions::Read;
        if (ph->flags & PF_W) permissions = permissions | GuestMemory::Permissions::Write;
        if (ph->flags & PF_X) permissions = permissions | GuestMemory::Permissions::Execute;

        if (!memory.map(pageVaddr, physicalCursor, mappedSize, permissions)) {
            fail("failed to map ELF load segment", error);
            return false;
        }

        for (std::uint64_t j = 0; j < ph->filesz; ++j) {
            if (!memory.write8(ph->vaddr + j, image[static_cast<std::size_t>(ph->offset + j)])) {
                fail("failed to copy ELF load segment", error);
                return false;
            }
        }

        physicalCursor += mappedSize;
        loadedSegment = true;
    }

    if (!loadedSegment) {
        fail("ELF contains no loadable segments", error);
        return false;
    }

    entry = header->entry;
    return true;
}
