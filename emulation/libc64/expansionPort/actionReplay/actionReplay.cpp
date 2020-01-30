
#include "actionReplay.h"
#include "../../system/system.h"
#include "actionReplayMK2.h"
#include "actionReplayMK3.h"
#include "actionReplayMK4.h"
#include "actionReplayV4.h"

namespace LIBC64 {

ActionReplay* actionReplay = nullptr;
    
ActionReplay::ActionReplay(bool game, bool exrom) : Freezer( game, exrom ) {

    setId( Interface::ExpansionIdActionReplay );
}

auto ActionReplay::assign( Cart* cart ) -> void {
    bool inUse = this == system->expansionPort;

    delete this;

    actionReplay = (ActionReplay*)cart;
    
    system->setExpansionCallbacks( actionReplay );

    if (inUse)            
        system->expansionPort = actionReplay;
    
}

auto ActionReplay::create( Interface::CartridgeId cartridgeId ) -> Cart* {
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
            
        default:
            // forgot a rom
            cart = new ActionReplay;
            break;
    }
    
    return cart;
}
  
}

