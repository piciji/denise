
#pragma once

namespace LIBC64 {

    struct Comal80 : GameCart {

        Comal80(System* system) : GameCart(system, false, false) {

        }

        auto writeIo1(uint16_t addr, uint8_t data) -> void override {

            uint8_t bank;

            if (cartridgeRevision == 1) {
                bank = data & 3;

                switch ((data & 0x60) >> 5) {
                    case 0: // 16K GAME
                        game = false;
                        exRom = false;
                        break;
                    case 1: // ULTIMAX
                        game = false;
                        exRom = true;
                        break;
                    case 2: // 8K GAME
                        game = true;
                        exRom = false;
                        break;
                    case 3: // RAM / cart off
                        game = true;
                        exRom = true;
                        break;
                }
            } else {
                // Black COMAL80: bit 0-2 select bank, bit 6 disables cart
                bank = data & 7;

                if (data & 0x40) {
                    game = true;
                    exRom = true;
                } else {
                    game = false;
                    exRom = false;
                }
            }

            system->changeExpansionPortMemoryMode( exRom, game );

            cRomL = getChip(bank);
            cRomH = getChip(bank);
        }

        auto reset(bool softReset = false) -> void override {

            cRomL = getChip(0);
            cRomH = getChip(0);

            if (softReset) {
                game = exRom = false;
            }
        }

        auto assumeChips( ) -> void override {

            Cart::assumeChips( {16384} );
        }
    };

}
