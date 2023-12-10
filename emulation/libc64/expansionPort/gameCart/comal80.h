
#pragma once

namespace LIBC64 {

    struct Comal80 : GameCart {

        Comal80(System* system) : GameCart(system, false, false) {

        }

        auto writeIo1( uint16_t addr, uint8_t data ) -> void {
            if (data & 0x20) {
                game = exRom = true;
            } else if (data & 0x80) {
                game = exRom = false;
            } else if (data & 0x40) {
                game = true;
                exRom = false;
            }

            system->changeExpansionPortMemoryMode( exRom, game );

            uint8_t bank = data & (chips.size() == 4 ? 3 : 7);
            cRomL = getChip(bank);
            cRomH = getChip(bank);
        }

        auto reset(bool softReset = false) -> void {

            cRomL = getChip(0);
            cRomH = getChip(0);

            if (softReset) {
                game = exRom = false;
            }
        }

        auto assumeChips( ) -> void {

            Cart::assumeChips( {16384} );
        }
    };

}
