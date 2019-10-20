
#include "system.h"
#include "../expansionPort/gameCart/gameCart.h"
#include "../expansionPort/reu/reu.h"

namespace LIBC64 {
 
auto System::serializeExpansion(Emulator::Serializer& s) -> void {
    
    s.integer( (unsigned&)expansionPort->id );
    
    s.integer( (unsigned&)expansionPort->cartridgeId );
       
    if ( s.mode() == Emulator::Serializer::Mode::Load ) {

        uint8_t* rom = expansionPort->rom;
        unsigned romSize = expansionPort->romSize;
        Interface::CartridgeId cartridgeId = expansionPort->cartridgeId;
        
        auto expansion = interface->getExpansionById( expansionPort->id );

        setExpansion( expansion ? *expansion : interface->expansions[0] );
        
        expansionPort->setCartridgeId( cartridgeId );
        expansionPort->setRom( rom, romSize );
    }
    
    expansionPort->serialize( s );
}
    
auto System::unsetExpansion() -> void {
    if (expansionPort)
        delete expansionPort;
    
    expansionPort = new ExpansionPort;
}    
    
auto System::setExpansion( Emulator::Interface::Expansion& expansion ) -> void {
    
    if (expansionPort)
        delete expansionPort;
    
    switch(expansion.id) {
        default:
        case Interface::ExpansionIdNone:
            expansionPort = new ExpansionPort;
            return; // no callbacks needed
            
        case Interface::ExpansionIdGame:
            expansionPort = new Reu;
            return; // no callbacks needed
            
        case Interface::ExpansionIdReu:
            expansionPort = new GameCart;
            break;
    }
    // arm callbacks
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
    
    if (expansion.id == Interface::ExpansionIdReu)
        
        expansionPort->vicBA = [this]() {   

            return vicII->reuBaLow();
        };
    else
        expansionPort->vicBA = [this]() {   

            return vicII->isBaLow();
        };
}  
    
}
