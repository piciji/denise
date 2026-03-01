
#pragma once

namespace LIBC64 {

    struct Mach5 : GameCart {

        Mach5(System* system) : GameCart(system, true, false) {

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

    };

}
