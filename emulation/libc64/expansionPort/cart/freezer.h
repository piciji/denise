
#pragma once

#include "../../interface.h"
#include "cart.h"
#include "../../../tools/rand.h"

namespace LIBC64 {
    
struct Freezer : Cart {
    
    Freezer(bool game, bool exrom) : Cart( game, exrom ) {}
    
    unsigned cyclesTillFreeze = 0;
    bool freezeArmed = false;
    
    bool unbeatable = false;
    unsigned writesInARow = 0;
    
    auto hasFreezeButton() -> bool { return true; }            
    
    virtual auto isBootable( ) -> bool { return false; }
    
    virtual auto didFreeze() -> void {}
    virtual auto blockFreeze() -> bool { return false; } 
    
    auto freeze() -> void {
        if (blockFreeze())
            return;
        // UI events are processed only one time between frames.
        // real freeze trigger could happen at any frame position.
        cyclesTillFreeze = Emulator::Rand::rand( 1, vicII->cyclesPerFrame() );
        freezeArmed = false;
        writesInARow = 0;
    }

    virtual auto clock() -> void {

        if (freezeArmed && metCondition()) {
            exRom = true;
            game = false;
            system->changeExpansionPortMemoryMode(exRom, game);
            freezeArmed = false;
            writesInARow = 0;
            didFreeze();
        }

        if (cyclesTillFreeze && (--cyclesTillFreeze == 0)) {
            nmiCall(true);
            if (unbeatable)
                irqCall(true);

            freezeArmed = true;
        }
    }
    
    virtual auto cycleLo() -> void {

        if (!freezeArmed)
            return;
        
        if (metCondition()) {
            exRom = true;
            game = false;
            system->changeExpansionPortMemoryMode(exRom, game);
            freezeArmed = false;
            writesInARow = 0;
            didFreeze();
        }                    
    }

    virtual auto cycleHi() -> void {

        if (cyclesTillFreeze && (--cyclesTillFreeze == 0) ) {
            nmiCall( true );    
            if (unbeatable)
                irqCall( true );
            
            freezeArmed = true;
        }
    }
    
    auto metCondition() -> bool {
        // cart has already sent NMI.
        // now cart listen at address bus till NMI vector 0xfffa is placed on bus.
        // cart pulls exrom and PLA switches to ULTIMAX mode.
        // in ULTIMAX mode the NMI vector points to cart and so it can take over control.
        uint16_t _addr = system->cpu->addressBus();
        bool _write = system->cpu->isWriteCycle();
        
        if (!unbeatable) {
            if (!_write && (_addr == 0xfffa))
                return true;               
            
        } else {
            if (!_write) {
                if (writesInARow == 3)
                    return true;
                
                writesInARow = 0;
                return false;                
            }
            
            if ( (_addr & 0xff00) == 0x0100 ) // check for stack write
                writesInARow++;         
        }
        
        return false;        
    }
    
    virtual auto serializeStep2(Emulator::Serializer& s) -> void {

        Cart::serializeStep2( s );

        s.integer( cyclesTillFreeze );
        s.integer( freezeArmed );
        s.integer( unbeatable );
        s.integer( writesInARow );
    }
    
};    
    

}