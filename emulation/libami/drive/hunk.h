
#pragma once

#include <cstdint>
#include <vector>

namespace LIBAMI {

struct HunkSection {
    enum class Type : uint32_t {
        NAME = 1000, CODE = 1001, DATA = 1002, BSS = 1003, RELOC32 = 1004,
        END = 1010, BREAK = 1014
    } type;

    uint32_t offset;
    uint32_t size;

    uint32_t relocDest;
    std::vector<uint32_t> relocs;
};

struct Hunk {
    uint32_t flags;
    std::vector<HunkSection> sections;

    auto size() -> uint32_t { return (flags & 0x3fffffff) << 2; }
};

struct ProgramUnit {

    std::vector<Hunk> hunks;

    auto parse(const uint8_t* code, unsigned size) -> void;
};

}
