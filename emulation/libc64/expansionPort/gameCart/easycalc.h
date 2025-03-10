
#pragma once

namespace LIBC64 {

    struct EasyCalc : GameCart {

        EasyCalc(System* system) : GameCart(system, false, false) {

        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void {
            Chip* _c;
            if ((_c = getChip(1)) && (_c->bank != 1))
                 addr ^= 1;

            for( auto& chip : chips ) {
                if (chip.addr == 0xa000) {
                    if (chip.bank == (addr & 1) ) {
                        cRomH = &chip;
                        break;
                    }
                }
            }
        }

        auto assumeChips( ) -> void { // when using BIN file
            Cart::assumeChips( );

            for (auto& chip : chips) {
                if (chip.id == 0)
                    chip.addr = 0x8000;
                else if (chip.id == 1) {
                    chip.addr = 0xA000;
                    chip.bank = 1;
                } else if (chip.id == 2) {
                    chip.addr = 0xA000;
                    chip.bank = 0;
                }
            }
        }

        auto reset(bool softReset = false) -> void {
            game = false;
            exRom = false;
            cRomL = getChip(0);
            cRomH = getChip(1);
        }

    };

}
