
#include "system.h"
#include "../expansionPort/gameCart/gameCart.h"
#include "../expansionPort/reu/reu.h"
#include "../expansionPort/freezer/freezer.h"
#include "../expansionPort/easyFlash/easyFlash.h"
#include "../expansionPort/easyFlash/easyFlash3.h"
#include "../expansionPort/retroReplay/retroReplay.h"
#include "../expansionPort/gmod/gmod2.h"
#include "../expansionPort/geoRam/geoRam.h"
#include "../expansionPort/acia/acia.h"
#include "../expansionPort/fastloader/fastloader.h"

namespace LIBC64 {
 
auto System::serializeExpansion(Emulator::Serializer& s) -> void {
    
    unsigned expansionPortId = expansionPort->id;
    
    s.integer( expansionPortId );  
       
    if ( s.mode() == Emulator::Serializer::Mode::Load ) {
        
        if (expansionPortId != expansionPort->id)
            memoryCpu.unmap(0x0, 0xff);
        
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
			reu->setExpander( nullptr );
            break;
            
        case Interface::ExpansionIdFreezer:
            expansionPort = freezer;
            break;
            
        case Interface::ExpansionIdEasyFlash:
            expansionPort = easyFlash;
            break;

        case Interface::ExpansionIdEasyFlash3:
            expansionPort = easyFlash3;
            break;

        case Interface::ExpansionIdRetroReplay:
            expansionPort = retroReplay;
            break;
			
		case Interface::ExpansionIdReuRetroReplay:
			expansionPort = reu;
			reu->setExpander( retroReplay );
			break;
			
		case Interface::ExpansionIdGeoRam:
            expansionPort = geoRam;
            break;

        case Interface::ExpansionIdRS232:
            expansionPort = acia;
            break;

        case Interface::ExpansionIdFastloader:
            expansionPort = fastloader;
            break;
    }
    
}  
    
auto System::createExpansions() -> void {
    
    reu = new Reu;
    gameCart = new GameCart;
    freezer = new Freezer;
    easyFlash = new EasyFlash;
    easyFlash3 = new EasyFlash3;
    retroReplay = new RetroReplay;
	gmod2 = new Gmod2;
	geoRam = new GeoRam;
	acia = new Acia;
	fastloader = new Fastloader;
    noExpansion = new ExpansionPort;
    
    expansionPort = noExpansion;
    
    setExpansionCallbacks( reu );
    setExpansionCallbacks( freezer );
    setExpansionCallbacks( retroReplay );    
    setExpansionCallbacks( easyFlash3 );
    setExpansionCallbacks( acia );
}

auto System::destroyExpansions() -> void {
    
    delete reu;
    delete gameCart;
    delete freezer;
    delete easyFlash;
    delete easyFlash3;
    delete retroReplay;
	delete gmod2;
	delete geoRam;
	delete acia;
    delete noExpansion;	
}

auto System::analyzeExpansion(uint8_t* data, unsigned size, std::string suffix) -> Emulator::Interface::Expansion* {
    
    if (suffix == "reu")
        return &interface->expansions[Interface::ExpansionIdReu];
    
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
        case Interface::CartridgeIdFinalCartridge:
        case Interface::CartridgeIdFinalCartridgePlus:
        case Interface::CartridgeIdFinalCartridge3:
        case Interface::CartridgeIdAtomicPower:
            useExpansion = &interface->expansions[Interface::ExpansionIdFreezer];
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
