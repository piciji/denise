
#pragma once

#include "../../interface.h"
#include "cart.h"
#include "../../../tools/rand.h"

namespace LIBC64 {
    
struct FreezeButton : Cart {

    FreezeButton(System* system, bool game, bool exrom) : Cart( system, game, exrom ) {}
    
    unsigned cyclesTillFreeze = 0;
    bool freezeArmed = false;
    Emulator::Rand randomizer;
    
    uint8_t unbeatable = 0;
    unsigned writesInARow = 0;
    
    auto hasFreezeButton() -> bool override { return true; }

    auto isBootable( ) -> bool override { return false; }
    
    virtual auto didFreeze() -> void {}
    virtual auto blockFreeze() -> bool { return false; }
    virtual auto arm() -> bool { return false; }
    virtual auto switchToUltimax() -> bool { return true; }
    
    auto freeze() -> void override {
        if (blockFreeze())
            return;
        // UI events are processed only one time between frames.
        // real freeze trigger could happen at any frame position.
        cyclesTillFreeze = randomizer.xorShift( 1, vicII->cyclesPerFrame() );
        freezeArmed = false;
        writesInARow = 0;
    }

    auto clock() -> void override {

        if (freezeArmed && conditionMet()) {
            if (switchToUltimax())
                system->changeExpansionPortMemoryMode(exRom = true, game = false );

            freezeArmed = arm();
            writesInARow = 0;
            didFreeze();
        }

        if (cyclesTillFreeze && (--cyclesTillFreeze == 0)) {
            nmiCall(true);
            if (unbeatable & 2)
                irqCall(true);

            freezeArmed = true;
        }
    }
    
    auto conditionMet() -> bool {
        // cart has already sent NMI.
        // now cart listen at address bus till NMI vector 0xfffa is placed on bus.
        // cart pulls exrom and PLA switches to ULTIMAX mode.
        // in ULTIMAX mode the NMI vector points to cart and so it can take over control.
        uint16_t _addr = system->cpu.addressBus();
        bool _write = system->cpu.isWriteCycle();
        
        if ((unbeatable & 1) == 0) {
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
    
    auto serialize(Emulator::Serializer& s) -> void override {

        Cart::serialize( s );

        s.integer( cyclesTillFreeze );
        s.integer( freezeArmed );
        s.integer( unbeatable );
        s.integer( writesInARow );
        s.integer( randomizer.xorShift32 );
    }

    auto serializeNoCart(Emulator::Serializer& s) -> void {

        s.integer( cyclesTillFreeze );
        s.integer( freezeArmed );
        s.integer( writesInARow );
        s.integer( randomizer.xorShift32 );
        
        ExpansionPort::serialize(s);
    }

    auto resetFreeze() -> void {
        freezeArmed = false;
        cyclesTillFreeze = 0;
        randomizer.initXorShift();
    }
    
};    
    

}