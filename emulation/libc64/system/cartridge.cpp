
#include "system.h"
#include "../expansionPort/gameCart/gameCart.h"
#include "../expansionPort/reu/reu.h"

namespace LIBC64 {
    
auto System::loadCartridge( Interface::CartridgeId cartridgeId, uint8_t* data, unsigned size ) -> void {
    
    unloadCartridge( false );
    
    switch(cartridgeId) {
        case Interface::CartridgeId::None:
            break;
            
        case Interface::CartridgeId::Reu:
            expansionPort = new Reu( size );            
            break;
            
        default:
            expansionPort = GameCart::getInstance( cartridgeId, data, size );
            break;
    }
    
    if (!expansionPort)
        expansionPort = new ExpansionPort;

    expansionPort->irqCall = [this](bool state) {
        if (state)
            nmiIncomming |= 4;
        else
            nmiIncomming &= ~4;

        cpu->setNmi(nmiIncomming != 0);
    };
    
    expansionPort->nmiCall = [this](bool state) {
        if (state)
            irqIncomming |= 4;
        else
            irqIncomming &= ~4;

        cpu->setIrq(irqIncomming != 0);
    };

    expansionPort->dmaCall = [this](bool state) {
        if (state)
            rdyIncomming |= 2;
        else
            rdyIncomming &= ~2;      
        
        cpu->setRdy( rdyIncomming != 0 );
    };
    
    
    if (cartridgeId == Interface::CartridgeId::Reu)
        expansionPort->vicBA = [this]() {   

            return vicII->reuBaLow();
        };
    else
        expansionPort->vicBA = [this]() {   

            return vicII->isBaLow();
        };
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
