
#pragma once

namespace LIBC64 {

    struct Mach5 : GameCart {

        Mach5() : GameCart(true, false) {

        }

        auto isBootable( ) -> bool {
            return false;
        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void {

            system->changeExpansionPortMemoryMode( exRom = false, game = true );
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

    };

}
