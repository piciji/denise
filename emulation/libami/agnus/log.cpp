
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
        case BUS_USAGE_COPPER: system->interface->log("cop",0); break;
        case BUS_USAGE_SPRITE: system->interface->log("spr",0); break;
        case BUS_USAGE_DMAL: system->interface->log("dml",0); break;
        case BUS_USAGE_CPU: system->interface->log("cpu",0); break;
        case BUS_USAGE_REFRESH: system->interface->log("rfs",0); break;
    }

}

inline auto Agnus::logDmaCondition() -> bool {
    //if (vPos == 232 && (hPos >= 120 || hPos == 0 || hPos == 1) ) return true;
    if (vPos == 79) return true;

    return false;
}

}