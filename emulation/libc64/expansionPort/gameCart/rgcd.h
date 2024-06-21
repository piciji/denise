
#pragma once

namespace LIBC64 {

    struct RGCD : GameCart {

        RGCD(System* system) : GameCart(system, true, false) {}

        auto writeIo1( uint16_t addr, uint8_t data ) -> void {
            if (data & 8) {
                exRom = true;
                return system->changeExpansionPortMemoryMode( exRom, game );
            }

            if (version == 0x101) // hucky
                data ^= 7;

            uint8_t mask = chips.size() - 1;

            cRomL = getChip(data & mask);
        }

        auto reset(bool softReset = false) -> void {

            if (version == 0x101) // hucky
                cRomL = getChip(chips.size() - 1);
            else
                cRomL = getChip(0);

            if (softReset)
                exRom = false;
        }

    };

}
