
#pragma once

#include "../../interface.h"
#include "cart.h"
#include "../../../tools/rand.h"

namespace LIBC64 {
    
struct Freezer : Cart {
    
    Freezer(bool game, bool exrom) : Cart( game, exrom ) {}
    
    unsigned cyclesTillFreeze = 0;
    bool freezeArmed = false;
    
    auto hasFreezeButton() -> bool { return true; }            
    
    virtual auto isBootable( ) -> bool { return false; }
    
    virtual auto didFreeze() -> void {}
    
    auto freeze() -> void {

        // UI events are processed only one time between frames.
        // real freeze trigger could happen at any frame position.
        cyclesTillFreeze = Emulator::Rand::rand( 1, system->ntsc ? C64_CYCLES_FRAME_NTSC : C64_CYCLES_FRAME_PAL );
        freezeArmed = false;
    }

    virtual auto cycleLo() -> void {

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

    virtual auto cycleHi() -> void {

        if (cyclesTillFreeze && (--cyclesTillFreeze == 0) ) {
            nmiCall( true );    
            freezeArmed = true;
        }
    }
    
    virtual auto serializeStep2(Emulator::Serializer& s) -> void {

        Cart::serializeStep2( s );

        s.integer( cyclesTillFreeze );
        s.integer( freezeArmed );
    }
    
};    
    

}