
#include "retroReplay.h"
#include "../../system/system.h"

namespace LIBC64 {

RetroReplay* retroReplay = nullptr;
    
RetroReplay::RetroReplay(bool game, bool exrom) : Freezer( game, exrom ) {

    setId( Interface::ExpansionIdRetroReplay );
}

auto RetroReplay::assign( Cart* cart ) -> void {
    bool inUse = this == system->expansionPort;

    delete this;

    retroReplay = (RetroReplay*)cart;
    
    system->setExpansionCallbacks( retroReplay );

    if (inUse)            
        system->expansionPort = retroReplay;
    
}

auto RetroReplay::create( Interface::CartridgeId cartridgeId ) -> Cart* {
    Cart* cart = nullptr;
    
    switch(cartridgeId) {

        case Interface::CartridgeIdRetroReplay:
            cart = new RetroReplay;
            break;
            
        case Interface::CartridgeIdNordicReplay:
            //cart = new ActionReplayMK3;
            break;
            
        default:
            // forgot a rom
            cart = new RetroReplay;
            break;
    }
    
    return cart;
}

auto RetroReplay::writeIo1( uint16_t addr, uint8_t value ) -> void {
    
    if (!enabled)
        return;
    
    
    
    switch(addr & 0xff) {
        case 0:
            exRom = (value >> 1) & 1;
            game = (value & 1) ^ 1;

            if (flashJumper && !(game && exRom) ) { // flash mode + cartridge mode
                // 8k mode
                game = true;
                exRom = false;
            }
            
            bank = ((value >> 3) & 3) | ((value >> 5) & 4);
            
            ramMode = !!(value & 0x20);
            
            if (value & 0x40) {
                frozen = false;
            }
            
            if (frozen) {
                // keep Ultimax
                exRom = true;
                game = false;
            }
            
            if (value & 4) { // Feierabend
                active = false;
                exRom = true;
                game = true;
            }
            
            system->changeExpansionPortMemoryMode(exRom, game);
            
            break;
        case 1:
            break;
    }
}

}
