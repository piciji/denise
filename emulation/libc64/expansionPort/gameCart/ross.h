
#pragma once

namespace LIBC64 {

    struct Ross : GameCart {

        Ross(System* system) : GameCart(system, false, false) {

        }

        auto peekIo1( uint16_t addr ) -> uint8_t override {
            return 0;
        }

        auto readIo1( uint16_t addr ) -> uint8_t override {

            if (chips.size() > 1)
                cRomL = cRomH = getChip(1);

            return 0;
        }

        auto peekIo2( uint16_t addr ) -> uint8_t override {
            return 0;
        }

        auto readIo2( uint16_t addr ) -> uint8_t override {

            system->changeExpansionPortMemoryMode( exRom = true, game = true );

            return 0;
        }

        auto assumeChips( ) -> void override {

            Cart::assumeChips( {16384} );
        }

        auto reset(bool softReset = false) -> void override {

            cRomL = getChip(0);

            cRomH = getChip(0);
        }
    };

}
