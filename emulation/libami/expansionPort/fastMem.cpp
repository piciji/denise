
#include "fastMem.h"
#include "../agnus/agnus.h"

namespace LIBAMI {

auto FastMemExpansion::pages() const -> unsigned {
    return agnus.fastMemSize >> 16;
}

auto FastMemExpansion::reset(bool softReset) -> void {
    ExpansionPort::reset(softReset);
    if (agnus.fastMemSize)
        boardState = BoardState::AutoConf;
}

auto FastMemExpansion::serialize(Emulator::Serializer& s, bool light) -> void {
    ExpansionPort::serialize(s, light);

    if (!light && (s.mode() == Emulator::Serializer::Mode::Load)) {
        if (boardState == BoardState::Configured)
            add();
    }
}

auto FastMemExpansion::readAutoConf(uint32_t addr) -> uint8_t {
    addr &= 0xffff;
    if (addr & 1)
        return 0xff;
    
    switch(addr) {
        // type and size
        case 0:
        case 2: {
            uint8_t res = type() | getSizeBits();
            return addr == 0 ? res & 0xf0 : res << 4;
        }
        // product
        case 4: return encNibble(5); // hi
        case 6: return encNibble(1); // lo
        // flags
        case 8:   // any space ok and can be shut up
        case 0xa: return encNibble(0);
        // reserved
        case 0xc:
        case 0xe: return encNibble(0);
        // manufacturer Hi byte
        case 0x10: return encNibble(0);
        case 0x12: return encNibble(2);
        // manufacturer Lo byte
        case 0x14: return encNibble(0xd);
        case 0x16: return encNibble(0xb);
        // serial number 4 bytes
        case 0x18: case 0x1a:
        case 0x1c: case 0x1e:
        case 0x20: case 0x22:
        case 0x24:
            return encNibble(0);
        case 0x26:
            return encNibble(1);
        // rom vector 2 byte
        case 0x28:
        case 0x2a:
            return encNibble(0);
        case 0x2c:
        case 0x2e:
            return encNibble(0);
        // reserved area
        // Interrupt support
        case 0x40:
        case 0x42:
            return 0; // not inverted
    }

    return 0xff;
}

}
