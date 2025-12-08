
#pragma once

#include "gameCart.h"

namespace LIBC64 {

    struct WarpSpeed : GameCart {

        WarpSpeed(System* system) : GameCart(system, false, false) {

        }

        auto isBootable( ) -> bool {
            return false;
        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void {

            system->changeExpansionPortMemoryMode( exRom = false, game = false );
        }

        auto writeIo2( uint16_t addr, uint8_t value ) -> void {

            system->changeExpansionPortMemoryMode( exRom = true, game = true );
        }

        auto peekIo1( uint16_t addr ) -> uint8_t {
            return readIo1( addr );
        }

        auto readIo1( uint16_t addr ) -> uint8_t {

            return *(cRomL->ptr + (0x1e00 + (addr & 0xff)));
        }

        auto peekIo2( uint16_t addr ) -> uint8_t {
            return readIo2( addr );
        }

        auto readIo2( uint16_t addr ) -> uint8_t {

            return *(cRomL->ptr + (0x1f00 + (addr & 0xff)));
        }

        auto assumeChips( ) -> void {

            Cart::assumeChips( {16384} );
        }

        auto reset(bool softReset = false) -> void {
            game = false;
            exRom = false;
            Cart::reset( softReset );
        }
    };

}
