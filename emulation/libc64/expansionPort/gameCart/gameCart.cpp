
#include "gameCart.h"
#include "../../system/system.h"
#include "funplay.h"
#include "ocean.h"
#include "zaxxon.h"
#include "system3.h"
#include "supergames.h"
#include "cart16k.h"
#include "magicDesk.h"
#include "magicDesk2.h"
#include "simonsBasic.h"
#include "warpSpeed.h"
#include "mach5.h"
#include "ross.h"
#include "westermann.h"
#include "pagefox.h"
#include "dinamic.h"
#include "comal80.h"
#include "silverrock.h"
#include "easycalc.h"
#include "hyperBasic.h"
#include "businessBasic.h"
#include "rgcd.h"
#include "structuredBasic.h"
#include "prophet64.h"

namespace LIBC64 {

GameCart::GameCart(System* system, bool game, bool exrom) : Cart( system, game, exrom ) {

    setId( Interface::ExpansionIdGame );
}

auto GameCart::setRom(Emulator::Interface::Media* media, uint8_t* rom, unsigned romSize) -> void {

    if ( (this->rom == nullptr) && (rom == nullptr) )
        return;

    auto _cartridgeId = (media && media->pcbLayout) ? media->pcbLayout->id : 0;

    auto newCart = build( (Interface::CartridgeId)_cartridgeId, rom, romSize );

    newCart->media = media;

    assign( newCart );
}

auto GameCart::build( Interface::CartridgeId cartridgeId, uint8_t* _rom, unsigned _romSize ) -> GameCart* {

    if (!_rom || (_romSize == 0) )
        cartridgeId = Interface::CartridgeIdNoRom;

    GameCart* cart = create( cartridgeId, _romSize );

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

auto GameCart::serialize(Emulator::Serializer& s) -> void {

    unsigned _cartridgeId = cartridgeId;
    s.integer(_cartridgeId);

    if (s.mode() == Emulator::Serializer::Mode::Load) {

        if ( (_cartridgeId == Interface::CartridgeIdDefault) &&
                (cartridgeId == Interface::CartridgeIdDefault8k || cartridgeId == Interface::CartridgeIdDefault16k || cartridgeId == Interface::CartridgeIdUltimax ));
        // state file of a standard cartridge was created from a CRT and reloaded from a BIN.
        // don't recreate because standard CRT cart id is same for 8k, 16k and ultimax.
        // serialization frame is identical for these cartridges, so no problem
        else if (cartridgeId != _cartridgeId) {
            // cartridge id of state file mismatches with loaded one.
            // it seems the cart which was loaded while creating this save state isn't present anymore.
            // we need to recreate the expected cartridge in order to unserialize the right data.
            // probably the loaded state file is unusable but we don't want to crash the emulation
            // on top of that when data is unserialized in wrong order.

            auto cart = create( (Interface::CartridgeId)_cartridgeId, 0 );

            if (_cartridgeId != Interface::CartridgeIdNoRom) {
                cart->rom = rom;
                cart->romSize = romSize;
                cart->readHeader();
            }

            // force cart id from state.
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

auto GameCart::serializeSwitchedIn(Emulator::Serializer& s) -> void {
    Cart::serialize( s );
}

auto GameCart::assign( GameCart* cart ) -> void {
    bool inUse = this == system->expansionPort;

    System* ptrSystem = system;

    delete this;

    ptrSystem->gameCart = cart;

    if (inUse)
        ptrSystem->setExpansion( Interface::ExpansionIdGame );
}

auto GameCart::create( Interface::CartridgeId cartridgeId, unsigned _size ) -> GameCart* {
    GameCart* cart = nullptr;
    
    switch(cartridgeId) {
        case Interface::CartridgeIdFunplay:            
            cart = new Funplay(system);
            break;
        case Interface::CartridgeIdOcean:
            cart = new Ocean(system);
            break;
        case Interface::CartridgeIdSystem3:
            cart = new System3(system);
            break;
        case Interface::CartridgeIdSuperGames:
            cart = new SuperGames(system);
            break;
        case Interface::CartridgeIdZaxxon:
            cart = new Zaxxon(system);
            break;
        case Interface::CartridgeIdDefault:
            if (_size == 16384) {
                cart = new Cart16k(system);
                break;
            }
            // fall through
        case Interface::CartridgeIdDefault8k:
            cart = new GameCart(system, true, false);
            break;            
            
        case Interface::CartridgeIdDefault16k:
            cart = new Cart16k(system);
            break;            
            
        case Interface::CartridgeIdUltimax:
            cart = new GameCart(system, false, true);
            break;

        case Interface::CartridgeIdMagicDesk:
            cart = new MagicDesk(system);
            break;

        case Interface::CartridgeIdSimonsBasic:
            cart = new SimonsBasic(system);
            break;

        case Interface::CartridgeIdWarpSpeed:
            cart = new WarpSpeed(system);
            break;

        case Interface::CartridgeIdMach5:
            cart = new Mach5(system);
            break;

        case Interface::CartridgeIdRoss:
            cart = new Ross(system);
            break;

        case Interface::CartridgeIdWestermann:
            cart = new Westermann(system);
            break;

        case Interface::CartridgeIdPagefox:
            cart = new Pagefox(system);
            break;

        case Interface::CartridgeIdDinamic:
            cart = new Dinamic(system);
            break;

        case Interface::CartridgeIdComal80:
            cart = new Comal80(system);
            break;

        case Interface::CartridgeIdSilverrock:
            cart = new Silverrock(system);
            break;

        case Interface::CartridgeIdRGCD:
            cart = new RGCD(system);
            break;

        case Interface::CartridgeIdRGCDHucky: // only when user selected for BIN file
            cart = new RGCD(system);
            cart->version = 0x101;
            break;

        case Interface::CartridgeIdEasyCalc:
            cart = new EasyCalc(system);
            break;

        case Interface::CartridgeIdHyperBasic:
            if (_size < (1 * 1024 * 1024)) {
                cart = new HyperBasic(system);
                break;
            }
            // fallthrough ... CRT uses the already registered ID of HyperBasic
        case Interface::CartridgeIdMagicDesk2:
            cart = new MagicDesk2(system);
            break;

        case Interface::CartridgeIdBusinessBasic:
            cart = new BusinessBasic(system);
            break;

        case Interface::CartridgeIdStructuredBasic:
            cart = new StructuredBasic(system);
            break;

        case Interface::CartridgeIdProphet64:
            cart = new Prophet64(system);
            break;

        default:
            // forgot a rom
            cart = new GameCart(system);
            break;            

    }
    
    return cart;
}
    
}
