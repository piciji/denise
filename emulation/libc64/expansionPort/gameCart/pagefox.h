
#pragma once

namespace LIBC64 {

    struct Pagefox : GameCart {

        uint8_t* ram = nullptr;
        bool useRam = false;
        uint8_t bank = 0;

        Pagefox(System* system) : GameCart(system, true, true) {
            ram = new uint8_t[ 32 * 1024 ];
        }

        ~Pagefox() override {
            delete[] ram;
        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void override {

            if ((addr & 0x80) == 0)
                return;

            cRomL = cRomH = getChip( (value >> 1) & 3 );

            useRam = (value & 0xc) == 8;

            bank = (value >> 1) & 1;

            if ( value & 0x10 )
                game = exRom = true;
            else
                game = exRom = false;

            system->changeExpansionPortMemoryMode( exRom, game );
        }

        auto peekRomL( uint16_t addr ) -> uint8_t override {
            return readRomL(addr);
        }

        auto readRomL( uint16_t addr ) -> uint8_t override {

            if (useRam)
                return ram[ addr + (bank << 14) ];

            return Cart::readRomL( addr );
        }

        auto writeRomL( uint16_t addr, uint8_t data ) -> void override {

            if (useRam)
                ram[ (addr & 0x1fff) + (bank << 14) ] = data;

            Cart::writeRomL(addr, data);
        }

        auto peekRomH( uint16_t addr ) -> uint8_t override {
            return readRomH(addr);
        }

        auto readRomH( uint16_t addr ) -> uint8_t override {

            if (useRam)
                return ram[ 0x2000 + addr + (bank << 14) ];

            return Cart::readRomH( addr );
        }

        auto writeRomH( uint16_t addr, uint8_t data ) -> void override {

            if (useRam)
                ram[ 0x2000 + (addr & 0x1fff) + (bank << 14) ] = data;

            Cart::writeRomH(addr, data);
        }

        auto listenToWritesAt80To9F( uint16_t addr, uint8_t data ) -> void override {

            if (useRam)
                ram[ (addr & 0x1fff) + (bank << 14) ] = data;
        }

        auto listenToWritesAtA0ToBF( uint16_t addr, uint8_t data ) -> void override {

            if (useRam)
                ram[ 0x2000 + (addr & 0x1fff) + (bank << 14) ] = data;
        }

        auto reset(bool softReset = false) -> void override {
            cRomH = cRomL = getChip(0);
            bank = 0;
            useRam = false;
            game = exRom = false;
            std::memset(ram, 0, 32 * 1024);
        }

        auto serializeSwitchedIn(Emulator::Serializer& s) -> void override {

            Cart::serialize( s );

            s.integer( bank );
            s.integer( useRam );
            s.array( ram, 32 * 1024 );
        }

        auto assumeChips( ) -> void override {

            Cart::assumeChips( {16384} );
        }
    };

}
