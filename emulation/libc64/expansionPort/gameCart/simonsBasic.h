
#pragma once

namespace LIBC64 {

    struct SimonsBasic : GameCart {

        SimonsBasic(System* system) : GameCart(system, true, false) {

        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void override {

            system->changeExpansionPortMemoryMode( exRom = false, game = false );
        }

        auto peekIo1( uint16_t addr ) -> uint8_t override {
            return ExpansionPort::readIo1( addr );
        }

        auto readIo1( uint16_t addr ) -> uint8_t override {

            system->changeExpansionPortMemoryMode( exRom = false, game = true );

            return ExpansionPort::readIo1( addr );
        }

    };

}
