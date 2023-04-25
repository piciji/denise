
#pragma once

namespace LIBC64 {

    struct SimonsBasic : GameCart {

        SimonsBasic(System* system) : GameCart(system, true, false) {

        }

        auto writeIo1( uint16_t addr, uint8_t value ) -> void {

            system->changeExpansionPortMemoryMode( exRom = false, game = false );
        }

        auto readIo1( uint16_t addr ) -> uint8_t {

            system->changeExpansionPortMemoryMode( exRom = false, game = true );

            return ExpansionPort::readIo1( addr );
        }

    };

}
