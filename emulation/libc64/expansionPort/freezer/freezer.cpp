
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

namespace LIBC64 {

Freezer* freezer = nullptr;

Freezer::Freezer(bool game, bool exrom) : FreezeButton( game, exrom ) {

    setId( Interface::ExpansionIdFreezer );
}

auto Freezer::assign( Cart* cart ) -> void {
    bool inUse = this == expansionPort;

    delete this;

    freezer = (Freezer*)cart;
    
    system->setExpansionCallbacks( freezer );

    if (inUse)            
        expansionPort = freezer;
    
}

auto Freezer::create( Interface::CartridgeId cartridgeId ) -> Cart* {
    Cart* cart = nullptr;
    
    switch(cartridgeId) {

        case Interface::CartridgeIdActionReplayMK2:
            cart = new ActionReplayMK2;
            break;
            
        case Interface::CartridgeIdActionReplayMK3:
            cart = new ActionReplayMK3;
            break;
            
        case Interface::CartridgeIdActionReplayMK4:
            cart = new ActionReplayMK4;
            break;

        case Interface::CartridgeIdActionReplayV41AndHigher:
        case Interface::CartridgeIdDefault:
            cart = new ActionReplayV4;
            break;

        case Interface::CartridgeIdFinalCartridge:
            cart = new FinalCartridge;
            break;

        case Interface::CartridgeIdFinalCartridge3:
            cart = new FinalCartridge3;
            break;

        case Interface::CartridgeIdFinalCartridgePlus:
            cart = new FinalCartridgePlus;
            break;

        case Interface::CartridgeIdAtomicPower:
            cart = new AtomicPower;
            break;

        default:
            // forgot a rom
            cart = new Freezer;
            break;
    }
    
    return cart;
}
  
}

