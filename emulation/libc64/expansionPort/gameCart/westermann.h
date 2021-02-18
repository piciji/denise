
#pragma once

namespace LIBC64 {

    struct Westermann : GameCart {

        Westermann() : GameCart(false, false) {

        }

        auto readIo2( uint16_t addr ) -> uint8_t {

            system->changeExpansionPortMemoryMode( exRom = false, game = true );

            return 0;
        }

        auto assumeChips( ) -> void {

            Cart::assumeChips( {16384} );
        }
    };

}
