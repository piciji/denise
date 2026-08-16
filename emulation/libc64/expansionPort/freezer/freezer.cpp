
#include "freezer.h"
#include "../../system/system.h"
#include "actionReplayMK2.h"
#include "actionReplayMK3.h"
#include "actionReplayMK4.h"
#include "actionReplayV4.h"
#include "finalCartridge.h"
#include "finalCartridge3.h"
#include "finalCartridgePlus.h"
#include "atomicPower.h"
#include "diashowMaker.h"
#include "superSnapshotV5.h"
#include "kcsPower.h"

namespace LIBC64 {

Freezer::Freezer(System* system, bool game, bool exrom) : FreezeButton( system, game, exrom ) {

    setId( Interface::ExpansionIdFreezer );
}

auto Freezer::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {

    if ( (this->rom == nullptr) && (rom == nullptr) )
        return;

    auto _cartridgeId = (media && media->pcbLayout) ? media->pcbLayout->id : 0;

    auto newCart = build( (Interface::CartridgeId)_cartridgeId, rom, romSize );

    newCart->media = media;

    assign( newCart );
}

auto Freezer::build( Interface::CartridgeId cartridgeId, uint8_t* _rom, unsigned _romSize ) -> Freezer* {

    if (!_rom || (_romSize == 0) )
        cartridgeId = Interface::CartridgeIdNoRom;

    Freezer* cart = create( cartridgeId, _romSize );

    cart->rom = _rom;
    cart->romSize = _romSize;

    if( cart->readHeader( ) ) {

        if (cart->cartridgeId != cartridgeId) {
            cartridgeId = cart->cartridgeId;
            // if user doesn't request a specific cart and analyzing header detects a specific cart
            delete cart;
            // lets recreate by detected type
            return build( cartridgeId, _rom, _romSize );
        }
    } else
        cart->cartridgeId = cartridgeId;

    if ( !cart->readChips() ) {
        // no chip headers found, we assume it by user requested type
        cart->assumeChips();
    }

    return cart;
}

auto Freezer::serialize(Emulator::Serializer& s) -> void {

    unsigned _cartridgeId = cartridgeId;
    s.integer(_cartridgeId);

    if (s.mode() == Emulator::Serializer::Mode::Load) {

        if (cartridgeId != _cartridgeId) {
            auto cart = create( (Interface::CartridgeId)_cartridgeId, 0 );

            if (_cartridgeId != Interface::CartridgeIdNoRom) {
                cart->rom = rom;
                cart->romSize = romSize;
                cart->readHeader();
            }

            cart->cartridgeId = (Interface::CartridgeId)_cartridgeId;
            if (!cart->readChips())
                cart->assumeChips();

            assign( cart );
            cart->serializeSwitchedIn( s );

            return;
        }
    }

    serializeSwitchedIn( s );
}

auto Freezer::serializeSwitchedIn(Emulator::Serializer& s) -> void {
    FreezeButton::serialize( s );
}

auto Freezer::assign( Freezer* cart ) -> void {
    bool inUse = this == system->expansionPort;
    System* ptrSystem = system;

    delete this;

    ptrSystem->freezer = cart;

    ptrSystem->setExpansionCallbacks( ptrSystem->freezer );

    if (inUse)
        ptrSystem->setExpansion( Interface::ExpansionIdFreezer );
}

auto Freezer::create( Interface::CartridgeId cartridgeId, unsigned _size ) -> Freezer* {
    Freezer* cart = nullptr;
    
    switch(cartridgeId) {

        case Interface::CartridgeIdActionReplayMK2:
            cart = new ActionReplayMK2(system);
            break;
            
        case Interface::CartridgeIdActionReplayMK3:
            cart = new ActionReplayMK3(system);
            break;
            
        case Interface::CartridgeIdActionReplayMK4:
            cart = new ActionReplayMK4(system);
            break;

        case Interface::CartridgeIdActionReplayV41AndHigher:
        case Interface::CartridgeIdDefault:
            cart = new ActionReplayV4(system);
            break;

        case Interface::CartridgeIdFinalCartridge:
            cart = new FinalCartridge(system);
            break;

        case Interface::CartridgeIdFinalCartridge3:
            cart = new FinalCartridge3(system);
            break;

        case Interface::CartridgeIdFinalCartridgePlus:
            cart = new FinalCartridgePlus(system);
            break;

        case Interface::CartridgeIdAtomicPower:
            cart = new AtomicPower(system);
            break;

        case Interface::CartridgeIdKcsPower:
            cart = new KCSPower(system);
            break;

        case Interface::CartridgeIdDiashowMaker:
            cart = new DiashowMaker(system);
            break;

        case Interface::CartridgeIdSuperSnapshotV5:
            cart = new SuperSnapshotV5(system);
            break;

        default:
            // forgot a rom
            cart = new Freezer(system);
            break;
    }
    
    return cart;
}
  
}

