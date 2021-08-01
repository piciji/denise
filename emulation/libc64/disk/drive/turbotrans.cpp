
#include "drive1541.h"

namespace LIBC64 {

auto Drive1541::turboTransWriteControl(uint16_t addr, uint8_t data) -> bool {
    if ((addr & 0xf800) == 0x1000) {
        turboTransVisible |= 1;
        return true;
    }
    else if ((addr & 0xf800) == 0x800) {
        turboTransVisible &= ~1;
        return true;
    }
    else if ((addr & 0xf800) == 0x2800) {
        turboTransVisible |= 2;
        return true;
    }
    else if ((addr & 0xf800) == 0x3000) {
        turboTransVisible &= ~2;
        return true;
    }
    else if ((addr & 0xf800) == 0x4800) {
        turboTransPage = 0;
        return true;
    }
    else if ((addr & 0xf800) == 0x5000) {
        turboTransPage++;
        return true;
    }
    else if (turboTransVisible & 2) {
        if ((addr & 0xf800) == 0x6800) {

                turboTrans[(turboTransPage << 10) | (addr & 0x3ff)] = data;
            return true;
        }
        else if ((addr & 0xf800) == 0x7000) {

            turboTrans[ 0x40000 | ((turboTransPage << 10) | (addr & 0x3ff))] = data;
            return true;
        }
    }

    if ((expandMemory & (uint8_t) ExpandedMemMode::MA0) && ((addr & 0xe000) == 0xA000)) {
        if (turboTransVisible & 1) {
            this->ramA0ToBF[addr & 0x1fff] = data;
        }
        return true;
    }

    return false;
}

}
