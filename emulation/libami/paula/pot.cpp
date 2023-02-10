
#include "paula.h"
#include "../input/input.h"
#include "../input/controlPort/controlPort.h"
#include "../../tools/clamp.h"

#define POT_DIR_X0 0x200
#define POT_DIR_Y0 0x800
#define POT_DIR_X1 0x2000
#define POT_DIR_Y1 0x8000

#define POT_DAT_X0 0x100
#define POT_DAT_Y0 0x400
#define POT_DAT_X1 0x1000
#define POT_DAT_Y1 0x4000

namespace LIBAMI {

auto Paula::pot0Dat() -> uint16_t {
    return (pot.cntY0 << 8) | pot.cntX0;
}

auto Paula::pot1Dat() -> uint16_t {
    return (pot.cntY1 << 8) | pot.cntX1;
}

auto Paula::potGoR() -> uint16_t {
    uint16_t out = 0;

    if (pot.go & POT_DIR_X0) pot.capX0 = input.controlPort1->getPotX();
    if (pot.go & POT_DIR_Y0) pot.capY0 = input.controlPort1->getPotY();
    if (pot.go & POT_DIR_X1) pot.capX1 = input.controlPort2->getPotX();
    if (pot.go & POT_DIR_Y1) pot.capY1 = input.controlPort2->getPotY();

    if (pot.capX0 == 255) out |= POT_DAT_X0;
    if (pot.capY0 == 255) out |= POT_DAT_Y0;
    if (pot.capX1 == 255) out |= POT_DAT_X1;
    if (pot.capY1 == 255) out |= POT_DAT_Y1;

    return out;
}

auto Paula::potGo(uint16_t value) -> void {
    pot.go = value;

    if (pot.go & POT_DIR_X0) pot.capX0 = (pot.go & POT_DAT_X0) ? 255 : 0;
    if (pot.go & POT_DIR_Y0) pot.capY0 = (pot.go & POT_DAT_Y0) ? 255 : 0;
    if (pot.go & POT_DIR_X1) pot.capX1 = (pot.go & POT_DAT_X1) ? 255 : 0;
    if (pot.go & POT_DIR_Y1) pot.capY1 = (pot.go & POT_DAT_Y1) ? 255 : 0;

    if (value & 1) {
        pot.cntY0 = pot.cntX0 = pot.cntY1 = pot.cntX1 = 0;
        pot.dischargeCounter = agnus.ntsc ? 7 : 8;
        pot.running = true;
    }
}

auto Paula::progressPot() -> void {
    uint16_t sum;

    if (pot.dischargeCounter) {
        if (--pot.dischargeCounter)  {
            // probably not fully discharged after a single line
            if ((pot.go & POT_DIR_X0) == 0) pot.capX0 = 0;
            if ((pot.go & POT_DIR_Y0) == 0) pot.capY0 = 0;
            if ((pot.go & POT_DIR_X1) == 0) pot.capX1 = 0;
            if ((pot.go & POT_DIR_Y1) == 0) pot.capY1 = 0;
        }
    } else {
        pot.running = false;
        // charge unit is not connected in output mode.
        // charge/discharge fast in output mode, hence counter is free running in output + lo and stopped in output + hi
        if (pot.capX0 != 255) {
            pot.cntX0++;
            if ((pot.go & POT_DIR_X0) == 0) {
                sum = pot.capX0 + input.controlPort1->getPotX();
                pot.capX0 = sum > 0xff ? 0xff : sum;
            }
            pot.running = true;
        }
        if (pot.capY0 != 255) {
            pot.cntY0++;
            if ((pot.go & POT_DIR_Y0) == 0) {
                sum = pot.capY0 + input.controlPort1->getPotY();
                pot.capY0 = sum > 0xff ? 0xff : sum;
            }
            pot.running = true;
        }
        if (pot.capX1 != 255) {
            pot.cntX1++;
            if ((pot.go & POT_DIR_X1) == 0) {
                sum = pot.capX1 + input.controlPort2->getPotX();
                pot.capX1 = sum > 0xff ? 0xff : sum;
            }
            pot.running = true;
        }
        if (pot.capY1 != 255) {
            pot.cntY1++;
            if ((pot.go & POT_DIR_Y1) == 0) {
                sum = pot.capY1 + input.controlPort2->getPotY();
                pot.capY1 = sum > 0xff ? 0xff : sum;
            }
            pot.running = true;
        }
    }
}

}
