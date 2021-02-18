
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

namespace LIBC64 {

GameCart* gameCart = nullptr;
    
GameCart::GameCart(bool game, bool exrom) : Cart( game, exrom ) {

    setId( Interface::ExpansionIdGame );
}

auto GameCart::assign( Cart* cart ) -> void {
    bool inUse = this == expansionPort;

	if (!protectFromDeletion())
		delete this;

    gameCart = (GameCart*)cart;

    if (inUse)            
        expansionPort = gameCart;
    
}

auto GameCart::create( Interface::CartridgeId cartridgeId ) -> Cart* {
    Cart* cart = nullptr;
    
    switch(cartridgeId) {
        case Interface::CartridgeIdFunplay:            
            cart = new Funplay;
            break;
        case Interface::CartridgeIdOcean:
            cart = new Ocean;
            break;
        case Interface::CartridgeIdSystem3:
            cart = new System3;
            break;
        case Interface::CartridgeIdSuperGames:
            cart = new SuperGames;            
            break;
        case Interface::CartridgeIdZaxxon:
            cart = new Zaxxon;
            break;
        case Interface::CartridgeIdDefault:
        case Interface::CartridgeIdDefault8k:
            cart = new GameCart(true, false);
            break;            
            
        case Interface::CartridgeIdDefault16k:
            cart = new Cart16k;
            break;            
            
        case Interface::CartridgeIdUltimax:
            cart = new GameCart(false, true);
            break;

        case Interface::CartridgeIdGmod2:
			// we don't recreate the card because of additional complexity
            cart = gmod2;
            break;

        case Interface::CartridgeIdMagicDesk:
            cart = new MagicDesk;
            break;

        case Interface::CartridgeIdSimonsBasic:
            cart = new SimonsBasic;
            break;

        case Interface::CartridgeIdWarpSpeed:
            cart = new WarpSpeed;
            break;

        case Interface::CartridgeIdMach5:
            cart = new Mach5;
            break;

        case Interface::CartridgeIdRoss:
            cart = new Ross;
            break;

        case Interface::CartridgeIdWestermann:
            cart = new Westermann;
            break;

        case Interface::CartridgeIdPagefox:
            cart = new Pagefox;
            break;
            
        default:
            // forgot a rom
            cart = new GameCart;
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

