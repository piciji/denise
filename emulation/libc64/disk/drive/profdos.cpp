
#include "drive.h"

namespace LIBC64 {

auto Drive::readProfDosEncoder(uint16_t addr)->uint8_t {
    if ( (addr & 0xf000) == 0x7000) {
        if (!(addr & 0x0800)) {
            addr = (uint16_t)((addr & 0xff0f) | (nibble << 4));
        } else {
            addr = (uint16_t)((addr & 0xff00) | (nibble << 4) | ((addr >> 4) & 15));
        }

        nibble = addr & 15;
    }

    return this->romExpanded[ ((profDosFallback ? 0x4000 : 0) | (addr & 0x1fff)) & romExpandedMask];
}

auto Drive::readProfDosEncoderV1(uint16_t addr)->uint8_t {
    if ( (addr & 0xf000) == 0x8000) {
        if (!(addr & 0x0100)) {
            addr = (uint16_t)((addr & 0xff0f) | (nibble << 4));
        } else {
            addr = (uint16_t)((addr & 0xff00) | (nibble << 4) | ((addr >> 4) & 15));
        }

        nibble = addr & 15;
    }

    return this->romExpanded[ (0x2000 | (addr & 0x1fff)) & romExpandedMask ];
}

auto Drive::profDosClockControl(uint16_t addr)->void {

    profDosAutoSpeed = false;
    if (addr & 2) {
        setCycleSpeed(false);

    } else if (addr & 4) {
        setCycleSpeed(true);

    } else {
        profDosAutoSpeed = true;
        setCycleSpeed((addr & 0x4000) == 0);
    }
}

}
