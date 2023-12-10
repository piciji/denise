
#include "gameCart.h"
#include "../../system/system.h"
#include "funplay.h"
#include "ocean.h"
#include "zaxxon.h"
#include "system3.h"
#include "supergames.h"
#include "cart16k.h"
#include "magicDesk.h"
#include "../gmod/gmod2.h"
#include "simonsBasic.h"
#include "warpSpeed.h"
#include "mach5.h"
#include "ross.h"
#include "westermann.h"
#include "pagefox.h"
#include "dinamic.h"
#include "comal80.h"

namespace LIBC64 {

GameCart::GameCart(System* system, bool game, bool exrom) : Cart( system, game, exrom ) {

    setId( Interface::ExpansionIdGame );
}

auto GameCart::assign( Cart* cart ) -> void {
    bool inUse = this == system->expansionPort;

    System* ptrSystem = system;

	if (!protectFromDeletion())
		delete this;

    ptrSystem->gameCart = (GameCart*)cart;

    if (inUse)
        ptrSystem->setExpansion( Interface::ExpansionIdGame );
}

auto GameCart::create( Interface::CartridgeId cartridgeId ) -> Cart* {
    Cart* cart = nullptr;
    
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
        case Interface::CartridgeIdDefault8k:
            cart = new GameCart(system, true, false);
            break;            
            
        case Interface::CartridgeIdDefault16k:
            cart = new Cart16k(system);
            break;            
            
        case Interface::CartridgeIdUltimax:
            cart = new GameCart(system, false, true);
            break;

        case Interface::CartridgeIdGmod2:
			// we don't recreate the card because of additional complexity
            cart = system->gmod2;
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

        default:
            // forgot a rom
            cart = new GameCart(system);
            break;            

    }
    
    return cart;
}

auto GameCart::createImage(unsigned& imageSize, uint8_t id) -> uint8_t* {
	
    if (id < 2)
        return Gmod2::createImage( imageSize, id );
    
    return nullptr;
}

    
}

