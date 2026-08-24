
#pragma once

#include "gameCart.h"

namespace LIBC64 {

    struct WarpSpeed : GameCart {

        WarpSpeed(System* system) : GameCart(system, false, false) {

        }

        auto isBootable( ) -> bool override {
            return false;
        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void override {

            system->changeExpansionPortMemoryMode( exRom = false, game = false );
        }

        auto writeIo2( uint16_t addr, uint8_t value ) -> void override {

            system->changeExpansionPortMemoryMode( exRom = true, game = true );
        }

        auto peekIo1( uint16_t addr ) -> uint8_t override {
            return readIo1( addr );
        }

        auto readIo1( uint16_t addr ) -> uint8_t override {

            return *(cRomL->ptr + (0x1e00 + (addr & 0xff)));
        }

        auto peekIo2( uint16_t addr ) -> uint8_t override {
            return readIo2( addr );
        }

        auto readIo2( uint16_t addr ) -> uint8_t override {

            return *(cRomL->ptr + (0x1f00 + (addr & 0xff)));
        }

        auto assumeChips( ) -> void override {

            Cart::assumeChips( {16384} );
        }

        auto reset(bool softReset = false) -> void override {
            game = false;
            exRom = false;
            Cart::reset( softReset );
        }
    };

}
