
#include "system.h"
#include "../expansionPort/gameCart/gameCart.h"

namespace LIBC64 {
    
auto System::loadCartridge( Interface::CartridgeId cartridgeId, uint8_t* data, unsigned size ) -> void {
    
    unloadCartridge( false );
    
    switch(cartridgeId) {
        case Interface::CartridgeId::None:
            break;
            
        case Interface::CartridgeId::Reu:
            break;
            
        default:
            expansionPort = GameCart::getInstance( cartridgeId, data, size );
            break;
    }
    
    if (!expansionPort)
        expansionPort = new ExpansionPort;
}

auto System::unloadCartridge(bool reinit) -> void {
        
    if (expansionPort)
        delete expansionPort;
    
    expansionPort = nullptr;
    // noting connected in expansion port
    if (reinit)
        expansionPort = new ExpansionPort;
}

}
