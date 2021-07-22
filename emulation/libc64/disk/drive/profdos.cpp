
#include "drive1541.h"

namespace LIBC64 {

auto Drive1541::readProfDosEncoder(uint16_t addr)->uint8_t {
    if ( (addr & 0xf000) == 0x7000) {
        if (!(addr & 0x0800)) {
            addr = (uint16_t)((addr & 0xff0f) | (nibble << 4));
        } else {
            addr = (uint16_t)((addr & 0xff00) | (nibble << 4) | ((addr >> 4) & 15));
        }

        nibble = addr & 15;
    }

    return this->romExpanded[addr & romExpandedMask];
}

auto Drive1541::readProfDosEncoderV1(uint16_t addr)->uint8_t {
    if ( (addr & 0xf000) == 0x8000) {
        if (!(addr & 0x0100)) {
            addr = (uint16_t)((addr & 0xff0f) | (nibble << 4));
        } else {
            addr = (uint16_t)((addr & 0xff00) | (nibble << 4) | ((addr >> 4) & 15));
        }

        nibble = addr & 15;
    }

    return this->romExpanded[ 0x2000 + (addr & romExpandedMask) ];
}

auto Drive1541::profDosClockControl(uint16_t addr)->void {

    profDosAutoSpeed = false;
    if (addr & 2) {
        if (refCyclesInCpuCycle == 8) {
            updateCycleSpeed(false, false);
        //    system->interface->log("force 1 mhz", 1);
        }
    } else if (addr & 4) {
        if (refCyclesInCpuCycle == 16) {
            updateCycleSpeed(true, false);
          //  system->interface->log("force 2 mhz", 1);
        }
    } else {
        profDosAutoSpeed = true;
        profDosAutoClockControl(addr);
    }
}

auto Drive1541::profDosAutoClockControl(uint16_t addr)->void {

    bool mhz2 = (addr & 0x4000) == 0;

    if (mhz2) {
        if (refCyclesInCpuCycle == 16) {
            updateCycleSpeed(true, false);
        //    system->interface->log("auto 2", 1);
        }
    } else {
        if (refCyclesInCpuCycle == 8) {
            updateCycleSpeed(false, false);
          //  system->interface->log("auto 1", 1);
        }
    }
}

}
