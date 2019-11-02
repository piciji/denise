
#include "system.h"
#include "../expansionPort/gameCart/gameCart.h"
#include "../expansionPort/reu/reu.h"

namespace LIBC64 {
 
auto System::serializeExpansion(Emulator::Serializer& s) -> void {
    
    unsigned expansionPortId = expansionPort->id;
    
    s.integer( expansionPortId );  
       
    if ( s.mode() == Emulator::Serializer::Mode::Load ) {
        
        auto expansion = interface->getExpansionById( expansionPortId );

        setExpansion( expansion ? *expansion : interface->expansions[0] );
    }
    
    expansionPort->serialize( s );
}
    
auto System::setExpansion( Emulator::Interface::Expansion& expansion ) -> void {
    
    switch(expansion.id) {
        default:
        case Interface::ExpansionIdNone:
            expansionPort = noExpansion;
            break;
            
        case Interface::ExpansionIdGame:
            expansionPort = gameCart;
            break;
            
        case Interface::ExpansionIdReu:
            expansionPort = reu;
            break;
    }
    
}  
    
auto System::createExpansions() -> void {
    
    reu = new Reu( &events );
    gameCart = new GameCart;
    noExpansion = new ExpansionPort;
    
    expansionPort = noExpansion;
    
    setExpansionCallbacks( reu );
}

auto System::destroyExpansions() -> void {
    
    delete reu;
    delete gameCart;
    delete noExpansion;
}

auto System::setExpansionCallbacks( ExpansionPort* expansionPtr ) -> void {
        
    expansionPtr->nmiCall = [this](bool state) {
        if (state)
            nmiIncomming |= 4;
        else
            nmiIncomming &= ~4;

        cpu->setNmi(nmiIncomming != 0);
    };
    
    expansionPtr->irqCall = [this](bool state) {
        if (state)
            irqIncomming |= 4;
        else
            irqIncomming &= ~4;

        cpu->setIrq(irqIncomming != 0);
    };

    expansionPtr->dmaCall = [this](bool state) {
        if (state)
            rdyIncomming |= 2;
        else
            rdyIncomming &= ~2;      
        
        cpu->setRdy( rdyIncomming != 0 );
    };
    
    if (expansionPtr->id == Interface::ExpansionIdReu)
        
        expansionPtr->vicBA = [this]() {   

            return vicII->reuBaLow();
        };
    else
        expansionPtr->vicBA = [this]() {   

            return vicII->isBaLow();
        };
}
    
}
