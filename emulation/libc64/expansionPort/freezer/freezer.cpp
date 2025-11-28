
#include "freezer.h"
#include "../../system/system.h"
#include "actionReplayMK2.h"
#include "actionReplayMK3.h"
#include "actionReplayMK4.h"
#include "actionReplayV4.h"
#include "finalCartridge.h"
#include "finalCartridge3.h"
#include "finalCartridgePlus.h"
#include "atomicPower.h"
#include "diashowMaker.h"
#include "superSnapshotV5.h"
#include "kcsPower.h"

namespace LIBC64 {

Freezer::Freezer(System* system, bool game, bool exrom) : FreezeButton( system, game, exrom ) {

    setId( Interface::ExpansionIdFreezer );
}

auto Freezer::assign( Cart* cart ) -> void {
    bool inUse = this == system->expansionPort;
    System* ptrSystem = system;

    delete this;

    ptrSystem->freezer = (Freezer*)cart;

    ptrSystem->setExpansionCallbacks( ptrSystem->freezer );

    if (inUse)
        ptrSystem->setExpansion( Interface::ExpansionIdFreezer );
}

auto Freezer::create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Cart* {
    Cart* cart = nullptr;
    
    switch(cartridgeId) {

        case Interface::CartridgeIdActionReplayMK2:
            cart = new ActionReplayMK2(system);
            break;
            
        case Interface::CartridgeIdActionReplayMK3:
            cart = new ActionReplayMK3(system);
            break;
            
        case Interface::CartridgeIdActionReplayMK4:
            cart = new ActionReplayMK4(system);
            break;

        case Interface::CartridgeIdActionReplayV41AndHigher:
        case Interface::CartridgeIdDefault:
            cart = new ActionReplayV4(system);
            break;

        case Interface::CartridgeIdFinalCartridge:
            cart = new FinalCartridge(system);
            break;

        case Interface::CartridgeIdFinalCartridge3:
            cart = new FinalCartridge3(system);
            break;

        case Interface::CartridgeIdFinalCartridgePlus:
            cart = new FinalCartridgePlus(system);
            break;

        case Interface::CartridgeIdAtomicPower:
            cart = new AtomicPower(system);
            break;

        case Interface::CartridgeIdKcsPower:
            cart = new KCSPower(system);
            break;

        case Interface::CartridgeIdDiashowMaker:
            cart = new DiashowMaker(system);
            break;

        case Interface::CartridgeIdSuperSnapshotV5:
            cart = new SuperSnapshotV5(system);
            break;

        default:
            // forgot a rom
            cart = new Freezer(system);
            break;
    }
    
    return cart;
}
  
}

