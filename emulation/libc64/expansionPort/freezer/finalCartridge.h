
#pragma once

namespace LIBC64 {

    struct FinalCartridge : Freezer {

        FinalCartridge(System* system) : Freezer(system, true, true) {

        }

        virtual auto bootSpeed() -> float { return 2.2; }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void {

            nmiCall(false);
            system->changeExpansionPortMemoryMode( exRom = true, game = true );
        }

        auto peekIo1( uint16_t addr ) -> uint8_t {
            return *(cRomL->ptr + (0x1e00 | (addr & 0xff)) );
        }

        auto readIo1( uint16_t addr ) -> uint8_t {

            nmiCall(false);
            system->changeExpansionPortMemoryMode( exRom = true, game = true );

            return *(cRomL->ptr + (0x1e00 | (addr & 0xff)) );
        }

        auto writeIo2( uint16_t addr, uint8_t value ) -> void {

            nmiCall(false);
            system->changeExpansionPortMemoryMode( exRom = false, game = false );
        }

        auto peekIo2( uint16_t addr ) -> uint8_t {
            return *(cRomL->ptr + (0x1f00 | (addr & 0xff)) );
        }

        auto readIo2( uint16_t addr ) -> uint8_t {

            nmiCall(false);
            system->changeExpansionPortMemoryMode( exRom = false, game = false );

            return *(cRomL->ptr + (0x1f00 | (addr & 0xff)) );
        }

        auto didFreeze() -> void {
            nmiCall(false);
        }

        auto assumeChips( ) -> void {

            Cart::assumeChips( {16384} );
        }

        auto reset(bool softReset = false) -> void {

            resetFreeze();
        }
    };

}
