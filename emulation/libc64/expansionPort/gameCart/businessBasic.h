
#pragma once

namespace LIBC64 {

    struct BusinessBasic : GameCart {

        BusinessBasic(System* system) : GameCart(system, false, false) {
        }

        auto peekIo1( uint16_t addr) -> uint8_t {
            return 0;
        }

        auto readIo1( uint16_t addr) -> uint8_t {
            cRomH = getChip(1);
            system->changeExpansionPortMemoryMode( exRom = false, game = false );
            return 0;
        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void {
            cRomH = getChip(2);
            system->changeExpansionPortMemoryMode( exRom = true, game = false );
        }

        auto reset(bool softReset = false) -> void {
            cRomL = getChip(0);
            system->changeExpansionPortMemoryMode( exRom = false, game = false );
        }

        auto isBootable( ) -> bool { return false; }
    };

}
