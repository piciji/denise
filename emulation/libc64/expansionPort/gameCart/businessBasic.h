
#pragma once

namespace LIBC64 {

    struct BusinessBasic : GameCart {

        BusinessBasic(System* system) : GameCart(system, false, false) {
        }

        auto peekIo1( uint16_t addr) -> uint8_t override {
            return 0;
        }

        auto readIo1( uint16_t addr) -> uint8_t override {
            cRomH = getChip(1);
            system->changeExpansionPortMemoryMode( exRom = false, game = false );
            return 0;
        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void override {
            cRomH = getChip(2);
            system->changeExpansionPortMemoryMode( exRom = true, game = false );
        }

        auto reset(bool softReset = false) -> void override {
            cRomL = getChip(0);
            system->changeExpansionPortMemoryMode( exRom = false, game = false );
        }

        auto isBootable( ) -> bool override { return false; }
    };

}
