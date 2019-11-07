
#include "actionReplay.h"
#include "../../system/system.h"
#include "actionReplayMK2.h"
#include "actionReplayMK3.h"
#include "actionReplayMK4.h"
#include "actionReplayV4.h"
#include "../../../tools/rand.h"

namespace LIBC64 {

ActionReplay* actionReplay = nullptr;
    
ActionReplay::ActionReplay(bool game, bool exrom) : Cart( game, exrom ) {

    setId( Interface::ExpansionIdActionReplay );
}

auto ActionReplay::freeze() -> void {
    
    // UI events are processed only one time between frames.
    // real freeze trigger could happen at any frame position.
    cyclesTillFreeze = Emulator::Rand::rand( 1, system->ntsc ? C64_CYCLES_FRAME_NTSC : C64_CYCLES_FRAME_PAL );
    freezeArmed = false;
}

auto ActionReplay::cycleLo() -> void {

    if (freezeArmed) {
        // cart has already sent NMI.
        // now cart listen at address bus till NMI vector 0xfffa is placed on bus.
        // cart pulls exrom and PLA switches to ULTIMAX mode.
        // in ULTIMAX mode the NMI vector points to cart and so it can take over control.
        if (!system->cpu->isWriteCycle()) {
            if (system->cpu->addressBus() == 0xfffa) {
                exRom = true;
                game = false;
                system->changeExpansionPortMemoryMode(exRom, game);
                freezeArmed = false;
                didFreeze();
            }
        }
    }
}

auto ActionReplay::cycleHi() -> void {
         
    if (cyclesTillFreeze && (--cyclesTillFreeze == 0) ) {
        nmiCall( true );    
        freezeArmed = true;
    }
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
        default:
            cart = new ActionReplayV4;
            break;                     
    }
    
    return cart;
}

auto ActionReplay::serializeStep2(Emulator::Serializer& s) -> void {
    
    Cart::serializeStep2( s );

    s.integer( cyclesTillFreeze );
    s.integer( freezeArmed );
}
    
}
