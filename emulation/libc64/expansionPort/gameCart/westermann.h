
#pragma once

namespace LIBC64 {

    struct Westermann : GameCart {

        Westermann(System* system) : GameCart(system, false, false) {

        }

        auto peekIo2( uint16_t addr ) -> uint8_t override {
            return 0;
        }

        auto readIo2( uint16_t addr ) -> uint8_t override {

            system->changeExpansionPortMemoryMode( exRom = false, game = true );

            return 0;
        }

        auto assumeChips( ) -> void override {

            Cart::assumeChips( {16384} );
        }
    };

}
