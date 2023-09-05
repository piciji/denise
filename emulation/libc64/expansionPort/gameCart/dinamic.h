
#pragma once

namespace LIBC64 {

    struct Dinamic : GameCart {

        Dinamic(System* system) : GameCart(system, true, false) {

        }

        auto readIo1( uint16_t addr) -> uint8_t {
            for( auto& chip : chips ) {
                if (chip.bank == (addr & 0xf) ) {
                    cRomL = &chip;
                    cRomH = &chip;
                    break;
                }
            }

            return 0;
        }

        auto reset(bool softReset = false) -> void {

            cRomL = getChip(0);
            cRomH = getChip(0);
        }

    };

}
