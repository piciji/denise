
#pragma once

namespace LIBC64 {

struct DiashowMaker : Freezer {

    DiashowMaker(System* system) : Freezer(system, true, false) {}

    bool enable = true;

    auto switchToUltimax() -> bool {
        return false;
    }

    auto readIo1( uint16_t addr ) -> uint8_t {

        if (addr == 0xde00) {
            system->changeExpansionPortMemoryMode( exRom = true, game = true );
            enable = false;
        }

        return 0;
    }

    auto writeIo1( uint16_t addr, uint8_t value ) -> void {

        if (addr == 0xde00) {
            system->changeExpansionPortMemoryMode( exRom = true, game = true );
            enable = false;
        }
    }

    auto didFreeze() -> void {
        nmiCall(false);
        cRomH = cRomL = getChip(0);
        system->changeExpansionPortMemoryMode( exRom = false, game = true );
        enable = true;
    }

    auto reset(bool softReset = false) -> void {
        cRomH = cRomL = getChip(0);
        enable = true;
    }

    auto serializeStep2(Emulator::Serializer& s) -> void {

        FreezeButton::serializeStep2( s );

        s.integer( enable );
    }
};

}
