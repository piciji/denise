
#pragma once

namespace LIBC64 {

    struct WarpSpeed : GameCart {

        WarpSpeed() : GameCart(false, false) {

        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void {

            system->changeExpansionPortMemoryMode( exRom = false, game = false );
        }

        auto writeIo2( uint16_t addr, uint8_t value ) -> void {

            system->changeExpansionPortMemoryMode( exRom = true, game = true );
        }

        auto readIo1( uint16_t addr ) -> uint8_t {

            return *(cRomL->ptr + (0x1e00 + (addr & 0xff)));
        }

        auto readIo2( uint16_t addr ) -> uint8_t {

            return *(cRomL->ptr + (0x1f00 + (addr & 0xff)));
        }

        auto assumeChips( ) -> void {

            Cart::assumeChips( {16384} );
        }

        auto reset() -> void {
            game = false;
            exRom = false;
            Cart::reset();
        }
    };

}
