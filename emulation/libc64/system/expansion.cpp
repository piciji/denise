
#include "system.h"
#include "../expansionPort/gameCart/gameCart.h"
#include "../expansionPort/reu/reu.h"
#include "../expansionPort/actionReplay/actionReplay.h"
#include "../expansionPort/easyFlash/easyFlash.h"
#include "../expansionPort/retroReplay/retroReplay.h"

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
            
        case Interface::ExpansionIdActionReplay:
            expansionPort = actionReplay;
            break;
            
        case Interface::ExpansionIdEasyFlash:
            expansionPort = easyFlash;
            break;
            
        case Interface::ExpansionIdRetroReplay:
            expansionPort = retroReplay;
            break;
    }
    
}  
    
auto System::createExpansions() -> void {
    
    reu = new Reu;
    gameCart = new GameCart;
    actionReplay = new ActionReplay;
    easyFlash = new EasyFlash;
    retroReplay = new RetroReplay;
    noExpansion = new ExpansionPort;    
    
    expansionPort = noExpansion;
    
    setExpansionCallbacks( reu );
    setExpansionCallbacks( actionReplay );
    setExpansionCallbacks( retroReplay );
}

auto System::destroyExpansions() -> void {
    
    delete reu;
    delete gameCart;
    delete actionReplay;
    delete easyFlash;
    delete retroReplay;
    delete noExpansion;
}

auto System::analyzeExpansion(uint8_t* data, unsigned size) -> Emulator::Interface::Expansion* {
    
    auto cart = new Cart;
    cart->rom = data;
    cart->romSize = size;
    Emulator::Interface::Expansion* useExpansion = &interface->expansions[Interface::ExpansionIdGame];
    
    if (!cart->readHeader())
        goto end;
    
    switch(cart->cartridgeId) {
        case Interface::CartridgeIdActionReplayMK2:
        case Interface::CartridgeIdActionReplayMK3:
        case Interface::CartridgeIdActionReplayMK4:
        case Interface::CartridgeIdActionReplayV41AndHigher:
            useExpansion = &interface->expansions[Interface::ExpansionIdActionReplay];
            break; 
        case Interface::CartridgeIdEasyFlash:
            useExpansion = &interface->expansions[Interface::ExpansionIdEasyFlash];
            break;
        case Interface::CartridgeIdRetroReplay:
            useExpansion = &interface->expansions[Interface::ExpansionIdRetroReplay];
            break;
    }    
    
    end:
            
    delete cart;
    return useExpansion;
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
