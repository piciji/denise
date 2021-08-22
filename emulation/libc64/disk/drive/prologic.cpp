
#include "drive.h"

namespace LIBC64 {

auto Drive::prologicControlClassic(uint8_t addr, uint8_t data) -> void {

    if (addr & 1) {
        prologic40TrackMode = (data & 8) == 0;
        //system->interface->log("40 t");
        //system->interface->log( prologic40TrackMode, 0 );
    } else {
        prologic2Mhz = data;

        bool mhz2 = (data & 1) == 0;

        if (mhz2) {
            if (refCyclesInCpuCycle == 16) {
                updateCycleSpeed(true, false);
            }
        } else {
            if (refCyclesInCpuCycle == 8) {
                updateCycleSpeed(false, false);
            }
        }
    }
}

auto Drive::prologicControl(uint16_t addr) -> void {

    if ((addr & 0xf000) == 0xb000) {
        prologic40TrackMode = addr & 0x20;

    } else  if ((addr & 0xf000) == 0xa000) {
        prologic2Mhz = (addr & 0x20) ? 1 : 0;

        if (prologic2Mhz) {
            if (refCyclesInCpuCycle == 16) {
               // system->interface->log("2 mhz");
                updateCycleSpeed(true, false);
            }
        } else {
            if (refCyclesInCpuCycle == 8) {
               // system->interface->log("1 mhz");
                updateCycleSpeed(false, false);
            }
        }
    }
}


}