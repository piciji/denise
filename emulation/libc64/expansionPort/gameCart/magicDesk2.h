
#pragma once

namespace LIBC64 {

struct MagicDesk2 : GameCart {

    MagicDesk2(System* system) : GameCart(system, false, false) {

    }

    auto writeIo1( uint16_t addr, uint8_t value ) -> void {
        exRom = (value >> 7) & 1;
        game = exRom;

        value &= 0x7f;
        value %= chips.size();

        for( auto& chip : chips ) {
            if (chip.bank == value) {
                cRomL = &chip;
                cRomH = &chip;
                break;
            }
        }

        system->changeExpansionPortMemoryMode( exRom, game );
    }

    auto reset(bool softReset = false) -> void {

        cRomL = getChip(0);
        cRomH = getChip(0);
        game = false;
        exRom = false;
    }

    auto assumeChips( ) -> void {

        Cart::assumeChips( {16384} );
    }
};

}
