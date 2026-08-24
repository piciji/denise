
#pragma once

namespace LIBC64 {

    struct HyperBasic : GameCart {

        HyperBasic(System* system) : GameCart(system, true, false) {

        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void {

            if (((value & 3) == 1) || ((value & 3) == 2)) {
                uint8_t bank = (value >> 4) & 3;
                bank |= ((value & 1) ^ 1) << 2;
                cRomL = getChip(bank);
                exRom = !!(value & 0x80);
            } else
                exRom = true;

            system->changeExpansionPortMemoryMode( exRom, game );
        }

        auto reset(bool softReset = false) -> void {

            cRomL = getChip(3);
        }

        auto isBootable( ) -> bool override { return false; }

    };

}
