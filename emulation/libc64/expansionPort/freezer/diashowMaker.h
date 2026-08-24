
#pragma once

namespace LIBC64 {

struct DiashowMaker : Freezer {

    DiashowMaker(System* system) : Freezer(system, true, false) {}

    bool enable = true;

    auto switchToUltimax() -> bool override {
        return false;
    }

    auto peekIo1( uint16_t addr ) -> uint8_t override {
        return 0;
    }

    auto readIo1( uint16_t addr ) -> uint8_t override {

        if (addr == 0xde00) {
            system->changeExpansionPortMemoryMode( exRom = true, game = true );
            enable = false;
        }

        return 0;
    }

    auto writeIo1( uint16_t addr, uint8_t value ) -> void override {

        if (addr == 0xde00) {
            system->changeExpansionPortMemoryMode( exRom = true, game = true );
            enable = false;
        }
    }

    auto didFreeze() -> void override {
        nmiCall(false);
        cRomH = cRomL = getChip(0);
        system->changeExpansionPortMemoryMode( exRom = false, game = true );
        enable = true;
    }

    auto reset(bool softReset = false) -> void override {
        cRomH = cRomL = getChip(0);
        enable = true;
        resetFreeze();
    }

    auto serializeSwitchedIn(Emulator::Serializer& s) -> void override {

        FreezeButton::serialize( s );

        s.integer( enable );
    }
};

}
