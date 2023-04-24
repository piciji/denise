
#pragma once

namespace LIBC64 {

    struct MagicDesk : GameCart {

        MagicDesk(System* system) : GameCart(system, true, false) {

        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void {

            if (addr != 0xde00)
                return;

            exRom = (value >> 7) & 1;

            value &= 0x7f;
            value %= chips.size();

            for( auto& chip : chips ) {
                if (chip.bank == value) {
                    cRomL = &chip;
                    break;
                }
            }

            system->changeExpansionPortMemoryMode( exRom, game = true );
        }

        auto reset(bool softReset = false) -> void {

            cRomL = getChip(0);
            cRomH = nullptr;
            // sub hunter: override wrongly seted header
            game = true;
            exRom = false;
        }
    };

}
