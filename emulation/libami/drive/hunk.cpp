#include "hunk.h"
#include "../../tools/error.h"
#include "../../tools/buffer.h"

namespace LIBAMI {

auto ProgramUnit::parse(const uint8_t* code, unsigned size) -> void {
    unsigned offset = 0;
    typedef HunkSection::Type HType;
    typedef Emulator::Error::Type EType;

    hunks.clear();    

    auto read = [&]() {
        if (offset + 4 > size)
            throw Emulator::Error(EType::HUNK_UNEXPECTED_END);

        auto result = ToU32BE(code + offset);
        offset += 4;
        return result;
    };

    if (read() != 0x3f3) // magic header
        throw Emulator::Error(EType::HUNK_BAD_HEADER);

    for (auto N = read(); N != 0; N = read()) // N long words of names (zero ends this)
        for (int i = 0; i < N; i++)
            read();

    auto hunkCount = read();
    if (!hunkCount || (read() != 0) || (read() != (hunkCount - 1))) // first = zero, last = hunks - 1
        throw Emulator::Error(EType::HUNK_BAD_HEADER);

    for (int i = 0; i < hunkCount; i++) {
        Hunk hunk;
        hunk.flags = read();
        hunks.push_back(hunk);
    }

    for (int h = 0; h < hunkCount; ) {
        Hunk& hunk = hunks[h];
        hunk.sections.push_back({});
        HunkSection& section = hunk.sections.back();
        section.type = (HType)read();
        section.offset = offset - 4;
        section.size = 0;
        section.relocDest = 0;

        switch (section.type) {
        case HType::NAME:
        case HType::CODE:
        case HType::DATA:
            section.size = read() << 2;
            if (section.size > hunk.size())
                throw Emulator::Error(EType::HUNK_BAD_SIZE);
            offset += section.size;
            break;
        case HType::BSS:
            section.size = read() << 2;
            break;
        case HType::RELOC32:
            for (auto count = read(); count; count = read()) {
                section.size += count << 2;
                section.relocDest = read();
                if (section.relocDest >= hunkCount)
                    throw Emulator::Error(EType::HUNK_BAD_RELOC);

                section.relocs.reserve(section.relocs.size() + count);
                while (count--) {
                    uint32_t reloc = read();
                    if (reloc + 3 > hunk.size())
                        throw Emulator::Error(EType::HUNK_BAD_SIZE);
                    section.relocs.push_back(reloc);
                }
            }
            break;
        case HType::END:
        case HType::BREAK:
            h++;
            break;
        default:
            throw Emulator::Error(EType::HUNK_UNIMPLEMENTED, (int)section.type);
        }
    }
}

}
