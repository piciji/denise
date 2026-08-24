
#pragma once

namespace LIBC64 {

    struct FinalCartridge3 : Freezer {

        bool enable;

        FinalCartridge3(System* system) : Freezer(system, true, true) {

        }

        // auto isBootable( ) -> bool { return true; }

        auto bootSpeed() -> float override { return 1.0; }

        auto writeIo2( uint16_t addr, uint8_t value ) -> void override {

            if ( enable && ((addr & 0xff) == 0xff) ) {

                uint8_t bank = value & 15;
                bank %= chips.size();

                cRomL = cRomH = getChip( bank );

                exRom = (value >> 4) & 1;
                game = (value >> 5) & 1;

                nmiCall( (value & 0x40) == 0 );

                enable = (value & 0x80) == 0;

                system->changeExpansionPortMemoryMode( exRom, game );
            }
        }

        auto peekIo1( uint16_t addr ) -> uint8_t override {
            return readIo1( addr );
        }

        auto readIo1( uint16_t addr ) -> uint8_t override {

            return *(cRomL->ptr + (0x1e00 | (addr & 0xff)) );
        }

        auto peekIo2( uint16_t addr ) -> uint8_t override {
            return readIo2( addr );
        }

        auto readIo2( uint16_t addr ) -> uint8_t override {

            return *(cRomL->ptr + (0x1f00 | (addr & 0xff)) );
        }

        auto didFreeze() -> void override {
            enable = true;
            vicII->setUltimaxPhi1( false );
        }

        auto assumeChips( ) -> void override {

            Cart::assumeChips( {16384} );
        }

        auto serializeSwitchedIn(Emulator::Serializer& s) -> void override {

            FreezeButton::serialize( s );

            s.integer( enable );
        }

        auto reset(bool softReset = false) -> void override {
            enable = true;
            game = false;
            exRom = false;
            cRomL = getChip(0);
            cRomH = getChip(0);
            resetFreeze();
        }
    };

}
