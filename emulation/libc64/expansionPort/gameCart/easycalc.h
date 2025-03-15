
#pragma once

namespace LIBC64 {

    struct EasyCalc : GameCart {

        EasyCalc(System* system) : GameCart(system, false, false) {

        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void {
            Chip* _c;

            for( auto& chip : chips ) {
                if (chip.addr == 0xa000) {
                    if (chip.bank == (addr & 1) ) {
                        cRomH = &chip;
                        break;
                    }
                }
            }
        }

        auto reset(bool softReset = false) -> void {
            game = false;
            exRom = false;
            cRomL = getChip(0);
            cRomH = getChip(1);

            bool inv = cRomH && cRomH->ptr[0] != 4; // cartconv BUG?

            for (auto& chip : chips) {
                if (chip.id == 0) {
                    if (binFormat)
                        chip.addr = 0x8000;
                } else if (chip.id == 1) {
                    if (binFormat)
                        chip.addr = 0xA000;
                    chip.bank = inv ? 1 : 0;
                } else if (chip.id == 2) {
                    if (binFormat)
                        chip.addr = 0xA000;
                    chip.bank = inv ? 0 : 1;
                }
            }
        }

    };

}
