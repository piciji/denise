
#include "agnus.h"

namespace LIBAMI {

auto Agnus::logDmaUsage(bool waitForCpu) -> void {
    if (!logDmaCondition())
        return;

    if (!waitForCpu)
        system->interface->log(hPos,1,1);

    switch(busUsage) {
        // case BUS_FREE: system->interface->log("free",0); break;
        case BUS_USAGE_BLITTER: system->interface->log("bli",0); break;
        case BUS_USAGE_BPL: system->interface->log("bpl",0); break;
        case BUS_USAGE_COPPER:
            system->interface->log("cop",0);
            system->interface->log((copper.ir1 & 1) ? 0 : (copper.ir1 & 0x1fe), 0, 1);
            system->interface->log( (copper.state == Copper::Read2) ? "R1" : ((copper.state == Copper::Read1) ? "R2" : std::to_string(copper.state)), 0);
            system->interface->log(vPos, 0, 1);
            break;
        case BUS_USAGE_SPRITE: system->interface->log("spr",0); break;
        case BUS_USAGE_DMAL: system->interface->log("dml",0); break;
        case BUS_USAGE_CPU: system->interface->log("cpu",0); break;
        case BUS_USAGE_REFRESH: system->interface->log("rfs",0); break;
    }

}

inline auto Agnus::logDmaCondition() -> bool {
    //if (vPos == 232 && (hPos >= 120 || hPos == 0 || hPos == 1) ) return true;
    if (vPos == 113 || vPos == 112 /* || vPos == 0x71 || vPos == 0x72  || vPos == 0x81*/ ) return true;

    return false;
}

}