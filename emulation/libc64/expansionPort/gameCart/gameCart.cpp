
#include "gameCart.h"
#include "../../system/system.h"
#include "funplay.h"
#include "ocean.h"
#include "zaxxon.h"
#include "system3.h"
#include "supergames.h"
#include "cart16k.h"
#include "../gmod/gmod2.h"

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
            
        default:
            // forgot a rom
            cart = new GameCart;
            break;            

    }
    
    return cart;
}

auto GameCart::createImage(unsigned& imageSize) -> uint8_t* {
	//todo: redesign if there are more expansions in this group with writable memory
	return Gmod2::createImage( imageSize );
}

auto GameCart::createSecondaryImage(unsigned& imageSize) -> uint8_t* {
	return Gmod2::createSecondaryImage( imageSize );
}
    
}

